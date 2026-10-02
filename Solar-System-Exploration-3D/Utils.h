#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <GL/freeglut.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <unordered_set>
#include <cstdint>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

// ============================================================
// Constants
// ============================================================
#define PI           3.14159265358979323846f
#define TWO_PI       (2.0f * PI)
#define DEG2RAD      (PI / 180.0f)
#define RAD2DEG      (180.0f / PI)
#define NUM_STARS    1500
#define NUM_ASTEROIDS 800

// ============================================================
// Shared Structs
// ============================================================
struct CelestialBody {
    std::string name;
    float distance;
    float radius;
    float orbitSpeed;
    float rotationSpeed;
    float inclination;
    float r, g, b;
    int moonCount;
    std::string info;
    float curX, curY, curZ;
    GLuint textureID;
};

struct Asteroid {
    float angle, dist, height, size, rot, rotSpeed;
    float scaleX, scaleY, scaleZ;
    int shapeType;
};

static const int METEOR_TRAIL_POINTS = 20;
struct Meteor {
    int id;
    std::string name;
    float x, y, z;
    float vx, vy, vz;
    float scale, life, rot;
    float rotAxis[3];
    float trail[METEOR_TRAIL_POINTS][3];
};

struct DetailObj {
    float x, y, z, size, rotY;
    int type;
    float r, g, b;
};

// ============================================================
// Inline Math Helpers
// ============================================================
inline float clampf(float v, float minV, float maxV) {
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float smoothstepf(float edge0, float edge1, float x) {
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline int clampI(int v, int minV, int maxV) {
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

inline long long makeChunkKey(int cx, int cz) {
    return (static_cast<long long>(cx) << 32) ^ static_cast<unsigned int>(cz);
}

// ============================================================
// Math / Geometry Helpers (declared here, defined in Utils.cpp)
// ============================================================
float wrapAngle360(float a);
float distance3D(float x1, float y1, float z1, float x2, float y2, float z2);
bool  checkSphereCollision(float x1, float y1, float z1, float r1,
                            float x2, float y2, float z2, float r2);
float randRange(float minV, float maxV);
void  normalize3(float& x, float& y, float& z);

// ============================================================
// Noise Functions
// ============================================================
float noiseHash(int x, int y);
float noiseHash2(int x, int y);
float smoothedNoise(float x, float y);
float smoothedNoise2(float x, float y);
float fbmNoise(float x, float y, int octaves, float persistence = 0.5f);
float ridgeNoise(float x, float y, int octaves);
float warpedFBM(float x, float y, int octaves, float warpStr = 25.0f);
float plateauNoise(float x, float y, int octaves);
float seamlessNoise(float u, float v, float scaleX, float scaleY);
float smoothDamp(float current, float target, float& velocity, float smoothTime, float dt);

// ============================================================
// OpenGL Helpers
// ============================================================
void drawString(void* font, float x, float y, const char* str);

// ============================================================
// Global Shared State (defined in Utils.cpp)
// ============================================================
extern float timeSpeed;
extern float currentTime;
extern bool  isPaused;
extern bool  showOrbits;
extern bool  showLabels;
extern bool  showHUD;
extern float deltaTime;
extern float smoothDeltaTime;
extern int   lastFrameTime;

// Render scene enum
enum RenderScene {
    SCENE_SPACE,
    SCENE_EARTH,
    SCENE_VENUS,
    SCENE_MARS,
    SCENE_NEPTUNE
};

// Scene mode flags
extern bool inTerrainMode;
extern bool inMarsMode;
extern bool inVenusTerrainMode;
extern bool inNeptuneMode;
extern bool transActive;
extern bool transToTerrain;
extern bool transToMars;
extern bool transToVenusTerrain;
extern bool transToNeptune;
extern bool transPastMid;
extern float transAlpha;
extern int   lastExploredPlanet;

// FPS mouse and movement keys
extern bool fpsMouseActive;
extern bool fpsFirstFrame;
extern bool keyW, keyA, keyS, keyD;
extern float terrainMoveSpeed;
extern float mouseSensitivity;

// Mouse smoothing
static const int MOUSE_SMOOTH_SAMPLES = 4;
extern float mouseSmoothX[MOUSE_SMOOTH_SAMPLES];
extern float mouseSmoothY[MOUSE_SMOOTH_SAMPLES];
extern int   mouseSmoothIdx;

// Planet data
extern std::vector<CelestialBody> planets;

// Asteroid belt
extern std::vector<Asteroid> asteroids;

// FPS tracking
extern float fpsValue;
extern int   fpsFrameCount;
extern float fpsAccumTime;

// Competition UI
extern bool showStartScreen;
extern bool showControlsPanel;
extern bool showPerformanceHUD;
extern bool cinematicTourActive;
extern int  cinematicStep;
extern float cinematicTimer;

// Window state
extern int   cachedWindowW;
extern int   cachedWindowH;
extern float cachedAspect;
extern bool  projectionDirty;
extern float lastProjectionFOV;
extern float lastProjectionNear;
extern float lastProjectionFar;

// Projection helper
void applyPerspectiveIfNeeded(float fov, float nearPlane, float farPlane);
