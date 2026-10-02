#include "Venus.h"
#include "TextureManager.h"
#include "Camera.h"

// Venus global state
bool Venus::vtBuilt = false;
int  Venus::lastStreamCX = 999999;
int  Venus::lastStreamCZ = 999999;
std::vector<VenusTerrainChunk*>      Venus::activeChunks;
std::unordered_set<long long>        Venus::activeChunkKeys;
std::vector<VenusChunkCandidate>     Venus::cachedCandidates;
bool Venus::showHUDFlag = true;


// ============================================================
//  Venus Terrain System — Volcanic Surface Exploration
// ============================================================
static float getVenusTerrainHeight(float wx, float wz) {
    float warpX = fbmNoise(wx * 0.004f + 200.0f, wz * 0.004f + 300.0f, 3) * 22.0f;
    float warpZ = fbmNoise(wx * 0.004f + 500.0f, wz * 0.004f + 600.0f, 3) * 22.0f;
    float wx2 = wx + warpX;
    float wz2 = wz + warpZ;

    float highland = smoothedNoise(wx2 * 0.004f + 5.0f, wz2 * 0.004f + 3.0f);
    highland += smoothedNoise(wx2 * 0.008f + 2.0f, wz2 * 0.008f + 1.0f) * 0.5f;
    float hlMask = smoothstepf(-0.1f, 0.3f, highland);

    float volcWX = smoothedNoise2(wx * 0.006f + 10.0f, wz * 0.006f + 12.0f) * 15.0f;
    float volcWZ = smoothedNoise2(wx * 0.006f + 20.0f, wz * 0.006f + 22.0f) * 15.0f;
    float volc = ridgeNoise((wx2 + volcWX) * 0.012f + 1.0f, (wz2 + volcWZ) * 0.012f + 2.0f, 5) * 18.0f;
    volc *= hlMask * hlMask;

    float plains = fbmNoise(wx2 * 0.020f + 7.0f, wz2 * 0.020f + 9.0f, 5, 0.48f) * 5.0f;
    float lava = ridgeNoise(wx * 0.025f + 40.0f, wz * 0.025f + 50.0f, 3);
    lava = (1.0f - lava) * 2.5f * (1.0f - hlMask);

    float plat = plateauNoise(wx2 * 0.010f + 30.0f, wz2 * 0.010f + 40.0f, 3) * 3.5f;
    plat *= hlMask * smoothstepf(0.2f, 0.5f, highland);

    float detail = fbmNoise(wx * 0.08f + 1.0f, wz * 0.08f + 5.0f, 3, 0.50f) * 0.9f;
    float micro = smoothedNoise(wx * 0.35f + 15.0f, wz * 0.35f + 10.0f) * 0.2f;

    return highland * 3.0f + volc + plains + plat - lava + detail + micro;
}

static void getVenusTerrainColor(float h, float slope, float volcanic, float& cr, float& cg, float& cb) {
    if (h < -2.0f) {
        float t = clampf((h + 5.0f) / 3.0f, 0.0f, 1.0f);
        cr = lerpf(0.12f, 0.18f, t); cg = lerpf(0.08f, 0.12f, t); cb = lerpf(0.06f, 0.08f, t);
    }
    else if (h < 1.0f) {
        float t = (h + 2.0f) / 3.0f;
        cr = lerpf(0.22f, 0.38f, t); cg = lerpf(0.14f, 0.22f, t); cb = lerpf(0.08f, 0.10f, t);
        if (volcanic > 0.6f) { float glow = (volcanic - 0.6f) * 2.5f; cr += glow * 0.25f; cg += glow * 0.08f; }
    }
    else if (h < 4.0f) {
        float t = (h - 1.0f) / 3.0f;
        cr = lerpf(0.38f, 0.45f, t); cg = lerpf(0.22f, 0.28f, t); cb = lerpf(0.10f, 0.14f, t);
    }
    else if (h < 8.0f) {
        float t = (h - 4.0f) / 4.0f;
        float sm = clampf(slope * 3.0f, 0.0f, 1.0f);
        float fr = lerpf(0.42f, 0.50f, t), fg = lerpf(0.26f, 0.32f, t), fb = lerpf(0.14f, 0.18f, t);
        float rr = lerpf(0.35f, 0.40f, t), rg = lerpf(0.22f, 0.28f, t), rb = lerpf(0.16f, 0.20f, t);
        cr = lerpf(fr, rr, sm); cg = lerpf(fg, rg, sm); cb = lerpf(fb, rb, sm);
    }
    else if (h < 13.0f) {
        float t = (h - 8.0f) / 5.0f;
        cr = lerpf(0.40f, 0.48f, t); cg = lerpf(0.30f, 0.38f, t); cb = lerpf(0.22f, 0.30f, t);
    }
    else {
        float t = clampf((h - 13.0f) / 5.0f, 0.0f, 1.0f);
        cr = lerpf(0.48f, 0.55f, t); cg = lerpf(0.38f, 0.48f, t); cb = lerpf(0.26f, 0.22f, t);
    }

    float ao = 1.0f - clampf(slope * 0.40f, 0.0f, 0.25f);
    cr *= ao; cg *= ao; cb *= ao;
}

