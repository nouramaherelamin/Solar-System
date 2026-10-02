#include "Earth.h"
#include "TextureManager.h"
#include "Camera.h"

// Earth global state
bool earthNightMode   = false;
bool showTerrainHUD   = true;
bool weatherClouds    = false;
bool weatherRain      = false;
bool weatherThunder   = false;
float rainIntensity   = 0.5f;
float thunderFlash    = 0.0f;
float thunderTimer    = 0.0f;
float thunderShakeX   = 0.0f;
float thunderShakeY   = 0.0f;

std::vector<EarthChunk*>      Earth::activeChunks;
std::unordered_set<long long> Earth::activeChunkKeys;
std::vector<ChunkCandidate>   Earth::cachedCandidates;
bool Earth::etBuilt = false;
int  Earth::lastStreamCX = 999999;
int  Earth::lastStreamCZ = 999999;

static EarthChunk* generateEarthChunk(int cx, int cz) {
    EarthChunk* chunk = new EarthChunk();
    chunk->cx = cx;
    chunk->cz = cz;
    chunk->valid = true;

    float baseWX = cx * ET_CHUNK_SIZE - ET_CHUNK_SIZE * 0.5f;
    float baseWZ = cz * ET_CHUNK_SIZE - ET_CHUNK_SIZE * 0.5f;

    int cells = ET_CHUNK_CELLS;
    int gridPoints = cells + 1;
    int total = gridPoints * gridPoints;
    std::vector<float> cH(total);
    std::vector<float> cNX(total), cNY(total), cNZ(total);
    std::vector<float> cCR(total), cCG(total), cCB(total);

    // Pass 1: Compute all heights (1 noise call per vertex)
    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            float wx = baseWX + i * ET_SPACING;
            float wz = baseWZ + j * ET_SPACING;
            cH[j * gridPoints + i] = getTerrainHeightV2(wx, wz);
        }
    }

    // Pass 2: Compute normals — use real noise at chunk edges to prevent seams
    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            int idx = j * gridPoints + i;
            float wx = baseWX + i * ET_SPACING;
            float wz = baseWZ + j * ET_SPACING;

            float hL = (i > 0) ? cH[j * gridPoints + (i - 1)] : getTerrainHeightV2(wx - ET_SPACING, wz);
            float hR = (i < cells) ? cH[j * gridPoints + (i + 1)] : getTerrainHeightV2(wx + ET_SPACING, wz);
            float hD = (j > 0) ? cH[(j - 1) * gridPoints + i] : getTerrainHeightV2(wx, wz - ET_SPACING);
            float hU = (j < cells) ? cH[(j + 1) * gridPoints + i] : getTerrainHeightV2(wx, wz + ET_SPACING);

            float nx = (hL - hR) / (2.0f * ET_SPACING);
            float nz = (hD - hU) / (2.0f * ET_SPACING);
            float len = sqrtf(nx * nx + 1.0f + nz * nz);
            cNX[idx] = nx / len;
            cNY[idx] = 1.0f / len;
            cNZ[idx] = nz / len;
        }
    }

    // Pass 3: Compute colors and place detail objects
    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            int idx = j * gridPoints + i;
            float wx = baseWX + i * ET_SPACING;
            float wz = baseWZ + j * ET_SPACING;
            float h = cH[idx];
            float slope = 1.0f - cNY[idx];
            float moisture = (fbmNoise(wx * 0.012f + 50.0f, wz * 0.012f + 50.0f, 3) + 1.0f) * 0.5f;
            getTerrainColorV2(h, slope, moisture, cCR[idx], cCG[idx], cCB[idx]);

            // Roads in city zones - paint gray grid lines on terrain
            float cityZoneCol = (smoothedNoise(wx * 0.015f + 700.0f, wz * 0.015f + 800.0f) + 1.0f) * 0.5f;
            if (cityZoneCol > 0.58f && h > T2_WATER + 1.0f && h < 5.0f && slope < 0.12f) {
                // Grid-aligned roads using modular arithmetic
                float roadGridX = fmodf(fabsf(wx) + 0.5f, 3.6f);
                float roadGridZ = fmodf(fabsf(wz) + 0.5f, 3.6f);
                bool onRoadX = (roadGridX < 0.6f);
                bool onRoadZ = (roadGridZ < 0.6f);
                if (onRoadX || onRoadZ) {
                    // Asphalt gray
                    cCR[idx] = 0.22f; cCG[idx] = 0.22f; cCB[idx] = 0.23f;
                    // Road markings (center line)
                    if (onRoadX && roadGridX > 0.25f && roadGridX < 0.35f) {
                        float mark = fmodf(fabsf(wz), 1.2f);
                        if (mark < 0.5f) { cCR[idx] = 0.85f; cCG[idx] = 0.80f; cCB[idx] = 0.25f; }
                    }
                    if (onRoadZ && roadGridZ > 0.25f && roadGridZ < 0.35f) {
                        float mark = fmodf(fabsf(wx), 1.2f);
                        if (mark < 0.5f) { cCR[idx] = 0.85f; cCG[idx] = 0.80f; cCB[idx] = 0.25f; }
                    }
                }
                // Sidewalks along roads
                else if ((roadGridX < 0.9f) || (roadGridZ < 0.9f)) {
                    cCR[idx] = 0.55f; cCG[idx] = 0.54f; cCB[idx] = 0.50f;
                }
            }

            // Sky ambient + AO bake
            float skyContrib = (cNY[idx] + 1.0f) * 0.5f;
            float ao = 1.0f - clampf(slope * 0.18f, 0.0f, 0.12f);
            cCR[idx] *= (0.85f + skyContrib * 0.15f) * ao;
            cCG[idx] *= (0.85f + skyContrib * 0.15f) * ao;
            cCB[idx] *= (0.82f + skyContrib * 0.18f) * ao;

            // Detail objects
            if (i >= 2 && i <= cells - 2 && j >= 2 && j <= cells - 2) {
                if ((i % 4 == 0) && (j % 4 == 0)) {
                    if (h < T2_WATER + 1.5f || h > 16.0f || slope > 0.40f) continue;

                    float rng = (noiseHash(cx * 1000 + i * 17 + 7, cz * 1000 + j * 31 + 13) + 1.0f) * 0.5f;

                    // City zone check — suppress trees in urban areas
                    float cityZone = (smoothedNoise(wx * 0.015f + 700.0f, wz * 0.015f + 800.0f) + 1.0f) * 0.5f;
                    bool inCityZone = (cityZone > 0.62f && h > T2_WATER + 1.5f && h < 5.0f && slope < 0.10f);

                    bool canTree = (h > T2_WATER + 2.5f && h < 10.0f && slope < 0.22f && moisture > 0.28f && !inCityZone);
                    if (canTree && rng < 0.32f) {
                        DetailObj d;
                        d.x = wx + noiseHash(cx * 1000 + i + 1, cz * 1000 + j) * ET_SPACING * 0.4f;
                        d.z = wz + noiseHash(cx * 1000 + i, cz * 1000 + j + 1) * ET_SPACING * 0.4f;
                        d.y = getTerrainHeightV2(d.x, d.z);
                        d.size = 0.5f + rng * 1.1f;
                        d.type = (rng < 0.18f) ? 2 : 1;
                        d.rotY = rng * 360.0f;
                        float cv = noiseHash(i * 3, j * 5) * 0.08f;
                        if (d.type == 1) { d.r = 0.06f + cv; d.g = 0.28f + cv * 2.0f; d.b = 0.04f + cv * 0.5f; }
                        else { d.r = 0.12f + cv; d.g = 0.38f + cv * 1.5f; d.b = 0.06f + cv; }
                        chunk->details.push_back(d);
                    }

                    bool canRock = (h > 5.0f && slope > 0.12f) || (h > 11.0f && slope > 0.05f);
                    if (canRock && rng > 0.72f) {
                        DetailObj d;
                        d.x = wx; d.z = wz; d.y = h;
                        d.size = 0.25f + (1.0f - rng) * 0.7f;
                        d.type = 0;
                        d.rotY = rng * 360.0f;
                        float rv = noiseHash(i * 7, j * 11) * 0.06f;
                        d.r = 0.42f + rv; d.g = 0.40f + rv; d.b = 0.36f + rv;
                        chunk->details.push_back(d);
                    }

                    // City buildings on flat lowlands
                    float cityNoise = (noiseHash(cx * 500 + i * 13 + 99, cz * 500 + j * 19 + 77) + 1.0f) * 0.5f;
                    if (inCityZone && cityNoise < 0.45f) {
                        DetailObj d;
                        d.x = wx; d.z = wz; d.y = h;
                        d.rotY = ((int)(cityNoise * 4.0f)) * 90.0f;
                        float bv = noiseHash(i * 9 + 3, j * 13 + 7) * 0.08f;
                        if (cityZone > 0.75f && cityNoise < 0.18f) {
                            d.size = 1.5f + cityNoise * 4.0f;
                            d.type = 4;
                            d.r = 0.55f + bv; d.g = 0.58f + bv; d.b = 0.62f + bv;
                        }
                        else if (cityNoise < 0.15f) {
                            // Shop/storefront
                            d.size = 0.5f + cityNoise * 1.0f;
                            d.type = 8;
                            d.r = 0.80f + bv; d.g = 0.75f + bv; d.b = 0.65f + bv;
                        }
                        else {
                            d.size = 0.6f + cityNoise * 1.5f;
                            d.type = 3;
                            d.r = 0.72f + bv; d.g = 0.68f + bv; d.b = 0.60f + bv;
                        }
                        chunk->details.push_back(d);
                    }

                    // Street lamps along roads in city zones
                    float lampRng = (noiseHash(cx * 333 + i * 11 + 55, cz * 333 + j * 17 + 66) + 1.0f) * 0.5f;
                    if (inCityZone && lampRng < 0.22f) {
                        float roadGridX2 = fmodf(fabsf(wx) + 0.5f, 3.6f);
                        float roadGridZ2 = fmodf(fabsf(wz) + 0.5f, 3.6f);
                        bool nearRoad = (roadGridX2 < 0.9f || roadGridZ2 < 0.9f);
                        if (nearRoad) {
                            DetailObj d;
                            d.x = wx; d.z = wz; d.y = h;
                            d.size = 0.6f + lampRng * 0.3f;
                            d.type = 7;
                            d.rotY = lampRng * 360.0f;
                            d.r = 0.35f; d.g = 0.35f; d.b = 0.38f;
                            chunk->details.push_back(d);
                        }
                    }

                    // Cars on roads
                    float carRng = (noiseHash(cx * 888 + i * 29 + 44, cz * 888 + j * 37 + 55) + 1.0f) * 0.5f;
                    if (inCityZone && carRng < 0.20f) {
                        float roadGridX3 = fmodf(fabsf(wx) + 0.5f, 3.6f);
                        float roadGridZ3 = fmodf(fabsf(wz) + 0.5f, 3.6f);
                        bool onRoad = (roadGridX3 < 0.6f || roadGridZ3 < 0.6f);
                        if (onRoad) {
                            DetailObj d;
                            d.x = wx; d.z = wz; d.y = h;
                            d.size = 0.7f + carRng * 0.5f;
                            d.type = 6;
                            d.rotY = (roadGridX3 < 0.6f) ? 0.0f : 90.0f;
                            // Random car colors
                            float cv2 = noiseHash(i * 41 + 9, j * 43 + 11) * 0.5f + 0.5f;
                            if (cv2 < 0.25f) { d.r = 0.85f; d.g = 0.12f; d.b = 0.10f; } // red
                            else if (cv2 < 0.50f) { d.r = 0.15f; d.g = 0.20f; d.b = 0.75f; } // blue
                            else if (cv2 < 0.75f) { d.r = 0.90f; d.g = 0.90f; d.b = 0.88f; } // white
                            else { d.r = 0.12f; d.g = 0.12f; d.b = 0.12f; } // black
                            chunk->details.push_back(d);
                        }
                    }

                    // Motorcycles on roads
                    float motoRng = (noiseHash(cx * 555 + i * 31 + 88, cz * 555 + j * 41 + 99) + 1.0f) * 0.5f;
                    if (inCityZone && motoRng < 0.12f) {
                        float roadGridX4 = fmodf(fabsf(wx) + 0.5f, 3.6f);
                        float roadGridZ4 = fmodf(fabsf(wz) + 0.5f, 3.6f);
                        bool onRoad2 = (roadGridX4 < 0.6f || roadGridZ4 < 0.6f);
                        if (onRoad2) {
                            DetailObj d;
                            d.x = wx + 0.15f; d.z = wz + 0.15f; d.y = h;
                            d.size = 0.6f + motoRng * 0.3f;
                            d.type = 9;
                            d.rotY = (roadGridX4 < 0.6f) ? 0.0f : 90.0f;
                            float mv2 = noiseHash(i * 47 + 3, j * 53 + 7) * 0.5f + 0.5f;
                            if (mv2 < 0.33f) { d.r = 0.10f; d.g = 0.10f; d.b = 0.10f; }
                            else if (mv2 < 0.66f) { d.r = 0.80f; d.g = 0.15f; d.b = 0.10f; }
                            else { d.r = 0.20f; d.g = 0.25f; d.b = 0.70f; }
                            chunk->details.push_back(d);
                        }
                    }

                    // Humans near cities and paths
                    float humanRng = (noiseHash(cx * 777 + i * 23 + 111, cz * 777 + j * 29 + 222) + 1.0f) * 0.5f;
                    bool nearCity = (cityZone > 0.55f && h > T2_WATER + 1.0f && h < 6.0f && slope < 0.15f);
                    if (nearCity && humanRng < 0.12f) {
                        DetailObj d;
                        d.x = wx + noiseHash(i + 50, j + 60) * ET_SPACING * 0.3f;
                        d.z = wz + noiseHash(i + 70, j + 80) * ET_SPACING * 0.3f;
                        d.y = getTerrainHeightV2(d.x, d.z);
                        if (d.y > T2_WATER + 0.5f) {
                            d.size = 0.8f + humanRng * 0.4f;
                            d.type = 5;
                            d.rotY = humanRng * 360.0f;
                            // Skin tone variation
                            float skinV = noiseHash(i * 33 + 5, j * 41 + 9) * 0.15f;
                            d.r = 0.75f + skinV; d.g = 0.58f + skinV * 0.8f; d.b = 0.45f + skinV * 0.5f;
                            chunk->details.push_back(d);
                        }
                    }
                }
            }
        }
    }

    // Build LOD display lists
    for (int lod = 0; lod < ET_LOD_COUNT; lod++) {
        int step = ET_LOD_STEP[lod];
        chunk->lists[lod] = glGenLists(1);
        glNewList(chunk->lists[lod], GL_COMPILE);

        for (int j = 0; j < cells; j += step) {
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= cells; i += step) {
                for (int dj = 1; dj >= 0; dj--) {
                    int cj = clampI(j + dj * step, 0, cells);
                    int idx = cj * gridPoints + i;
                    float px = baseWX + i * ET_SPACING;
                    float pz = baseWZ + cj * ET_SPACING;
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

struct ChunkCandidate {
    int cx, cz;
    float priority;
};

static int lastEarthStreamCX = 999999;
static int lastEarthStreamCZ = 999999;
static std::vector<ChunkCandidate> cachedEarthCandidates;

static bool earthChunkExists(int testCX, int testCZ) {
    return activeEarthChunkKeys.find(makeChunkKey(testCX, testCZ)) != activeEarthChunkKeys.end();
}

static void addEarthChunk(int cx, int cz) {
    long long key = makeChunkKey(cx, cz);
    if (activeEarthChunkKeys.find(key) != activeEarthChunkKeys.end()) {
        return;
    }

    EarthChunk* chunk = generateEarthChunk(cx, cz);
    activeEarthChunks.push_back(chunk);
    activeEarthChunkKeys.insert(key);
}

static void updateEarthTerrainStreaming() {
    if (!inTerrainMode) return;

    int playerCX = (int)roundf(tCamX / ET_CHUNK_SIZE);
    int playerCZ = (int)roundf(tCamZ / ET_CHUNK_SIZE);

    int unloadRadius = ET_PRELOAD_CHUNKS + 2;
    for (auto it = activeEarthChunks.begin(); it != activeEarthChunks.end(); ) {
        EarthChunk* chunk = *it;
        if (abs(chunk->cx - playerCX) > unloadRadius || abs(chunk->cz - playerCZ) > unloadRadius) {
            for (int i = 0; i < ET_LOD_COUNT; i++) {
                if (chunk->lists[i] != 0) glDeleteLists(chunk->lists[i], 1);
            }

            activeEarthChunkKeys.erase(makeChunkKey(chunk->cx, chunk->cz));
            delete chunk;
            it = activeEarthChunks.erase(it);
        }
        else {
            ++it;
        }
    }

    bool changedChunk = (playerCX != lastEarthStreamCX || playerCZ != lastEarthStreamCZ);

    if (changedChunk || cachedEarthCandidates.empty()) {
        lastEarthStreamCX = playerCX;
        lastEarthStreamCZ = playerCZ;
        cachedEarthCandidates.clear();

        float fwdX = sinf(tCamYaw * DEG2RAD);
        float fwdZ = cosf(tCamYaw * DEG2RAD);

        float predictX = tCamX + tCamVelX * 1.0f;
        float predictZ = tCamZ + tCamVelZ * 1.0f;
        int predictCX = (int)roundf(predictX / ET_CHUNK_SIZE);
        int predictCZ = (int)roundf(predictZ / ET_CHUNK_SIZE);

        for (int dz = -ET_PRELOAD_CHUNKS; dz <= ET_PRELOAD_CHUNKS; dz++) {
            for (int dx = -ET_PRELOAD_CHUNKS; dx <= ET_PRELOAD_CHUNKS; dx++) {
                int testCX = playerCX + dx;
                int testCZ = playerCZ + dz;

                if (earthChunkExists(testCX, testCZ)) continue;

                float dist2 = (float)(dx * dx + dz * dz);
                float distFromPlayer = sqrtf(dist2);

                float predDX = (float)(testCX - predictCX);
                float predDZ = (float)(testCZ - predictCZ);
                float distFromPredict = sqrtf(predDX * predDX + predDZ * predDZ);

                float dirBias = 0.0f;
                if (distFromPlayer > 0.5f) {
                    float toDirX = (float)dx / distFromPlayer;
                    float toDirZ = (float)dz / distFromPlayer;
                    float dot = toDirX * fwdX + toDirZ * fwdZ;
                    dirBias = (1.0f - dot) * 2.0f;
                }

                float priority = distFromPlayer * 0.5f + distFromPredict * 0.3f + dirBias;
                cachedEarthCandidates.push_back({ testCX, testCZ, priority });
            }
        }

        std::sort(cachedEarthCandidates.begin(), cachedEarthCandidates.end(),
            [](const ChunkCandidate& a, const ChunkCandidate& b) {
                return a.priority < b.priority;
            });
    }

    int chunksLoaded = 0;
    for (auto it = cachedEarthCandidates.begin();
        it != cachedEarthCandidates.end() && chunksLoaded < ET_MAX_CHUNKS_PER_FRAME; ) {

        if (!earthChunkExists(it->cx, it->cz)) {
            addEarthChunk(it->cx, it->cz);
            chunksLoaded++;
        }

        it = cachedEarthCandidates.erase(it);
    }
}

static void buildEarthTerrain() {
    int playerCX = (int)roundf(tCamX / ET_CHUNK_SIZE);
    int playerCZ = (int)roundf(tCamZ / ET_CHUNK_SIZE);

    cachedEarthCandidates.clear();
    lastEarthStreamCX = 999999;
    lastEarthStreamCZ = 999999;

    int initRadius = 2;
    for (int dz = -initRadius; dz <= initRadius; dz++) {
        for (int dx = -initRadius; dx <= initRadius; dx++) {
            addEarthChunk(playerCX + dx, playerCZ + dz);
        }
    }
    etBuilt = true;
}

static void cleanupEarthTerrain() {
    for (EarthChunk* chunk : activeEarthChunks) {
        if (!chunk) continue;
        for (int i = 0; i < ET_LOD_COUNT; i++) {
            if (chunk->lists[i] != 0) glDeleteLists(chunk->lists[i], 1);
        }
        delete chunk;
    }
    activeEarthChunks.clear();
    activeEarthChunkKeys.clear();
    cachedEarthCandidates.clear();
    lastEarthStreamCX = 999999;
    lastEarthStreamCZ = 999999;
    etBuilt = false;
}

static void drawAtmosphericSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);

    float skyR = 420.0f;
    int stacks = 24, slices = 48;

    float sunDX = cosf(T2_SUN_EL) * sinf(T2_SUN_AZ);
    float sunDY = sinf(T2_SUN_EL);
    float sunDZ = cosf(T2_SUN_EL) * cosf(T2_SUN_AZ);

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

                float t2 = t * t;
                float br = lerpf(0.56f, 0.08f, t2);
                float bg = lerpf(0.68f, 0.14f, t2);
                float bb = lerpf(0.84f, 0.42f, t2);

                float sunDot = vdx * sunDX + vdy * sunDY + vdz * sunDZ;
                sunDot = clampf(sunDot, 0.0f, 1.0f);

                float sunGlow = powf(sunDot, 16.0f) * 0.45f;
                float sunHalo = powf(sunDot, 3.0f) * 0.18f;
                float horizonBoost = (1.0f - t) * powf(sunDot, 2.0f) * 0.12f;

                br += sunGlow * 1.0f + sunHalo * 0.85f + horizonBoost * 1.0f;
                bg += sunGlow * 0.88f + sunHalo * 0.55f + horizonBoost * 0.65f;
                bb += sunGlow * 0.45f + sunHalo * 0.25f + horizonBoost * 0.20f;

                br = clampf(br, 0.0f, 1.0f);
                bg = clampf(bg, 0.0f, 1.0f);
                bb = clampf(bb, 0.0f, 1.0f);

                glColor3f(br, bg, bb);
                glVertex3f(tCamX + vRad * cx, vY, tCamZ + vRad * cz);
            }
        }
        glEnd();
    }

    glColor3f(0.05f, 0.07f, 0.04f);
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

