#include "Utils.h"

// ============================================================
// Global Shared State — Definitions
// ============================================================
float timeSpeed    = 1.0f;
float currentTime  = 0.0f;
bool  isPaused     = false;
bool  showOrbits   = true;
bool  showLabels   = true;
bool  showHUD      = true;

int   lastFrameTime    = 0;
float deltaTime        = 0.016f;
float smoothDeltaTime  = 0.016f;

// Scene mode flags
bool inTerrainMode      = false;
bool inMarsMode         = false;
bool inVenusTerrainMode = false;
bool inNeptuneMode      = false;
bool transActive        = false;
bool transToTerrain     = false;
bool transToMars        = false;
bool transToVenusTerrain = false;
bool transToNeptune     = false;
bool transPastMid       = false;
float transAlpha        = 0.0f;
int   lastExploredPlanet = 2;

// FPS mouse and movement keys
bool fpsMouseActive  = false;
bool fpsFirstFrame   = true;
bool keyW = false, keyA = false, keyS = false, keyD = false;
float terrainMoveSpeed = 0.0f;
float mouseSensitivity = 0.12f;

// Mouse smoothing
float mouseSmoothX[MOUSE_SMOOTH_SAMPLES] = {};
float mouseSmoothY[MOUSE_SMOOTH_SAMPLES] = {};
int   mouseSmoothIdx = 0;

// Competition UI
bool  showStartScreen    = true;
bool  showControlsPanel  = false;
bool  showPerformanceHUD = true;
bool  cinematicTourActive = false;
int   cinematicStep      = 0;
float cinematicTimer     = 0.0f;

// FPS tracking
float fpsValue      = 0.0f;
int   fpsFrameCount = 0;
float fpsAccumTime  = 0.0f;

// Window / projection
int   cachedWindowW       = 1280;
int   cachedWindowH       = 720;
float cachedAspect        = 1280.0f / 720.0f;
bool  projectionDirty     = true;
float lastProjectionFOV   = -1.0f;
float lastProjectionNear  = -1.0f;
float lastProjectionFar   = -1.0f;

// Planet data
std::vector<CelestialBody> planets = {
    {"Mercury", 6.0f,  0.38f, 4.7f, 0.5f,  7.0f, 0.7f, 0.7f, 0.7f, 0,
     "Mercury: Smallest planet. Extreme temperatures.", 0,0,0,0},
    {"Venus",   10.0f, 0.95f, 3.5f, 0.1f,  3.4f, 0.9f, 0.8f, 0.4f, 0,
     "Venus: Hottest planet. Thick toxic atmosphere.", 0,0,0,0},
    {"Earth",   14.0f, 1.0f,  2.9f, 20.0f, 0.0f, 0.2f, 0.5f, 1.0f, 1,
     "Earth: Home planet. Rich in resources.", 0,0,0,0},
    {"Mars",    20.0f, 0.53f, 2.4f, 19.0f, 1.85f,1.0f, 0.4f, 0.2f, 2,
     "Mars: Red planet. Cold desert world.", 0,0,0,0},
    {"Jupiter", 35.0f, 2.8f,  1.3f, 45.0f, 1.3f, 0.9f, 0.7f, 0.5f, 79,
     "Jupiter: Largest planet. Strong gravity well.", 0,0,0,0},
    {"Saturn",  48.0f, 2.4f,  0.9f, 42.0f, 2.5f, 0.9f, 0.8f, 0.6f, 82,
     "Saturn: Famous for its ring system.", 0,0,0,0},
    {"Uranus",  62.0f, 1.6f,  0.6f, 30.0f, 0.8f, 0.5f, 0.8f, 0.9f, 27,
     "Uranus: Ice giant with a tilted axis.", 0,0,0,0},
    {"Neptune", 75.0f, 1.5f,  0.5f, 32.0f, 1.8f, 0.3f, 0.3f, 0.9f, 14,
     "Neptune: Deep blue and very windy.", 0,0,0,0}
};

// Asteroid belt
std::vector<Asteroid> asteroids;

// ============================================================
// Math / Geometry Helpers
// ============================================================
float wrapAngle360(float a) {
    while (a < 0.0f)    a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
}

float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

bool checkSphereCollision(float x1, float y1, float z1, float r1,
                           float x2, float y2, float z2, float r2) {
    return distance3D(x1,y1,z1,x2,y2,z2) <= (r1+r2);
}

float randRange(float minV, float maxV) {
    return minV + (maxV - minV) * ((float)rand() / (float)RAND_MAX);
}

