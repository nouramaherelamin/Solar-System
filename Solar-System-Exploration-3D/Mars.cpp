#include "Mars.h"
#include "TextureManager.h"
#include "Camera.h"

// Mars global state
std::vector<MarsChunk*>       Mars::activeChunks;
std::unordered_set<long long> Mars::activeChunkKeys;
bool Mars::mtBuilt = false;
std::vector<MarsParticle>     Mars::particles;

static float getMarsTerrainHeight(float wx, float wz);

static void drawMarsHUD() {
    if (!showTerrainHUD) return;

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

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

    // Top bar
    drawGlassPanel(0, (float)(h - 42), (float)w, 42.0f, 1.0f);

    glColor4f(1.0f, 0.55f, 0.25f, 0.92f);
    drawString(GLUT_BITMAP_HELVETICA_18, 18.0f, (float)(h - 28), "MARS SURFACE EXPLORATION");

    // Status indicator
    const char* modeStr;
    float modeR, modeG, modeB;
    if (marsCamY < 4.0f) {
        modeStr = "SURFACE"; modeR = 1.0f; modeG = 0.6f; modeB = 0.3f;
    }
    else if (marsCamY < 15.0f) {
        modeStr = "LOW ATMOSPHERE"; modeR = 1.0f; modeG = 0.5f; modeB = 0.25f;
    }
    else {
        modeStr = "DUST STORM LAYER"; modeR = 1.0f; modeG = 0.35f; modeB = 0.15f;
    }

    float pulse = 0.6f + 0.4f * sinf(currentTime * 3.0f);
    glColor4f(modeR, modeG, modeB, pulse);
    glPointSize(6.0f);
    glBegin(GL_POINTS); glVertex2f(260.0f, (float)(h - 26)); glEnd();
    glColor4f(modeR, modeG, modeB, 0.85f);
    drawString(GLUT_BITMAP_HELVETICA_12, 270.0f, (float)(h - 30), modeStr);

    // Storm warning
    if (marsLightning > 0.3f) {
        float warnPulse = 0.5f + 0.5f * sinf(currentTime * 8.0f);
        glColor4f(1.0f, 0.3f, 0.1f, warnPulse * 0.9f);
        drawString(GLUT_BITMAP_HELVETICA_12, (float)(w / 2 - 40), (float)(h - 30), "! STORM ACTIVE !");
    }

    // Altitude readout
    char altBuf[64];
    float altMeters = marsCamY * 100.0f;
    if (altMeters > 1000.0f)
        sprintf(altBuf, "ALT  %.1f km", altMeters / 1000.0f);
    else
        sprintf(altBuf, "ALT  %.0f m", altMeters);
    glColor4f(1.0f, 0.8f, 0.55f, 0.85f);
    drawString(GLUT_BITMAP_HELVETICA_18, (float)(w - 195), (float)(h - 28), altBuf);

    // Speed
    char spdBuf[32];
    sprintf(spdBuf, "SPD %.0f", marsMoveSpeed * 100.0f);
    glColor4f(1.0f, 0.75f, 0.5f, 0.60f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w - 290), (float)(h - 30), spdBuf);

    // Compass
    float heading = wrapAngle360(marsCamYaw);
    const char* compassDir;
    if (heading < 22.5f || heading >= 337.5f) compassDir = "N";
    else if (heading < 67.5f)   compassDir = "NE";
    else if (heading < 112.5f)  compassDir = "E";
    else if (heading < 157.5f)  compassDir = "SE";
    else if (heading < 202.5f)  compassDir = "S";
    else if (heading < 247.5f)  compassDir = "SW";
    else if (heading < 292.5f)  compassDir = "W";
    else                         compassDir = "NW";
    char compassBuf[32];
    sprintf(compassBuf, "%s  %.0f", compassDir, heading);
    glColor4f(1.0f, 0.7f, 0.5f, 0.65f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w / 2 - 20), (float)(h - 30), compassBuf);

    float gH = getMarsTerrainHeight(marsCamX, marsCamZ);
    float gHx = getMarsTerrainHeight(marsCamX + 1.0f, marsCamZ);
    float gHz = getMarsTerrainHeight(marsCamX, marsCamZ + 1.0f);
    float nx = gH - gHx;
    float nz = gH - gHz;
    float nlen = sqrtf(nx * nx + 1.0f + nz * nz);
    float gSlope = 1.0f - (1.0f / nlen);
    const char* biome = getMarsBiomeName(gH, gSlope);
    glColor4f(1.0f, 0.65f, 0.4f, 0.60f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w / 2 + 50), (float)(h - 30), biome);

    // Altitude bar (right side)
    float barX = (float)(w - 22), barY = 80.0f, barH = (float)(h - 160), barW = 8.0f;
    float altNorm = clampf(marsCamY / 50.0f, 0.0f, 1.0f);

    glColor4f(0.08f, 0.03f, 0.01f, 0.50f);
    glBegin(GL_QUADS);
    glVertex2f(barX, barY); glVertex2f(barX + barW, barY);
    glVertex2f(barX + barW, barY + barH); glVertex2f(barX, barY + barH);
    glEnd();

    float fillH = altNorm * barH;
    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.5f, 0.2f, 0.60f);
    glVertex2f(barX + 1, barY); glVertex2f(barX + barW - 1, barY);
    glColor4f(0.8f, 0.3f, 0.1f, 0.60f);
    glVertex2f(barX + barW - 1, barY + fillH); glVertex2f(barX + 1, barY + fillH);
    glEnd();

    glColor4f(1.0f, 0.8f, 0.5f, 0.80f);
    glBegin(GL_QUADS);
    glVertex2f(barX - 3, barY + fillH - 2); glVertex2f(barX + barW + 3, barY + fillH - 2);
    glVertex2f(barX + barW + 3, barY + fillH + 2); glVertex2f(barX - 3, barY + fillH + 2);
    glEnd();

    // Crosshair
    float ccx = w / 2.0f, ccy = h / 2.0f;
    glColor4f(1.0f, 0.6f, 0.3f, 0.18f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(ccx - 14, ccy); glVertex2f(ccx - 5, ccy);
    glVertex2f(ccx + 5, ccy);  glVertex2f(ccx + 14, ccy);
    glVertex2f(ccx, ccy - 14); glVertex2f(ccx, ccy - 5);
    glVertex2f(ccx, ccy + 5);  glVertex2f(ccx, ccy + 14);
    glEnd();
    glPointSize(2.0f);
    glColor4f(1.0f, 0.6f, 0.3f, 0.30f);
    glBegin(GL_POINTS); glVertex2f(ccx, ccy); glEnd();

    // Bottom bar
    drawGlassPanel(0, 0, (float)w, 30.0f, 0.75f);
    glColor4f(1.0f, 0.7f, 0.5f, 0.55f);
    drawString(GLUT_BITMAP_HELVETICA_12, 18.0f, 10.0f,
        "WASD: Move  |  Mouse: Look  |  Scroll: Altitude  |  M / ESC: Return to Space  |  H: Toggle HUD");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

static void resetTerrainCamera() {
    tCamX = targetTCamX = 0.0f;
    tCamZ = targetTCamZ = 0.0f;
    tCamY = targetTCamY = 25.0f;
    tCamYaw = targetTCamYaw = 0.0f;
    tCamPitch = targetTCamPitch = -15.0f;
    keyW = keyA = keyS = keyD = false;
    terrainMoveSpeed = 0.0f;
    tCamVelX = tCamVelZ = tCamVelY = 0.0f;
    // Reset mouse smoothing buffer
    for (int i = 0; i < MOUSE_SMOOTH_SAMPLES; i++) {
        mouseSmoothX[i] = 0.0f;
        mouseSmoothY[i] = 0.0f;
    }
    mouseSmoothIdx = 0;
}

// Smooth damp helper (critically damped spring)
static float smoothDamp(float current, float target, float& velocity, float smoothTime, float dt) {
    float omega = 2.0f / fmaxf(smoothTime, 0.0001f);
    float x = omega * dt;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    float temp = (velocity + omega * change) * dt;
    velocity = (velocity - omega * temp) * exp;
    return target + (change + temp) * exp;
}

static void updateTerrainCamera() {
    float moveX = 0.0f, moveZ = 0.0f;
    if (keyW || keyA || keyS || keyD) {
        // Frame-rate independent movement.
        float speed = 45.0f * timeSpeed * deltaTime;
        float yawRad = tCamYaw * DEG2RAD;

        // Horizontal-only movement (no pitch influence)
        float fx = sinf(yawRad);
        float fz = cosf(yawRad);
        float rx = cosf(yawRad);
        float rz = -sinf(yawRad);

        if (keyW) { moveX += fx * speed; moveZ += fz * speed; }
        if (keyS) { moveX -= fx * speed; moveZ -= fz * speed; }
        if (keyA) { moveX -= rx * speed; moveZ -= rz * speed; }
        if (keyD) { moveX += rx * speed; moveZ += rz * speed; }

        targetTCamX += moveX;
        targetTCamZ += moveZ;
    }

    float instantSpeed = (deltaTime > 0.0001f)
        ? sqrtf(moveX * moveX + moveZ * moveZ) / deltaTime
        : 0.0f;
    float speedSmooth = 1.0f - expf(-8.0f * deltaTime);
    terrainMoveSpeed += (instantSpeed - terrainMoveSpeed) * speedSmooth;

    // Store velocity for predictive streaming
    tCamVelX = (targetTCamX - tCamX) * 6.0f;
    tCamVelZ = (targetTCamZ - tCamZ) * 6.0f;

    float posSmooth = 1.0f - expf(-9.0f * deltaTime);
    float lookSmooth = 1.0f - expf(-12.0f * deltaTime);

    tCamX += (targetTCamX - tCamX) * posSmooth;
    tCamY += (targetTCamY - tCamY) * posSmooth;
    tCamZ += (targetTCamZ - tCamZ) * posSmooth;
    tCamYaw += (targetTCamYaw - tCamYaw) * lookSmooth;
    tCamPitch += (targetTCamPitch - tCamPitch) * lookSmooth;

    targetTCamPitch = clampf(targetTCamPitch, -85.0f, 85.0f);
    tCamPitch = clampf(tCamPitch, -85.0f, 85.0f);

    // Terrain streaming control
    if (inTerrainMode) {
        updateEarthTerrainStreaming();
    }
    else if (inVenusTerrainMode) {
        updateVenusTerrainStreaming();
    }

    // Ground collision
    float groundH = inVenusTerrainMode
        ? sampleVenusTerrainHeight(tCamX, tCamZ)
        : sampleTerrainHeightV2(tCamX, tCamZ);

    float minH = groundH + (inVenusTerrainMode ? 1.6f : 2.0f);
    if (minH < 2.0f) minH = 2.0f;
    if (tCamY < minH) tCamY = minH;
    if (targetTCamY < minH) targetTCamY = minH;
    targetTCamY = clampf(targetTCamY, 2.0f, 75.0f);
}

static float getMarsTerrainHeight(float wx, float wz) {
    float warpX = fbmNoise(wx * 0.005f + 150.0f, wz * 0.005f + 150.0f, 3) * 20.0f;
    float warpZ = fbmNoise(wx * 0.005f + 300.0f, wz * 0.005f + 300.0f, 3) * 20.0f;
    float wx2 = wx + warpX;
    float wz2 = wz + warpZ;

    float base = fbmNoise(wx2 * 0.002f, wz2 * 0.002f, 5, 0.45f) * 40.0f - 8.0f;

    // Craters
    float crater = 0.0f;
    float cx = fmodf(wx2 * 0.012f + 100.0f, 1.0f) - 0.5f;
    float cz = fmodf(wz2 * 0.012f + 100.0f, 1.0f) - 0.5f;
    float cDist = sqrtf(cx * cx + cz * cz) * 2.0f;
    if (cDist < 1.0f) {
        float rim = smoothstepf(0.6f, 0.8f, cDist) - smoothstepf(0.8f, 1.0f, cDist);
        float floor = smoothstepf(0.0f, 0.6f, cDist);
        crater = rim * 6.0f - (1.0f - floor) * 12.0f;
    }

    float mountains = ridgeNoise(wx2 * 0.004f, wz2 * 0.004f, 6) * 45.0f;
    float dunes = sinf(wx * 0.15f + fbmNoise(wx * 0.05f, wz * 0.05f, 2) * 2.0f) * 1.5f;
    float detail = fbmNoise(wx * 0.08f, wz * 0.08f, 4, 0.5f) * 2.0f;

    return base + crater + mountains + dunes + detail;
}

static void getMarsTerrainColor(float h, float slope, float& cr, float& cg, float& cb) {
    if (h < -4.0f) { // Crater basin
        cr = 0.40f; cg = 0.15f; cb = 0.08f;
    }
    else if (h < 6.0f) { // Plains / dunes
        cr = 0.60f; cg = 0.25f; cb = 0.12f;
    }
    else if (h < 18.0f) { // Highlands
        cr = 0.52f; cg = 0.22f; cb = 0.10f;
    }
    else { // Peaks
        cr = 0.38f; cg = 0.14f; cb = 0.07f;
    }

    if (slope > 0.3f) {
        cr *= 0.8f; cg *= 0.8f; cb *= 0.8f;
    }

    float ao = 1.0f - clampf(slope * 0.5f, 0.0f, 0.3f);
    cr *= ao; cg *= ao; cb *= ao;
}

static float sampleMarsTerrainHeight(float wx, float wz) {
    return getMarsTerrainHeight(wx, wz);
}

static MarsChunk* generateMarsChunk(int cx, int cz) {
    MarsChunk* chunk = new MarsChunk();
    chunk->cx = cx;
    chunk->cz = cz;
    chunk->valid = true;

    float baseWX = cx * MT_CHUNK_SIZE - MT_CHUNK_SIZE * 0.5f;
    float baseWZ = cz * MT_CHUNK_SIZE - MT_CHUNK_SIZE * 0.5f;

    int cells = MT_CHUNK_CELLS;
    int gridPoints = cells + 1;
    int total = gridPoints * gridPoints;

    std::vector<float> cH(total);
    std::vector<float> cNX(total), cNY(total), cNZ(total);
    std::vector<float> cCR(total), cCG(total), cCB(total);

    // Pass 1: cache heights once per vertex.
    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            float wx = baseWX + i * MT_SPACING;
            float wz = baseWZ + j * MT_SPACING;
            cH[j * gridPoints + i] = getMarsTerrainHeight(wx, wz);
        }
    }

    // Pass 2: normals/colors/details using cached heights when possible.
    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            int idx = j * gridPoints + i;
            float wx = baseWX + i * MT_SPACING;
            float wz = baseWZ + j * MT_SPACING;
            float h = cH[idx];

            float hL = (i > 0) ? cH[j * gridPoints + (i - 1)] : getMarsTerrainHeight(wx - MT_SPACING, wz);
            float hR = (i < cells) ? cH[j * gridPoints + (i + 1)] : getMarsTerrainHeight(wx + MT_SPACING, wz);
            float hD = (j > 0) ? cH[(j - 1) * gridPoints + i] : getMarsTerrainHeight(wx, wz - MT_SPACING);
            float hU = (j < cells) ? cH[(j + 1) * gridPoints + i] : getMarsTerrainHeight(wx, wz + MT_SPACING);

            float nx = (hL - hR) / (2.0f * MT_SPACING);
            float nz = (hD - hU) / (2.0f * MT_SPACING);
            float len = sqrtf(nx * nx + 1.0f + nz * nz);

            cNX[idx] = nx / len;
            cNY[idx] = 1.0f / len;
            cNZ[idx] = nz / len;

            float slope = 1.0f - cNY[idx];
            getMarsTerrainColor(h, slope, cCR[idx], cCG[idx], cCB[idx]);

            if (i >= 2 && i <= cells - 2 && j >= 2 && j <= cells - 2) {
                if ((i % 4 == 0) && (j % 4 == 0)) {
                    float rng = (noiseHash(cx * 1000 + i * 17 + 7, cz * 1000 + j * 31 + 13) + 1.0f) * 0.5f;
                    if (slope < 0.2f && rng > 0.85f) {
                        MarsRock r;
                        r.x = wx;
                        r.z = wz;
                        r.y = h;
                        r.size = 0.8f + (1.0f - rng) * 3.5f;
                        r.shape = (int)(rng * 100) % 3;
                        r.rotY = rng * 360.0f;
                        r.rotAxis = rng;
                        float rv = noiseHash(i * 7, j * 11) * 0.1f;
                        r.r = 0.6f + rv;
                        r.g = 0.3f + rv;
                        r.b = 0.15f + rv;
                        chunk->rocks.push_back(r);
                    }
                }
            }
        }
    }

    for (int lod = 0; lod < MT_LOD_COUNT; lod++) {
        int step = MT_LOD_STEP[lod];
        chunk->lists[lod] = glGenLists(1);
        glNewList(chunk->lists[lod], GL_COMPILE);

        for (int j = 0; j < cells; j += step) {
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= cells; i += step) {
                for (int dj = 1; dj >= 0; dj--) {
                    int cj = clampI(j + dj * step, 0, cells);
                    int idx = cj * gridPoints + i;
                    float px = baseWX + i * MT_SPACING;
                    float pz = baseWZ + cj * MT_SPACING;
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


static bool marsChunkExists(int testCX, int testCZ) {
    return activeMarsChunkKeys.find(makeChunkKey(testCX, testCZ)) != activeMarsChunkKeys.end();
}

static void addMarsChunk(int cx, int cz) {
    long long key = makeChunkKey(cx, cz);
    if (activeMarsChunkKeys.find(key) != activeMarsChunkKeys.end()) {
        return;
    }

    MarsChunk* chunk = generateMarsChunk(cx, cz);
    activeMarsChunks.push_back(chunk);
    activeMarsChunkKeys.insert(key);
}

static void updateMarsTerrainStreaming() {
    if (!inMarsMode) return;

    int playerCX = (int)roundf(marsCamX / MT_CHUNK_SIZE);
    int playerCZ = (int)roundf(marsCamZ / MT_CHUNK_SIZE);

    for (auto it = activeMarsChunks.begin(); it != activeMarsChunks.end(); ) {
        MarsChunk* chunk = *it;
        if (abs(chunk->cx - playerCX) > MT_VIEW_CHUNKS || abs(chunk->cz - playerCZ) > MT_VIEW_CHUNKS) {
            for (int i = 0; i < MT_LOD_COUNT; i++) {
                if (chunk->lists[i] != 0) glDeleteLists(chunk->lists[i], 1);
            }

            activeMarsChunkKeys.erase(makeChunkKey(chunk->cx, chunk->cz));
            delete chunk;
            it = activeMarsChunks.erase(it);
        }
        else {
            ++it;
        }
    }

    bool loadedThisFrame = false;
    for (int ring = 0; ring <= MT_VIEW_CHUNKS && !loadedThisFrame; ring++) {
        for (int dz = -ring; dz <= ring && !loadedThisFrame; dz++) {
            for (int dx = -ring; dx <= ring && !loadedThisFrame; dx++) {
                if (abs(dx) == ring || abs(dz) == ring) {
                    int testCX = playerCX + dx;
                    int testCZ = playerCZ + dz;

                    if (!marsChunkExists(testCX, testCZ)) {
                        addMarsChunk(testCX, testCZ);
                        loadedThisFrame = true;
                    }
                }
            }
        }
    }
}

static void buildMarsTerrain() {
    int playerCX = (int)roundf(marsCamX / MT_CHUNK_SIZE);
    int playerCZ = (int)roundf(marsCamZ / MT_CHUNK_SIZE);

    for (int dz = -2; dz <= 2; dz++) {
        for (int dx = -2; dx <= 2; dx++) {
            addMarsChunk(playerCX + dx, playerCZ + dz);
        }
    }
    mtBuilt = true;
}

static void cleanupMarsTerrain() {
    for (MarsChunk* chunk : activeMarsChunks) {
        if (!chunk) continue;
        for (int i = 0; i < MT_LOD_COUNT; i++) {
            if (chunk->lists[i] != 0) glDeleteLists(chunk->lists[i], 1);
        }
        delete chunk;
    }
    activeMarsChunks.clear();
    activeMarsChunkKeys.clear();
    mtBuilt = false;
    marsParticles.clear();
}

static void drawMarsTerrainChunks() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat tSpec[] = { 0.1f, 0.05f, 0.02f, 1.0f };
    GLfloat tSh[] = { 8.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, tSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, tSh);

    float forwardX = sinf(marsCamYaw * DEG2RAD);
    float forwardZ = cosf(marsCamYaw * DEG2RAD);

    for (MarsChunk* chunk : activeMarsChunks) {
        float cx = chunk->cx * MT_CHUNK_SIZE;
        float cz = chunk->cz * MT_CHUNK_SIZE;

        float dx = cx - marsCamX;
        float dz = cz - marsCamZ;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist > (MT_VIEW_CHUNKS + 1) * MT_CHUNK_SIZE) continue;

        if (dist > MT_CHUNK_SIZE * 1.5f) {
            float toDirX = dx / dist;
            float toDirZ = dz / dist;
            float dot = toDirX * forwardX + toDirZ * forwardZ;
            if (dot < -0.3f) continue;
        }

        int lod;
        if (dist < 150.0f)      lod = 0;
        else if (dist < 300.0f) lod = 1;
        else if (dist < 600.0f) lod = 2;
        else                    lod = 3;

        glCallList(chunk->lists[lod]);
    }
}

static void drawMarsRocks() {
    float viewDist = 300.0f;
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    for (MarsChunk* chunk : activeMarsChunks) {
        float cx = chunk->cx * MT_CHUNK_SIZE;
        float cz = chunk->cz * MT_CHUNK_SIZE;
        float cDist = sqrtf(powf(cx - marsCamX, 2) + powf(cz - marsCamZ, 2));

        if (cDist > viewDist + MT_CHUNK_SIZE * 0.8f) continue;

        for (const auto& r : chunk->rocks) {
            float dx = r.x - marsCamX;
            float dz = r.z - marsCamZ;
            float dist2 = dx * dx + dz * dz;
            if (dist2 > viewDist * viewDist) continue;

            glPushMatrix();
            glTranslatef(r.x, r.y, r.z);
            glRotatef(r.rotY, 0, 1, 0);
            glRotatef(r.rotAxis * 360.0f, 1, 0, 1);
            glColor3f(r.r, r.g, r.b);
            float s = r.size;
            if (r.shape == 0) {
                glScalef(1.0f, 0.6f, 0.8f);
                glutSolidSphere(s, 6, 6);
            }
            else if (r.shape == 1) {
                glScalef(0.8f, 1.0f, 0.6f);
                glutSolidSphere(s, 5, 5);
            }
            else {
                glScalef(1.2f, 0.4f, 1.0f);
                glutSolidSphere(s, 7, 5);
            }
            glPopMatrix();
        }
    }
}

static void updateMarsCamera() {
    float moveX = 0.0f, moveZ = 0.0f, moveY = 0.0f;
    if (keyW || keyA || keyS || keyD) {
        // Frame-rate independent movement.
        float speed = 55.0f * timeSpeed * deltaTime;
        float yawRad = marsCamYaw * DEG2RAD;
        float pitRad = marsCamPitch * DEG2RAD;

        float fx = sinf(yawRad) * cosf(pitRad);
        float fy = sinf(pitRad);
        float fz = cosf(yawRad) * cosf(pitRad);

        float rx = cosf(yawRad);
        float rz = -sinf(yawRad);

        if (keyW) { moveX += fx * speed; moveY += fy * speed; moveZ += fz * speed; }
        if (keyS) { moveX -= fx * speed; moveY -= fy * speed; moveZ -= fz * speed; }
        if (keyA) { moveX -= rx * speed; moveZ -= rz * speed; }
        if (keyD) { moveX += rx * speed; moveZ += rz * speed; }

        targetMarsCamX += moveX;
        targetMarsCamY += moveY;
        targetMarsCamZ += moveZ;
    }

    float instantSpeed = (deltaTime > 0.0001f)
        ? sqrtf(moveX * moveX + moveZ * moveZ + moveY * moveY) / deltaTime
        : 0.0f;

    float speedSmooth = 1.0f - expf(-8.0f * deltaTime);
    float posSmooth = 1.0f - expf(-9.0f * deltaTime);
    float lookSmooth = 1.0f - expf(-12.0f * deltaTime);

    marsMoveSpeed += (instantSpeed - marsMoveSpeed) * speedSmooth;

    marsCamX += (targetMarsCamX - marsCamX) * posSmooth;
    marsCamY += (targetMarsCamY - marsCamY) * posSmooth;
    marsCamZ += (targetMarsCamZ - marsCamZ) * posSmooth;
    marsCamYaw += (targetMarsCamYaw - marsCamYaw) * lookSmooth;
    marsCamPitch += (targetMarsCamPitch - marsCamPitch) * lookSmooth;

    targetMarsCamPitch = clampf(targetMarsCamPitch, -85.0f, 85.0f);
    marsCamPitch = clampf(marsCamPitch, -85.0f, 85.0f);

    float groundH = sampleMarsTerrainHeight(marsCamX, marsCamZ);
    float minH = groundH + 2.0f;
    if (marsCamY < minH) marsCamY = minH;
    if (targetMarsCamY < minH) targetMarsCamY = minH;

    if (marsLightning > 0.1f) {
        marsCamShakeX = (noiseHash(glutGet(GLUT_ELAPSED_TIME), 0) * 2.0f - 1.0f) * marsLightning * 0.3f;
        marsCamShakeY = (noiseHash(0, glutGet(GLUT_ELAPSED_TIME)) * 2.0f - 1.0f) * marsLightning * 0.3f;
    }
    else {
        marsCamShakeX = marsCamShakeY = 0.0f;
    }

    updateMarsTerrainStreaming();
}

static void startTransitionToTerrain() {
    if (transActive) return;
    transActive = true;
    transToTerrain = true;
    transToVenusTerrain = false;
    transToMars = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 2;
}

static void startTransitionToMars() {
    if (transActive) return;
    transActive = true;
    transToMars = true;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 3;
}

static void startTransitionToNeptune() {
    if (transActive) return;
    transActive = true;
    transToNeptune = true;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 7;
}

static void startTransitionToVenusTerrain() {
    if (transActive) return;
    transActive = true;
    transToVenusTerrain = true;
    transToTerrain = false;
    transToMars = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 1;
}

static void startTransitionToSpace() {
    if (transActive) return;
    transActive = true;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    fpsMouseActive = false;
    keyW = keyA = keyS = keyD = false;
    glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
}

static void updateTransition() {
    if (!transActive) return;

    if (!transPastMid) {
        transAlpha += 1.35f * deltaTime;
        if (transAlpha >= 1.0f) {
            transAlpha = 1.0f;
            transPastMid = true;

            if (transToVenusTerrain) {
                resetTerrainCamera();
                tCamY = targetTCamY = 22.0f;
                tCamPitch = targetTCamPitch = -12.0f;
                if (!vtBuilt) buildVenusTerrainV2();
                inVenusTerrainMode = true;
                inTerrainMode = false;
                inMarsMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToTerrain) {
                resetTerrainCamera();
                if (!etBuilt) buildEarthTerrain();
                inTerrainMode = true;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToMars) {
                marsCamX = targetMarsCamX = 0.0f;
                marsCamY = 250.0f;
                targetMarsCamY = 20.0f;
                marsCamZ = targetMarsCamZ = 0.0f;
                marsCamYaw = targetMarsCamYaw = 0.0f;
                marsCamPitch = targetMarsCamPitch = -15.0f;
                if (!mtBuilt) buildMarsTerrain();
                inMarsMode = true;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToNeptune) {
                if (!nepSnowInit) initNeptuneSnow();
                resetNeptuneCamera();
                inNeptuneMode = true;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else {
                if (inTerrainMode) cleanupEarthTerrain();
                if (inVenusTerrainMode) cleanupVenusTerrain();
                if (inMarsMode) cleanupMarsTerrain();
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inNeptuneMode = false;
                cameraMode = CAMERA_FOCUS;
                focusPlanet(lastExploredPlanet);
            }
        }
    }
    else {
        transAlpha -= 1.15f * deltaTime;
        if (transAlpha <= 0.0f) {
            transAlpha = 0.0f;
            transActive = false;
            transPastMid = false;
        }
    }
}

static void drawTransitionOverlay() {
    if (transAlpha <= 0.001f) return;

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

    float eased = smoothstepf(0.0f, 1.0f, transAlpha);

    glBegin(GL_QUADS);
    if (transToMars || (transActive && !transToTerrain && !transToVenusTerrain && !transToNeptune && lastExploredPlanet == 3)) {
        glColor4f(0.18f, 0.06f, 0.02f, eased * 0.60f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.30f, 0.10f, 0.04f, eased * 0.60f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToVenusTerrain || (transActive && !transToTerrain && !transToMars && !transToNeptune && lastExploredPlanet == 1)) {
        glColor4f(0.28f, 0.10f, 0.02f, eased * 0.62f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.78f, 0.28f, 0.07f, eased * 0.62f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToNeptune || (transActive && !transToTerrain && !transToVenusTerrain && !transToMars && lastExploredPlanet == 7)) {
        glColor4f(0.01f, 0.03f, 0.14f, eased * 0.62f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.02f, 0.12f, 0.34f, eased * 0.62f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else {
        glColor4f(0.02f, 0.06f, 0.18f, eased * 0.60f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.04f, 0.10f, 0.30f, eased * 0.60f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    glEnd();

    float cx = w / 2.0f, cy = h / 2.0f;
    float maxR = sqrtf((float)(w * w + h * h)) * 0.6f;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.01f, 0.04f, 0.12f, 0.0f);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 48; i++) {
        float angle = TWO_PI * (float)i / 48.0f;
        if (transToMars || (transActive && !transToTerrain && !transToVenusTerrain && !transToNeptune && lastExploredPlanet == 3)) {
            glColor4f(0.12f, 0.04f, 0.01f, eased * 0.45f);
        }
        else if (transToVenusTerrain || (transActive && !transToTerrain && !transToMars && !transToNeptune && lastExploredPlanet == 1)) {
            glColor4f(0.22f, 0.07f, 0.01f, eased * 0.48f);
        }
        else if (transToNeptune || (transActive && !transToTerrain && !transToVenusTerrain && !transToMars && lastExploredPlanet == 7)) {
            glColor4f(0.01f, 0.04f, 0.16f, eased * 0.48f);
        }
        else {
            glColor4f(0.01f, 0.04f, 0.12f, eased * 0.45f);
        }
        glVertex2f(cx + maxR * cosf(angle), cy + maxR * sinf(angle));
    }
    glEnd();

    if (transAlpha > 0.4f) {
        float textAlpha = clampf((transAlpha - 0.4f) * 3.5f, 0.0f, 1.0f);

        if (transToVenusTerrain) {
            glColor4f(1.0f, 0.70f, 0.30f, textAlpha * 0.92f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 125.0f, h / 2.0f + 10.0f, "Entering Venus Atmosphere...");
            glColor4f(1.0f, 0.55f, 0.20f, textAlpha * 0.55f);
            const char* venusSub = (!vtBuilt) ? "Generating infinite volcanic chunks..." : "Descending to volcanic surface";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 112.0f, h / 2.0f - 15.0f, venusSub);
        }
        else if (transToTerrain) {
            glColor4f(0.55f, 0.80f, 1.0f, textAlpha * 0.90f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 95.0f, h / 2.0f + 10.0f, "Entering Earth's Atmosphere...");
            glColor4f(0.45f, 0.65f, 0.85f, textAlpha * 0.50f);
            const char* earthSub = (!etBuilt) ? "Generating Earth terrain chunks..." : "Initiating surface descent";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 105.0f, h / 2.0f - 15.0f, earthSub);
        }
        else if (transToMars) {
            glColor4f(1.0f, 0.6f, 0.3f, textAlpha * 0.90f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 110.0f, h / 2.0f + 10.0f, "Entering Mars Surface...");
            glColor4f(0.85f, 0.45f, 0.25f, textAlpha * 0.50f);
            const char* marsSub = (!mtBuilt) ? "Generating Mars terrain chunks..." : "Descending to the red terrain";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 105.0f, h / 2.0f - 15.0f, marsSub);
        }
        else if (transToNeptune) {
            glColor4f(0.45f, 0.72f, 1.0f, textAlpha * 0.92f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 125.0f, h / 2.0f + 10.0f, "Entering Neptune Interior...");
            glColor4f(0.35f, 0.58f, 0.95f, textAlpha * 0.55f);
            const char* neptuneSub = (!nepSnowInit) ? "Generating Neptune snow field..." : "Descending into the icy storm";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 110.0f, h / 2.0f - 15.0f, neptuneSub);
        }
        else {
            glColor4f(0.55f, 0.80f, 1.0f, textAlpha * 0.90f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 85.0f, h / 2.0f + 10.0f, "Ascending to Orbit...");
            glColor4f(0.45f, 0.65f, 0.85f, textAlpha * 0.50f);
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 70.0f, h / 2.0f - 15.0f, "Returning to solar system view");
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void drawVignette(float strength) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    float cx = w / 2.0f, cy = h / 2.0f;
    float maxR = sqrtf((float)(w * w + h * h)) * 0.58f;

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

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0, 0, 0, 0);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 64; i++) {
        float angle = TWO_PI * (float)i / 64.0f;
        glColor4f(0, 0, 0, strength);
        glVertex2f(cx + maxR * cosf(angle), cy + maxR * sinf(angle));
    }
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void drawScreenGlow(float cx, float cy, float r, float alpha, float cr, float cg, float cb) {
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(cr, cg, cb, alpha);
    glVertex2f(cx, cy);
    glColor4f(cr, cg, cb, 0.0f);
    for (int i = 0; i <= 24; i++) {
        float a = TWO_PI * (float)i / 24.0f;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

static void drawLensFlare() {
    if (inTerrainMode || inVenusTerrainMode || inMarsMode || inNeptuneMode || cameraMode == CAMERA_EXPLORE) return;

    GLdouble mv[16], pj[16], sx, sy, sz;
    GLint vp[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    glGetDoublev(GL_PROJECTION_MATRIX, pj);
    glGetIntegerv(GL_VIEWPORT, vp);
    gluProject(0, 0, 0, mv, pj, vp, &sx, &sy, &sz);

    if (sz < 0 || sz > 1) return;
    int w = vp[2], h = vp[3];
    if (sx < -100 || sx > w + 100 || sy < -100 || sy > h + 100) return;

    float fcx = w / 2.0f, fcy = h / 2.0f;
    float fdx = fcx - (float)sx, fdy = fcy - (float)sy;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    drawScreenGlow((float)sx, (float)sy, 50.0f, 0.06f, 1.0f, 0.9f, 0.6f);
    drawScreenGlow((float)sx, (float)sy, 25.0f, 0.12f, 1.0f, 0.95f, 0.8f);

    for (int i = 1; i <= 6; i++) {
        float t = i * 0.16f;
        float fx = (float)sx + fdx * t;
        float fy = (float)sy + fdy * t;
        float size = 12.0f + i * 5.0f;
        float a = 0.035f - i * 0.004f;
        if (a < 0.005f) a = 0.005f;
        float r = (i % 2 == 0) ? 0.6f : 0.7f;
        float g = 0.7f;
        float b = (i % 2 == 0) ? 1.0f : 0.9f;
        drawScreenGlow(fx, fy, size, a, r, g, b);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}


static void updateMarsParticles() {
    if (!inMarsMode) return;

    while (marsParticles.size() < 400) {
        MarsParticle p;
        float a = (rand() % 360) * DEG2RAD;
        float d = 20.0f + (rand() % 250);
        float h = (rand() % 200) - 100.0f;
        p.x = marsCamX + cosf(a) * d;
        p.z = marsCamZ + sinf(a) * d;
        p.y = marsCamY + h;
        p.vx = ((rand() % 100) - 50) * 0.02f;
        p.vy = ((rand() % 100) - 50) * 0.01f + 0.5f;
        p.vz = ((rand() % 100) - 50) * 0.02f;
        p.life = 1.0f;
        marsParticles.push_back(p);
    }

    for (auto it = marsParticles.begin(); it != marsParticles.end(); ) {
        it->x += it->vx * timeSpeed * 2.0f;
        it->y += it->vy * timeSpeed * 2.0f;
        it->z += it->vz * timeSpeed * 2.0f;

        it->vx += sinf(currentTime + it->y * 0.05f) * 0.05f;
        it->vz += cosf(currentTime + it->x * 0.05f) * 0.05f;

        it->life -= 0.005f * timeSpeed;
        float dx = it->x - marsCamX;
        float dy = it->y - marsCamY;
        float dz = it->z - marsCamZ;
        if (it->life <= 0 || sqrtf(dx * dx + dy * dy + dz * dz) > 350.0f) {
            it = marsParticles.erase(it);
        }
        else {
            ++it;
        }
    }
}

static void drawMarsParticles() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (const auto& p : marsParticles) {
        float alpha = p.life;
        if (p.life > 0.8f) alpha = (1.0f - p.life) * 5.0f;
        glColor4f(1.0f, 0.6f, 0.2f, alpha);
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
}

static void drawMarsVolumetricClouds() {
    float cloudSize = 800.0f;
    int cloudRes = 24;
    float cellSize = cloudSize * 2.0f / cloudRes;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    float cxBase = floorf(marsCamX / cellSize) * cellSize;
    float czBase = floorf(marsCamZ / cellSize) * cellSize;

    for (int layer = -2; layer <= 2; layer++) {
        float alt = layer * 150.0f + 50.0f * sinf(currentTime * 0.5f + layer);
        float dist = fabsf(marsCamY - alt);
        if (dist > 500.0f) continue;

        float baseAlpha = clampf(1.0f - dist / 500.0f, 0.0f, 1.0f) * 0.6f;
        if (dist < 30.0f) baseAlpha *= dist / 30.0f;

        for (int j = 0; j < cloudRes; j++) {
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= cloudRes; i++) {
                for (int dj = 1; dj >= 0; dj--) {
                    float cx = cxBase - cloudSize + i * cellSize;
                    float cz = czBase - cloudSize + (j + dj) * cellSize;

                    float windT = currentTime * (1.5f + layer * 0.2f);
                    float density = fbmNoise(cx * 0.003f + windT, cz * 0.003f + windT, 3);
                    density = smoothstepf(-0.2f, 0.6f, density);

                    float alpha = density * baseAlpha;

                    float r = lerpf(0.9f, 1.0f, density);
                    float g = lerpf(0.3f, 0.6f, density);
                    float b = lerpf(0.1f, 0.2f, density);

                    if (marsLightning > 0) {
                        r += marsLightning * 0.5f;
                        g += marsLightning * 0.4f;
                        b += marsLightning * 0.2f;
                    }

                    glColor4f(r, g, b, alpha);
                    glVertex3f(cx, alt + density * 50.0f, cz);
                }
            }
            glEnd();
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void displayMarsScene() {
    glClearColor(0.18f, 0.06f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    applyPerspectiveIfNeeded(60.0f, 0.15f, 2000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawR = marsCamYaw * DEG2RAD;
    float pitR = marsCamPitch * DEG2RAD;
    float lookX = marsCamX + cosf(pitR) * sinf(yawR);
    float lookY = marsCamY + sinf(pitR);
    float lookZ = marsCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(marsCamX + marsCamShakeX, marsCamY + marsCamShakeY, marsCamZ, lookX + marsCamShakeX, lookY + marsCamShakeY, lookZ, 0, 1, 0);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    float sunDirX = cosf(MARS_SUN_EL) * sinf(MARS_SUN_AZ);
    float sunDirY = sinf(MARS_SUN_EL);
    float sunDirZ = cosf(MARS_SUN_EL) * cosf(MARS_SUN_AZ);
    GLfloat sunDir[] = { sunDirX, sunDirY, sunDirZ, 0.0f };
    GLfloat sunAmb[] = { 0.15f, 0.05f, 0.02f, 1.0f };
    GLfloat sunDif[] = { 1.2f, 0.6f, 0.3f, 1.0f };
    GLfloat sunSpc[] = { 0.8f, 0.4f, 0.2f, 1.0f };
    if (marsLightning > 0) {
        sunDif[0] += marsLightning * 0.8f;
        sunDif[1] += marsLightning * 0.6f;
        sunDif[2] += marsLightning * 0.4f;
    }
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);

    GLfloat skyDir[] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat skyAmb[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat skyDif[] = { 0.15f, 0.05f, 0.02f, 1.0f };
    GLfloat skySpc[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, skyDir);
    glLightfv(GL_LIGHT1, GL_AMBIENT, skyAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, skyDif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, skySpc);

    glEnable(GL_FOG);
    GLfloat fogCol[] = { 0.25f, 0.10f, 0.04f, 1.0f };
    if (marsLightning > 0) {
        fogCol[0] += marsLightning * 0.4f;
        fogCol[1] += marsLightning * 0.3f;
        fogCol[2] += marsLightning * 0.2f;
    }
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.003f);

    if (marsGasTexture) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, marsGasTexture);

        glPushMatrix();
        glTranslatef(marsCamX, marsCamY, marsCamZ);

        glPushMatrix();
        glRotatef(currentTime * 2.0f, 0, 1, 0);
        glColor4f(1.0f, 0.8f, 0.6f, 0.6f);
        GLUquadric* q = gluNewQuadric();
        gluQuadricTexture(q, GL_TRUE);
        gluQuadricOrientation(q, GLU_INSIDE);
        gluSphere(q, 800.0f, 32, 32);

        glRotatef(currentTime * -3.5f, 1, 1, 0);
        glColor4f(1.0f, 0.6f, 0.4f, 0.4f);
        gluSphere(q, 600.0f, 32, 32);

        glRotatef(currentTime * 5.0f, 0, 1, 1);
        glColor4f(0.8f, 0.4f, 0.2f, 0.3f);
        gluSphere(q, 400.0f, 32, 32);

        gluDeleteQuadric(q);
        glPopMatrix();
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    drawMarsTerrainChunks();
    drawMarsRocks();
    drawMarsVolumetricClouds();

    if (marsLightning > 0.0f) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        glColor4f(1.0f, 0.9f, 0.7f, marsLightning * 0.8f);
        glPushMatrix();
        glTranslatef(marsCamX + cosf(yawR) * 400.0f, marsCamY + 100.0f, marsCamZ + sinf(yawR) * 400.0f);
        glutSolidSphere(250.0f, 16, 16);
        glPopMatrix();

        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    drawMarsParticles();

    glDisable(GL_FOG);
    drawVignette(0.35f);
    drawMarsHUD();
}


float sampleMarsTerrainHeight(float wx, float wz) { return Mars::getTerrainHeight(wx, wz); }
void  updateMarsTerrainStreaming()                 { Mars::updateStreaming(); }