static float sampleVenusTerrainHeight(float wx, float wz) {
    // Infinite Venus terrain is procedural, so sampling is always valid anywhere.
    return getVenusTerrainHeight(wx, wz);
}

static VenusTerrainChunk* generateVenusChunk(int cx, int cz) {
    VenusTerrainChunk* chunk = new VenusTerrainChunk();
    chunk->cx = cx;
    chunk->cz = cz;
    chunk->valid = true;
    for (int i = 0; i < VT_LOD_COUNT; i++) chunk->lists[i] = 0;

    float baseWX = cx * VT_CHUNK_SIZE - VT_CHUNK_SIZE * 0.5f;
    float baseWZ = cz * VT_CHUNK_SIZE - VT_CHUNK_SIZE * 0.5f;

    int cells = VT_CHUNK_CELLS;
    int gridPoints = cells + 1;
    int total = gridPoints * gridPoints;

    std::vector<float> cH(total);
    std::vector<float> cNX(total), cNY(total), cNZ(total);
    std::vector<float> cCR(total), cCG(total), cCB(total);

    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            float wx = baseWX + i * VT_SPACING;
            float wz = baseWZ + j * VT_SPACING;
            cH[j * gridPoints + i] = getVenusTerrainHeight(wx, wz);
        }
    }

    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            int idx = j * gridPoints + i;
            float wx = baseWX + i * VT_SPACING;
            float wz = baseWZ + j * VT_SPACING;

            float hL = (i > 0) ? cH[j * gridPoints + (i - 1)] : getVenusTerrainHeight(wx - VT_SPACING, wz);
            float hR = (i < cells) ? cH[j * gridPoints + (i + 1)] : getVenusTerrainHeight(wx + VT_SPACING, wz);
            float hD = (j > 0) ? cH[(j - 1) * gridPoints + i] : getVenusTerrainHeight(wx, wz - VT_SPACING);
            float hU = (j < cells) ? cH[(j + 1) * gridPoints + i] : getVenusTerrainHeight(wx, wz + VT_SPACING);

            float nx = (hL - hR) / (2.0f * VT_SPACING);
            float nz = (hD - hU) / (2.0f * VT_SPACING);
            float len = sqrtf(nx * nx + 1.0f + nz * nz);
            cNX[idx] = nx / len;
            cNY[idx] = 1.0f / len;
            cNZ[idx] = nz / len;
        }
    }

    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            int idx = j * gridPoints + i;
            float wx = baseWX + i * VT_SPACING;
            float wz = baseWZ + j * VT_SPACING;
            float h = cH[idx];
            float slope = 1.0f - cNY[idx];

            float volcanic = (fbmNoise(wx * 0.015f + 80.0f, wz * 0.015f + 80.0f, 3) + 1.0f) * 0.5f;
            getVenusTerrainColor(h, slope, volcanic, cCR[idx], cCG[idx], cCB[idx]);

            float sky = (cNY[idx] + 1.0f) * 0.5f;
            cCR[idx] *= 0.90f + sky * 0.10f;
            cCG[idx] *= 0.85f + sky * 0.08f;
            cCB[idx] *= 0.80f + sky * 0.05f;

            if (i >= 2 && i <= cells - 2 && j >= 2 && j <= cells - 2 && (i % 5 == 0) && (j % 5 == 0)) {
                if (slope > 0.48f) continue;

                float rng = (noiseHash(cx * 2000 + i * 13 + 3, cz * 2000 + j * 29 + 7) + 1.0f) * 0.5f;
                float fissure = ridgeNoise(wx * 0.025f + 40.0f, wz * 0.025f + 50.0f, 3);

                if (rng > 0.64f || fissure > 0.76f) {
                    DetailObj d;
                    d.x = wx + noiseHash(cx * 3000 + i + 2, cz * 3000 + j) * VT_SPACING * 0.35f;
                    d.z = wz + noiseHash(cx * 3000 + i, cz * 3000 + j + 2) * VT_SPACING * 0.35f;
                    d.y = getVenusTerrainHeight(d.x, d.z);
                    d.size = 0.35f + rng * 1.10f;
                    d.type = 0;
                    d.rotY = rng * 360.0f;

                    float rv = noiseHash(cx * 1700 + i * 7, cz * 1700 + j * 11) * 0.06f;
                    d.r = 0.38f + rv;
                    d.g = 0.27f + rv * 0.75f;
                    d.b = 0.16f + rv * 0.45f;
                    chunk->details.push_back(d);
                }
            }
        }
    }

    for (int lod = 0; lod < VT_LOD_COUNT; lod++) {
        int step = VT_LOD_STEP[lod];
        chunk->lists[lod] = glGenLists(1);
        glNewList(chunk->lists[lod], GL_COMPILE);

        for (int j = 0; j < cells; j += step) {
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= cells; i += step) {
                for (int dj = 1; dj >= 0; dj--) {
                    int cj = clampI(j + dj * step, 0, cells);
                    int idx = cj * gridPoints + i;
                    float px = baseWX + i * VT_SPACING;
                    float pz = baseWZ + cj * VT_SPACING;
                    float py = cH[idx];

                    glNormal3f(cNX[idx], cNY[idx], cNZ[idx]);
                    glColor3f(cCR[idx], cCG[idx], cCB[idx]);
                    glVertex3f(px, py, pz);
                }
            }
            glEnd();
        }

        glEndList();
    }

    return chunk;
}

