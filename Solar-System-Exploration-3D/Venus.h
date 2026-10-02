#pragma once
#include "Utils.h"

static const int   VT_CHUNK_CELLS    = 60;
static const float VT_SPACING        = 0.52f;
static const float VT_CHUNK_SIZE     = VT_CHUNK_CELLS * VT_SPACING;
static const int   VT_LOD_COUNT      = 4;
static const int   VT_VIEW_CHUNKS    = 5;
static const int   VT_PRELOAD_CHUNKS = 6;
static const int   VT_MAX_CHUNKS_PER_FRAME = 2;
static const float VT_SUN_AZ = 1.1f;
static const float VT_SUN_EL = 0.55f;

struct VenusTerrainChunk { int cx,cz; GLuint lists[4]; bool valid; std::vector<DetailObj> details; };
struct VenusChunkCandidate { int cx,cz; float priority; };

extern bool Venus_showHUD;

class Venus {
public:
    static std::vector<VenusTerrainChunk*>   activeChunks;
    static std::unordered_set<long long>     activeChunkKeys;
    static std::vector<VenusChunkCandidate>  cachedCandidates;
    static bool vtBuilt;
    static int  lastStreamCX, lastStreamCZ;
    static bool showHUDFlag;

    static float getTerrainHeight(float wx, float wz);
    static void  build();
    static void  cleanup();
    static void  updateStreaming();
    static void  display();
    static void  drawHUD();
};

float sampleVenusTerrainHeight(float wx, float wz);
void  updateVenusTerrainStreaming();
