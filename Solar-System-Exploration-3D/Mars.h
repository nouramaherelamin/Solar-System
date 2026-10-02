#pragma once
#include "Utils.h"

extern float marsLightning;
extern float marsCamX, marsCamY, marsCamZ;
extern float marsCamYaw, marsCamPitch;
extern float targetMarsCamX, targetMarsCamY, targetMarsCamZ;
extern float targetMarsCamYaw, targetMarsCamPitch;
extern float marsCamShakeX, marsCamShakeY;
extern float marsMoveSpeed;
extern bool  showTerrainHUD;

static const int   MT_CHUNK_CELLS = 40;
static const float MT_SPACING     = 3.0f;
static const float MT_CHUNK_SIZE  = MT_CHUNK_CELLS * MT_SPACING;
static const int   MT_LOD_COUNT   = 4;
static const int   MT_LOD_STEP[MT_LOD_COUNT];
static const int   MT_VIEW_CHUNKS = 8;
static const float MARS_SUN_AZ    = 0.85f;
static const float MARS_SUN_EL    = 0.32f;

struct MarsRock { float x,y,z,size,rotY,rotAxis; int shape; float r,g,b; };
struct MarsChunk { int cx,cz; GLuint lists[MT_LOD_COUNT]; bool valid; std::vector<MarsRock> rocks; };
struct MarsParticle { float x,y,z,vx,vy,vz,life,size; };

class Mars {
public:
    static std::vector<MarsChunk*>       activeChunks;
    static std::unordered_set<long long> activeChunkKeys;
    static bool mtBuilt;
    static std::vector<MarsParticle>     particles;

    static float getTerrainHeight(float wx, float wz);
    static void  build();
    static void  cleanup();
    static void  updateStreaming();
    static void  display();
    static void  drawHUD();
};

float sampleMarsTerrainHeight(float wx, float wz);
void  updateMarsTerrainStreaming();