static bool venusChunkExists(int testCX, int testCZ) {
    return activeVenusChunkKeys.find(makeChunkKey(testCX, testCZ)) != activeVenusChunkKeys.end();
}

static void addVenusChunk(int cx, int cz) {
    long long key = makeChunkKey(cx, cz);
    if (activeVenusChunkKeys.find(key) != activeVenusChunkKeys.end()) return;

    VenusTerrainChunk* chunk = generateVenusChunk(cx, cz);
    activeVenusChunks.push_back(chunk);
    activeVenusChunkKeys.insert(key);
}

static void updateVenusTerrainStreaming() {
    if (!inVenusTerrainMode) return;

    int playerCX = (int)roundf(tCamX / VT_CHUNK_SIZE);
    int playerCZ = (int)roundf(tCamZ / VT_CHUNK_SIZE);

    int unloadRadius = VT_PRELOAD_CHUNKS + 2;
    for (auto it = activeVenusChunks.begin(); it != activeVenusChunks.end(); ) {
        VenusTerrainChunk* chunk = *it;
        if (abs(chunk->cx - playerCX) > unloadRadius || abs(chunk->cz - playerCZ) > unloadRadius) {
            for (int i = 0; i < VT_LOD_COUNT; i++) {
                if (chunk->lists[i] != 0) glDeleteLists(chunk->lists[i], 1);
            }

            activeVenusChunkKeys.erase(makeChunkKey(chunk->cx, chunk->cz));
            delete chunk;
            it = activeVenusChunks.erase(it);
        }
        else {
            ++it;
        }
    }

    bool changedChunk = (playerCX != lastVenusStreamCX || playerCZ != lastVenusStreamCZ);

    if (changedChunk || cachedVenusCandidates.empty()) {
        lastVenusStreamCX = playerCX;
        lastVenusStreamCZ = playerCZ;
        cachedVenusCandidates.clear();

        float fwdX = sinf(tCamYaw * DEG2RAD);
        float fwdZ = cosf(tCamYaw * DEG2RAD);
        float predictX = tCamX + tCamVelX * 1.0f;
        float predictZ = tCamZ + tCamVelZ * 1.0f;
        int predictCX = (int)roundf(predictX / VT_CHUNK_SIZE);
        int predictCZ = (int)roundf(predictZ / VT_CHUNK_SIZE);

        for (int dz = -VT_PRELOAD_CHUNKS; dz <= VT_PRELOAD_CHUNKS; dz++) {
            for (int dx = -VT_PRELOAD_CHUNKS; dx <= VT_PRELOAD_CHUNKS; dx++) {
                int testCX = playerCX + dx;
                int testCZ = playerCZ + dz;

                if (venusChunkExists(testCX, testCZ)) continue;

                float distFromPlayer = sqrtf((float)(dx * dx + dz * dz));
                float predDX = (float)(testCX - predictCX);
                float predDZ = (float)(testCZ - predictCZ);
                float distFromPredict = sqrtf(predDX * predDX + predDZ * predDZ);

                float dirBias = 0.0f;
                if (distFromPlayer > 0.5f) {
                    float toDirX = (float)dx / distFromPlayer;
                    float toDirZ = (float)dz / distFromPlayer;
                    float dot = toDirX * fwdX + toDirZ * fwdZ;
                    dirBias = (1.0f - dot) * 1.6f;
                }

                float priority = distFromPlayer * 0.55f + distFromPredict * 0.30f + dirBias;
                cachedVenusCandidates.push_back({ testCX, testCZ, priority });
            }
        }

        std::sort(cachedVenusCandidates.begin(), cachedVenusCandidates.end(),
            [](const VenusChunkCandidate& a, const VenusChunkCandidate& b) {
                return a.priority < b.priority;
            });
    }

    int chunksLoaded = 0;
    for (auto it = cachedVenusCandidates.begin(); it != cachedVenusCandidates.end() && chunksLoaded < VT_MAX_CHUNKS_PER_FRAME; ) {
        if (!venusChunkExists(it->cx, it->cz)) {
            addVenusChunk(it->cx, it->cz);
            chunksLoaded++;
        }
        it = cachedVenusCandidates.erase(it);
    }
}