static void drawTerrainSun() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    float sunPX = tCamX + cosf(T2_SUN_EL) * sinf(T2_SUN_AZ) * 300.0f;
    float sunPY = sinf(T2_SUN_EL) * 300.0f;
    float sunPZ = tCamZ + cosf(T2_SUN_EL) * cosf(T2_SUN_AZ) * 300.0f;

    glPushMatrix();
    glTranslatef(sunPX, sunPY, sunPZ);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 0.85f, 0.5f, 0.03f);  glutSolidSphere(45.0f, 16, 16);
    glColor4f(1.0f, 0.88f, 0.55f, 0.06f); glutSolidSphere(30.0f, 16, 16);
    glColor4f(1.0f, 0.92f, 0.65f, 0.14f); glutSolidSphere(18.0f, 16, 16);
    glColor4f(1.0f, 0.98f, 0.88f, 0.70f); glutSolidSphere(8.0f, 18, 18);
    glColor4f(1.0f, 1.0f, 0.96f, 0.92f);  glutSolidSphere(4.5f, 16, 16);

    glPopMatrix();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

static void drawCloudLayer() {
    float cloudAlt = 48.0f;
    float cloudSize = ET_VIEW_CHUNKS * ET_CHUNK_SIZE * 0.55f;
    int cloudRes = 45;
    float cellSize = cloudSize * 2.0f / cloudRes;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (int j = 0; j < cloudRes; j++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= cloudRes; i++) {
            for (int dj = 1; dj >= 0; dj--) {
                float lx = -cloudSize + i * cellSize;
                float lz = -cloudSize + (j + dj) * cellSize;
                float wx = tCamX + lx;
                float wz = tCamZ + lz;

                float windT = currentTime * 0.25f;
                float nx = wx * 0.006f + windT;
                float nz = wz * 0.006f + windT * 0.65f;
                float density = fbmNoise(nx, nz, 4, 0.52f);
                density = smoothstepf(-0.10f, 0.40f, density);

                float edgeDist = fmaxf(fabsf(lx), fabsf(lz)) / cloudSize;
                float edgeFade = 1.0f - smoothstepf(0.65f, 1.0f, edgeDist);

                float alpha = density * 0.40f * edgeFade;

                float camDist = fabsf(tCamY - cloudAlt);
                if (camDist < 10.0f) alpha *= camDist / 10.0f;

                if (tCamY > cloudAlt) {
                    alpha *= 0.85f;
                }

                float sunSide = clampf(lx * sinf(T2_SUN_AZ) + lz * cosf(T2_SUN_AZ), 0.0f, cloudSize);
                sunSide /= cloudSize;
                float r = lerpf(0.88f, 1.0f, sunSide * 0.6f);
                float g = lerpf(0.90f, 1.0f, sunSide * 0.4f);
                float b = lerpf(0.94f, 1.0f, sunSide * 0.2f);

                glColor4f(r, g, b, alpha);
                glVertex3f(wx, cloudAlt, wz);
            }
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void drawWaterSurface() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    GLfloat wAmbient[] = { 0.008f, 0.025f, 0.07f, 1.0f };
    GLfloat wDiffuse[] = { 0.02f, 0.10f, 0.25f, 0.85f };
    GLfloat wSpecular[] = { 1.00f, 0.98f, 0.92f, 1.0f };
    GLfloat wSh[] = { 96.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, wAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, wDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, wSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, wSh);

    float halfSz = ET_VIEW_CHUNKS * ET_CHUNK_SIZE * 0.8f;
    int wGrid = 80;
    float wStep = halfSz * 2.0f / wGrid;

    float viewAngle = clampf((tCamPitch + 90.0f) / 180.0f, 0.0f, 1.0f);
    float fresnelBase = lerpf(0.80f, 0.30f, viewAngle);
    fresnelBase *= clampf(1.0f - tCamY / 60.0f, 0.20f, 1.0f);

    float sunDirX = cosf(T2_SUN_EL) * sinf(T2_SUN_AZ);
    float sunDirZ = cosf(T2_SUN_EL) * cosf(T2_SUN_AZ);

    for (int j = 0; j < wGrid; j++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= wGrid; i++) {
            for (int dj = 1; dj >= 0; dj--) {
                float wx = tCamX - halfSz + i * wStep;
                float wz = tCamZ - halfSz + (j + dj) * wStep;

                float wy = T2_WATER
                    + 0.10f * sinf(wx * 0.18f + currentTime * 1.4f)
                    * cosf(wz * 0.14f + currentTime * 1.0f)
                    + 0.05f * sinf(wx * 0.42f + currentTime * 2.1f + 1.5f)
                    * cosf(wz * 0.35f + currentTime * 1.7f + 0.8f)
                    + 0.025f * sinf(wx * 0.85f + currentTime * 2.9f)
                    * cosf(wz * 0.80f + currentTime * 2.3f)
                    + 0.012f * sinf(wx * 1.6f + currentTime * 3.8f + 2.0f)
                    * cosf(wz * 1.4f + currentTime * 3.2f);

                float wnx = -0.10f * 0.18f * cosf(wx * 0.18f + currentTime * 1.4f)
                    * cosf(wz * 0.14f + currentTime * 1.0f)
                    - 0.05f * 0.42f * cosf(wx * 0.42f + currentTime * 2.1f + 1.5f)
                    * cosf(wz * 0.35f + currentTime * 1.7f + 0.8f);
                float wnz = 0.10f * 0.14f * sinf(wx * 0.18f + currentTime * 1.4f)
                    * sinf(wz * 0.14f + currentTime * 1.0f)
                    + 0.05f * 0.35f * sinf(wx * 0.42f + currentTime * 2.1f + 1.5f)
                    * sinf(wz * 0.35f + currentTime * 1.7f + 0.8f);
                float wny = 1.0f;
                float wlen = sqrtf(wnx * wnx + wny * wny + wnz * wnz);
                glNormal3f(wnx / wlen, wny / wlen, wnz / wlen);

                float terrainH = sampleTerrainHeightV2(wx, wz);
                float depth = T2_WATER - terrainH;
                if (depth < 0.0f) depth = 0.0f;

                float depthT = clampf(depth / 10.0f, 0.0f, 1.0f);
                float cr = lerpf(0.08f, 0.01f, depthT);
                float cg = lerpf(0.28f, 0.04f, depthT);
                float cb = lerpf(0.45f, 0.15f, depthT);

                float skyRefl = fresnelBase * 0.3f;
                cr += skyRefl * 0.25f;
                cg += skyRefl * 0.35f;
                cb += skyRefl * 0.50f;

                float toCamX = tCamX - wx, toCamZ = tCamZ - wz;
                float tcLen = sqrtf(toCamX * toCamX + toCamZ * toCamZ + tCamY * tCamY);
                if (tcLen > 0.01f) {
                    float halfX = (sunDirX + toCamX / tcLen);
                    float halfZ = (sunDirZ + toCamZ / tcLen);
                    float halfY = (sinf(T2_SUN_EL) + tCamY / tcLen);
                    float hLen = sqrtf(halfX * halfX + halfY * halfY + halfZ * halfZ);
                    if (hLen > 0.01f) {
                        float spec = clampf((wnx / wlen * halfX / hLen + wny / wlen * halfY / hLen + wnz / wlen * halfZ / hLen), 0.0f, 1.0f);
                        float glint = powf(spec, 256.0f) * 0.6f;
                        cr += glint; cg += glint * 0.95f; cb += glint * 0.8f;
                    }
                }

                float foam = 0.0f;
                if (depth < 1.2f && depth > 0.0f) {
                    float foamBase = 1.0f - depth / 1.2f;
                    float foamWave = 0.5f + 0.5f * sinf(wx * 1.8f + currentTime * 2.8f)
                        * cosf(wz * 1.5f + currentTime * 2.2f);
                    foam = foamBase * foamWave * 0.6f;
                }
                cr += foam * 0.75f; cg += foam * 0.72f; cb += foam * 0.65f;

                float alpha = fresnelBase;
                if (depth < 0.5f) alpha *= depth / 0.5f;

                if (depth > 0.3f && depth < 3.0f) {
                    float caustic = fbmNoise(wx * 0.3f + currentTime * 0.8f, wz * 0.3f + currentTime * 0.6f, 2, 0.6f);
                    caustic = clampf(caustic * 0.5f + 0.5f, 0.0f, 1.0f);
                    float causticStr = (1.0f - clampf((depth - 0.3f) / 2.7f, 0.0f, 1.0f)) * 0.08f;
                    cr += caustic * causticStr;
                    cg += caustic * causticStr * 1.2f;
                    cb += caustic * causticStr * 0.6f;
                }

                glColor4f(clampf(cr, 0, 1), clampf(cg, 0, 1), clampf(cb, 0, 1), alpha);
                glVertex3f(wx, wy, wz);
            }
        }
        glEnd();
    }

    glDisable(GL_BLEND);
}

