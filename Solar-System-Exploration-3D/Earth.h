#pragma once
#include "Utils.h"

// ============================================================
// Earth — terrain chunks, LOD, weather, water, city details
// ============================================================

// Earth-specific globals (declared extern, defined in Earth.cpp)
extern bool earthNightMode;
extern bool showTerrainHUD;
extern bool weatherClouds, weatherRain, weatherThunder;
extern float rainIntensity;
extern float thunderFlash, thunderTimer;
extern float thunderShakeX, thunderShakeY;

static const int   ET_CHUNK_CELLS  = 100;
static const float ET_SPACING      = 0.36f;
static const float ET_CHUNK_SIZE   = ET_CHUNK_CELLS * ET_SPACING;
static const int   ET_LOD_COUNT    = 3;
static const int   ET_LOD_STEP[ET_LOD_COUNT] = {1,5,20};
static const int   ET_VIEW_CHUNKS  = 6;
static const int   ET_PRELOAD_CHUNKS = 7;
static const int   ET_MAX_CHUNKS_PER_FRAME = 2;
static const float T2_WATER        = -1.5f;
static const float T2_SUN_AZ       = 0.45f;
static const float T2_SUN_EL       = 0.75f;

struct EarthChunk {
    int cx, cz;
    GLuint lists[ET_LOD_COUNT];
    bool valid;
    std::vector<DetailObj> details;
};

struct ChunkCandidate { int cx,cz; float priority; };

class Earth {
public:
    static std::vector<EarthChunk*>         activeChunks;
    static std::unordered_set<long long>    activeChunkKeys;
    static std::vector<ChunkCandidate>      cachedCandidates;
    static bool etBuilt;
    static int  lastStreamCX, lastStreamCZ;

    static float getTerrainHeight(float wx, float wz);
    static void  build();
    static void  cleanup();
    static void  updateStreaming();

    // Draw functions
    static void drawTerrainChunks();
    static void drawDetailObjects();
    static void drawAtmosphericSky();
    static void drawTerrainSun();
    static void drawCloudLayer();
    static void drawWaterSurface();
    static void drawHUD();
    static void display();   // full Earth scene render
};

float sampleTerrainHeightV2(float wx, float wz);
void  updateEarthTerrainStreaming();