static void buildVenusTerrainV2() {
    if (vtBuilt) return;

    int playerCX = (int)roundf(tCamX / VT_CHUNK_SIZE);
    int playerCZ = (int)roundf(tCamZ / VT_CHUNK_SIZE);

    cachedVenusCandidates.clear();
    lastVenusStreamCX = 999999;
    lastVenusStreamCZ = 999999;

    int initRadius = 2;
    for (int dz = -initRadius; dz <= initRadius; dz++) {
        for (int dx = -initRadius; dx <= initRadius; dx++) {
            addVenusChunk(playerCX + dx, playerCZ + dz);
        }
    }

    vtBuilt = true;
}

static void cleanupVenusTerrain() {
    for (VenusTerrainChunk* chunk : activeVenusChunks) {
        if (!chunk) continue;
        for (int i = 0; i < VT_LOD_COUNT; i++) {
            if (chunk->lists[i] != 0) glDeleteLists(chunk->lists[i], 1);
        }
        delete chunk;
    }

    activeVenusChunks.clear();
    activeVenusChunkKeys.clear();
    cachedVenusCandidates.clear();
    lastVenusStreamCX = 999999;
    lastVenusStreamCZ = 999999;
    vtBuilt = false;
}

static void drawVenusAtmosphericSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);

    float skyR = 420.0f;
    int stacks = 20, slices = 40;
    float sunDX = cosf(VT_SUN_EL) * sinf(VT_SUN_AZ);
    float sunDY = sinf(VT_SUN_EL);
    float sunDZ = cosf(VT_SUN_EL) * cosf(VT_SUN_AZ);

    for (int si = 0; si < stacks; si++) {
        float t0 = (float)si / stacks;
        float t1 = (float)(si + 1) / stacks;
        float phi0 = t0 * PI * 0.5f;
        float phi1 = t1 * PI * 0.5f;
        float y0 = skyR * sinf(phi0), y1 = skyR * sinf(phi1);
        float rad0 = skyR * cosf(phi0), rad1 = skyR * cosf(phi1);
        glBegin(GL_QUAD_STRIP);
        for (int sj = 0; sj <= slices; sj++) {
            float theta = (float)sj / slices * TWO_PI;
            float cx = cosf(theta), cz = sinf(theta);
            for (int v = 1; v >= 0; v--) {
                float t = (v == 1) ? t1 : t0;
                float vRad = (v == 1) ? rad1 : rad0;
                float vY = (v == 1) ? y1 : y0;
                float vPhi = (v == 1) ? phi1 : phi0;
                float vdx = cosf(vPhi) * cx;
                float vdy = sinf(vPhi);
                float vdz = cosf(vPhi) * cz;
                float br = lerpf(0.85f, 0.45f, t * t);
                float bg = lerpf(0.55f, 0.15f, t * t);
                float bb = lerpf(0.20f, 0.05f, t * t);
                float sunDot = clampf(vdx * sunDX + vdy * sunDY + vdz * sunDZ, 0.0f, 1.0f);
                float sunGlow = powf(sunDot, 12.0f) * 0.4f;
                float sunHalo = powf(sunDot, 3.0f) * 0.2f;
                br += sunGlow + sunHalo * 0.8f;
                bg += sunGlow * 0.7f + sunHalo * 0.4f;
                bb += sunGlow * 0.2f + sunHalo * 0.1f;
                glColor3f(clampf(br, 0.0f, 1.0f), clampf(bg, 0.0f, 1.0f), clampf(bb, 0.0f, 1.0f));
                glVertex3f(tCamX + vRad * cx, vY, tCamZ + vRad * cz);
            }
        }
        glEnd();
    }

    glColor3f(0.15f, 0.05f, 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(tCamX - skyR, -12.0f, tCamZ - skyR);
    glVertex3f(tCamX + skyR, -12.0f, tCamZ - skyR);
    glVertex3f(tCamX + skyR, -12.0f, tCamZ + skyR);
    glVertex3f(tCamX - skyR, -12.0f, tCamZ + skyR);
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

static void drawVenusTerrainSun() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float sx = tCamX + cosf(VT_SUN_EL) * sinf(VT_SUN_AZ) * 180.0f;
    float sy = sinf(VT_SUN_EL) * 180.0f + 40.0f;
    float sz = tCamZ + cosf(VT_SUN_EL) * cosf(VT_SUN_AZ) * 180.0f;

    glPushMatrix();
    glTranslatef(sx, sy, sz);
    glColor4f(1.0f, 0.72f, 0.32f, 0.35f);
    glutSolidSphere(18.0f, 24, 12);
    glColor4f(1.0f, 0.55f, 0.18f, 0.18f);
    glutSolidSphere(34.0f, 24, 12);
    glPopMatrix();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void drawVenusTerrainChunks() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat tSpec[] = { 0.15f, 0.10f, 0.05f, 1.0f };
    GLfloat tSh[] = { 12.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, tSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, tSh);

    float forwardX = sinf(tCamYaw * DEG2RAD);
    float forwardZ = cosf(tCamYaw * DEG2RAD);

    for (VenusTerrainChunk* chunk : activeVenusChunks) {
        if (!chunk || !chunk->valid) continue;

        float ccx = chunk->cx * VT_CHUNK_SIZE;
        float ccz = chunk->cz * VT_CHUNK_SIZE;
        float dx = ccx - tCamX;
        float dz = ccz - tCamZ;
        float dist2 = dx * dx + dz * dz;

        float maxDrawDist = (VT_VIEW_CHUNKS + 1.5f) * VT_CHUNK_SIZE;
        if (dist2 > maxDrawDist * maxDrawDist) continue;

        float dist = sqrtf(dist2);
        if (dist > VT_CHUNK_SIZE * 2.0f) {
            float safeDist = fmaxf(dist, 0.0001f);
            float dot = (dx / safeDist) * forwardX + (dz / safeDist) * forwardZ;
            if (dot < -0.45f && dist > VT_CHUNK_SIZE * 3.0f) continue;
        }

        int lod = (dist < VT_CHUNK_SIZE * 1.1f) ? 0 :
            (dist < VT_CHUNK_SIZE * 2.2f) ? 1 :
            (dist < VT_CHUNK_SIZE * 3.8f) ? 2 : 3;

        if (chunk->lists[lod] != 0) glCallList(chunk->lists[lod]);
    }
}