static void drawConiferTree(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size;
    glColor3f(0.30f, 0.18f, 0.08f);
    glPushMatrix(); glScalef(s * 0.08f, s * 0.55f, s * 0.08f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(d.r, d.g, d.b);
    glPushMatrix(); glTranslatef(0, s * 0.30f, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(s * 0.38f, s * 0.55f, 7, 2); glPopMatrix();
    glColor3f(d.r * 1.1f, d.g * 1.1f, d.b * 1.05f);
    glPushMatrix(); glTranslatef(0, s * 0.58f, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(s * 0.28f, s * 0.45f, 6, 2); glPopMatrix();
    glColor3f(d.r * 1.15f, d.g * 1.2f, d.b * 1.1f);
    glPushMatrix(); glTranslatef(0, s * 0.82f, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(s * 0.18f, s * 0.35f, 5, 2); glPopMatrix();
    glPopMatrix();
}

static void drawDeciduousTree(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size;
    glColor3f(0.32f, 0.20f, 0.10f);
    glPushMatrix(); glScalef(s * 0.07f, s * 0.60f, s * 0.07f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(d.r, d.g, d.b);
    glPushMatrix(); glTranslatef(0, s * 0.65f, 0); glScalef(1.0f, 0.75f, 1.0f); glutSolidSphere(s * 0.38f, 8, 6); glPopMatrix();
    glColor3f(d.r * 0.9f, d.g * 0.95f, d.b * 0.9f);
    glPushMatrix(); glTranslatef(s * 0.10f, s * 0.75f, s * 0.08f); glScalef(1.0f, 0.70f, 1.0f); glutSolidSphere(s * 0.25f, 7, 5); glPopMatrix();
    glPopMatrix();
}

static void drawRockFormation(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size;
    glColor3f(d.r, d.g, d.b);
    glPushMatrix(); glScalef(1.0f, 0.50f, 0.80f); glutSolidSphere(s * 0.30f, 7, 5); glPopMatrix();
    glColor3f(d.r * 0.92f, d.g * 0.90f, d.b * 0.88f);
    glPushMatrix(); glTranslatef(s * 0.18f, -s * 0.05f, s * 0.12f); glScalef(0.85f, 0.45f, 0.72f); glutSolidSphere(s * 0.20f, 6, 4); glPopMatrix();
    glColor3f(d.r * 1.05f, d.g * 1.02f, d.b * 0.95f);
    glPushMatrix(); glTranslatef(-s * 0.15f, -s * 0.08f, -s * 0.10f); glScalef(0.70f, 0.40f, 0.65f); glutSolidSphere(s * 0.12f, 5, 4); glPopMatrix();
    glPopMatrix();
}

// --- City buildings ---
static void drawBuilding(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size;

    // Main building body
    glColor3f(d.r, d.g, d.b);
    glPushMatrix();
    glTranslatef(0, s * 0.5f, 0);
    glScalef(s * 0.35f, s, s * 0.30f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Roof
    glColor3f(d.r * 0.6f, d.g * 0.6f, d.b * 0.65f);
    glPushMatrix();
    glTranslatef(0, s * 1.02f, 0);
    glScalef(s * 0.38f, s * 0.06f, s * 0.33f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Windows (small darker rectangles)
    glColor3f(0.6f, 0.75f, 0.9f); // window blue tint
    int floors = (int)(s / 0.3f);
    if (floors < 1) floors = 1;
    if (floors > 8) floors = 8;
    for (int f = 0; f < floors; f++) {
        float fy = 0.15f + f * (s / floors);
        for (int w = -1; w <= 1; w += 2) {
            glPushMatrix();
            glTranslatef(s * 0.176f, fy, w * s * 0.08f);
            glScalef(0.01f, s * 0.08f, s * 0.06f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }
    }
    glPopMatrix();
}

// --- Cars ---
static void drawCar(const DetailObj& d) {
    glPushMatrix();
    // Cars move along roads
    float moveOffset = sinf(d.x * 0.5f + currentTime * 1.2f + d.rotY) * 1.5f;
    glTranslatef(d.x + moveOffset * cosf(d.rotY * DEG2RAD), d.y + 0.06f, d.z + moveOffset * sinf(d.rotY * DEG2RAD));
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size * 0.15f;

    // Car body
    glColor3f(d.r, d.g, d.b);
    glPushMatrix();
    glTranslatef(0, s * 1.5f, 0);
    glScalef(s * 3.0f, s * 1.0f, s * 1.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Cabin
    glColor3f(d.r * 0.85f, d.g * 0.85f, d.b * 0.85f);
    glPushMatrix();
    glTranslatef(s * 0.3f, s * 2.3f, 0);
    glScalef(s * 1.8f, s * 0.8f, s * 1.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Windshield
    glColor3f(0.5f, 0.6f, 0.8f);
    glPushMatrix();
    glTranslatef(s * 1.3f, s * 2.3f, 0);
    glScalef(s * 0.05f, s * 0.65f, s * 1.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Wheels
    glColor3f(0.15f, 0.15f, 0.15f);
    for (int wx2 = -1; wx2 <= 1; wx2 += 2) {
        for (int wz2 = -1; wz2 <= 1; wz2 += 2) {
            glPushMatrix();
            glTranslatef(wx2 * s * 1.1f, s * 0.5f, wz2 * s * 0.8f);
            glutSolidSphere(s * 0.4f, 6, 4);
            glPopMatrix();
        }
    }

    // Headlights
    glColor3f(1.0f, 0.95f, 0.7f);
    glPushMatrix(); glTranslatef(s * 1.5f, s * 1.5f, s * 0.5f); glutSolidSphere(s * 0.12f, 4, 4); glPopMatrix();
    glPushMatrix(); glTranslatef(s * 1.5f, s * 1.5f, -s * 0.5f); glutSolidSphere(s * 0.12f, 4, 4); glPopMatrix();

    // Taillights
    glColor3f(0.9f, 0.1f, 0.05f);
    glPushMatrix(); glTranslatef(-s * 1.5f, s * 1.5f, s * 0.5f); glutSolidSphere(s * 0.10f, 4, 4); glPopMatrix();
    glPushMatrix(); glTranslatef(-s * 1.5f, s * 1.5f, -s * 0.5f); glutSolidSphere(s * 0.10f, 4, 4); glPopMatrix();

    glPopMatrix();
}

// --- Street lamp / traffic light ---
static void drawStreetLamp(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    float s = d.size;

    // Pole
    glColor3f(0.35f, 0.35f, 0.38f);
    glPushMatrix();
    glTranslatef(0, s * 0.5f, 0);
    glScalef(0.03f, s, 0.03f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Lamp arm
    glPushMatrix();
    glTranslatef(0.08f, s * 0.95f, 0);
    glScalef(0.18f, 0.02f, 0.02f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Light (glowing)
    float glow = 0.5f + 0.5f * sinf(currentTime * 0.5f + d.x * 2.0f);
    glColor3f(1.0f, 0.95f, 0.7f * glow + 0.3f);
    glPushMatrix();
    glTranslatef(0.16f, s * 0.92f, 0);
    glutSolidSphere(0.04f, 6, 6);
    glPopMatrix();

    // Traffic light box (on some lamps)
    if (d.rotY > 180.0f) {
        glColor3f(0.2f, 0.2f, 0.22f);
        glPushMatrix();
        glTranslatef(0, s * 0.75f, 0);
        glScalef(0.05f, 0.14f, 0.04f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Traffic light colors (cycle based on time)
        float cycle = fmodf(currentTime * 0.3f + d.x, 3.0f);
        float rL = (cycle < 1.0f) ? 1.0f : 0.15f;
        float yL = (cycle >= 1.0f && cycle < 2.0f) ? 1.0f : 0.15f;
        float gL = (cycle >= 2.0f) ? 1.0f : 0.15f;
        glColor3f(rL, 0.05f, 0.05f);
        glPushMatrix(); glTranslatef(0.026f, s * 0.79f, 0); glutSolidSphere(0.015f, 5, 5); glPopMatrix();
        glColor3f(yL, yL * 0.8f, 0.05f);
        glPushMatrix(); glTranslatef(0.026f, s * 0.75f, 0); glutSolidSphere(0.015f, 5, 5); glPopMatrix();
        glColor3f(0.05f, gL, 0.05f);
        glPushMatrix(); glTranslatef(0.026f, s * 0.71f, 0); glutSolidSphere(0.015f, 5, 5); glPopMatrix();
    }

    glPopMatrix();
}

// --- Shop / storefront ---
static void drawShop(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size;

    // Shop body (wider than building)
    glColor3f(d.r, d.g, d.b);
    glPushMatrix();
    glTranslatef(0, s * 0.35f, 0);
    glScalef(s * 0.5f, s * 0.7f, s * 0.35f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Awning
    float awR = fabsf(sinf(d.x * 5.0f + 1.0f));
    float awG = fabsf(sinf(d.z * 5.0f + 2.0f));
    float awB = fabsf(sinf(d.x * 3.0f + d.z * 4.0f));
    glColor3f(awR * 0.6f + 0.3f, awG * 0.4f + 0.1f, awB * 0.3f + 0.1f);
    glPushMatrix();
    glTranslatef(s * 0.15f, s * 0.68f, 0);
    glScalef(s * 0.25f, s * 0.04f, s * 0.40f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Shopfront window
    glColor3f(0.55f, 0.70f, 0.85f);
    glPushMatrix();
    glTranslatef(s * 0.251f, s * 0.30f, 0);
    glScalef(0.01f, s * 0.30f, s * 0.25f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Door
    glColor3f(0.30f, 0.20f, 0.10f);
    glPushMatrix();
    glTranslatef(s * 0.251f, s * 0.18f, s * 0.13f);
    glScalef(0.01f, s * 0.35f, s * 0.08f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

static void drawSkyscraper(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size;

    // Tower body
    glColor3f(d.r, d.g, d.b);
    glPushMatrix();
    glTranslatef(0, s * 0.5f, 0);
    glScalef(s * 0.22f, s, s * 0.22f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Glass bands
    glColor3f(0.5f, 0.65f, 0.85f);
    int bands = (int)(s / 0.5f);
    if (bands > 6) bands = 6;
    for (int b = 0; b < bands; b++) {
        float by = 0.3f + b * (s * 0.85f / bands);
        glPushMatrix();
        glTranslatef(0, by, 0);
        glScalef(s * 0.225f, s * 0.04f, s * 0.225f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Antenna
    glColor3f(0.5f, 0.5f, 0.5f);
    glPushMatrix();
    glTranslatef(0, s * 1.0f, 0);
    glScalef(0.02f, s * 0.15f, 0.02f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

// --- Humans ---
static void drawHuman(const DetailObj& d) {
    glPushMatrix();
    glTranslatef(d.x, d.y, d.z);
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size * 0.12f; // humans are small

    // Head
    glColor3f(d.r, d.g, d.b);
    glPushMatrix();
    glTranslatef(0, s * 7.5f, 0);
    glutSolidSphere(s * 0.7f, 6, 5);
    glPopMatrix();

    // Body
    float cr2 = d.r * 0.5f, cg2 = d.g * 0.5f, cb2 = d.b * 0.8f;
    glColor3f(cr2, cg2, cb2);
    glPushMatrix();
    glTranslatef(0, s * 5.0f, 0);
    glScalef(s * 0.9f, s * 2.2f, s * 0.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Legs
    glColor3f(0.25f, 0.22f, 0.20f);
    for (int leg = -1; leg <= 1; leg += 2) {
        glPushMatrix();
        glTranslatef(leg * s * 0.3f, s * 2.0f, 0);
        glScalef(s * 0.35f, s * 2.0f, s * 0.4f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Arms
    glColor3f(cr2, cg2, cb2);
    float swing = sinf(d.x * 3.0f + currentTime * 2.5f) * 15.0f;
    for (int arm = -1; arm <= 1; arm += 2) {
        glPushMatrix();
        glTranslatef(arm * s * 1.1f, s * 5.8f, 0);
        glRotatef(arm * swing, 1, 0, 0);
        glScalef(s * 0.3f, s * 1.8f, s * 0.3f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    glPopMatrix();
}

static void drawMotorcycle(const DetailObj& d) {
    glPushMatrix();
    float moveOffset = sinf(d.x * 0.7f + currentTime * 2.0f + d.rotY * 0.1f) * 2.0f;
    glTranslatef(d.x + moveOffset * cosf(d.rotY * DEG2RAD), d.y + 0.03f, d.z + moveOffset * sinf(d.rotY * DEG2RAD));
    glRotatef(d.rotY, 0, 1, 0);
    float s = d.size * 0.10f;

    // Body frame
    glColor3f(d.r, d.g, d.b);
    glPushMatrix();
    glTranslatef(0, s * 1.5f, 0);
    glScalef(s * 2.5f, s * 0.5f, s * 0.6f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Seat
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(-s * 0.3f, s * 2.0f, 0);
    glScalef(s * 1.0f, s * 0.3f, s * 0.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Wheels
    glColor3f(0.12f, 0.12f, 0.12f);
    glPushMatrix(); glTranslatef(s * 1.0f, s * 0.6f, 0); glutSolidSphere(s * 0.5f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef(-s * 1.0f, s * 0.6f, 0); glutSolidSphere(s * 0.5f, 6, 4); glPopMatrix();

    // Headlight
    glColor3f(1.0f, 0.95f, 0.7f);
    glPushMatrix(); glTranslatef(s * 1.3f, s * 1.7f, 0); glutSolidSphere(s * 0.12f, 4, 4); glPopMatrix();

    // Rider
    glColor3f(0.20f, 0.20f, 0.25f);
    glPushMatrix(); glTranslatef(-s * 0.3f, s * 3.2f, 0); glScalef(s * 0.5f, s * 1.2f, s * 0.4f); glutSolidCube(1.0f); glPopMatrix();
    // Helmet
    glColor3f(0.30f, 0.30f, 0.35f);
    glPushMatrix(); glTranslatef(-s * 0.3f, s * 4.2f, 0); glutSolidSphere(s * 0.35f, 6, 5); glPopMatrix();

    glPopMatrix();
}

static void drawEarthDetailObjects() {
    float viewDist = 55.0f;

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glDisable(GL_TEXTURE_2D);

    GLfloat dSpec[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    GLfloat dSh[] = { 4.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, dSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, dSh);

    for (EarthChunk* chunk : activeEarthChunks) {
        float ccx = chunk->cx * ET_CHUNK_SIZE;
        float ccz = chunk->cz * ET_CHUNK_SIZE;
        float cDist = sqrtf(powf(ccx - tCamX, 2) + powf(ccz - tCamZ, 2));
        if (cDist > viewDist + ET_CHUNK_SIZE * 0.8f) continue;

        for (const auto& d : chunk->details) {
            float dx = d.x - tCamX;
            float dz = d.z - tCamZ;
            float dist2 = dx * dx + dz * dz;
            if (dist2 > viewDist * viewDist) continue;

            switch (d.type) {
            case 0: drawRockFormation(d); break;
            case 1: drawConiferTree(d);   break;
            case 2: drawDeciduousTree(d); break;
            case 3: drawBuilding(d);      break;
            case 4: drawSkyscraper(d);     break;
            case 5: drawHuman(d);          break;
            case 6: drawCar(d);            break;
            case 7: drawStreetLamp(d);     break;
            case 8: drawShop(d);           break;
            case 9: drawMotorcycle(d);     break;
            }
        }
    }
}

static void drawEarthTerrainChunks() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat tSpec[] = { 0.06f, 0.06f, 0.06f, 1.0f };
    GLfloat tSh[] = { 6.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, tSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, tSh);

    float forwardX = sinf(tCamYaw * DEG2RAD);
    float forwardZ = cosf(tCamYaw * DEG2RAD);

    for (EarthChunk* chunk : activeEarthChunks) {
        float cx = chunk->cx * ET_CHUNK_SIZE;
        float cz = chunk->cz * ET_CHUNK_SIZE;

        float dx = cx - tCamX;
        float dz = cz - tCamZ;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist > (ET_VIEW_CHUNKS + 1) * ET_CHUNK_SIZE) continue;

        if (dist > ET_CHUNK_SIZE * 1.5f) {
            float toDirX = dx / dist;
            float toDirZ = dz / dist;
            float dot = toDirX * forwardX + toDirZ * forwardZ;
            if (dot < -0.35f) continue;
        }

        int lod;
        if (dist < 50.0f)       lod = 0;
        else if (dist < 150.0f) lod = 1;
        else                     lod = 2;

        glCallList(chunk->lists[lod]);
    }
}

static void drawGlassPanel(float x, float y, float w, float h, float alpha) {
    glColor4f(0.02f, 0.04f, 0.08f, alpha * 0.78f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glColor4f(0.25f, 0.55f, 0.85f, alpha * 0.45f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glColor4f(0.40f, 0.70f, 1.0f, alpha * 0.25f);
    glBegin(GL_LINES);
    glVertex2f(x + 1, y + h - 1); glVertex2f(x + w - 1, y + h - 1);
    glEnd();
}

static const char* getBiomeName(float h, float moisture) {
    if (h < T2_WATER + 0.3f) return "OCEAN";
    if (h < T2_WATER + 1.0f) return "COASTAL";
    if (h < T2_WATER + 2.2f) return "BEACH";
    if (h < 5.0f) {
        if (moisture > 0.55f) return "TROPICAL FOREST";
        if (moisture > 0.30f) return "GRASSLAND";
        return "ARID SCRUBLAND";
    }
    if (h < 9.0f) return "HIGHLAND FOREST";
    if (h < 14.0f) return "ALPINE ROCK";
    if (h < 17.0f) return "SNOW LINE";
    return "SNOW PEAK";
}

static void drawTerrainHUD() {
    if (!showTerrainHUD) return;

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

    glColor4f(0.50f, 0.85f, 1.0f, 0.90f);
    drawString(GLUT_BITMAP_HELVETICA_18, 18.0f, (float)(h - 28), "EARTH EXPLORATION");

    const char* modeStr;
    float modeR, modeG, modeB;
    if (tCamY < 8.0f) {
        modeStr = "SURFACE"; modeR = 0.4f; modeG = 0.9f; modeB = 0.5f;
    }
    else if (tCamY < 30.0f) {
        modeStr = "LOW ATMOSPHERE"; modeR = 0.5f; modeG = 0.8f; modeB = 1.0f;
    }
    else {
        modeStr = "HIGH ATMOSPHERE"; modeR = 0.7f; modeG = 0.7f; modeB = 1.0f;
    }

    float pulse = 0.6f + 0.4f * sinf(currentTime * 3.0f);
    glColor4f(modeR, modeG, modeB, pulse);
    glPointSize(6.0f);
    glBegin(GL_POINTS); glVertex2f(200.0f, (float)(h - 26)); glEnd();
    glColor4f(modeR, modeG, modeB, 0.85f);
    drawString(GLUT_BITMAP_HELVETICA_12, 210.0f, (float)(h - 30), modeStr);

    char altBuf[64];
    float altMeters = tCamY * 1000.0f;
    if (altMeters > 1000.0f)
        sprintf(altBuf, "ALT  %.1f km", altMeters / 1000.0f);
    else
        sprintf(altBuf, "ALT  %.0f m", altMeters);
    glColor4f(0.7f, 0.95f, 0.8f, 0.85f);
    drawString(GLUT_BITMAP_HELVETICA_18, (float)(w - 195), (float)(h - 28), altBuf);

    float groundH = sampleTerrainHeightV2(tCamX, tCamZ);
    float moisture = (fbmNoise(tCamX * 0.012f + 50.0f, tCamZ * 0.012f + 50.0f, 3) + 1.0f) * 0.5f;
    const char* biome = getBiomeName(groundH, moisture);
    glColor4f(0.6f, 0.75f, 0.9f, 0.60f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w / 2 + 60), (float)(h - 30), biome);

    char spdBuf[32];
    sprintf(spdBuf, "SPD %.0f", terrainMoveSpeed * 100.0f);
    glColor4f(0.8f, 0.85f, 0.6f, 0.60f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w - 290), (float)(h - 30), spdBuf);

    float heading = wrapAngle360(tCamYaw);
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
    glColor4f(0.7f, 0.8f, 0.9f, 0.65f);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(w / 2 - 20), (float)(h - 30), compassBuf);

    float barX = (float)(w - 22), barY = 80.0f, barH = (float)(h - 160), barW = 8.0f;
    float altNorm = clampf(tCamY / 80.0f, 0.0f, 1.0f);

    glColor4f(0.02f, 0.04f, 0.08f, 0.50f);
    glBegin(GL_QUADS);
    glVertex2f(barX, barY); glVertex2f(barX + barW, barY);
    glVertex2f(barX + barW, barY + barH); glVertex2f(barX, barY + barH);
    glEnd();

    float fillH = altNorm * barH;
    glBegin(GL_QUADS);
    glColor4f(0.2f, 0.8f, 0.4f, 0.60f);
    glVertex2f(barX + 1, barY); glVertex2f(barX + barW - 1, barY);
    glColor4f(0.3f, 0.6f, 1.0f, 0.60f);
    glVertex2f(barX + barW - 1, barY + fillH); glVertex2f(barX + 1, barY + fillH);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 0.80f);
    glBegin(GL_QUADS);
    glVertex2f(barX - 3, barY + fillH - 2); glVertex2f(barX + barW + 3, barY + fillH - 2);
    glVertex2f(barX + barW + 3, barY + fillH + 2); glVertex2f(barX - 3, barY + fillH + 2);
    glEnd();

    drawGlassPanel(0, 0, (float)w, 30.0f, 0.75f);
    glColor4f(0.65f, 0.70f, 0.75f, 0.55f);
    drawString(GLUT_BITMAP_HELVETICA_12, 18.0f, 10.0f,
        "WASD: Move | R: Night | 1: Clouds | 2: Rain | 3: Thunder | +/-: Rain Power | ESC: Return");

    float cx = w / 2.0f, cy = h / 2.0f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.15f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(cx - 14, cy); glVertex2f(cx - 5, cy);
    glVertex2f(cx + 5, cy);  glVertex2f(cx + 14, cy);
    glVertex2f(cx, cy - 14); glVertex2f(cx, cy - 5);
    glVertex2f(cx, cy + 5);  glVertex2f(cx, cy + 14);
    glEnd();
    glPointSize(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
    glBegin(GL_POINTS); glVertex2f(cx, cy); glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

static const char* getMarsBiomeName(float h, float slope) {
    if (h < -3.0f) return "CRATER BASIN";
    if (h < 0.0f) return "CANYON FLOOR";
    if (h < 2.5f) {
        if (slope > 0.25f) return "EROSION CHANNEL";
        return "DUST PLAINS";
    }
    if (h < 6.0f) {
        if (slope > 0.30f) return "SCARP FACE";

    if (earthNightMode) {
        // Night sky — deep dark blue
        glClearColor(0.01f, 0.01f, 0.04f, 1.0f);
    }
    else {
        // Realistic sky clear color with subtle warm tint
        glClearColor(0.55f, 0.68f, 0.85f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    applyPerspectiveIfNeeded(55.0f, 0.12f, 2200.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawR = tCamYaw * DEG2RAD;
    float pitR = tCamPitch * DEG2RAD;
    float lookX = tCamX + cosf(pitR) * sinf(yawR);
    float lookY = tCamY + sinf(pitR);
    float lookZ = tCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(tCamX, tCamY, tCamZ, lookX, lookY, lookZ, 0, 1, 0);

    // Fog
    float heightNorm = clampf(tCamY / 55.0f, 0.0f, 1.0f);
    glEnable(GL_FOG);
    if (earthNightMode) {
        float fogDensity = lerpf(0.008f, 0.003f, heightNorm);
        GLfloat fogCol[] = { 0.02f, 0.02f, 0.06f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogCol);
        glFogf(GL_FOG_DENSITY, fogDensity);
    }
    else {
        float fogDensity = lerpf(0.005f, 0.001f, heightNorm);
        GLfloat fogCol[] = { 0.58f, 0.70f, 0.86f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogCol);
        glFogf(GL_FOG_DENSITY, fogDensity);
    }
    glFogi(GL_FOG_MODE, GL_EXP2);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    if (earthNightMode) {
        // Moonlight — brighter blue-white
        GLfloat moonDir[] = { -0.3f, 0.8f, 0.5f, 0.0f };
        GLfloat moonAmb[] = { 0.06f, 0.07f, 0.10f, 1.0f };
        GLfloat moonDif[] = { 0.22f, 0.25f, 0.35f, 1.0f };
        GLfloat moonSpc[] = { 0.12f, 0.14f, 0.18f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, moonDir);
        glLightfv(GL_LIGHT0, GL_AMBIENT, moonAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, moonDif);
        glLightfv(GL_LIGHT0, GL_SPECULAR, moonSpc);

        // Very faint sky fill
        GLfloat skyDir[] = { 0.0f, 1.0f, 0.0f, 0.0f };
        GLfloat skyAmb[] = { 0.01f, 0.01f, 0.02f, 1.0f };
        GLfloat skyDif[] = { 0.02f, 0.03f, 0.05f, 1.0f };
        GLfloat skySpc[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, skyDir);
        glLightfv(GL_LIGHT1, GL_AMBIENT, skyAmb);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, skyDif);
        glLightfv(GL_LIGHT1, GL_SPECULAR, skySpc);

        // Draw stars in night sky
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);

        // --- Stars ---
        glPointSize(1.2f);
        glBegin(GL_POINTS);
        for (int s = 0; s < 500; s++) {
            float sx = tCamX + sinf(s * 137.5f) * 900.0f;
            float sy = 180.0f + cosf(s * 73.3f) * 170.0f;
            float sz = tCamZ + cosf(s * 213.7f) * 900.0f;
            float twinkle = 0.6f + 0.4f * fabsf(sinf(s * 17.3f + currentTime * 0.5f + s * 0.1f));
            // Some stars are brighter/bigger
            if (s % 7 == 0) {
                glEnd();
                glPointSize(3.5f);
                glBegin(GL_POINTS);
                glColor3f(twinkle, twinkle * 0.95f, twinkle * 1.15f);
                glVertex3f(sx, sy, sz);
                glEnd();
                glPointSize(1.5f);
                glBegin(GL_POINTS);
            }
            else {
                glColor3f(twinkle * 0.9f, twinkle * 0.9f, twinkle);
                glVertex3f(sx, sy, sz);
            }
        }
        glEnd();

        // --- Moon ---
        float moonAngle = 0.8f; // position in sky
        float moonX = tCamX + cosf(moonAngle) * 400.0f;
        float moonY = 250.0f;
        float moonZ = tCamZ + sinf(moonAngle) * 400.0f;

        // Moon glow halo
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPointSize(80.0f);
        glBegin(GL_POINTS);
        glColor4f(0.25f, 0.30f, 0.40f, 0.25f);
        glVertex3f(moonX, moonY, moonZ);
        glEnd();
        glPointSize(40.0f);
        glBegin(GL_POINTS);
        glColor4f(0.40f, 0.45f, 0.55f, 0.35f);
        glVertex3f(moonX, moonY, moonZ);
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        // Moon sphere
        glEnable(GL_LIGHTING);
        GLfloat moonEmit[] = { 0.90f, 0.92f, 0.98f, 1.0f };
        GLfloat moonDiff[] = { 0.95f, 0.95f, 1.00f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, moonEmit);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, moonDiff);
        glPushMatrix();
        glTranslatef(moonX, moonY, moonZ);
        glutSolidSphere(12.0f, 20, 16);
        glPopMatrix();
        GLfloat noEmit[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmit);
        glDisable(GL_LIGHTING);

        glEnable(GL_LIGHTING);
        glEnable(GL_FOG);
    }
    else {
        float sunDirX = cosf(T2_SUN_EL) * sinf(T2_SUN_AZ);
        float sunDirY = sinf(T2_SUN_EL);
        float sunDirZ = cosf(T2_SUN_EL) * cosf(T2_SUN_AZ);
        GLfloat sunDir[] = { sunDirX, sunDirY, sunDirZ, 0.0f };

        // Warm sunlight
        GLfloat sunAmb[] = { 0.22f, 0.24f, 0.30f, 1.0f };
        GLfloat sunDif[] = { 1.05f, 0.95f, 0.82f, 1.0f };
        GLfloat sunSpc[] = { 0.50f, 0.48f, 0.40f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
        glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif);
        glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);

        // Cool sky fill light from above
        GLfloat skyDir[] = { -sunDirX * 0.3f, 1.0f, -sunDirZ * 0.3f, 0.0f };
        GLfloat skyAmb[] = { 0.08f, 0.10f, 0.16f, 1.0f };
        GLfloat skyDif[] = { 0.18f, 0.22f, 0.32f, 1.0f };
        GLfloat skySpc[] = { 0.02f, 0.02f, 0.04f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, skyDir);
        glLightfv(GL_LIGHT1, GL_AMBIENT, skyAmb);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, skyDif);
        glLightfv(GL_LIGHT1, GL_SPECULAR, skySpc);
    }

    if (!earthNightMode) {
        drawAtmosphericSky();
        drawTerrainSun();
    }
    drawEarthTerrainChunks();
    drawWaterSurface();
    drawEarthDetailObjects();

    // Night city light pillars
    if (earthNightMode) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        for (EarthChunk* chunk : activeEarthChunks) {
            float ccx = chunk->cx * ET_CHUNK_SIZE;
            float ccz = chunk->cz * ET_CHUNK_SIZE;
            float cDist = sqrtf(powf(ccx - tCamX, 2) + powf(ccz - tCamZ, 2));
            if (cDist > 60.0f) continue;

            for (const auto& d : chunk->details) {
                // Building window glow
                if (d.type == 3 || d.type == 4 || d.type == 8) {
                    float dx2 = d.x - tCamX;
                    float dz2 = d.z - tCamZ;
                    if (dx2 * dx2 + dz2 * dz2 > 50.0f * 50.0f) continue;

                    float winR, winG, winB;
                    if (d.type == 4) { winR = 0.4f; winG = 0.5f; winB = 0.7f; }
                    else if (d.type == 8) { winR = 0.8f; winG = 0.65f; winB = 0.2f; }
                    else { winR = 0.7f; winG = 0.6f; winB = 0.3f; }

                    float bPulse = 0.7f + 0.3f * sinf(currentTime * 0.8f + d.x * 1.5f);

                    // Building ground glow
                    float glowR = d.size * 0.6f;
                    glBegin(GL_TRIANGLE_FAN);
                    glColor4f(winR, winG, winB, 0.25f * bPulse);
                    glVertex3f(d.x, d.y + 0.02f, d.z);
                    glColor4f(winR, winG, winB, 0.0f);
                    for (int a = 0; a <= 10; a++) {
                        float ang = a * 3.14159f * 2.0f / 10.0f;
                        glVertex3f(d.x + cosf(ang) * glowR, d.y + 0.02f, d.z + sinf(ang) * glowR);
                    }
                    glEnd();

                    // Small upward glow from windows
                    float miniH = d.size * 1.2f;
                    float miniW = 0.06f;
                    glBegin(GL_TRIANGLE_STRIP);
                    glColor4f(winR, winG, winB, 0.18f * bPulse);
                    glVertex3f(d.x - miniW, d.y, d.z);
                    glVertex3f(d.x + miniW, d.y, d.z);
                    glColor4f(winR, winG, winB, 0.0f);
                    glVertex3f(d.x - miniW * 2.0f, d.y + miniH, d.z);
                    glVertex3f(d.x + miniW * 2.0f, d.y + miniH, d.z);
                    glEnd();
                }

                // MASSIVE LIGHT PILLARS at street lamps
                if (d.type == 7) {
                    float dx2 = d.x - tCamX;
                    float dz2 = d.z - tCamZ;
                    if (dx2 * dx2 + dz2 * dz2 > 55.0f * 55.0f) continue;

                    float pulse = 0.8f + 0.2f * sinf(currentTime * 1.0f + d.x * 2.5f + d.z * 1.8f);
                    float lampTop = d.y + d.size * 0.9f;
                    float spreadR = 3.5f;

                    // Horizontal light disc at lamp height — radiates outward
                    glBegin(GL_TRIANGLE_FAN);
                    glColor4f(1.0f, 0.95f, 0.65f, 0.10f * pulse);
                    glVertex3f(d.x, lampTop, d.z);
                    glColor4f(1.0f, 0.90f, 0.55f, 0.0f);
                    for (int a = 0; a <= 20; a++) {
                        float ang = a * 3.14159f * 2.0f / 20.0f;
                        glVertex3f(d.x + cosf(ang) * spreadR, lampTop, d.z + sinf(ang) * spreadR);
                    }
                    glEnd();

                    // Second horizontal disc slightly lower — wider spread
                    glBegin(GL_TRIANGLE_FAN);
                    glColor4f(1.0f, 0.92f, 0.55f, 0.06f * pulse);
                    glVertex3f(d.x, lampTop - 0.15f, d.z);
                    glColor4f(1.0f, 0.88f, 0.45f, 0.0f);
                    for (int a = 0; a <= 20; a++) {
                        float ang = a * 3.14159f * 2.0f / 20.0f;
                        glVertex3f(d.x + cosf(ang) * (spreadR * 1.5f), lampTop - 0.15f, d.z + sinf(ang) * (spreadR * 1.5f));
                    }
                    glEnd();

                    // Ground light pool
                    float poolR = 3.0f;
                    glBegin(GL_TRIANGLE_FAN);
                    glColor4f(1.0f, 0.92f, 0.55f, 0.08f * pulse);
                    glVertex3f(d.x, d.y + 0.03f, d.z);
                    glColor4f(1.0f, 0.90f, 0.50f, 0.0f);
                    for (int a = 0; a <= 16; a++) {
                        float ang = a * 3.14159f * 2.0f / 16.0f;
                        glVertex3f(d.x + cosf(ang) * poolR, d.y + 0.03f, d.z + sinf(ang) * poolR);
                    }
                    glEnd();

                    // Downward cone from lamp head
                    glBegin(GL_TRIANGLE_FAN);
                    glColor4f(1.0f, 0.95f, 0.7f, 0.10f * pulse);
                    glVertex3f(d.x + 0.16f, lampTop, d.z);
                    glColor4f(1.0f, 0.90f, 0.6f, 0.0f);
                    float coneR = 1.5f;
                    for (int a = 0; a <= 12; a++) {
                        float ang = a * 3.14159f * 2.0f / 12.0f;
                        glVertex3f(d.x + 0.16f + cosf(ang) * coneR, d.y + 0.05f, d.z + sinf(ang) * coneR);
                    }
                    glEnd();
                }
            }
        }

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    if (weatherClouds) drawCloudLayer();

    // === WEATHER SYSTEM ===
    // Storm clouds overlay
    if (weatherClouds) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        float cloudAlpha = 0.45f + rainIntensity * 0.25f;
        for (int c = 0; c < 60; c++) {
            float cx2 = tCamX + sinf(c * 47.3f + currentTime * 0.02f) * 120.0f;
            float cz2 = tCamZ + cosf(c * 63.7f + currentTime * 0.015f) * 120.0f;
            float cy2 = 45.0f + sinf(c * 31.1f) * 8.0f;
            float cSize = 15.0f + fabsf(sinf(c * 17.5f)) * 25.0f;

            float cBright = 0.30f + 0.15f * sinf(c * 11.3f);
            if (weatherThunder && thunderFlash > 0.3f) {
                cBright += thunderFlash * 0.5f;
            }

            glBegin(GL_TRIANGLE_FAN);
            glColor4f(cBright, cBright, cBright + 0.02f, cloudAlpha);
            glVertex3f(cx2, cy2, cz2);
            glColor4f(cBright, cBright, cBright + 0.02f, 0.0f);
            for (int a = 0; a <= 12; a++) {
                float ang = a * 3.14159f * 2.0f / 12.0f;
                glVertex3f(cx2 + cosf(ang) * cSize, cy2 + sinf(ang * 0.3f) * 3.0f, cz2 + sinf(ang) * cSize);
            }
            glEnd();
        }
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // Rain particles
    if (weatherRain) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        int numDrops = (int)(1200 * rainIntensity);
        if (numDrops < 150) numDrops = 150;

        // Wind direction (shifts slowly)
        float windX = sinf(currentTime * 0.15f) * 0.12f * rainIntensity;
        float windZ = cosf(currentTime * 0.11f) * 0.08f * rainIntensity;

        // --- Rain streaks ---
        glBegin(GL_LINES);
        for (int r = 0; r < numDrops; r++) {
            // Randomized positions around camera
            float spread = 35.0f + rainIntensity * 15.0f;
            float rx = tCamX + sinf(r * 73.7f + r * 0.01f) * spread + cosf(r * 31.3f) * spread * 0.3f;
            float rz = tCamZ + cosf(r * 97.3f + r * 0.013f) * spread + sinf(r * 41.7f) * spread * 0.3f;

            // Varying fall speed per drop
            float speed = 18.0f + rainIntensity * 22.0f + fabsf(sinf(r * 13.7f)) * 8.0f;
            float ry = fmodf(55.0f - fmodf(currentTime * speed + r * 3.7f, 55.0f), 55.0f) + tCamY - 28.0f;

            // Varying drop length and alpha
            float dropLen = 0.25f + rainIntensity * 0.45f + fabsf(sinf(r * 7.3f)) * 0.15f;
            float dropAlpha = 0.12f + rainIntensity * 0.22f;
            float thickness = (r % 5 == 0) ? 0.015f : 0.005f; // some thicker drops

            // Top of drop (brighter)
            glColor4f(0.72f, 0.77f, 0.88f, dropAlpha);
            glVertex3f(rx, ry, rz);
            // Bottom of drop (dimmer, wind-shifted)
            glColor4f(0.50f, 0.55f, 0.68f, dropAlpha * 0.2f);
            glVertex3f(rx + windX + thickness, ry - dropLen, rz + windZ + thickness);
        }
        glEnd();

        // --- Splash effects on ground ---
        int numSplashes = (int)(120 * rainIntensity);
        if (numSplashes < 20) numSplashes = 20;
        for (int sp = 0; sp < numSplashes; sp++) {
            float sx = tCamX + sinf(sp * 53.1f) * 30.0f + cosf(sp * 71.3f) * 10.0f;
            float sz = tCamZ + cosf(sp * 89.7f) * 30.0f + sinf(sp * 37.9f) * 10.0f;
            float groundH = sampleTerrainHeightV2(sx, sz);

            // Splash lifecycle
            float splashTime = fmodf(currentTime * 4.0f + sp * 2.3f, 1.5f);
            if (splashTime > 0.6f) continue; // only show briefly
            float splashR = splashTime * 0.3f;
            float splashAlpha = (0.6f - splashTime) * 0.4f * rainIntensity;

            glBegin(GL_LINE_LOOP);
            glColor4f(0.75f, 0.80f, 0.90f, splashAlpha);
            for (int a = 0; a < 8; a++) {
                float ang = a * 3.14159f * 2.0f / 8.0f;
                glVertex3f(sx + cosf(ang) * splashR, groundH + 0.05f, sz + sinf(ang) * splashR);
            }
            glEnd();
        }

        // --- Rain mist/fog (heavy rain only) ---
        if (rainIntensity > 0.4f) {
            float mistAlpha = (rainIntensity - 0.4f) * 0.3f;
            glDisable(GL_DEPTH_TEST);
            for (int m = 0; m < 20; m++) {
                float mx = tCamX + sinf(m * 97.1f + currentTime * 0.04f) * 50.0f;
                float mz = tCamZ + cosf(m * 67.3f + currentTime * 0.03f) * 50.0f;
                float my = tCamY - 3.0f + sinf(m * 41.7f) * 4.0f;
                float mSize = 8.0f + fabsf(sinf(m * 23.5f)) * 12.0f;

                glBegin(GL_TRIANGLE_FAN);
                glColor4f(0.65f, 0.68f, 0.75f, mistAlpha);
                glVertex3f(mx, my, mz);
                glColor4f(0.65f, 0.68f, 0.75f, 0.0f);
                for (int a = 0; a <= 10; a++) {
                    float ang = a * 3.14159f * 2.0f / 10.0f;
                    glVertex3f(mx + cosf(ang) * mSize, my + sinf(ang * 0.5f) * 1.5f, mz + sinf(ang) * mSize);
                }
                glEnd();
            }
            glEnable(GL_DEPTH_TEST);
        }

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // Thunder & lightning
    if (weatherThunder) {
        thunderTimer -= deltaTime;
        if (thunderTimer <= 0.0f) {
            float interval = 3.0f + (1.0f - rainIntensity) * 8.0f;
            thunderTimer = interval + fabsf(sinf(currentTime * 7.7f)) * 4.0f;
            thunderFlash = 1.0f;
            thunderShakeX = (noiseHash(glutGet(GLUT_ELAPSED_TIME), 99) * 2.0f - 1.0f) * 0.4f;
            thunderShakeY = (noiseHash(99, glutGet(GLUT_ELAPSED_TIME)) * 2.0f - 1.0f) * 0.3f;
        }
        if (thunderFlash > 0.0f) {
            thunderFlash *= 0.88f;
            if (thunderFlash < 0.01f) { thunderFlash = 0.0f; thunderShakeX = 0.0f; thunderShakeY = 0.0f; }

            // White flash overlay
            int w2 = glutGet(GLUT_WINDOW_WIDTH);
            int h2 = glutGet(GLUT_WINDOW_HEIGHT);
            glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, w2, 0, h2);
            glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
            glColor4f(0.9f, 0.92f, 1.0f, thunderFlash * 0.5f);
            glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f((float)w2, 0); glVertex2f((float)w2, (float)h2); glVertex2f(0, (float)h2);
            glEnd();
            glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
            glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        }
    }

    glDisable(GL_LIGHT1);

    glDisable(GL_FOG);
    drawVignette(earthNightMode ? 0.45f : 0.28f);
    drawTerrainHUD();
}


// C-linkage aliases for Camera.cpp forward declarations
float sampleTerrainHeightV2(float wx, float wz) { return Earth::getTerrainHeight(wx, wz); }
void  updateEarthTerrainStreaming()              { Earth::updateStreaming(); }