void normalize3(float& x, float& y, float& z) {
    float len = sqrtf(x*x + y*y + z*z);
    if (len < 0.00001f) { x=0.0f; y=1.0f; z=0.0f; return; }
    x/=len; y/=len; z/=len;
}

// ============================================================
// Noise Functions
// ============================================================
float noiseHash(int x, int y) {
    int n = x + y * 7927;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

float noiseHash2(int x, int y) {
    int n = x * 1619 + y * 31337;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

float smoothedNoise(float x, float y) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    fx = fx*fx*(3.0f - 2.0f*fx);
    fy = fy*fy*(3.0f - 2.0f*fy);
    float v00 = noiseHash(ix,iy),   v10 = noiseHash(ix+1,iy);
    float v01 = noiseHash(ix,iy+1), v11 = noiseHash(ix+1,iy+1);
    float i0 = v00 + fx*(v10-v00);
    float i1 = v01 + fx*(v11-v01);
    return i0 + fy*(i1-i0);
}

float smoothedNoise2(float x, float y) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    fx = fx*fx*(3.0f - 2.0f*fx);
    fy = fy*fy*(3.0f - 2.0f*fy);
    float v00 = noiseHash2(ix,iy),   v10 = noiseHash2(ix+1,iy);
    float v01 = noiseHash2(ix,iy+1), v11 = noiseHash2(ix+1,iy+1);
    float i0 = v00 + fx*(v10-v00);
    float i1 = v01 + fx*(v11-v01);
    return i0 + fy*(i1-i0);
}

float fbmNoise(float x, float y, int octaves, float persistence) {
    float value=0.0f, amplitude=1.0f, freq=1.0f, maxVal=0.0f;
    for (int i=0; i<octaves; i++) {
        value  += smoothedNoise(x*freq, y*freq) * amplitude;
        maxVal += amplitude;
        amplitude *= persistence;
        freq      *= 2.0f;
    }
    return value / maxVal;
}

float ridgeNoise(float x, float y, int octaves) {
    float value=0.0f, amplitude=1.0f, freq=1.0f, maxVal=0.0f;
    for (int i=0; i<octaves; i++) {
        float n = smoothedNoise(x*freq, y*freq);
        n = 1.0f - fabsf(n);
        n = n*n;
        value  += n * amplitude;
        maxVal += amplitude;
        amplitude *= 0.45f;
        freq      *= 2.1f;
    }
    return value / maxVal;
}

float warpedFBM(float x, float y, int octaves, float warpStr) {
    float wx = fbmNoise(x*0.003f+100.0f, y*0.003f+200.0f, 3) * warpStr;
    float wy = fbmNoise(x*0.003f+300.0f, y*0.003f+400.0f, 3) * warpStr;
    return fbmNoise((x+wx)*0.005f, (y+wy)*0.005f, octaves, 0.55f);
}

float plateauNoise(float x, float y, int octaves) {
    float n = fbmNoise(x, y, octaves, 0.5f);
    float steps = 5.0f;
    return floorf(n*steps + 0.5f) / steps;
}

float seamlessNoise(float u, float v, float scaleX, float scaleY) {
    float x1 = u * scaleX, x2 = (u-1.0f) * scaleX, y = v * scaleY;
    float w = u; w = w*w*(3.0f - 2.0f*w);
    return fbmNoise(x1,y,4,0.5f)*(1.0f-w) + fbmNoise(x2,y,4,0.5f)*w;
}

float smoothDamp(float current, float target, float& velocity, float smoothTime, float dt) {
    float omega = 2.0f / fmaxf(smoothTime, 0.0001f);
    float x = omega * dt;
    float exp = 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x);
    float change = current - target;
    float temp = (velocity + omega * change) * dt;
    velocity = (velocity - omega * temp) * exp;
    return target + (change + temp) * exp;
}

// ============================================================
// OpenGL Helpers
// ============================================================
void drawString(void* font, float x, float y, const char* str) {
    glRasterPos2f(x, y);
    for (const char* c = str; *c != '\0'; ++c)
        glutBitmapCharacter(font, *c);
}

void applyPerspectiveIfNeeded(float fov, float nearPlane, float farPlane) {
    if (!projectionDirty &&
        fabsf(fov       - lastProjectionFOV)   < 0.001f &&
        fabsf(nearPlane - lastProjectionNear)   < 0.001f &&
        fabsf(farPlane  - lastProjectionFar)    < 0.001f) {
        glMatrixMode(GL_MODELVIEW);
        return;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, cachedAspect, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
    lastProjectionFOV  = fov;
    lastProjectionNear = nearPlane;
    lastProjectionFar  = farPlane;
    projectionDirty    = false;
}