static void drawVenusDetailObjects() {
    float viewDist = 70.0f;
    float viewDist2 = viewDist * viewDist;

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glDisable(GL_TEXTURE_2D);

    GLfloat dSpec[] = { 0.1f, 0.08f, 0.05f, 1.0f };
    GLfloat dSh[] = { 8.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, dSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, dSh);

    for (VenusTerrainChunk* chunk : activeVenusChunks) {
        if (!chunk) continue;

        float ccx = chunk->cx * VT_CHUNK_SIZE;
        float ccz = chunk->cz * VT_CHUNK_SIZE;
        float cdx = ccx - tCamX;
        float cdz = ccz - tCamZ;
        if (cdx * cdx + cdz * cdz > (viewDist + VT_CHUNK_SIZE) * (viewDist + VT_CHUNK_SIZE)) continue;

        for (const auto& d : chunk->details) {
            float dx = d.x - tCamX;
            float dz = d.z - tCamZ;
            if (dx * dx + dz * dz > viewDist2) continue;
            drawRockFormation(d);
        }
    }
}

static void drawVenusTerrainHUD() {
    if (!showVenusTerrainHUD) return;

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    if (h <= 0) h = 1;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    drawGlassPanel(0, (float)(h - 42), (float)w, 42.0f, 1.0f);

    glColor4f(1.0f, 0.70f, 0.30f, 0.92f);
    drawString(GLUT_BITMAP_HELVETICA_18, 18.0f, (float)(h - 28), "VENUS INFINITE TERRAIN");

    char altBuf[64];
    float altMeters = tCamY * 1000.0f;
    if (altMeters > 1000.0f) sprintf(altBuf, "ALT  %.1f km", altMeters / 1000.0f);
    else sprintf(altBuf, "ALT  %.0f m", altMeters);

    glColor4f(0.95f, 0.80f, 0.50f, 0.88f);
    drawString(GLUT_BITMAP_HELVETICA_18, (float)(w - 195), (float)(h - 28), altBuf);

    char spdBuf[32];
    sprintf(spdBuf, "SPD %.0f", terrainMoveSpeed * 100.0f);
    glColor4f(0.95f, 0.85f, 0.60f, 0.65f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w - 290), (float)(h - 30), spdBuf);

    glColor4f(0.02f, 0.01f, 0.00f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, 30); glVertex2f(0, 30);
    glEnd();

    glColor4f(1.0f, 0.68f, 0.32f, 0.70f);
    drawString(GLUT_BITMAP_HELVETICA_12, 18.0f, 10.0f, "WASD: Move  |  Mouse: Look  |  V/ESC/R: Exit Venus  |  H: Toggle HUD");

    float cx = w / 2.0f, cy = h / 2.0f;
    glColor4f(1.0f, 0.85f, 0.55f, 0.20f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(cx - 14, cy); glVertex2f(cx - 5, cy);
    glVertex2f(cx + 5, cy); glVertex2f(cx + 14, cy);
    glVertex2f(cx, cy - 14); glVertex2f(cx, cy - 5);
    glVertex2f(cx, cy + 5); glVertex2f(cx, cy + 14);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

static void displayVenusTerrainScene() {
    glClearColor(0.85f, 0.45f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    applyPerspectiveIfNeeded(52.0f, 0.15f, 900.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawR = tCamYaw * DEG2RAD;
    float pitR = tCamPitch * DEG2RAD;
    float lookX = tCamX + cosf(pitR) * sinf(yawR);
    float lookY = tCamY + sinf(pitR);
    float lookZ = tCamZ + cosf(pitR) * cosf(yawR);

    gluLookAt(tCamX, tCamY, tCamZ, lookX, lookY, lookZ, 0, 1, 0);

    float fogDensity = 0.008f + 0.010f * (1.0f - clampf(tCamY / 55.0f, 0.0f, 1.0f));
    glEnable(GL_FOG);
    GLfloat fogCol[] = { 0.85f, 0.45f, 0.15f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, fogDensity);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    float sunDirX = cosf(VT_SUN_EL) * sinf(VT_SUN_AZ);
    float sunDirY = sinf(VT_SUN_EL);
    float sunDirZ = cosf(VT_SUN_EL) * cosf(VT_SUN_AZ);
    GLfloat sunDir[] = { sunDirX, sunDirY, sunDirZ, 0.0f };
    GLfloat sunAmb[] = { 0.25f, 0.15f, 0.10f, 1.0f };
    GLfloat sunDif[] = { 1.20f, 0.85f, 0.50f, 1.0f };
    GLfloat sunSpc[] = { 1.00f, 0.65f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);

    GLfloat skyDir[] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat skyAmb[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat skyDif[] = { 0.20f, 0.10f, 0.05f, 1.0f };
    GLfloat skySpc[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, skyDir);
    glLightfv(GL_LIGHT1, GL_AMBIENT, skyAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, skyDif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, skySpc);

    drawVenusAtmosphericSky();
    drawVenusTerrainSun();
    drawVenusTerrainChunks();
    drawVenusDetailObjects();

    glDisable(GL_LIGHT1);
    glDisable(GL_FOG);

    drawVignette(0.35f);
    drawVenusTerrainHUD();
}



float sampleVenusTerrainHeight(float wx, float wz) { return Venus::getTerrainHeight(wx, wz); }
void  updateVenusTerrainStreaming()                  { Venus::updateStreaming(); }
