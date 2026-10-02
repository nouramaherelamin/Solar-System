
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

#include <GL/freeglut.h>
#include "stb_image.h"
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

#define PI 3.14159265358979323846f
#define TWO_PI (2.0f * PI)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)
#define NUM_STARS 1500
#define NUM_ASTEROIDS 800

// =========================
// Globals
// =========================
static float timeSpeed = 1.0f;
static float currentTime = 0.0f;

static bool isPaused = false;
static bool showOrbits = true;
static bool showLabels = true;
static bool showHUD = true;

// =========================
// Competition Presentation Layer
// =========================
static bool showStartScreen = true;
static bool showControlsPanel = false;
static bool showPerformanceHUD = true;
static bool cinematicTourActive = false;
static int cinematicStep = 0;
static float cinematicTimer = 0.0f;
static float fpsValue = 0.0f;
static int fpsFrameCount = 0;
static float fpsAccumTime = 0.0f;

// =========================
// Satellite / Impact
// =========================
float satelliteOrbitSpeed = 8.0f;
float satelliteDistance = 2.5f;
float satelliteSize = 0.15f;

bool launchImpactAsteroid = false;
bool asteroidHitEarth = false;
bool earthHasCrater = false;

float impactAsteroidX = 0.0f;
float impactAsteroidY = 0.0f;
float impactAsteroidZ = 0.0f;

float impactStartX = 0.0f;
float impactStartY = 0.0f;
float impactStartZ = 0.0f;

float impactProgress = 0.0f;
float impactSpeed = 0.014f;

bool showImpactExplosion = false;
float explosionProgress = 0.0f;
float explosionDuration = 0.55f;
float explosionX = 0.0f;
float explosionY = 0.0f;
float explosionZ = 0.0f;

float craterFlash = 0.0f;
const float IMPACT_ASTEROID_RADIUS = 0.32f;

// =========================
// Camera
// =========================
enum CameraMode {
    CAMERA_FREE,
    CAMERA_FOCUS,
    CAMERA_INSPECT,
    CAMERA_EXPLORE,
    CAMERA_PLANET_VIEW
};

static CameraMode cameraMode = CAMERA_FREE;

static float camYaw = -45.0f;
static float camPitch = 45.0f;
static float camDistance = 90.0f;

static float targetCamYaw = -45.0f;
static float targetCamPitch = 45.0f;
static float targetCamDistance = 90.0f;

static float camTargetX = 0.0f;
static float camTargetY = 0.0f;
static float camTargetZ = 0.0f;

static float targetLookX = 0.0f;
static float targetLookY = 0.0f;
static float targetLookZ = 0.0f;

static float cameraFOV = 55.0f;
static float targetFOV = 55.0f;

// Focus local pan offsets
static float focusOffsetX = 0.0f;
static float focusOffsetY = 0.0f;
static float focusOffsetZ = 0.0f;

static float targetFocusOffsetX = 0.0f;
static float targetFocusOffsetY = 0.0f;
static float targetFocusOffsetZ = 0.0f;

// Explore Mode
static float exploreAnchorLat = 0.0f;
static float exploreAnchorLon = 0.0f;
static float targetExploreAnchorLat = 0.0f;
static float targetExploreAnchorLon = 0.0f;

static float exploreLookYaw = 0.0f;
static float exploreLookPitch = -10.0f;
static float targetExploreLookYaw = 0.0f;
static float targetExploreLookPitch = -10.0f;

static float exploreDistance = 0.35f;
static float targetExploreDistance = 0.35f;

// Mouse
static int lastMouseX = 0;
static int lastMouseY = 0;
static bool leftMouseDown = false;
static bool rightMouseDown = false;
static bool middleMouseDown = false;

// Double click
static int lastClickTime = 0;
static int lastClickPlanetIndex = -2;
static int lastClickX = 0;
static int lastClickY = 0;
static const int DOUBLE_CLICK_MS = 300;

// Planet interaction
static int hoveredPlanetIndex = -1;
static int focusedPlanetIndex = -1;

// Fullscreen
static bool isFullscreen = false;

// Separate cinematic planet view
static bool isPlanetSelectedView = false;
static float selectedViewCamDistance = 10.0f;
static float selectedViewCamAngleY = 0.0f;
static float selectedViewCamAngleX = 15.0f;
static int selectedPlanetIndex = -1;


// =========================
// Earth / Mars Terrain Exploration Add-on
// =========================
// Delta time tracking
static int   lastFrameTime = 0;
static float deltaTime = 0.016f;
static float smoothDeltaTime = 0.016f;

// FPS-style terrain controls
static bool fpsMouseActive = false;
static bool fpsFirstFrame = true;
static bool keyW = false, keyA = false, keyS = false, keyD = false;
static float terrainMoveSpeed = 0.0f;
static float gCamEyeX = 0.0f, gCamEyeY = 0.0f, gCamEyeZ = 0.0f;

// Velocity-based Earth camera
static float tCamVelX = 0.0f, tCamVelZ = 0.0f, tCamVelY = 0.0f;
static float tCamAccel = 18.0f;
static float tCamDecel = 8.0f;
static float tCamMaxSpeed = 12.0f;

// Mouse smoothing
static const int MOUSE_SMOOTH_SAMPLES = 4;
static float mouseSmoothX[MOUSE_SMOOTH_SAMPLES] = {};
static float mouseSmoothY[MOUSE_SMOOTH_SAMPLES] = {};
static int   mouseSmoothIdx = 0;
static float mouseSensitivity = 0.12f;

static const float T2_WATER = -1.5f;

// Earth infinite terrain constants
static const int   ET_CHUNK_CELLS = 100;
static const float ET_SPACING = 0.36f;
static const float ET_CHUNK_SIZE = ET_CHUNK_CELLS * ET_SPACING;
static const int   ET_LOD_COUNT = 3;
static const int   ET_LOD_STEP[ET_LOD_COUNT] = { 1, 5, 20 };
static const int   ET_VIEW_CHUNKS = 6;
static const int   ET_PRELOAD_CHUNKS = 7;
static const int   ET_MAX_CHUNKS_PER_FRAME = 2;

struct DetailObj {
    float x, y, z, size, rotY;
    int type;
    float r, g, b;
};

struct EarthChunk {
    int cx, cz;
    GLuint lists[ET_LOD_COUNT];
    bool valid;
    std::vector<DetailObj> details;
};

static std::vector<EarthChunk*> activeEarthChunks;
static std::unordered_set<long long> activeEarthChunkKeys;
static bool etBuilt = false;
static bool inTerrainMode = false;
static bool earthNightMode = false;

// Weather system for Earth terrain
static bool  weatherClouds = false;
static bool  weatherRain = false;
static bool  weatherThunder = false;
static float rainIntensity = 0.5f;
static float thunderFlash = 0.0f;
static float thunderTimer = 0.0f;
static float thunderShakeX = 0.0f, thunderShakeY = 0.0f;

static float tCamX = 0.0f, tCamY = 25.0f, tCamZ = 0.0f;
static float tCamYaw = 0.0f, tCamPitch = -20.0f;
static float targetTCamX = 0.0f, targetTCamY = 25.0f, targetTCamZ = 0.0f;
static float targetTCamYaw = 0.0f, targetTCamPitch = -20.0f;

// Transition state
static float transAlpha = 0.0f;
static bool  transActive = false;
static bool  transToTerrain = false;
static bool  transPastMid = false;
static bool  transToMars = false;
static int   lastExploredPlanet = 2;

static const float T2_SUN_AZ = 0.45f;
static const float T2_SUN_EL = 0.75f;
static bool showTerrainHUD = true;

// Mars terrain system
static bool inMarsMode = false;
static float marsCamX = 0.0f, marsCamY = 8.0f, marsCamZ = 0.0f;
static float marsCamYaw = 0.0f, marsCamPitch = -8.0f;
static float targetMarsCamX = 0.0f, targetMarsCamY = 8.0f, targetMarsCamZ = 0.0f;
static float targetMarsCamYaw = 0.0f, targetMarsCamPitch = -8.0f;
static float marsLightning = 0.0f;
static float marsLightningTimer = 0.0f;
static float marsCamShakeX = 0.0f, marsCamShakeY = 0.0f;
static float marsMoveSpeed = 0.0f;
static GLuint marsGasTexture = 0;

static const int   MT_CHUNK_CELLS = 40;
static const float MT_SPACING = 3.0f;
static const float MT_CHUNK_SIZE = MT_CHUNK_CELLS * MT_SPACING;
static const int   MT_LOD_COUNT = 4;
static const int   MT_LOD_STEP[MT_LOD_COUNT] = { 1, 2, 5, 10 };
static const int   MT_VIEW_CHUNKS = 8;

struct MarsRock {
    float x, y, z, size, rotY, rotAxis;
    int shape;
    float r, g, b;
};

struct MarsChunk {
    int cx, cz;
    GLuint lists[MT_LOD_COUNT];
    bool valid;
    std::vector<MarsRock> rocks;
};

static std::vector<MarsChunk*> activeMarsChunks;
static std::unordered_set<long long> activeMarsChunkKeys;
static bool mtBuilt = false;

struct MarsParticle { float x, y, z, vx, vy, vz, life, size; };
static std::vector<MarsParticle> marsParticles;

static const float MARS_SUN_AZ = 0.85f;
static const float MARS_SUN_EL = 0.32f;

// Venus volcanic surface exploration system — infinite chunk streaming
static bool inVenusTerrainMode = false;
static bool transToVenusTerrain = false;

// Venus now uses the same streaming philosophy as Earth/Mars:
// chunks are generated around the player, kept in a small active window,
// and unloaded when far away. No fixed terrain boundary.
static const int   VT_CHUNK_CELLS = 48;
static const float VT_SPACING = 1.0f;
static const float VT_CHUNK_SIZE = VT_CHUNK_CELLS * VT_SPACING;
static const int   VT_LOD_COUNT = 4;
static const int   VT_LOD_STEP[VT_LOD_COUNT] = { 1, 2, 4, 8 };
static const int   VT_VIEW_CHUNKS = 5;
static const int   VT_PRELOAD_CHUNKS = 6;
static const int   VT_MAX_CHUNKS_PER_FRAME = 2;

struct VenusTerrainChunk {
    int cx, cz;
    GLuint lists[VT_LOD_COUNT];
    bool valid;
    std::vector<DetailObj> details;
};

struct VenusChunkCandidate {
    int cx, cz;
    float priority;
};

static std::vector<VenusTerrainChunk*> activeVenusChunks;
static std::unordered_set<long long> activeVenusChunkKeys;
static std::vector<VenusChunkCandidate> cachedVenusCandidates;
static int lastVenusStreamCX = 999999;
static int lastVenusStreamCZ = 999999;

static bool vtBuilt = false;
static bool showVenusTerrainHUD = true;

static const float VT_SUN_AZ = 0.55f;
static const float VT_SUN_EL = 0.30f;

// Neptune icy interior exploration system
static bool inNeptuneMode = false;
static bool transToNeptune = false;
static float nCamX = 0.0f, nCamY = 3.0f, nCamZ = 0.0f;
static float nCamYaw = 0.0f, nCamPitch = -5.0f;
static float targetNCamX = 0.0f, targetNCamY = 3.0f, targetNCamZ = 0.0f;
static float targetNCamYaw = 0.0f, targetNCamPitch = -5.0f;

struct NeptuneSnowflake {
    float x, y, z;
    float speed, drift, size;
};

// Tuned down from the donor's 25,000 particles for smoother real-time performance.
static const int NEPTUNE_SNOW_COUNT = 6000;
static NeptuneSnowflake nepSnow[NEPTUNE_SNOW_COUNT];
static bool nepSnowInit = false;
static bool showNeptuneHUD = true;


// Mercury rocky surface exploration system — harsh, crater-heavy, airless world
static bool inMercuryMode = false;
static bool transToMercury = false;
static bool showMercuryHUD = true;

static const int   MC_CHUNK_CELLS = 56;
static const float MC_SPACING = 1.35f;
static const float MC_CHUNK_SIZE = MC_CHUNK_CELLS * MC_SPACING;
static const int   MC_LOD_COUNT = 3;
static const int   MC_LOD_STEP[MC_LOD_COUNT] = { 1, 3, 7 };
static const int   MC_VIEW_CHUNKS = 5;
static const int   MC_PRELOAD_CHUNKS = 6;
static const int   MC_MAX_CHUNKS_PER_FRAME = 2;

struct MercuryChunk {
    int cx, cz;
    GLuint lists[MC_LOD_COUNT];
    bool valid;
};

struct MercuryChunkCandidate {
    int cx, cz;
    float priority;
};

static std::vector<MercuryChunk*> activeMercuryChunks;
static std::unordered_set<long long> activeMercuryChunkKeys;
static std::vector<MercuryChunkCandidate> cachedMercuryCandidates;
static int lastMercuryStreamCX = 999999;
static int lastMercuryStreamCZ = 999999;
static bool mcBuilt = false;

// Jupiter upper-atmosphere exploration system — floating storm-cloud layer, not rocky terrain
static bool inJupiterMode = false;
static bool transToJupiter = false;
static bool showJupiterHUD = true;

static float jCamX = 0.0f, jCamY = 12.0f, jCamZ = 0.0f;
static float jCamYaw = 0.0f, jCamPitch = -5.0f;
static float targetJCamX = 0.0f, targetJCamY = 12.0f, targetJCamZ = 0.0f;
static float targetJCamYaw = 0.0f, targetJCamPitch = -5.0f;
static float jMoveSpeed = 0.0f;
static float jStormPhase = 0.0f;
static float jLightning = 0.0f;
static float jCamShakeX = 0.0f, jCamShakeY = 0.0f;

struct JupiterParticle {
    float x, y, z;
    float speed, drift, size;
};

static const int JUPITER_PARTICLE_COUNT = 1800;
static JupiterParticle jParticles[JUPITER_PARTICLE_COUNT];
static bool jParticlesInit = false;

// Saturn upper-atmosphere exploration system — calm, pale, elegant cloud sea with visible rings
static bool inSaturnMode = false;
static bool transToSaturn = false;
static bool showSaturnHUD = true;

static float sCamX = 0.0f, sCamY = 18.0f, sCamZ = 0.0f;
static float sCamYaw = 8.0f, sCamPitch = -4.0f;
static float targetSCamX = 0.0f, targetSCamY = 18.0f, targetSCamZ = 0.0f;
static float targetSCamYaw = 8.0f, targetSCamPitch = -4.0f;
static float sMoveSpeed = 0.0f;
static float sCloudPhase = 0.0f;
static float sRingPhase = 0.0f;
static float sGlimmer = 0.0f;

struct SaturnMote {
    float x, y, z;
    float speed, drift, size, phase;
};

static const int SATURN_MOTE_COUNT = 1200;
static SaturnMote sMotes[SATURN_MOTE_COUNT];
static bool sMotesInit = false;

// Uranus atmospheric exploration system — cold, quiet methane haze, not a Neptune copy
static bool inUranusMode = false;
static bool transToUranus = false;
static bool showUranusHUD = true;

static float uCamX = 0.0f, uCamY = 14.0f, uCamZ = 0.0f;
static float uCamYaw = -12.0f, uCamPitch = -3.0f;
static float targetUCamX = 0.0f, targetUCamY = 14.0f, targetUCamZ = 0.0f;
static float targetUCamYaw = -12.0f, targetUCamPitch = -3.0f;
static float uMoveSpeed = 0.0f;
static float uHazePhase = 0.0f;
static float uAuroraPhase = 0.0f;

struct UranusHazeParticle {
    float x, y, z;
    float speed, drift, size, phase;
};

static const int URANUS_HAZE_COUNT = 1500;
static UranusHazeParticle uHaze[URANUS_HAZE_COUNT];
static bool uHazeInit = false;

static GLuint earthNightTexture = 0;
static GLuint earthCloudTexture = 0;

// Cached projection state to avoid rebuilding projection matrices every frame when nothing changed.
static int cachedWindowW = 1280;
static int cachedWindowH = 720;
static float cachedAspect = 1280.0f / 720.0f;
static bool projectionDirty = true;
static float lastProjectionFOV = -1.0f;
static float lastProjectionNear = -1.0f;
static float lastProjectionFar = -1.0f;

enum RenderScene {
    SCENE_SPACE,
    SCENE_MERCURY,
    SCENE_EARTH,
    SCENE_VENUS,
    SCENE_MARS,
    SCENE_JUPITER,
    SCENE_SATURN,
    SCENE_URANUS,
    SCENE_NEPTUNE
};

static RenderScene appliedRenderScene = (RenderScene)-1;

// =========================
// Asteroids
// =========================
struct Asteroid {
    float angle;
    float dist;
    float height;
    float size;
    float rot;
    float rotSpeed;
    float scaleX;
    float scaleY;
    float scaleZ;
    int shapeType;
};

std::vector<Asteroid> asteroids;

// =========================
// Meteor Shower System
// =========================
static bool showMeteorShower = true;
static const int METEOR_COUNT = 6;
static const int METEOR_TRAIL_POINTS = 20;

struct Meteor {
    int id;
    std::string name;
    float x, y, z;
    float vx, vy, vz;
    float scale;
    float life;
    float rot;
    float rotAxis[3];
    float trail[METEOR_TRAIL_POINTS][3];
};

static std::vector<Meteor> meteors;
static GLuint fireTexture = 0;
static GLUquadric* meteorQuadric = nullptr;
static int meteorCounter = 0;

static const float METEOR_RADIANT_X = -0.58f;
static const float METEOR_RADIANT_Y = -0.52f;
static const float METEOR_RADIANT_Z = 0.63f;
static const float METEOR_SPREAD = 12.0f * DEG2RAD;
static const float METEOR_RESPAWN_RADIUS = 260.0f;

// =========================
// OpenGL objects
// =========================
GLuint orbitsList = 0;
GLuint planetTextures[8] = { 0 };
GLuint sunTexture = 0;
GLuint asteroidTexture = 0;
GLuint skyboxTextures[6] = { 0 };
GLuint coneList = 0;
GLuint sphereList = 0;

bool texturesLoaded = false;
bool skyboxLoaded = false;

// =========================
// Celestial body
// =========================
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

std::vector<CelestialBody> planets = {
    {"Mercury", 6.0f, 0.38f, 4.7f, 0.5f, 7.0f, 0.7f, 0.7f, 0.7f, 0,
     "Mercury: Smallest planet. Extreme temperatures.", 0,0,0,0},

    {"Venus", 10.0f, 0.95f, 3.5f, 0.1f, 3.4f, 0.9f, 0.8f, 0.4f, 0,
     "Venus: Hottest planet. Thick toxic atmosphere.", 0,0,0,0},

    {"Earth", 14.0f, 1.0f, 2.9f, 20.0f, 0.0f, 0.2f, 0.5f, 1.0f, 1,
     "Earth: Home planet. Rich in resources.", 0,0,0,0},

    {"Mars", 20.0f, 0.53f, 2.4f, 19.0f, 1.85f, 1.0f, 0.4f, 0.2f, 2,
     "Mars: Red planet. Cold desert world.", 0,0,0,0},

    {"Jupiter", 35.0f, 2.8f, 1.3f, 45.0f, 1.3f, 0.9f, 0.7f, 0.5f, 79,
     "Jupiter: Largest planet. Strong gravity well.", 0,0,0,0},

    {"Saturn", 48.0f, 2.4f, 0.9f, 42.0f, 2.5f, 0.9f, 0.8f, 0.6f, 82,
     "Saturn: Famous for its ring system.", 0,0,0,0},

    {"Uranus", 62.0f, 1.6f, 0.6f, 30.0f, 0.8f, 0.5f, 0.8f, 0.9f, 27,
     "Uranus: Ice giant with a tilted axis.", 0,0,0,0},

    {"Neptune", 75.0f, 1.5f, 0.5f, 32.0f, 1.8f, 0.3f, 0.3f, 0.9f, 14,
     "Neptune: Deep blue and very windy.", 0,0,0,0}
};

// =========================
// Forward declarations
// =========================
void reshape(int w, int h);
void display();
void idle();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void passiveMotion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void special(int key, int x, int y);
void checkPlanetHover(int x, int y);
void updateCamera();

// Terrain forward declarations
// Mercury / Jupiter / Saturn / Uranus exploration forward declarations
static void startTransitionToMercury();
static void startTransitionToJupiter();
static void buildMercuryTerrain();
static void cleanupMercuryTerrain();
static void updateMercuryTerrainStreaming();
static float sampleMercuryTerrainHeight(float wx, float wz);
static void displayMercuryScene();
static void initJupiterParticles();
static void resetJupiterCamera();
static void updateJupiterCamera();
static void updateJupiterParticles();
static void displayJupiterScene();
static void startTransitionToSaturn();
static void initSaturnMotes();
static void resetSaturnCamera();
static void updateSaturnCamera();
static void updateSaturnMotes();
static void displaySaturnScene();
static void startTransitionToUranus();
static void initUranusHaze();
static void resetUranusCamera();
static void updateUranusCamera();
static void updateUranusHaze();
static void displayUranusScene();
static void startTransitionToTerrain();
static void startTransitionToMars();
static void startTransitionToSpace();
static void updateTransition();
static void drawTransitionOverlay();
static void displayTerrainScene();
static void displayMarsScene();
static void updateTerrainCamera();
static void updateMarsCamera();
static void updateMarsParticles();
static void createMarsGasTexture();
static void cleanupEarthTerrain();
static void cleanupMarsTerrain();
static void cleanupVenusTerrain();
static void resetSpaceLighting();
static void cleanupAllGLResources();
static void startTransitionToVenusTerrain();
static void buildVenusTerrainV2();
static void updateVenusTerrainStreaming();
static float sampleVenusTerrainHeight(float wx, float wz);
static void displayVenusTerrainScene();
static void startTransitionToNeptune();
static void initNeptuneSnow();
static void resetNeptuneCamera();
static void updateNeptuneCamera();
static void updateNeptuneSnow();
static void displayNeptuneScene();
static void initMeteors();
static void updateMeteors();
static void drawMeteorShower();
static void cleanupMeteors();
static void drawStartScreen();
static void drawControlsPanelOverlay();
static void drawPerformanceHUD();
static void drawCompetitionOverlays();
static void startCinematicTour();
static void stopCinematicTour();
static void updateCinematicTour();
static void begin2DOverlay(int& w, int& h);
static void end2DOverlay();

// =========================
// Helpers
// =========================
void drawString(void* font, float x, float y, const char* str) {
    glRasterPos2f(x, y);
    for (const char* c = str; *c != '\0'; ++c)
        glutBitmapCharacter(font, *c);
}

float clampf(float v, float minV, float maxV) {
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

float smoothstepf(float edge0, float edge1, float x) {
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

int clampI(int v, int minV, int maxV) {
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

static inline long long makeChunkKey(int cx, int cz) {
    return (static_cast<long long>(cx) << 32) ^ static_cast<unsigned int>(cz);
}

static void applyPerspectiveIfNeeded(float fov, float nearPlane, float farPlane) {
    if (!projectionDirty &&
        fabsf(fov - lastProjectionFOV) < 0.001f &&
        fabsf(nearPlane - lastProjectionNear) < 0.001f &&
        fabsf(farPlane - lastProjectionFar) < 0.001f) {
        glMatrixMode(GL_MODELVIEW);
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, cachedAspect, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);

    lastProjectionFOV = fov;
    lastProjectionNear = nearPlane;
    lastProjectionFar = farPlane;
    projectionDirty = false;
}

static float noiseHash(int x, int y) {
    int n = x + y * 7927;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

static float noiseHash2(int x, int y) {
    int n = x * 1619 + y * 31337;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

static float smoothedNoise(float x, float y) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - ix;
    float fy = y - iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float v00 = noiseHash(ix, iy);
    float v10 = noiseHash(ix + 1, iy);
    float v01 = noiseHash(ix, iy + 1);
    float v11 = noiseHash(ix + 1, iy + 1);
    float i0 = v00 + fx * (v10 - v00);
    float i1 = v01 + fx * (v11 - v01);
    return i0 + fy * (i1 - i0);
}

static float smoothedNoise2(float x, float y) {
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - ix;
    float fy = y - iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float v00 = noiseHash2(ix, iy);
    float v10 = noiseHash2(ix + 1, iy);
    float v01 = noiseHash2(ix, iy + 1);
    float v11 = noiseHash2(ix + 1, iy + 1);
    float i0 = v00 + fx * (v10 - v00);
    float i1 = v01 + fx * (v11 - v01);
    return i0 + fy * (i1 - i0);
}

static float fbmNoise(float x, float y, int octaves, float persistence = 0.5f) {
    float value = 0.0f, amplitude = 1.0f, freq = 1.0f, maxVal = 0.0f;
    for (int i = 0; i < octaves; i++) {
        value += smoothedNoise(x * freq, y * freq) * amplitude;
        maxVal += amplitude;
        amplitude *= persistence;
        freq *= 2.0f;
    }
    return value / maxVal;
}

static float ridgeNoise(float x, float y, int octaves) {
    float value = 0.0f, amplitude = 1.0f, freq = 1.0f, maxVal = 0.0f;
    for (int i = 0; i < octaves; i++) {
        float n = smoothedNoise(x * freq, y * freq);
        n = 1.0f - fabsf(n);
        n = n * n;
        value += n * amplitude;
        maxVal += amplitude;
        amplitude *= 0.45f;
        freq *= 2.1f;
    }
    return value / maxVal;
}

static float warpedFBM(float x, float y, int octaves, float warpStr = 25.0f) {
    float wx = fbmNoise(x * 0.003f + 100.0f, y * 0.003f + 200.0f, 3) * warpStr;
    float wy = fbmNoise(x * 0.003f + 300.0f, y * 0.003f + 400.0f, 3) * warpStr;
    return fbmNoise((x + wx) * 0.005f, (y + wy) * 0.005f, octaves, 0.55f);
}

static float plateauNoise(float x, float y, int octaves) {
    float n = fbmNoise(x, y, octaves, 0.5f);
    float steps = 5.0f;
    return floorf(n * steps + 0.5f) / steps;
}

static float seamlessNoise(float u, float v, float scaleX, float scaleY) {
    float x1 = u * scaleX;
    float x2 = (u - 1.0f) * scaleX;
    float y = v * scaleY;
    float w = u;
    w = w * w * (3.0f - 2.0f * w);
    return fbmNoise(x1, y, 4, 0.5f) * (1.0f - w) + fbmNoise(x2, y, 4, 0.5f) * w;
}


float wrapAngle360(float a) {
    while (a < 0.0f) a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
}

float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

bool checkSphereCollision(float x1, float y1, float z1, float r1,
    float x2, float y2, float z2, float r2) {
    return distance3D(x1, y1, z1, x2, y2, z2) <= (r1 + r2);
}

static float randRange(float minV, float maxV) {
    return minV + (maxV - minV) * ((float)rand() / (float)RAND_MAX);
}

static void normalize3(float& x, float& y, float& z) {
    float len = sqrtf(x * x + y * y + z * z);
    if (len < 0.00001f) {
        x = 0.0f; y = 1.0f; z = 0.0f;
        return;
    }
    x /= len; y /= len; z /= len;
}

float getPlanetMinDistance(int idx) {
    if (idx < 0 || idx >= (int)planets.size()) return 4.0f;
    return planets[idx].radius * 3.0f + 1.2f;
}

float getPlanetMaxDistance(int idx) {
    if (idx < 0 || idx >= (int)planets.size()) return 160.0f;
    return planets[idx].radius * 22.0f + 20.0f;
}

float getFocusOffsetLimit(int idx) {
    if (idx < 0 || idx >= (int)planets.size()) return 2.0f;
    float base = planets[idx].radius * 2.6f;
    if (cameraMode == CAMERA_INSPECT) base = planets[idx].radius * 1.5f;
    return clampf(base, 0.8f, 8.0f);
}

float getExploreMinDistance(int idx) {
    if (idx < 0 || idx >= (int)planets.size()) return 0.2f;
    return clampf(planets[idx].radius * 0.08f + 0.05f, 0.08f, 0.45f);
}

float getExploreMaxDistance(int idx) {
    if (idx < 0 || idx >= (int)planets.size()) return 2.0f;
    return clampf(planets[idx].radius * 1.8f, 0.7f, 4.0f);
}

GLuint loadTexture(const char* filename) {
    int width, height, channels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(filename, &width, &height, &channels, 3);

    if (!data) {
        printf("Failed to load texture: %s\n", filename);
        return 0;
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        data
    );

    stbi_image_free(data);

    printf("Loaded texture: %s\n", filename);
    return textureID;
}

bool LoadPlanetTextures() {
    bool ok = true;

    sunTexture = loadTexture("sun.jpg");
    if (sunTexture == 0) ok = false;

    const char* planetFiles[8] = {
        "mercury.jpg",
        "venus.jpg",
        "earth.jpg",
        "mars.jpg",
        "jupiter.jpg",
        "saturn.jpg",
        "uranus.jpg",
        "neptune.jpg"
    };

    for (int i = 0; i < 8; i++) {
        planetTextures[i] = loadTexture(planetFiles[i]);

        if (planetTextures[i] == 0) {
            ok = false;
        }
    }

    texturesLoaded = ok;

    if (!ok) {
        printf("Warning: Some textures failed to load. Program will continue with fallback colors.\n");
    }

    return ok;
}
bool LoadSkyboxTextures() {
    bool ok = true;

    const char* skyboxFiles[6] = {
        "skybox1.jpg",
        "skybox2.jpg",
        "skybox3.jpg",
        "skybox4.jpg",
        "skybox5.jpg",
        "skybox6.jpg"
    };

    for (int i = 0; i < 6; i++) {
        skyboxTextures[i] = loadTexture(skyboxFiles[i]);

        if (skyboxTextures[i] == 0) {
            ok = false;
        }
    }

    skyboxLoaded = ok;

    if (!ok) {
        printf("Warning: Some skybox textures failed to load.\n");
    }

    return ok;
}
void setMaterialColor(const GLfloat ambient[4], const GLfloat diffuse[4],
    const GLfloat specular[4], float shininess) {
    GLfloat sh[1] = { shininess };
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, sh);
}

void applyPlanetMaterial(const CelestialBody& p, int idx, bool highlighted, bool textured) {
    GLfloat ambient[4] = { 0.03f, 0.03f, 0.03f, 1.0f };
    GLfloat diffuse[4] = { textured ? 1.0f : p.r, textured ? 1.0f : p.g, textured ? 1.0f : p.b, 1.0f };
    GLfloat specular[4] = { 0.18f, 0.18f, 0.18f, 1.0f };
    float shininess = 20.0f;

    switch (idx) {
    case 0:
        ambient[0] = ambient[1] = ambient[2] = 0.04f;
        specular[0] = specular[1] = specular[2] = 0.06f;
        shininess = 8.0f;
        break;
    case 1:
        ambient[0] = 0.06f; ambient[1] = 0.05f; ambient[2] = 0.03f;
        specular[0] = 0.16f; specular[1] = 0.14f; specular[2] = 0.10f;
        shininess = 14.0f;
        break;
    case 2:
        ambient[0] = 0.02f; ambient[1] = 0.03f; ambient[2] = 0.05f;
        specular[0] = 0.35f; specular[1] = 0.35f; specular[2] = 0.38f;
        shininess = 42.0f;
        break;
    case 3:
        ambient[0] = 0.05f; ambient[1] = 0.03f; ambient[2] = 0.02f;
        specular[0] = 0.10f; specular[1] = 0.08f; specular[2] = 0.08f;
        shininess = 10.0f;
        break;
    case 4:
        ambient[0] = 0.05f; ambient[1] = 0.04f; ambient[2] = 0.03f;
        specular[0] = 0.18f; specular[1] = 0.17f; specular[2] = 0.15f;
        shininess = 16.0f;
        break;
    case 5:
        ambient[0] = 0.05f; ambient[1] = 0.045f; ambient[2] = 0.03f;
        specular[0] = 0.14f; specular[1] = 0.14f; specular[2] = 0.12f;
        shininess = 12.0f;
        break;
    case 6:
        ambient[0] = 0.03f; ambient[1] = 0.05f; ambient[2] = 0.06f;
        specular[0] = 0.28f; specular[1] = 0.30f; specular[2] = 0.32f;
        shininess = 34.0f;
        break;
    case 7:
        ambient[0] = 0.02f; ambient[1] = 0.03f; ambient[2] = 0.06f;
        specular[0] = 0.32f; specular[1] = 0.34f; specular[2] = 0.40f;
        shininess = 38.0f;
        break;
    }

    if (highlighted) {
        specular[0] = clampf(specular[0] + 0.08f, 0.0f, 1.0f);
        specular[1] = clampf(specular[1] + 0.08f, 0.0f, 1.0f);
        specular[2] = clampf(specular[2] + 0.08f, 0.0f, 1.0f);
        shininess += 8.0f;
    }

    setMaterialColor(ambient, diffuse, specular, shininess);
}

void applySatelliteMaterial(float brightness = 1.0f) {
    GLfloat ambient[4] = { 0.07f * brightness, 0.07f * brightness, 0.08f * brightness, 1.0f };
    GLfloat diffuse[4] = { 0.75f * brightness, 0.75f * brightness, 0.80f * brightness, 1.0f };
    GLfloat specular[4] = { 0.65f * brightness, 0.65f * brightness, 0.70f * brightness, 1.0f };
    setMaterialColor(ambient, diffuse, specular, 55.0f);
}

void applyAsteroidMaterial() {
    GLfloat ambient[4] = { 0.06f, 0.06f, 0.06f, 1.0f };
    GLfloat diffuse[4] = { 0.70f, 0.70f, 0.70f, 1.0f };
    GLfloat specular[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
    setMaterialColor(ambient, diffuse, specular, 10.0f);
}

static void buildPrimitiveLists() {
    if (coneList == 0) {
        coneList = glGenLists(1);
        glNewList(coneList, GL_COMPILE);
        glutSolidCone(1.0f, 1.0f, 8, 4);
        glEndList();
    }

    if (sphereList == 0) {
        sphereList = glGenLists(1);
        glNewList(sphereList, GL_COMPILE);
        glutSolidSphere(1.0f, 16, 16);
        glEndList();
    }
}

// =========================
// Camera Management
// =========================
void clearFocusOffsets() {
    focusOffsetX = targetFocusOffsetX = 0.0f;
    focusOffsetY = targetFocusOffsetY = 0.0f;
    focusOffsetZ = targetFocusOffsetZ = 0.0f;
}

void clearExploreState() {
    exploreAnchorLat = targetExploreAnchorLat = 0.0f;
    exploreAnchorLon = targetExploreAnchorLon = 0.0f;
    exploreLookYaw = targetExploreLookYaw = 0.0f;
    exploreLookPitch = targetExploreLookPitch = -10.0f;
    exploreDistance = targetExploreDistance = 0.35f;
}

void resetCamera() {
    camYaw = targetCamYaw = -45.0f;
    camPitch = targetCamPitch = 45.0f;
    camDistance = targetCamDistance = 90.0f;

    camTargetX = targetLookX = 0.0f;
    camTargetY = targetLookY = 0.0f;
    camTargetZ = targetLookZ = 0.0f;

    cameraFOV = targetFOV = 55.0f;
    focusedPlanetIndex = -1;
    cameraMode = CAMERA_FREE;
    clearFocusOffsets();
    clearExploreState();
}

void focusPlanet(int index) {
    if (index < 0 || index >= (int)planets.size()) return;

    bool samePlanet = (focusedPlanetIndex == index);
    focusedPlanetIndex = index;
    cameraMode = CAMERA_FOCUS;

    if (!samePlanet) {
        clearFocusOffsets();
        clearExploreState();
    }

    targetLookX = planets[index].curX;
    targetLookY = planets[index].curY;
    targetLookZ = planets[index].curZ;

    if (!samePlanet) {
        if (index == 2) {
            targetCamDistance = 9.0f;
            targetFOV = 28.0f;
            targetCamYaw = -25.0f;
            targetCamPitch = 25.0f;
        }
        else if (index == 4 || index == 5) {
            targetCamDistance = 18.0f;
            targetFOV = 32.0f;
            targetCamYaw = -35.0f;
            targetCamPitch = 25.0f;
        }
        else {
            targetCamDistance = 12.0f;
            targetFOV = 30.0f;
            targetCamYaw = -35.0f;
            targetCamPitch = 22.0f;
        }
    }
}

void inspectPlanet(int index) {
    if (index < 0 || index >= (int)planets.size()) return;

    bool samePlanet = (focusedPlanetIndex == index);
    focusedPlanetIndex = index;
    cameraMode = CAMERA_INSPECT;

    if (!samePlanet) {
        clearFocusOffsets();
        clearExploreState();
    }

    targetLookX = planets[index].curX;
    targetLookY = planets[index].curY;
    targetLookZ = planets[index].curZ;

    targetCamDistance = clampf(
        getPlanetMinDistance(index) + 0.8f,
        getPlanetMinDistance(index),
        getPlanetMaxDistance(index)
    );
    targetFOV = 22.0f;
    targetCamYaw = -20.0f;
    targetCamPitch = 15.0f;
}

void enterExploreMode(int index) {
    if (index < 0 || index >= (int)planets.size()) return;

    // Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, and Neptune use dedicated exploration systems.
    if (index == 0) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToMercury();
        return;
    }
    if (index == 1) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToVenusTerrain();
        return;
    }
    if (index == 2) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToTerrain();
        return;
    }
    if (index == 3) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToMars();
        return;
    }
    if (index == 4) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToJupiter();
        return;
    }
    if (index == 5) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToSaturn();
        return;
    }
    if (index == 6) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToUranus();
        return;
    }
    if (index == 7) {
        focusedPlanetIndex = index;
        cameraMode = CAMERA_EXPLORE;
        startTransitionToNeptune();
        return;
    }

    focusedPlanetIndex = index;
    cameraMode = CAMERA_EXPLORE;
    clearFocusOffsets();

    targetExploreAnchorLat = exploreAnchorLat = 0.0f;
    targetExploreAnchorLon = exploreAnchorLon = 0.0f;
    targetExploreLookYaw = exploreLookYaw = 0.0f;
    targetExploreLookPitch = exploreLookPitch = -8.0f;

    float startDist = clampf(planets[index].radius * 0.25f, getExploreMinDistance(index), getExploreMaxDistance(index));
    targetExploreDistance = exploreDistance = startDist;

    targetFOV = 28.0f;
}

void toggleFocusExploreInspect(int index) {
    if (index < 0 || index >= (int)planets.size()) return;

    if (focusedPlanetIndex != index) {
        focusPlanet(index);
        return;
    }

    if (cameraMode == CAMERA_FOCUS) {
        enterExploreMode(index);
    }
    else if (cameraMode == CAMERA_EXPLORE) {
        inspectPlanet(index);
    }
    else if (cameraMode == CAMERA_INSPECT) {
        focusPlanet(index);
    }
    else {
        focusPlanet(index);
    }
}

void toggleFullscreen() {
    if (!isFullscreen) {
        glutFullScreen();
        isFullscreen = true;
    }
    else {
        glutReshapeWindow(1280, 720);
        glutPositionWindow(100, 100);
        isFullscreen = false;
    }
}

void updateCamera() {
    if (cameraMode == CAMERA_EXPLORE && focusedPlanetIndex != -1 && focusedPlanetIndex < (int)planets.size()) {
        exploreAnchorLat += (targetExploreAnchorLat - exploreAnchorLat) * 0.14f;
        exploreAnchorLon += (targetExploreAnchorLon - exploreAnchorLon) * 0.14f;
        exploreLookYaw += (targetExploreLookYaw - exploreLookYaw) * 0.14f;
        exploreLookPitch += (targetExploreLookPitch - exploreLookPitch) * 0.14f;
        exploreDistance += (targetExploreDistance - exploreDistance) * 0.14f;

        exploreAnchorLon = wrapAngle360(exploreAnchorLon);
        targetExploreAnchorLon = wrapAngle360(targetExploreAnchorLon);

        exploreAnchorLat = clampf(exploreAnchorLat, -85.0f, 85.0f);
        targetExploreAnchorLat = clampf(targetExploreAnchorLat, -85.0f, 85.0f);

        exploreLookPitch = clampf(exploreLookPitch, -89.0f, 89.0f);
        targetExploreLookPitch = clampf(targetExploreLookPitch, -89.0f, 89.0f);

        float mn = getExploreMinDistance(focusedPlanetIndex);
        float mx = getExploreMaxDistance(focusedPlanetIndex);
        exploreDistance = clampf(exploreDistance, mn, mx);
        targetExploreDistance = clampf(targetExploreDistance, mn, mx);

        cameraFOV += (targetFOV - cameraFOV) * 0.10f;
        return;
    }

    focusOffsetX += (targetFocusOffsetX - focusOffsetX) * 0.12f;
    focusOffsetY += (targetFocusOffsetY - focusOffsetY) * 0.12f;
    focusOffsetZ += (targetFocusOffsetZ - focusOffsetZ) * 0.12f;

    if (focusedPlanetIndex != -1 && focusedPlanetIndex < (int)planets.size() && !isPlanetSelectedView) {
        float trackSmooth = (cameraMode == CAMERA_INSPECT) ? 0.06f : 0.12f;

        float desiredLookX = planets[focusedPlanetIndex].curX + focusOffsetX;
        float desiredLookY = planets[focusedPlanetIndex].curY + focusOffsetY;
        float desiredLookZ = planets[focusedPlanetIndex].curZ + focusOffsetZ;

        targetLookX += (desiredLookX - targetLookX) * trackSmooth;
        targetLookY += (desiredLookY - targetLookY) * trackSmooth;
        targetLookZ += (desiredLookZ - targetLookZ) * trackSmooth;

        float minDist = getPlanetMinDistance(focusedPlanetIndex);
        float maxDist = getPlanetMaxDistance(focusedPlanetIndex);
        targetCamDistance = clampf(targetCamDistance, minDist, maxDist);
    }

    float MAX_PAN = 85.0f;
    targetLookX = clampf(targetLookX, -MAX_PAN, MAX_PAN);
    targetLookZ = clampf(targetLookZ, -MAX_PAN, MAX_PAN);
    targetLookY = clampf(targetLookY, -20.0f, 20.0f);

    float lerpDist = sqrtf(
        (targetLookX - camTargetX) * (targetLookX - camTargetX) +
        (targetLookZ - camTargetZ) * (targetLookZ - camTargetZ)
    );
    float panSmooth = (lerpDist > 20.0f) ? 0.055f : 0.12f;

    camYaw += (targetCamYaw - camYaw) * 0.10f;
    camPitch += (targetCamPitch - camPitch) * 0.10f;
    camDistance += (targetCamDistance - camDistance) * 0.10f;

    camTargetX += (targetLookX - camTargetX) * panSmooth;
    camTargetY += (targetLookY - camTargetY) * panSmooth;
    camTargetZ += (targetLookZ - camTargetZ) * panSmooth;

    cameraFOV += (targetFOV - cameraFOV) * 0.10f;

    camPitch = clampf(camPitch, -85.0f, 85.0f);
    camDistance = clampf(camDistance, 2.0f, 200.0f);
}

void applyCamera() {
    float yawRad = camYaw * PI / 180.0f;
    float pitchRad = camPitch * PI / 180.0f;

    float ex = camTargetX + camDistance * cosf(pitchRad) * sinf(yawRad);
    float ey = camTargetY + camDistance * sinf(pitchRad);
    float ez = camTargetZ + camDistance * cosf(pitchRad) * cosf(yawRad);

    gluLookAt(ex, ey, ez, camTargetX, camTargetY, camTargetZ, 0.0f, 1.0f, 0.0f);
}

void applyExploreCamera() {
    if (focusedPlanetIndex < 0 || focusedPlanetIndex >= (int)planets.size()) {
        applyCamera();
        return;
    }

    CelestialBody& p = planets[focusedPlanetIndex];

    float latRad = exploreAnchorLat * PI / 180.0f;
    float lonRad = (exploreAnchorLon + currentTime * p.rotationSpeed) * PI / 180.0f;

    float nx = cosf(latRad) * cosf(lonRad);
    float ny = sinf(latRad);
    float nz = cosf(latRad) * sinf(lonRad);

    float upX = nx;
    float upY = ny;
    float upZ = nz;

    float eastX = -sinf(lonRad);
    float eastY = 0.0f;
    float eastZ = cosf(lonRad);

    float northX = -sinf(latRad) * cosf(lonRad);
    float northY = cosf(latRad);
    float northZ = -sinf(latRad) * sinf(lonRad);

    float headingRad = exploreLookYaw * PI / 180.0f;
    float pitchRad = exploreLookPitch * PI / 180.0f;

    float forwardBaseX = sinf(headingRad) * eastX + cosf(headingRad) * northX;
    float forwardBaseY = sinf(headingRad) * eastY + cosf(headingRad) * northY;
    float forwardBaseZ = sinf(headingRad) * eastZ + cosf(headingRad) * northZ;

    float forwardX = cosf(pitchRad) * forwardBaseX + sinf(pitchRad) * upX;
    float forwardY = cosf(pitchRad) * forwardBaseY + sinf(pitchRad) * upY;
    float forwardZ = cosf(pitchRad) * forwardBaseZ + sinf(pitchRad) * upZ;

    float eyeX = p.curX + nx * (p.radius + exploreDistance);
    float eyeY = p.curY + ny * (p.radius + exploreDistance);
    float eyeZ = p.curZ + nz * (p.radius + exploreDistance);

    float lookX = eyeX + forwardX;
    float lookY = eyeY + forwardY;
    float lookZ = eyeZ + forwardZ;

    gluLookAt(eyeX, eyeY, eyeZ, lookX, lookY, lookZ, upX, upY, upZ);
}

// =========================
// Surface Terrain Exploration Systems
// =========================
static void createMarsGasTexture() {
    int w = 512, h = 256;
    unsigned char* data = new unsigned char[w * h * 3];

    for (int y = 0; y < h; y++) {
        float v = (float)y / h;
        float py = (v - 0.5f) * 2.0f;

        for (int x = 0; x < w; x++) {
            float u = (float)x / w;

            float n1 = seamlessNoise(u, v, 20.0f, 10.0f);
            float n2 = seamlessNoise(u, v, 40.0f, 20.0f);

            float bandFreq = 14.0f;
            float bandStr = sinf(py * PI * bandFreq + n1 * 3.0f);
            bandStr = (bandStr + 1.0f) * 0.5f;

            float turb = seamlessNoise(u + n2 * 0.1f, v + n1 * 0.1f, 15.0f, 15.0f);

            float spotU = fmodf(u + 0.3f, 1.0f);
            float spotDist = sqrtf(powf((spotU - 0.5f) * 2.0f, 2.0f) + powf((py - (-0.3f)) * 2.0f, 2.0f));
            float spot = 1.0f - smoothstepf(0.0f, 0.3f, spotDist + turb * 0.1f);

            float r = lerpf(0.85f, 0.95f, bandStr);
            float g = lerpf(0.40f, 0.65f, bandStr);
            float b = lerpf(0.20f, 0.35f, bandStr);

            r -= turb * 0.15f;
            g -= turb * 0.20f;
            b -= turb * 0.15f;

            if (spot > 0) {
                float sr = lerpf(r, 0.95f, spot);
                float sg = lerpf(g, 0.30f, spot);
                float sb = lerpf(b, 0.10f, spot);
                float ring = smoothstepf(0.15f, 0.25f, spotDist + turb * 0.1f) - smoothstepf(0.25f, 0.35f, spotDist + turb * 0.1f);
                sr = lerpf(sr, 0.9f, ring * 0.5f);
                sg = lerpf(sg, 0.7f, ring * 0.5f);
                sb = lerpf(sb, 0.4f, ring * 0.5f);
                r = sr; g = sg; b = sb;
            }

            data[(y * w + x) * 3 + 0] = (unsigned char)(clampf(r, 0.0f, 1.0f) * 255);
            data[(y * w + x) * 3 + 1] = (unsigned char)(clampf(g, 0.0f, 1.0f) * 255);
            data[(y * w + x) * 3 + 2] = (unsigned char)(clampf(b, 0.0f, 1.0f) * 255);
        }
    }

    glGenTextures(1, &marsGasTexture);
    glBindTexture(GL_TEXTURE_2D, marsGasTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
}

static float getTerrainHeightV2(float wx, float wz) {
    float warpX = fbmNoise(wx * 0.003f + 137.0f, wz * 0.003f + 243.0f, 3) * 28.0f;
    float warpZ = fbmNoise(wx * 0.003f + 371.0f, wz * 0.003f + 419.0f, 3) * 28.0f;
    float wx2 = wx + warpX;
    float wz2 = wz + warpZ;

    float continent = smoothedNoise(wx2 * 0.005f + 3.7f, wz2 * 0.005f + 2.1f);
    continent += smoothedNoise(wx2 * 0.010f + 1.2f, wz2 * 0.010f + 0.8f) * 0.45f;

    float landMask = smoothstepf(-0.08f, 0.18f, continent);

    float oceanBase = fbmNoise(wx * 0.015f + 50.0f, wz * 0.015f + 50.0f, 4, 0.45f) * 3.5f - 9.0f;
    float oceanTrench = ridgeNoise(wx * 0.008f + 70.0f, wz * 0.008f + 90.0f, 3) * 4.0f;
    float oceanFloor = oceanBase - oceanTrench * 0.5f;

    float mtWarpX = smoothedNoise2(wx * 0.007f + 5.0f, wz * 0.007f + 8.0f) * 12.0f;
    float mtWarpZ = smoothedNoise2(wx * 0.007f + 15.0f, wz * 0.007f + 18.0f) * 12.0f;
    float mountains = ridgeNoise((wx2 + mtWarpX) * 0.014f + 0.3f,
        (wz2 + mtWarpZ) * 0.014f + 1.7f, 6) * 20.0f;
    mountains *= landMask * landMask;

    float hills = fbmNoise(wx2 * 0.030f + 8.1f, wz2 * 0.030f + 3.3f, 5, 0.52f) * 4.5f;
    hills *= landMask;

    float plat = plateauNoise(wx2 * 0.012f + 20.0f, wz2 * 0.012f + 30.0f, 3) * 3.0f;
    plat *= landMask * smoothstepf(0.3f, 0.6f, continent);

    float detail = fbmNoise(wx * 0.10f + 0.5f, wz * 0.10f + 4.2f, 3, 0.48f) * 0.7f;
    detail *= landMask;

    float micro = smoothedNoise(wx * 0.4f + 11.0f, wz * 0.4f + 7.0f) * 0.15f;
    micro *= landMask;

    float h = continent * 4.5f;
    h += (1.0f - landMask) * oceanFloor;
    h += mountains + hills + plat + detail + micro;

    // River channels - carved into lowland terrain
    if (landMask > 0.3f && h > T2_WATER + 0.5f && h < 8.0f) {
        // Multiple river paths using different noise frequencies
        float rPath1 = sinf(wz * 0.018f + wx * 0.006f + fbmNoise(wx * 0.004f + 100.0f, wz * 0.004f + 200.0f, 2) * 8.0f);
        float rPath2 = sinf(wx * 0.015f - wz * 0.008f + fbmNoise(wx * 0.005f + 300.0f, wz * 0.005f + 400.0f, 2) * 6.0f);
        float rWidth1 = 0.12f + 0.06f * sinf(wz * 0.003f + 7.0f); // varying width
        float rWidth2 = 0.10f + 0.04f * sinf(wx * 0.004f + 3.0f);
        float rCarve1 = clampf(1.0f - fabsf(rPath1) / rWidth1, 0.0f, 1.0f);
        float rCarve2 = clampf(1.0f - fabsf(rPath2) / rWidth2, 0.0f, 1.0f);
        rCarve1 *= rCarve1; rCarve2 *= rCarve2;
        float riverDepth = fmaxf(rCarve1, rCarve2) * 3.2f;
        h -= riverDepth;
    }

    // Inland lake depressions
    float lakeNoise = smoothedNoise(wx * 0.008f + 500.0f, wz * 0.008f + 600.0f);
    if (lakeNoise > 0.65f && landMask > 0.4f && h > T2_WATER - 1.0f && h < 6.0f) {
        float lakeFactor = smoothstepf(0.65f, 0.80f, lakeNoise);
        h -= lakeFactor * 4.0f;
    }

    return h;
}

static void getTerrainColorV2(float h, float slope, float moisture,
    float& cr, float& cg, float& cb) {

    // Deep ocean abyss
    if (h < -8.0f) {
        cr = 0.005f; cg = 0.015f; cb = 0.06f;
    }
    // Deep ocean
    else if (h < -3.5f) {
        float t = (h + 8.0f) / 4.5f;
        cr = lerpf(0.005f, 0.015f, t);
        cg = lerpf(0.015f, 0.045f, t);
        cb = lerpf(0.06f, 0.16f, t);
    }
    // Shallow ocean
    else if (h < -1.0f) {
        float t = (h + 3.5f) / 2.5f;
        cr = lerpf(0.015f, 0.035f, t);
        cg = lerpf(0.045f, 0.12f, t);
        cb = lerpf(0.16f, 0.28f, t);
    }
    // Near-shore shallow water
    else if (h < T2_WATER + 0.3f) {
        float t = (h + 1.0f) / (T2_WATER + 1.3f);
        cr = lerpf(0.035f, 0.06f, t);
        cg = lerpf(0.12f, 0.20f, t);
        cb = lerpf(0.28f, 0.35f, t);
    }
    // Beach/sand
    else if (h < T2_WATER + 1.0f) {
        float t = (h - T2_WATER - 0.3f) / 0.7f;
        cr = lerpf(0.06f, 0.72f, t);
        cg = lerpf(0.20f, 0.65f, t);
        cb = lerpf(0.35f, 0.46f, t);
    }
    // Coastal lowlands
    else if (h < T2_WATER + 2.2f) {
        float t = (h - T2_WATER - 1.0f) / 1.2f;
        cr = lerpf(0.72f, 0.40f, t);
        cg = lerpf(0.65f, 0.52f, t);
        cb = lerpf(0.46f, 0.22f, t);
        float wet = 1.0f - clampf((h - T2_WATER - 1.0f) / 0.5f, 0.0f, 1.0f);
        cr -= wet * 0.10f; cg -= wet * 0.06f; cb -= wet * 0.02f;
        // Grass transition
        float grassT = clampf(t * moisture * 2.0f, 0.0f, 1.0f);
        cr = lerpf(cr, 0.15f, grassT); cg = lerpf(cg, 0.42f, grassT); cb = lerpf(cb, 0.08f, grassT);
    }
    // Lowland biomes
    else if (h < 5.0f) {
        float t = (h - T2_WATER - 2.2f) / (5.0f - T2_WATER - 2.2f);
        if (moisture > 0.60f) {
            // Lush forest
            cr = lerpf(0.10f, 0.06f, t); cg = lerpf(0.38f, 0.30f, t); cb = lerpf(0.05f, 0.03f, t);
        }
        else if (moisture > 0.40f) {
            // Temperate grassland
            cr = lerpf(0.22f, 0.18f, t); cg = lerpf(0.42f, 0.36f, t); cb = lerpf(0.08f, 0.06f, t);
        }
        else if (moisture > 0.20f) {
            // Dry savanna
            cr = lerpf(0.48f, 0.42f, t); cg = lerpf(0.42f, 0.38f, t); cb = lerpf(0.18f, 0.14f, t);
        }
        else {
            // Arid/desert
            cr = lerpf(0.62f, 0.56f, t); cg = lerpf(0.52f, 0.46f, t); cb = lerpf(0.30f, 0.26f, t);
        }
    }
    // Highland/mountain forest
    else if (h < 9.0f) {
        float t = (h - 5.0f) / 4.0f;
        float slopeMix = clampf(slope * 3.5f, 0.0f, 1.0f);
        // Forest on flat areas
        float fr = lerpf(0.06f, 0.05f, t), fg = lerpf(0.26f, 0.18f, t), fb = lerpf(0.04f, 0.03f, t);
        // Rock on slopes
        float rr = lerpf(0.32f, 0.40f, t), rg = lerpf(0.28f, 0.36f, t), rb = lerpf(0.20f, 0.28f, t);
        cr = lerpf(fr, rr, slopeMix); cg = lerpf(fg, rg, slopeMix); cb = lerpf(fb, rb, slopeMix);
        // Moss on north-facing (gentle) slopes
        float moss = clampf((1.0f - slope) * moisture * 0.4f, 0.0f, 0.15f);
        cg += moss;
    }
    // Alpine/subalpine
    else if (h < 14.0f) {
        float t = (h - 9.0f) / 5.0f;
        cr = lerpf(0.36f, 0.48f, t); cg = lerpf(0.32f, 0.46f, t); cb = lerpf(0.26f, 0.44f, t);
        // Lichen and sparse vegetation
        float lichen = clampf(slope * 2.0f - 0.3f, 0.0f, 0.25f);
        cg += lichen * 0.06f; cr -= lichen * 0.02f;
    }
    // Snow transition
    else if (h < 17.0f) {
        float t = (h - 14.0f) / 3.0f;
        float snowAmount = t * (1.0f - clampf(slope * 4.0f, 0.0f, 0.8f));
        cr = lerpf(0.48f, 0.94f, snowAmount); cg = lerpf(0.46f, 0.94f, snowAmount); cb = lerpf(0.44f, 0.97f, snowAmount);
    }
    // Snow peaks
    else {
        float snowStrength = 1.0f - clampf(slope * 3.0f, 0.0f, 0.4f);
        cr = lerpf(0.48f, 0.96f, snowStrength); cg = lerpf(0.46f, 0.96f, snowStrength); cb = lerpf(0.44f, 0.99f, snowStrength);
    }

    // Ambient occlusion from slope
    float aoDarken = 1.0f - clampf(slope * 0.40f, 0.0f, 0.22f);
    cr *= aoDarken; cg *= aoDarken; cb *= aoDarken;
}

static float sampleTerrainHeightV2(float wx, float wz) {
    return getTerrainHeightV2(wx, wz);
}

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
        return "REGOLITH FLATS";
    }
    if (h < 10.0f) return "IRON RIDGE";
    if (h < 15.0f) return "OLYMPUS HIGHLANDS";
    return "SUMMIT PLATEAU";
}

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
    else if (inMercuryMode) {
        updateMercuryTerrainStreaming();
    }

    // Ground collision
    float groundH = sampleTerrainHeightV2(tCamX, tCamZ);
    float eyeOffset = 2.0f;
    if (inVenusTerrainMode) {
        groundH = sampleVenusTerrainHeight(tCamX, tCamZ);
        eyeOffset = 1.6f;
    }
    else if (inMercuryMode) {
        groundH = sampleMercuryTerrainHeight(tCamX, tCamZ);
        eyeOffset = 1.75f;
    }

    float minH = groundH + eyeOffset;
    if (minH < 1.8f) minH = 1.8f;
    if (tCamY < minH) tCamY = minH;
    if (targetTCamY < minH) targetTCamY = minH;
    targetTCamY = clampf(targetTCamY, 1.8f, 90.0f);
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

static void startTransitionToMercury() {
    if (transActive) return;
    transActive = true;
    transToMercury = true;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToSaturn = false;
    transToUranus = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 0;
}

static void startTransitionToTerrain() {
    if (transActive) return;
    transActive = true;
    transToTerrain = true;
    transToMercury = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToSaturn = false;
    transToUranus = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 2;
}

static void startTransitionToMars() {
    if (transActive) return;
    transActive = true;
    transToMars = true;
    transToMercury = false;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToJupiter = false;
    transToSaturn = false;
    transToUranus = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 3;
}

static void startTransitionToJupiter() {
    if (transActive) return;
    transActive = true;
    transToJupiter = true;
    transToMercury = false;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToSaturn = false;
    transToUranus = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 4;
}

static void startTransitionToSaturn() {
    if (transActive) return;
    transActive = true;
    transToSaturn = true;
    transToMercury = false;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToUranus = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 5;
}

static void startTransitionToUranus() {
    if (transActive) return;
    transActive = true;
    transToUranus = true;
    transToMercury = false;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToSaturn = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 6;
}

static void startTransitionToNeptune() {
    if (transActive) return;
    transActive = true;
    transToNeptune = true;
    transToMercury = false;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToSaturn = false;
    transToUranus = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 7;
}

static void startTransitionToVenusTerrain() {
    if (transActive) return;
    transActive = true;
    transToVenusTerrain = true;
    transToMercury = false;
    transToTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToSaturn = false;
    transToUranus = false;
    transToNeptune = false;
    transPastMid = false;
    transAlpha = 0.0f;
    lastExploredPlanet = 1;
}

static void startTransitionToSpace() {
    if (transActive) return;
    transActive = true;
    transToMercury = false;
    transToTerrain = false;
    transToVenusTerrain = false;
    transToMars = false;
    transToJupiter = false;
    transToSaturn = false;
    transToUranus = false;
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

            if (transToMercury) {
                resetTerrainCamera();
                tCamY = targetTCamY = 11.0f;
                tCamPitch = targetTCamPitch = -10.0f;
                if (!mcBuilt) buildMercuryTerrain();
                inMercuryMode = true;
                inVenusTerrainMode = false;
                inTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inUranusMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToVenusTerrain) {
                resetTerrainCamera();
                tCamY = targetTCamY = 22.0f;
                tCamPitch = targetTCamPitch = -12.0f;
                if (!vtBuilt) buildVenusTerrainV2();
                inVenusTerrainMode = true;
                inMercuryMode = false;
                inTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inUranusMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToTerrain) {
                resetTerrainCamera();
                if (!etBuilt) buildEarthTerrain();
                inTerrainMode = true;
                inMercuryMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inUranusMode = false;
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
                inMercuryMode = false;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inUranusMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToJupiter) {
                if (!jParticlesInit) initJupiterParticles();
                resetJupiterCamera();
                inJupiterMode = true;
                inMercuryMode = false;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inSaturnMode = false;
                inUranusMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToSaturn) {
                if (!sMotesInit) initSaturnMotes();
                resetSaturnCamera();
                inSaturnMode = true;
                inMercuryMode = false;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inUranusMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToUranus) {
                if (!uHazeInit) initUranusHaze();
                resetUranusCamera();
                inUranusMode = true;
                inMercuryMode = false;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inNeptuneMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else if (transToNeptune) {
                if (!nepSnowInit) initNeptuneSnow();
                resetNeptuneCamera();
                inNeptuneMode = true;
                inMercuryMode = false;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inUranusMode = false;
                fpsMouseActive = true;
                fpsFirstFrame = true;
                glutSetCursor(GLUT_CURSOR_NONE);
            }
            else {
                if (inMercuryMode) cleanupMercuryTerrain();
                if (inTerrainMode) cleanupEarthTerrain();
                if (inVenusTerrainMode) cleanupVenusTerrain();
                if (inMarsMode) cleanupMarsTerrain();
                inMercuryMode = false;
                inTerrainMode = false;
                inVenusTerrainMode = false;
                inMarsMode = false;
                inJupiterMode = false;
                inSaturnMode = false;
                inUranusMode = false;
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
    if (transToMercury || (transActive && lastExploredPlanet == 0 && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter && !transToNeptune)) {
        glColor4f(0.12f, 0.09f, 0.06f, eased * 0.62f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.52f, 0.36f, 0.18f, eased * 0.62f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToJupiter || (transActive && lastExploredPlanet == 4 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToSaturn && !transToUranus && !transToNeptune)) {
        glColor4f(0.28f, 0.10f, 0.02f, eased * 0.64f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.95f, 0.48f, 0.14f, eased * 0.64f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToSaturn || (transActive && lastExploredPlanet == 5 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter && !transToUranus && !transToNeptune)) {
        glColor4f(0.20f, 0.14f, 0.07f, eased * 0.60f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.94f, 0.74f, 0.42f, eased * 0.60f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToUranus || (transActive && lastExploredPlanet == 6 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter && !transToSaturn && !transToNeptune)) {
        glColor4f(0.01f, 0.10f, 0.13f, eased * 0.60f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.20f, 0.72f, 0.78f, eased * 0.60f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToMars || (transActive && lastExploredPlanet == 3 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToJupiter && !transToNeptune)) {
        glColor4f(0.18f, 0.06f, 0.02f, eased * 0.60f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.30f, 0.10f, 0.04f, eased * 0.60f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToVenusTerrain || (transActive && lastExploredPlanet == 1 && !transToMercury && !transToTerrain && !transToMars && !transToJupiter && !transToNeptune)) {
        glColor4f(0.28f, 0.10f, 0.02f, eased * 0.62f);
        glVertex2f(0, 0); glVertex2f((float)w, 0);
        glColor4f(0.78f, 0.28f, 0.07f, eased * 0.62f);
        glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    }
    else if (transToNeptune || (transActive && lastExploredPlanet == 7 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter)) {
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
        if (transToMercury || (transActive && lastExploredPlanet == 0 && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter && !transToNeptune)) {
            glColor4f(0.10f, 0.07f, 0.04f, eased * 0.48f);
        }
        else if (transToJupiter || (transActive && lastExploredPlanet == 4 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToSaturn && !transToUranus && !transToNeptune)) {
            glColor4f(0.26f, 0.08f, 0.02f, eased * 0.50f);
        }
        else if (transToSaturn || (transActive && lastExploredPlanet == 5 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter && !transToUranus && !transToNeptune)) {
            glColor4f(0.22f, 0.15f, 0.06f, eased * 0.46f);
        }
        else if (transToUranus || (transActive && lastExploredPlanet == 6 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToMars && !transToJupiter && !transToSaturn && !transToNeptune)) {
            glColor4f(0.02f, 0.12f, 0.15f, eased * 0.46f);
        }
        else if (transToMars || (transActive && lastExploredPlanet == 3 && !transToMercury && !transToTerrain && !transToVenusTerrain && !transToJupiter && !transToNeptune)) {
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

        if (transToMercury) {
            glColor4f(1.0f, 0.78f, 0.38f, textAlpha * 0.92f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 135.0f, h / 2.0f + 10.0f, "Entering Mercury Surface...");
            glColor4f(0.85f, 0.68f, 0.42f, textAlpha * 0.55f);
            const char* mercurySub = (!mcBuilt) ? "Generating crater-heavy terrain chunks..." : "Descending to airless crater field";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 128.0f, h / 2.0f - 15.0f, mercurySub);
        }
        else if (transToJupiter) {
            glColor4f(1.0f, 0.62f, 0.22f, textAlpha * 0.92f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 150.0f, h / 2.0f + 10.0f, "Entering Jupiter Storm Deck...");
            glColor4f(1.0f, 0.78f, 0.42f, textAlpha * 0.58f);
            const char* jupiterSub = (!jParticlesInit) ? "Initializing turbulent cloud particles..." : "Dropping into the upper atmosphere";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 130.0f, h / 2.0f - 15.0f, jupiterSub);
        }
        else if (transToSaturn) {
            glColor4f(1.0f, 0.86f, 0.55f, textAlpha * 0.92f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 150.0f, h / 2.0f + 10.0f, "Entering Saturn Ring Sky...");
            glColor4f(1.0f, 0.82f, 0.50f, textAlpha * 0.58f);
            const char* saturnSub = (!sMotesInit) ? "Preparing pale ring-lit cloud layers..." : "Floating beneath Saturn's rings";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 132.0f, h / 2.0f - 15.0f, saturnSub);
        }
        else if (transToUranus) {
            glColor4f(0.58f, 1.0f, 0.96f, textAlpha * 0.92f);
            drawString(GLUT_BITMAP_HELVETICA_18, w / 2.0f - 165.0f, h / 2.0f + 10.0f, "Entering Uranus Methane Haze...");
            glColor4f(0.45f, 0.92f, 0.90f, textAlpha * 0.56f);
            const char* uranusSub = (!uHazeInit) ? "Condensing quiet cyan haze layers..." : "Gliding through methane-blue silence";
            drawString(GLUT_BITMAP_HELVETICA_12, w / 2.0f - 125.0f, h / 2.0f - 15.0f, uranusSub);
        }
        else if (transToVenusTerrain) {
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
    if (inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode || cameraMode == CAMERA_EXPLORE) return;

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

static void displayTerrainScene() {
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


// ============================================================
// Neptune Interior Mode — Optimized Icy World Exploration
// ============================================================
static float getNeptuneTerrainHeight(float wx, float wz) {
    float base = fbmNoise(wx * 0.008f + 500.0f, wz * 0.008f + 700.0f, 4, 0.50f) * 2.0f;
    float mtn = ridgeNoise(wx * 0.015f + 300.0f, wz * 0.015f + 400.0f, 5) * 12.0f;
    float mask = smoothstepf(-0.1f, 0.4f, smoothedNoise(wx * 0.005f + 50.0f, wz * 0.005f + 60.0f));
    mtn *= mask * mask;
    float detail = fbmNoise(wx * 0.06f + 10.0f, wz * 0.06f + 20.0f, 3, 0.48f) * 0.8f;
    float micro = smoothedNoise(wx * 0.25f + 30.0f, wz * 0.25f + 40.0f) * 0.15f;
    return base + mtn + detail + micro;
}

static float nepWrap(float v, float half) {
    float range = half * 2.0f;
    v = fmodf(v + half, range);
    if (v < 0.0f) v += range;
    return v - half;
}

static void initNeptuneSnow() {
    for (int i = 0; i < NEPTUNE_SNOW_COUNT; i++) {
        nepSnow[i].x = (rand() % 2000 - 1000) * 0.1f;
        nepSnow[i].y = (rand() % 500) * 0.1f + 2.0f;
        nepSnow[i].z = (rand() % 2000 - 1000) * 0.1f;
        nepSnow[i].speed = 0.18f + (rand() % 30) * 0.005f;
        nepSnow[i].drift = (rand() % 100 - 50) * 0.002f;
        nepSnow[i].size = 1.2f + (rand() % 20) * 0.08f;
    }
    nepSnowInit = true;
}

static void resetNeptuneCamera() {
    nCamX = targetNCamX = 0.0f;
    nCamZ = targetNCamZ = 0.0f;
    nCamY = targetNCamY = 3.2f;
    nCamYaw = targetNCamYaw = 0.0f;
    nCamPitch = targetNCamPitch = -5.0f;
    keyW = keyA = keyS = keyD = false;
    terrainMoveSpeed = 0.0f;
}

static void updateNeptuneCamera() {
    float moveX = 0.0f;
    float moveZ = 0.0f;
    float speed = 38.0f * timeSpeed * deltaTime;

    if (keyW || keyA || keyS || keyD) {
        float yawRad = nCamYaw * DEG2RAD;
        float fx = sinf(yawRad);
        float fz = cosf(yawRad);
        float rx = cosf(yawRad);
        float rz = -sinf(yawRad);

        if (keyW) { moveX += fx * speed; moveZ += fz * speed; }
        if (keyS) { moveX -= fx * speed; moveZ -= fz * speed; }
        if (keyA) { moveX -= rx * speed; moveZ -= rz * speed; }
        if (keyD) { moveX += rx * speed; moveZ += rz * speed; }

        targetNCamX += moveX;
        targetNCamZ += moveZ;
    }

    float instantSpeed = (deltaTime > 0.0001f)
        ? sqrtf(moveX * moveX + moveZ * moveZ) / deltaTime
        : 0.0f;

    // Delta-time based smoothing.
    // These values mean:
    // higher number = faster response
    // lower number = smoother/slower response
    float speedSmooth = 1.0f - expf(-8.0f * deltaTime);
    float posSmooth = 1.0f - expf(-9.0f * deltaTime);
    float heightSmooth = 1.0f - expf(-7.0f * deltaTime);
    float lookSmooth = 1.0f - expf(-10.0f * deltaTime);

    terrainMoveSpeed += (instantSpeed - terrainMoveSpeed) * speedSmooth;

    nCamX += (targetNCamX - nCamX) * posSmooth;
    nCamZ += (targetNCamZ - nCamZ) * posSmooth;
    nCamY += (targetNCamY - nCamY) * heightSmooth;

    nCamYaw += (targetNCamYaw - nCamYaw) * lookSmooth;
    nCamPitch += (targetNCamPitch - nCamPitch) * lookSmooth;

    targetNCamPitch = clampf(targetNCamPitch, -85.0f, 85.0f);
    nCamPitch = clampf(nCamPitch, -85.0f, 85.0f);

    float groundH = getNeptuneTerrainHeight(nepWrap(nCamX, 100.0f), nepWrap(nCamZ, 100.0f)) + 1.8f;
    if (groundH < 1.8f) groundH = 1.8f;
    if (nCamY < groundH) nCamY = groundH;
    if (targetNCamY < groundH) targetNCamY = groundH;
    targetNCamY = clampf(targetNCamY, groundH, 70.0f);
}

static void updateNeptuneSnow() {
    if (!nepSnowInit) return;
    float frameScale = clampf(deltaTime * 60.0f, 0.0f, 3.0f);
    for (int i = 0; i < NEPTUNE_SNOW_COUNT; i++) {
        nepSnow[i].y -= nepSnow[i].speed * frameScale;
        nepSnow[i].x += (nepSnow[i].drift + sinf(currentTime * 2.5f + i * 0.1f) * 0.04f) * frameScale;
        nepSnow[i].z += cosf(currentTime * 2.0f + i * 0.07f) * 0.03f * frameScale;

        float dx = nepSnow[i].x - nCamX;
        float dz = nepSnow[i].z - nCamZ;
        if (nepSnow[i].y < 0.0f || dx * dx + dz * dz > 125.0f * 125.0f) {
            nepSnow[i].y = nCamY + 25.0f + (rand() % 150) * 0.1f;
            nepSnow[i].x = nCamX + (rand() % 2000 - 1000) * 0.1f;
            nepSnow[i].z = nCamZ + (rand() % 2000 - 1000) * 0.1f;
        }
    }
}

static void drawNeptuneSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);

    float skyR = 420.0f;
    int stacks = 18;
    int slices = 40;

    for (int si = 0; si < stacks; si++) {
        float t0 = (float)si / stacks;
        float t1 = (float)(si + 1) / stacks;
        float phi0 = t0 * PI * 0.5f;
        float phi1 = t1 * PI * 0.5f;
        float y0 = skyR * sinf(phi0);
        float y1 = skyR * sinf(phi1);
        float rad0 = skyR * cosf(phi0);
        float rad1 = skyR * cosf(phi1);

        glBegin(GL_QUAD_STRIP);
        for (int sj = 0; sj <= slices; sj++) {
            float theta = (float)sj / slices * TWO_PI;
            float cx = cosf(theta);
            float cz = sinf(theta);
            for (int v = 1; v >= 0; v--) {
                float t = (v == 1) ? t1 : t0;
                float vRad = (v == 1) ? rad1 : rad0;
                float vY = (v == 1) ? y1 : y0;
                float t2 = t * t;
                float storm = 0.5f + 0.5f * sinf(theta * 5.0f + currentTime * 0.25f + t * 4.0f);
                float br = lerpf(0.08f, 0.01f, t2) + storm * 0.015f;
                float bg = lerpf(0.12f, 0.02f, t2) + storm * 0.020f;
                float bb = lerpf(0.30f, 0.07f, t2) + storm * 0.035f;
                glColor3f(clampf(br, 0, 1), clampf(bg, 0, 1), clampf(bb, 0, 1));
                glVertex3f(nCamX + vRad * cx, vY, nCamZ + vRad * cz);
            }
        }
        glEnd();
    }

    glColor3f(0.02f, 0.03f, 0.06f);
    glBegin(GL_QUADS);
    glVertex3f(nCamX - skyR, -6.0f, nCamZ - skyR);
    glVertex3f(nCamX + skyR, -6.0f, nCamZ - skyR);
    glVertex3f(nCamX + skyR, -6.0f, nCamZ + skyR);
    glVertex3f(nCamX - skyR, -6.0f, nCamZ + skyR);
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);
}

static void drawNeptuneGround() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glDisable(GL_TEXTURE_2D);

    GLfloat tSpec[] = { 0.3f, 0.35f, 0.45f, 1.0f };
    GLfloat tSh[] = { 30.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, tSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, tSh);

    const float halfW = 110.0f;
    const float spacing = 2.0f;
    const int gridN = (int)(halfW * 2.0f / spacing);
    const int gp = gridN + 1;

    float ox = floorf(nCamX / spacing) * spacing;
    float oz = floorf(nCamZ / spacing) * spacing;

    std::vector<float> heights(gp * gp);
    for (int j = 0; j < gp; j++) {
        for (int i = 0; i < gp; i++) {
            float wx = ox + (i - gridN / 2) * spacing;
            float wz = oz + (j - gridN / 2) * spacing;
            heights[j * gp + i] = getNeptuneTerrainHeight(nepWrap(wx, halfW), nepWrap(wz, halfW));
        }
    }

    for (int j = 0; j < gridN; j++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= gridN; i++) {
            for (int dj = 1; dj >= 0; dj--) {
                int cj = clampI(j + dj, 0, gridN);
                int idx = cj * gp + i;
                float wx = ox + (i - gridN / 2) * spacing;
                float wz = oz + (cj - gridN / 2) * spacing;
                float h = heights[idx];

                int il = clampI(i - 1, 0, gridN);
                int ir = clampI(i + 1, 0, gridN);
                int jd = clampI(cj - 1, 0, gridN);
                int ju = clampI(cj + 1, 0, gridN);
                float hL = heights[cj * gp + il];
                float hR = heights[cj * gp + ir];
                float hD = heights[jd * gp + i];
                float hU = heights[ju * gp + i];

                float nx = (hL - hR) / (2.0f * spacing);
                float nz = (hD - hU) / (2.0f * spacing);
                float ny = 1.0f;
                float len = sqrtf(nx * nx + ny * ny + nz * nz);
                glNormal3f(nx / len, ny / len, nz / len);

                float slope = 1.0f - ny / len;
                float snowT = 1.0f - clampf(slope * 4.8f, 0.0f, 1.0f);
                float cr = lerpf(0.24f, 0.82f, snowT);
                float cg = lerpf(0.28f, 0.86f, snowT);
                float cb = lerpf(0.38f, 0.96f, snowT);
                float nvar = smoothedNoise(nepWrap(wx, halfW) * 0.1f + 99.0f, nepWrap(wz, halfW) * 0.1f + 77.0f) * 0.045f;
                cr += nvar;
                cg += nvar;
                cb += nvar * 1.25f;

                float dist = sqrtf((wx - nCamX) * (wx - nCamX) + (wz - nCamZ) * (wz - nCamZ));
                float fade = 1.0f - smoothstepf(halfW * 0.78f, halfW, dist);
                cr *= fade; cg *= fade; cb *= fade;

                glColor3f(clampf(cr, 0, 1), clampf(cg, 0, 1), clampf(cb, 0, 1));
                glVertex3f(wx, h, wz);
            }
        }
        glEnd();
    }
}

static void drawNeptuneMountains() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glDisable(GL_TEXTURE_2D);

    float viewDist = 85.0f;
    float halfW = 110.0f;
    float spacing = 2.0f;
    int step = 10;
    float ox = floorf(nCamX / (step * spacing)) * (step * spacing);
    float oz = floorf(nCamZ / (step * spacing)) * (step * spacing);

    for (int j = -5; j <= 5; j++) {
        for (int i = -5; i <= 5; i++) {
            float bx = ox + i * step * spacing;
            float bz = oz + j * step * spacing;
            float dx = bx - nCamX;
            float dz = bz - nCamZ;
            if (dx * dx + dz * dz > viewDist * viewDist) continue;

            float twx = nepWrap(bx, halfW);
            float twz = nepWrap(bz, halfW);
            float rng = (noiseHash((int)(twx * 7.0f) + 123, (int)(twz * 7.0f) + 456) + 1.0f) * 0.5f;
            if (rng < 0.58f) continue;

            float baseH = getNeptuneTerrainHeight(twx, twz);
            float mtnH = rng * 6.5f + 2.0f;
            float mtnR = rng * 3.2f + 1.3f;

            glPushMatrix();
            glTranslatef(bx, baseH, bz);
            glColor3f(0.68f + rng * 0.16f, 0.74f + rng * 0.12f, 0.90f + rng * 0.07f);
            glPushMatrix();
            glScalef(mtnR, mtnH, mtnR * 0.8f);
            if (coneList != 0) glCallList(coneList); else glutSolidCone(1.0f, 1.0f, 8, 4);
            glPopMatrix();

            glColor3f(0.90f, 0.92f, 0.98f);
            glPushMatrix();
            glTranslatef(0.0f, mtnH * 0.65f, 0.0f);
            glScalef(mtnR * 0.5f, mtnH * 0.4f, mtnR * 0.4f);
            glutSolidCone(1.0f, 1.0f, 6, 2);
            glPopMatrix();
            glPopMatrix();
        }
    }
}

static void drawNeptuneSnow() {
    if (!nepSnowInit) return;
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(2.3f);
    glBegin(GL_POINTS);
    for (int i = 0; i < NEPTUNE_SNOW_COUNT; i++) {
        float dx = nepSnow[i].x - nCamX;
        float dz = nepSnow[i].z - nCamZ;
        float dist2 = dx * dx + dz * dz;
        if (dist2 > 95.0f * 95.0f) continue;
        float t = clampf(dist2 / (95.0f * 95.0f), 0.0f, 1.0f);
        float alpha = (1.0f - t) * 0.68f;
        glColor4f(0.90f, 0.93f, 1.0f, alpha);
        glVertex3f(nepSnow[i].x, nepSnow[i].y, nepSnow[i].z);
    }
    glEnd();

    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < NEPTUNE_SNOW_COUNT / 4; i++) {
        float dx = nepSnow[i].x - nCamX;
        float dz = nepSnow[i].z - nCamZ;
        float dist2 = dx * dx + dz * dz;
        if (dist2 > 32.0f * 32.0f) continue;
        float t = clampf(dist2 / (32.0f * 32.0f), 0.0f, 1.0f);
        float alpha = (1.0f - t) * 0.50f;
        glColor4f(0.96f, 0.97f, 1.0f, alpha);
        glVertex3f(nepSnow[i].x, nepSnow[i].y + 0.05f, nepSnow[i].z);
    }
    glEnd();

    glPointSize(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void drawNeptuneHUD() {
    if (!showNeptuneHUD) return;

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
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4f(0.01f, 0.02f, 0.06f, 0.78f);
    glBegin(GL_QUADS);
    glVertex2f(0, (float)(h - 42));
    glVertex2f((float)w, (float)(h - 42));
    glVertex2f((float)w, (float)h);
    glVertex2f(0, (float)h);
    glEnd();

    glColor4f(0.15f, 0.40f, 0.85f, 0.45f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(0, (float)(h - 42));
    glVertex2f((float)w, (float)(h - 42));
    glEnd();

    glColor4f(0.30f, 0.60f, 1.0f, 0.90f);
    drawString(GLUT_BITMAP_HELVETICA_18, 18.0f, (float)(h - 28), "NEPTUNE INTERIOR");

    float pulse = 0.6f + 0.4f * sinf(currentTime * 2.5f);
    glColor4f(0.3f, 0.5f, 1.0f, pulse);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    glVertex2f(210.0f, (float)(h - 26));
    glEnd();

    glColor4f(0.4f, 0.6f, 1.0f, 0.85f);
    drawString(GLUT_BITMAP_HELVETICA_12, 220.0f, (float)(h - 30), "ICY SURFACE / STORM ZONE");

    char altBuf[64];
    sprintf(altBuf, "ALT  %.0f m", nCamY * 100.0f);
    glColor4f(0.5f, 0.7f, 1.0f, 0.85f);
    drawString(GLUT_BITMAP_HELVETICA_18, (float)(w - 195), (float)(h - 28), altBuf);

    glColor4f(0.01f, 0.02f, 0.06f, 0.60f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f((float)w, 0);
    glVertex2f((float)w, 30); glVertex2f(0, 30);
    glEnd();

    glColor4f(0.45f, 0.55f, 0.75f, 0.60f);
    drawString(GLUT_BITMAP_HELVETICA_12, 18.0f, 10.0f, "WASD: Move  |  Mouse: Look  |  Wheel: Altitude  |  N/ESC: Exit Neptune  |  H: Toggle HUD");

    float cx = w / 2.0f;
    float cy = h / 2.0f;
    glColor4f(0.5f, 0.7f, 1.0f, 0.20f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(cx - 14, cy); glVertex2f(cx - 5, cy);
    glVertex2f(cx + 5, cy); glVertex2f(cx + 14, cy);
    glVertex2f(cx, cy - 14); glVertex2f(cx, cy - 5);
    glVertex2f(cx, cy + 5); glVertex2f(cx, cy + 14);
    glEnd();

    glPointSize(2.0f);
    glColor4f(0.5f, 0.7f, 1.0f, 0.30f);
    glBegin(GL_POINTS);
    glVertex2f(cx, cy);
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

static void displayNeptuneScene() {
    glClearColor(0.03f, 0.05f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    applyPerspectiveIfNeeded(55.0f, 0.15f, 850.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yawR = nCamYaw * DEG2RAD;
    float pitR = nCamPitch * DEG2RAD;
    float lookX = nCamX + cosf(pitR) * sinf(yawR);
    float lookY = nCamY + sinf(pitR);
    float lookZ = nCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(nCamX, nCamY, nCamZ, lookX, lookY, lookZ, 0, 1, 0);

    float fogDensity = 0.010f + 0.010f * (1.0f - clampf(nCamY / 45.0f, 0.0f, 1.0f));
    glEnable(GL_FOG);
    GLfloat fogCol[] = { 0.04f, 0.06f, 0.14f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, fogDensity);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    GLfloat sunDir[] = { 0.3f, 0.6f, 0.4f, 0.0f };
    GLfloat sunAmb[] = { 0.06f, 0.07f, 0.12f, 1.0f };
    GLfloat sunDif[] = { 0.35f, 0.40f, 0.65f, 1.0f };
    GLfloat sunSpc[] = { 0.30f, 0.35f, 0.50f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);

    GLfloat skyDir[] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat skyAmb[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat skyDif[] = { 0.05f, 0.06f, 0.12f, 1.0f };
    GLfloat skySpc[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, skyDir);
    glLightfv(GL_LIGHT1, GL_AMBIENT, skyAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, skyDif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, skySpc);

    drawNeptuneSky();
    drawNeptuneGround();
    drawNeptuneMountains();
    drawNeptuneSnow();

    glDisable(GL_LIGHT1);
    glDisable(GL_FOG);
    drawVignette(0.40f);
    drawNeptuneHUD();
}

// ============================================================
// Mercury Exploration Mode — Rocky Crater World
// ============================================================
static float mercuryCraterLayer(float wx, float wz, float cellSize, float minR, float maxR, float depth, int seed) {
    int baseX = (int)floorf(wx / cellSize);
    int baseZ = (int)floorf(wz / cellSize);
    float result = 0.0f;

    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = baseX + dx;
            int cz = baseZ + dz;
            float rx = (noiseHash(cx * 91 + seed, cz * 47 - seed) + 1.0f) * 0.5f;
            float rz = (noiseHash(cx * 53 - seed, cz * 89 + seed) + 1.0f) * 0.5f;
            float rr = (noiseHash(cx * 37 + seed * 3, cz * 71 - seed * 2) + 1.0f) * 0.5f;
            float centerX = (cx + rx) * cellSize;
            float centerZ = (cz + rz) * cellSize;
            float radius = lerpf(minR, maxR, rr);
            float ddx = wx - centerX;
            float ddz = wz - centerZ;
            float d = sqrtf(ddx * ddx + ddz * ddz);
            float nd = d / fmaxf(radius, 0.001f);

            if (nd < 1.0f) {
                float bowl = 1.0f - nd;
                result -= depth * bowl * bowl * (0.65f + 0.35f * cosf(nd * PI));
            }
            if (nd >= 0.72f && nd < 1.30f) {
                float rim = smoothstepf(0.72f, 0.92f, nd) - smoothstepf(1.03f, 1.30f, nd);
                result += depth * 0.42f * rim;
            }
            if (nd < 0.10f) result += depth * 0.12f * (1.0f - nd / 0.10f);
        }
    }
    return result;
}

static float getMercuryTerrainHeight(float wx, float wz) {
    float warpX = fbmNoise(wx * 0.004f + 111.0f, wz * 0.004f + 222.0f, 3) * 10.0f;
    float warpZ = fbmNoise(wx * 0.004f + 333.0f, wz * 0.004f + 444.0f, 3) * 10.0f;
    float x = wx + warpX;
    float z = wz + warpZ;
    float broad = fbmNoise(x * 0.006f + 20.0f, z * 0.006f + 30.0f, 5, 0.52f) * 5.5f;
    float ridges = ridgeNoise(x * 0.018f + 70.0f, z * 0.018f + 90.0f, 5) * 5.0f;
    float scarps = ridgeNoise(x * 0.040f + 10.0f, z * 0.020f + 15.0f, 4) * 1.8f;
    float craters = 0.0f;
    craters += mercuryCraterLayer(wx, wz, 62.0f, 8.0f, 22.0f, 8.5f, 5);
    craters += mercuryCraterLayer(wx + 13.0f, wz - 31.0f, 31.0f, 3.0f, 10.0f, 3.2f, 11);
    craters += mercuryCraterLayer(wx - 9.0f, wz + 19.0f, 14.0f, 1.2f, 4.4f, 1.05f, 17);
    float micro = smoothedNoise(wx * 0.45f + 7.0f, wz * 0.45f + 9.0f) * 0.22f;
    return broad + ridges + scarps + craters + micro;
}

static float sampleMercuryTerrainHeight(float wx, float wz) {
    return getMercuryTerrainHeight(wx, wz);
}

static void getMercuryColor(float h, float slope, float craterDark, float& cr, float& cg, float& cb) {
    float t = clampf((h + 9.0f) / 24.0f, 0.0f, 1.0f);
    cr = lerpf(0.30f, 0.64f, t);
    cg = lerpf(0.285f, 0.58f, t);
    cb = lerpf(0.255f, 0.49f, t);
    float ironTint = fbmNoise(h * 0.03f + 14.0f, t * 11.0f + 2.0f, 2, 0.5f) * 0.06f;
    cr += ironTint * 0.8f;
    cg += ironTint * 0.35f;
    cb -= ironTint * 0.20f;
    float shadow = 1.0f - clampf(slope * 0.65f + craterDark * 0.22f, 0.0f, 0.55f);
    cr *= shadow; cg *= shadow; cb *= shadow;
}

static MercuryChunk* generateMercuryChunk(int cx, int cz) {
    MercuryChunk* chunk = new MercuryChunk();
    chunk->cx = cx;
    chunk->cz = cz;
    chunk->valid = true;
    float baseWX = cx * MC_CHUNK_SIZE - MC_CHUNK_SIZE * 0.5f;
    float baseWZ = cz * MC_CHUNK_SIZE - MC_CHUNK_SIZE * 0.5f;
    int cells = MC_CHUNK_CELLS;
    int gp = cells + 1;
    std::vector<float> H(gp * gp), NX(gp * gp), NY(gp * gp), NZ(gp * gp), CR(gp * gp), CG(gp * gp), CB(gp * gp);

    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            float wx = baseWX + i * MC_SPACING;
            float wz = baseWZ + j * MC_SPACING;
            H[j * gp + i] = getMercuryTerrainHeight(wx, wz);
        }
    }

    for (int j = 0; j <= cells; j++) {
        for (int i = 0; i <= cells; i++) {
            int il = clampI(i - 1, 0, cells), ir = clampI(i + 1, 0, cells);
            int jd = clampI(j - 1, 0, cells), ju = clampI(j + 1, 0, cells);
            float hL = H[j * gp + il], hR = H[j * gp + ir];
            float hD = H[jd * gp + i], hU = H[ju * gp + i];
            float nx = (hL - hR) / (2.0f * MC_SPACING);
            float ny = 1.0f;
            float nz = (hD - hU) / (2.0f * MC_SPACING);
            normalize3(nx, ny, nz);
            int idx = j * gp + i;
            NX[idx] = nx; NY[idx] = ny; NZ[idx] = nz;
            float wx = baseWX + i * MC_SPACING;
            float wz = baseWZ + j * MC_SPACING;
            float slope = 1.0f - clampf(ny, 0.0f, 1.0f);
            float craterDark = clampf(-mercuryCraterLayer(wx, wz, 31.0f, 3.0f, 10.0f, 3.2f, 11) / 7.0f, 0.0f, 1.0f);
            getMercuryColor(H[idx], slope, craterDark, CR[idx], CG[idx], CB[idx]);
        }
    }

    for (int lod = 0; lod < MC_LOD_COUNT; lod++) {
        int step = MC_LOD_STEP[lod];
        chunk->lists[lod] = glGenLists(1);
        glNewList(chunk->lists[lod], GL_COMPILE);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
        glDisable(GL_TEXTURE_2D);

        for (int j = 0; j < cells; j += step) {
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= cells; i += step) {
                for (int dj = 1; dj >= 0; dj--) {
                    int cj = clampI(j + dj * step, 0, cells);
                    int idx = cj * gp + i;
                    float wx = baseWX + i * MC_SPACING;
                    float wz = baseWZ + cj * MC_SPACING;
                    glNormal3f(NX[idx], NY[idx], NZ[idx]);
                    glColor3f(CR[idx], CG[idx], CB[idx]);
                    glVertex3f(wx, H[idx], wz);
                }
            }
            glEnd();
        }

        if (lod == 0) {
            for (int j = 2; j < cells; j += 5) {
                for (int i = 2; i < cells; i += 5) {
                    float rng = (noiseHash(cx * 1009 + i * 37 + 13, cz * 917 + j * 41 + 19) + 1.0f) * 0.5f;
                    if (rng < 0.88f) continue;
                    float wx = baseWX + i * MC_SPACING;
                    float wz = baseWZ + j * MC_SPACING;
                    float h = getMercuryTerrainHeight(wx, wz);
                    float sz = 0.20f + rng * 0.55f;
                    glPushMatrix();
                    glTranslatef(wx, h + 0.05f, wz);
                    glRotatef(rng * 360.0f, 0.0f, 1.0f, 0.0f);
                    glScalef(sz * 1.25f, sz * 0.55f, sz * 0.85f);
                    glColor3f(0.38f + rng * 0.13f, 0.36f + rng * 0.11f, 0.32f + rng * 0.08f);
                    glutSolidDodecahedron();
                    glPopMatrix();
                }
            }
        }
        glEndList();
    }
    return chunk;
}

static bool mercuryChunkExists(int cx, int cz) {
    return activeMercuryChunkKeys.find(makeChunkKey(cx, cz)) != activeMercuryChunkKeys.end();
}

static void addMercuryChunk(int cx, int cz) {
    if (mercuryChunkExists(cx, cz)) return;
    MercuryChunk* chunk = generateMercuryChunk(cx, cz);
    activeMercuryChunks.push_back(chunk);
    activeMercuryChunkKeys.insert(makeChunkKey(cx, cz));
}

static void updateMercuryTerrainStreaming() {
    int playerCX = (int)roundf(tCamX / MC_CHUNK_SIZE);
    int playerCZ = (int)roundf(tCamZ / MC_CHUNK_SIZE);

    for (auto it = activeMercuryChunks.begin(); it != activeMercuryChunks.end();) {
        MercuryChunk* c = *it;
        int dx = c->cx - playerCX;
        int dz = c->cz - playerCZ;
        if (abs(dx) > MC_VIEW_CHUNKS + 2 || abs(dz) > MC_VIEW_CHUNKS + 2) {
            for (int i = 0; i < MC_LOD_COUNT; i++) if (c->lists[i]) glDeleteLists(c->lists[i], 1);
            activeMercuryChunkKeys.erase(makeChunkKey(c->cx, c->cz));
            delete c;
            it = activeMercuryChunks.erase(it);
        }
        else ++it;
    }

    if (playerCX != lastMercuryStreamCX || playerCZ != lastMercuryStreamCZ || cachedMercuryCandidates.empty()) {
        cachedMercuryCandidates.clear();
        lastMercuryStreamCX = playerCX;
        lastMercuryStreamCZ = playerCZ;
        float yawRad = tCamYaw * DEG2RAD;
        float fwdX = sinf(yawRad);
        float fwdZ = cosf(yawRad);
        for (int dz = -MC_PRELOAD_CHUNKS; dz <= MC_PRELOAD_CHUNKS; dz++) {
            for (int dx = -MC_PRELOAD_CHUNKS; dx <= MC_PRELOAD_CHUNKS; dx++) {
                int cx = playerCX + dx;
                int cz = playerCZ + dz;
                if (mercuryChunkExists(cx, cz)) continue;
                float dist = sqrtf((float)(dx * dx + dz * dz));
                float dirBias = 0.0f;
                if (dist > 0.5f) {
                    float dot = ((float)dx / dist) * fwdX + ((float)dz / dist) * fwdZ;
                    dirBias = (1.0f - dot) * 1.5f;
                }
                cachedMercuryCandidates.push_back({ cx, cz, dist + dirBias });
            }
        }
        std::sort(cachedMercuryCandidates.begin(), cachedMercuryCandidates.end(),
            [](const MercuryChunkCandidate& a, const MercuryChunkCandidate& b) { return a.priority < b.priority; });
    }

    int loaded = 0;
    for (auto it = cachedMercuryCandidates.begin(); it != cachedMercuryCandidates.end() && loaded < MC_MAX_CHUNKS_PER_FRAME;) {
        if (!mercuryChunkExists(it->cx, it->cz)) {
            addMercuryChunk(it->cx, it->cz);
            loaded++;
        }
        it = cachedMercuryCandidates.erase(it);
    }
}

static void buildMercuryTerrain() {
    int playerCX = (int)roundf(tCamX / MC_CHUNK_SIZE);
    int playerCZ = (int)roundf(tCamZ / MC_CHUNK_SIZE);
    cachedMercuryCandidates.clear();
    lastMercuryStreamCX = 999999;
    lastMercuryStreamCZ = 999999;
    for (int dz = -2; dz <= 2; dz++) {
        for (int dx = -2; dx <= 2; dx++) addMercuryChunk(playerCX + dx, playerCZ + dz);
    }
    mcBuilt = true;
}

static void cleanupMercuryTerrain() {
    for (MercuryChunk* c : activeMercuryChunks) {
        if (!c) continue;
        for (int i = 0; i < MC_LOD_COUNT; i++) if (c->lists[i]) glDeleteLists(c->lists[i], 1);
        delete c;
    }
    activeMercuryChunks.clear();
    activeMercuryChunkKeys.clear();
    cachedMercuryCandidates.clear();
    lastMercuryStreamCX = 999999;
    lastMercuryStreamCZ = 999999;
    mcBuilt = false;
}

static void drawMercurySky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);
    float skyR = 520.0f;
    int stacks = 16, slices = 48;
    for (int si = 0; si < stacks; si++) {
        float t0 = (float)si / stacks, t1 = (float)(si + 1) / stacks;
        float y0 = skyR * sinf(t0 * PI * 0.5f), y1 = skyR * sinf(t1 * PI * 0.5f);
        float r0 = skyR * cosf(t0 * PI * 0.5f), r1 = skyR * cosf(t1 * PI * 0.5f);
        glBegin(GL_QUAD_STRIP);
        for (int sj = 0; sj <= slices; sj++) {
            float th = (float)sj / slices * TWO_PI;
            float cx = cosf(th), cz = sinf(th);
            glColor3f(0.002f, 0.002f, 0.005f); glVertex3f(tCamX + r0 * cx, y0 - 12.0f, tCamZ + r0 * cz);
            glColor3f(0.012f, 0.010f, 0.010f); glVertex3f(tCamX + r1 * cx, y1 - 12.0f, tCamZ + r1 * cz);
        }
        glEnd();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float sx = tCamX + 240.0f, sy = 120.0f, sz = tCamZ - 180.0f;
    for (int r = 5; r >= 1; r--) {
        glColor4f(1.0f, 0.82f, 0.46f, 0.035f * r);
        glPushMatrix(); glTranslatef(sx, sy, sz); glutSolidSphere(10.0f * r, 24, 12); glPopMatrix();
    }
    glColor4f(1.0f, 0.95f, 0.72f, 0.95f);
    glPushMatrix(); glTranslatef(sx, sy, sz); glutSolidSphere(9.0f, 32, 16); glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

static void drawMercuryTerrainChunks() {
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    for (MercuryChunk* c : activeMercuryChunks) {
        if (!c || !c->valid) continue;
        float centerX = c->cx * MC_CHUNK_SIZE;
        float centerZ = c->cz * MC_CHUNK_SIZE;
        float dx = centerX - tCamX;
        float dz = centerZ - tCamZ;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > MC_CHUNK_SIZE * (MC_VIEW_CHUNKS + 1)) continue;
        int lod = 0;
        if (dist > 165.0f) lod = 2;
        else if (dist > 72.0f) lod = 1;
        glCallList(c->lists[lod]);
    }
}

static void drawMercuryHUD() {
    if (!showMercuryHUD) return;
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    if (h <= 0) h = 1;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_FOG);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4f(0.035f, 0.028f, 0.020f, 0.82f);
    glBegin(GL_QUADS); glVertex2f(0, (float)(h - 42)); glVertex2f((float)w, (float)(h - 42)); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd();
    glColor4f(1.0f, 0.78f, 0.38f, 0.90f);
    drawString(GLUT_BITMAP_HELVETICA_18, 18.0f, (float)(h - 28), "MERCURY SURFACE EXPLORATION");
    glColor4f(0.78f, 0.68f, 0.52f, 0.84f);
    drawString(GLUT_BITMAP_HELVETICA_12, 285.0f, (float)(h - 30), "AIRLESS CRATER FIELD / EXTREME SOLAR CONTRAST");
    char altBuf[80]; sprintf(altBuf, "ALT %.0f m", tCamY * 100.0f);
    glColor4f(1.0f, 0.82f, 0.48f, 0.88f); drawString(GLUT_BITMAP_HELVETICA_18, (float)(w - 165), (float)(h - 28), altBuf);
    glColor4f(0.03f, 0.025f, 0.020f, 0.64f);
    glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, 30); glVertex2f(0, 30); glEnd();
    glColor4f(0.82f, 0.72f, 0.58f, 0.70f);
    drawString(GLUT_BITMAP_HELVETICA_12, 18.0f, 10.0f, "WASD: Move  |  Mouse: Look  |  Wheel: Altitude  |  1/ESC/R: Exit Mercury  |  H: Toggle HUD");
    float cx = w / 2.0f, cy = h / 2.0f;
    glColor4f(1.0f, 0.78f, 0.38f, 0.24f); glLineWidth(1.0f);
    glBegin(GL_LINES); glVertex2f(cx - 14, cy); glVertex2f(cx - 5, cy); glVertex2f(cx + 5, cy); glVertex2f(cx + 14, cy); glVertex2f(cx, cy - 14); glVertex2f(cx, cy - 5); glVertex2f(cx, cy + 5); glVertex2f(cx, cy + 14); glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

static void displayMercuryScene() {
    glClearColor(0.002f, 0.002f, 0.005f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    applyPerspectiveIfNeeded(55.0f, 0.10f, 900.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float yawR = tCamYaw * DEG2RAD, pitR = tCamPitch * DEG2RAD;
    float lookX = tCamX + cosf(pitR) * sinf(yawR), lookY = tCamY + sinf(pitR), lookZ = tCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(tCamX, tCamY, tCamZ, lookX, lookY, lookZ, 0, 1, 0);
    glDisable(GL_FOG); glEnable(GL_LIGHT0); glDisable(GL_LIGHT1);
    GLfloat sunDir[] = { 0.82f, 0.55f, -0.42f, 0.0f };
    GLfloat amb[] = { 0.015f, 0.013f, 0.011f, 1.0f };
    GLfloat dif[] = { 2.35f, 2.05f, 1.55f, 1.0f };
    GLfloat spc[] = { 0.42f, 0.36f, 0.28f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir); glLightfv(GL_LIGHT0, GL_AMBIENT, amb); glLightfv(GL_LIGHT0, GL_DIFFUSE, dif); glLightfv(GL_LIGHT0, GL_SPECULAR, spc);
    drawMercurySky(); drawMercuryTerrainChunks(); drawVignette(0.42f); drawMercuryHUD();
}

// ============================================================
// Jupiter Exploration Mode — Upper Atmosphere Storm World
// ============================================================
static void initJupiterParticles() {
    for (int i = 0; i < JUPITER_PARTICLE_COUNT; i++) {
        jParticles[i].x = (rand() % 2600 - 1300) * 0.08f;
        jParticles[i].y = (rand() % 500) * 0.05f - 2.0f;
        jParticles[i].z = (rand() % 2600 - 1300) * 0.08f;
        jParticles[i].speed = 0.22f + (rand() % 100) * 0.010f;
        jParticles[i].drift = (rand() % 200 - 100) * 0.004f;
        jParticles[i].size = 1.0f + (rand() % 100) * 0.018f;
    }
    jParticlesInit = true;
}

static void resetJupiterCamera() {
    jCamX = targetJCamX = 0.0f; jCamY = targetJCamY = 14.0f; jCamZ = targetJCamZ = 0.0f;
    jCamYaw = targetJCamYaw = 0.0f; jCamPitch = targetJCamPitch = -5.0f;
    jMoveSpeed = 0.0f; keyW = keyA = keyS = keyD = false;
}

static void updateJupiterCamera() {
    float moveX = 0.0f, moveZ = 0.0f;
    float speed = 52.0f * timeSpeed * deltaTime;
    if (keyW || keyA || keyS || keyD) {
        float yawRad = jCamYaw * DEG2RAD;
        float fx = sinf(yawRad), fz = cosf(yawRad), rx = cosf(yawRad), rz = -sinf(yawRad);
        if (keyW) { moveX += fx * speed; moveZ += fz * speed; }
        if (keyS) { moveX -= fx * speed; moveZ -= fz * speed; }
        if (keyA) { moveX -= rx * speed; moveZ -= rz * speed; }
        if (keyD) { moveX += rx * speed; moveZ += rz * speed; }
        targetJCamX += moveX; targetJCamZ += moveZ;
    }
    float instantSpeed = (deltaTime > 0.0001f) ? sqrtf(moveX * moveX + moveZ * moveZ) / deltaTime : 0.0f;
    float speedSmooth = 1.0f - expf(-8.0f * deltaTime), posSmooth = 1.0f - expf(-8.5f * deltaTime), lookSmooth = 1.0f - expf(-10.0f * deltaTime);
    jMoveSpeed += (instantSpeed - jMoveSpeed) * speedSmooth;
    jCamX += (targetJCamX - jCamX) * posSmooth; jCamY += (targetJCamY - jCamY) * posSmooth; jCamZ += (targetJCamZ - jCamZ) * posSmooth;
    jCamYaw += (targetJCamYaw - jCamYaw) * lookSmooth; jCamPitch += (targetJCamPitch - jCamPitch) * lookSmooth;
    targetJCamPitch = clampf(targetJCamPitch, -82.0f, 82.0f); jCamPitch = clampf(jCamPitch, -82.0f, 82.0f); targetJCamY = clampf(targetJCamY, -8.0f, 60.0f);
    jStormPhase += deltaTime * (0.8f + jMoveSpeed * 0.018f);
    if ((rand() % 240) < 3) jLightning = 1.0f; else jLightning *= 0.88f;
    if (jLightning > 0.05f) {
        jCamShakeX = noiseHash(glutGet(GLUT_ELAPSED_TIME), 81) * jLightning * 0.18f;
        jCamShakeY = noiseHash(17, glutGet(GLUT_ELAPSED_TIME)) * jLightning * 0.12f;
    }
    else jCamShakeX = jCamShakeY = 0.0f;
}

static void updateJupiterParticles() {
    if (!jParticlesInit) return;
    float frameScale = clampf(deltaTime * 60.0f, 0.0f, 3.0f);
    for (int i = 0; i < JUPITER_PARTICLE_COUNT; i++) {
        float wave = sinf(currentTime * 3.0f + i * 0.17f) * 0.15f;
        jParticles[i].x += (jParticles[i].speed + wave) * frameScale;
        jParticles[i].z += (jParticles[i].drift + cosf(currentTime * 2.0f + i * 0.11f) * 0.035f) * frameScale;
        jParticles[i].y += sinf(currentTime * 2.4f + i * 0.07f) * 0.010f * frameScale;
        float dx = jParticles[i].x - jCamX, dz = jParticles[i].z - jCamZ;
        if (dx * dx + dz * dz > 120.0f * 120.0f || fabsf(jParticles[i].y - jCamY) > 55.0f) {
            jParticles[i].x = jCamX - 95.0f + (rand() % 200) * 0.25f;
            jParticles[i].z = jCamZ + (rand() % 2000 - 1000) * 0.10f;
            jParticles[i].y = jCamY + (rand() % 800 - 400) * 0.05f;
        }
    }
}

static float getJupiterCloudHeight(float wx, float wz, float layer) {
    float n1 = fbmNoise(wx * 0.010f + layer * 31.0f + jStormPhase, wz * 0.006f + layer * 17.0f, 4, 0.55f);
    float n2 = ridgeNoise(wx * 0.018f - jStormPhase * 0.6f, wz * 0.012f + layer * 40.0f, 3);
    float band = sinf(wz * 0.035f + layer * 1.8f + jStormPhase * 0.7f);
    return n1 * 4.0f + n2 * 2.8f + band * 1.4f;
}

static void drawJupiterSky() {
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_FOG); glDepthMask(GL_FALSE);
    float skyR = 520.0f; int stacks = 20, slices = 56;
    for (int si = 0; si < stacks; si++) {
        float t0 = (float)si / stacks, t1 = (float)(si + 1) / stacks;
        float y0 = skyR * sinf(t0 * PI * 0.5f) - 110.0f, y1 = skyR * sinf(t1 * PI * 0.5f) - 110.0f;
        float r0 = skyR * cosf(t0 * PI * 0.5f), r1 = skyR * cosf(t1 * PI * 0.5f);
        glBegin(GL_QUAD_STRIP);
        for (int sj = 0; sj <= slices; sj++) {
            float th = (float)sj / slices * TWO_PI;
            float storm = 0.5f + 0.5f * sinf(th * 8.0f + jStormPhase * 0.55f + t0 * 4.0f);
            float cx = cosf(th), cz = sinf(th);
            glColor3f(0.25f + storm * 0.06f, 0.12f + storm * 0.04f, 0.045f); glVertex3f(jCamX + r0 * cx, y0, jCamZ + r0 * cz);
            glColor3f(0.62f + storm * 0.08f, 0.36f + storm * 0.06f, 0.16f + storm * 0.03f); glVertex3f(jCamX + r1 * cx, y1, jCamZ + r1 * cz);
        }
        glEnd();
    }
    glDepthMask(GL_TRUE); glEnable(GL_LIGHTING);
}

static void drawJupiterCloudLayer(float layerY, float layerId, float alpha, float scaleMul) {
    glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    const float halfW = 170.0f, spacing = 7.0f;
    int gridN = (int)(halfW * 2.0f / spacing);
    float ox = floorf(jCamX / spacing) * spacing, oz = floorf(jCamZ / spacing) * spacing;
    for (int j = 0; j < gridN; j++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= gridN; i++) {
            for (int dj = 1; dj >= 0; dj--) {
                int cj = j + dj;
                float wx = ox + (i - gridN / 2) * spacing, wz = oz + (cj - gridN / 2) * spacing;
                float h = layerY + getJupiterCloudHeight(wx * scaleMul, wz * scaleMul, layerId);
                float band = 0.5f + 0.5f * sinf(wz * 0.035f + layerId * 2.1f + jStormPhase * 0.65f);
                float swirl = 0.5f + 0.5f * sinf(wx * 0.025f - wz * 0.012f + jStormPhase * 1.3f);
                float fade = 1.0f - smoothstepf(halfW * 0.72f, halfW, sqrtf((wx - jCamX) * (wx - jCamX) + (wz - jCamZ) * (wz - jCamZ)));
                float cr = lerpf(0.58f, 0.95f, band) + swirl * 0.06f, cg = lerpf(0.30f, 0.70f, band) + swirl * 0.04f, cb = lerpf(0.12f, 0.34f, band);
                if (layerId > 1.5f) { cr *= 0.62f; cg *= 0.56f; cb *= 0.52f; }
                glColor4f(clampf(cr, 0, 1), clampf(cg, 0, 1), clampf(cb, 0, 1), alpha * fade);
                glVertex3f(wx, h, wz);
            }
        }
        glEnd();
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}

static void drawJupiterStormVortex(float cx, float cy, float cz, float radius, float spin, bool redSpot) {
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    int rings = 9, segs = 72;
    for (int r = rings; r >= 1; r--) {
        float rr0 = radius * (float)(r - 1) / rings, rr1 = radius * (float)r / rings;
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; s++) {
            float a = TWO_PI * (float)s / segs + spin + rr1 * 0.08f;
            for (int k = 0; k < 2; k++) {
                float rr = (k == 0) ? rr0 : rr1;
                float wave = sinf(a * 3.0f + jStormPhase * 2.0f) * 0.6f;
                float x = cx + cosf(a) * (rr + wave), z = cz + sinf(a) * (rr * 0.45f + wave * 0.3f), y = cy + sinf(a * 2.0f + jStormPhase) * 0.6f;
                float t = (float)r / rings;
                if (redSpot) glColor4f(0.85f - t * 0.20f, 0.24f + t * 0.12f, 0.08f, 0.20f * (1.0f - t * 0.45f));
                else glColor4f(0.95f - t * 0.18f, 0.68f - t * 0.25f, 0.30f, 0.13f * (1.0f - t * 0.35f));
                glVertex3f(x, y, z);
            }
        }
        glEnd();
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}

static void drawJupiterLightning() {
    if (jLightning < 0.04f) return;
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(2.5f + jLightning * 4.0f);
    for (int b = 0; b < 4; b++) {
        float bx = jCamX + noiseHash(b * 19, glutGet(GLUT_ELAPSED_TIME) + 3) * 70.0f;
        float bz = jCamZ + noiseHash(b * 23 + 4, glutGet(GLUT_ELAPSED_TIME) + 9) * 70.0f;
        float by = jCamY + 15.0f + b * 2.5f;
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < 7; i++) {
            glColor4f(0.90f, 0.94f, 1.0f, jLightning * (1.0f - i * 0.08f));
            glVertex3f(bx + noiseHash(i * 17 + b, glutGet(GLUT_ELAPSED_TIME)) * 5.0f, by - i * 5.0f, bz + noiseHash(i * 29 + b, glutGet(GLUT_ELAPSED_TIME) + 5) * 5.0f);
        }
        glEnd();
    }
    glLineWidth(1.0f); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

static void drawJupiterParticles() {
    if (!jParticlesInit) return;
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < JUPITER_PARTICLE_COUNT; i++) {
        float dx = jParticles[i].x - jCamX, dz = jParticles[i].z - jCamZ, dist2 = dx * dx + dz * dz;
        if (dist2 > 110.0f * 110.0f) continue;
        float a = (1.0f - clampf(dist2 / (110.0f * 110.0f), 0.0f, 1.0f)) * 0.30f;
        glColor4f(1.0f, 0.82f, 0.45f, a); glVertex3f(jParticles[i].x, jParticles[i].y, jParticles[i].z);
    }
    glEnd(); glPointSize(1.0f); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}

static void drawJupiterHUD() {
    if (!showJupiterHUD) return;
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    if (h <= 0) h = 1;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_FOG); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4f(0.08f, 0.035f, 0.015f, 0.80f);
    glBegin(GL_QUADS); glVertex2f(0, (float)(h - 42)); glVertex2f((float)w, (float)(h - 42)); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h); glEnd();
    glColor4f(1.0f, 0.62f, 0.22f, 0.94f); drawString(GLUT_BITMAP_HELVETICA_18, 18.0f, (float)(h - 28), "JUPITER UPPER ATMOSPHERE");
    glColor4f(1.0f, 0.78f, 0.42f, 0.86f); drawString(GLUT_BITMAP_HELVETICA_12, 285.0f, (float)(h - 30), "FLOATING STORM DECK / TURBULENT CLOUD BANDS");
    char altBuf[96]; sprintf(altBuf, "PRESSURE LAYER %.0f", jCamY * 10.0f);
    glColor4f(1.0f, 0.72f, 0.32f, 0.88f); drawString(GLUT_BITMAP_HELVETICA_18, (float)(w - 230), (float)(h - 28), altBuf);
    glColor4f(0.08f, 0.035f, 0.015f, 0.62f);
    glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, 30); glVertex2f(0, 30); glEnd();
    glColor4f(1.0f, 0.76f, 0.42f, 0.72f); drawString(GLUT_BITMAP_HELVETICA_12, 18.0f, 10.0f, "WASD: Drift  |  Mouse: Look  |  Wheel: Change Cloud Layer  |  5/ESC/R: Exit Jupiter  |  H: Toggle HUD");
    float cx = w / 2.0f, cy = h / 2.0f;
    glColor4f(1.0f, 0.62f, 0.22f, 0.24f + jLightning * 0.30f); glLineWidth(1.0f);
    glBegin(GL_LINES); glVertex2f(cx - 16, cy); glVertex2f(cx - 6, cy); glVertex2f(cx + 6, cy); glVertex2f(cx + 16, cy); glVertex2f(cx, cy - 16); glVertex2f(cx, cy - 6); glVertex2f(cx, cy + 6); glVertex2f(cx, cy + 16); glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_FOG);
}

static void displayJupiterScene() {
    glClearColor(0.15f, 0.07f, 0.025f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    applyPerspectiveIfNeeded(60.0f, 0.15f, 950.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float yawR = jCamYaw * DEG2RAD, pitR = jCamPitch * DEG2RAD;
    float lookX = jCamX + jCamShakeX + cosf(pitR) * sinf(yawR), lookY = jCamY + jCamShakeY + sinf(pitR), lookZ = jCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(jCamX + jCamShakeX, jCamY + jCamShakeY, jCamZ, lookX, lookY, lookZ, 0, 1, 0);
    glEnable(GL_FOG); GLfloat fogCol[] = { 0.43f, 0.22f, 0.075f, 1.0f }; glFogfv(GL_FOG_COLOR, fogCol); glFogi(GL_FOG_MODE, GL_EXP2); glFogf(GL_FOG_DENSITY, 0.0065f + 0.0040f * (1.0f - clampf(jCamY / 60.0f, 0.0f, 1.0f)));
    glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
    GLfloat sunDir[] = { 0.35f, 0.85f, 0.25f, 0.0f }, sunAmb[] = { 0.20f, 0.12f, 0.06f, 1.0f }, sunDif[] = { 1.25f, 0.78f, 0.38f, 1.0f }, sunSpc[] = { 0.45f, 0.32f, 0.18f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir); glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb); glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif); glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);
    GLfloat stormDir[] = { -0.2f, -0.4f, 0.7f, 0.0f }, stormAmb[] = { 0.02f, 0.015f, 0.01f, 1.0f }, stormDif[] = { 0.35f + jLightning * 1.2f, 0.32f + jLightning * 1.2f, 0.45f + jLightning * 1.5f, 1.0f }, stormSpc[] = { 0.20f, 0.20f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, stormDir); glLightfv(GL_LIGHT1, GL_AMBIENT, stormAmb); glLightfv(GL_LIGHT1, GL_DIFFUSE, stormDif); glLightfv(GL_LIGHT1, GL_SPECULAR, stormSpc);
    drawJupiterSky();
    drawJupiterCloudLayer(-8.0f, 3.0f, 0.42f, 1.05f);
    drawJupiterCloudLayer(0.0f, 2.0f, 0.50f, 1.00f);
    drawJupiterCloudLayer(9.0f, 1.0f, 0.38f, 0.90f);
    drawJupiterStormVortex(jCamX + 45.0f * sinf(jStormPhase * 0.25f), 2.5f, jCamZ + 72.0f, 34.0f, jStormPhase * 0.9f, true);
    drawJupiterStormVortex(jCamX - 65.0f, 6.0f, jCamZ - 50.0f, 21.0f, -jStormPhase * 1.2f, false);
    drawJupiterParticles(); drawJupiterLightning();
    glDisable(GL_LIGHT1); glDisable(GL_FOG); drawVignette(0.38f + jLightning * 0.08f); drawJupiterHUD();
}



// =========================
// Saturn Exploration Mode — Elegant Ring-Sky Atmosphere
// =========================
static void initSaturnMotes() {
    for (int i = 0; i < SATURN_MOTE_COUNT; i++) {
        sMotes[i].x = randRange(-180.0f, 180.0f);
        sMotes[i].y = randRange(-18.0f, 58.0f);
        sMotes[i].z = randRange(-180.0f, 180.0f);
        sMotes[i].speed = randRange(1.2f, 5.2f);
        sMotes[i].drift = randRange(-0.55f, 0.55f);
        sMotes[i].size = randRange(0.55f, 2.10f);
        sMotes[i].phase = randRange(0.0f, TWO_PI);
    }
    sMotesInit = true;
}

static void resetSaturnCamera() {
    sCamX = targetSCamX = 0.0f;
    sCamY = targetSCamY = 18.0f;
    sCamZ = targetSCamZ = 0.0f;
    sCamYaw = targetSCamYaw = 8.0f;
    sCamPitch = targetSCamPitch = -4.0f;
    sMoveSpeed = 0.0f;
    sCloudPhase = 0.0f;
    sRingPhase = 0.0f;
    sGlimmer = 0.0f;
}

static void updateSaturnCamera() {
    float yawRad = targetSCamYaw * DEG2RAD;
    float forwardX = sinf(yawRad);
    float forwardZ = cosf(yawRad);
    float rightX = cosf(yawRad);
    float rightZ = -sinf(yawRad);

    float desiredX = 0.0f, desiredZ = 0.0f;
    if (keyW) { desiredX += forwardX; desiredZ += forwardZ; }
    if (keyS) { desiredX -= forwardX; desiredZ -= forwardZ; }
    if (keyD) { desiredX += rightX; desiredZ += rightZ; }
    if (keyA) { desiredX -= rightX; desiredZ -= rightZ; }

    float len = sqrtf(desiredX * desiredX + desiredZ * desiredZ);
    if (len > 0.0001f) { desiredX /= len; desiredZ /= len; }

    float targetSpeed = len > 0.0f ? 22.0f : 0.0f;
    float smooth = len > 0.0f ? 1.8f : 1.1f;
    sMoveSpeed += (targetSpeed - sMoveSpeed) * clampf(smooth * deltaTime, 0.0f, 1.0f);

    targetSCamX += desiredX * sMoveSpeed * deltaTime;
    targetSCamZ += desiredZ * sMoveSpeed * deltaTime;

    // Saturn should feel huge and slow, so the camera floats rather than snaps.
    sCamX += (targetSCamX - sCamX) * 0.055f;
    sCamY += (targetSCamY - sCamY) * 0.045f;
    sCamZ += (targetSCamZ - sCamZ) * 0.055f;
    sCamYaw += (targetSCamYaw - sCamYaw) * 0.070f;
    sCamPitch += (targetSCamPitch - sCamPitch) * 0.070f;

    targetSCamY = clampf(targetSCamY, -10.0f, 72.0f);
    targetSCamPitch = clampf(targetSCamPitch, -75.0f, 72.0f);
    sCamPitch = clampf(sCamPitch, -75.0f, 72.0f);

    sCloudPhase += deltaTime * (0.18f + sMoveSpeed * 0.004f);
    sRingPhase += deltaTime * 0.08f;
    sGlimmer = 0.5f + 0.5f * sinf(sRingPhase * 1.7f);
}

static void updateSaturnMotes() {
    for (int i = 0; i < SATURN_MOTE_COUNT; i++) {
        sMotes[i].x += (0.45f + sMotes[i].drift) * deltaTime;
        sMotes[i].z -= sMotes[i].speed * deltaTime;
        sMotes[i].y += sinf(sCloudPhase + sMotes[i].phase) * 0.020f;

        if (sMotes[i].z < sCamZ - 190.0f) {
            sMotes[i].z = sCamZ + 190.0f;
            sMotes[i].x = sCamX + randRange(-180.0f, 180.0f);
            sMotes[i].y = randRange(-16.0f, 58.0f);
        }
        if (sMotes[i].x < sCamX - 190.0f) sMotes[i].x = sCamX + 190.0f;
        if (sMotes[i].x > sCamX + 190.0f) sMotes[i].x = sCamX - 190.0f;
    }
}

static float getSaturnCloudLift(float wx, float wz, float layer) {
    float n = fbmNoise(wx * 0.004f + layer * 21.0f + sCloudPhase * 0.08f,
        wz * 0.006f + layer * 13.0f - sCloudPhase * 0.035f, 4, 0.56f);
    float band = sinf((wz * 0.022f + layer * 1.7f) + n * 1.6f + sCloudPhase * 0.22f);
    return band * 1.20f + n * 2.4f;
}

static void drawSaturnSkyGradient() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.11f, 0.065f, 1.0f); glVertex2f(0, 0);
    glColor4f(0.15f, 0.11f, 0.065f, 1.0f); glVertex2f(1, 0);
    glColor4f(0.78f, 0.58f, 0.32f, 1.0f); glVertex2f(1, 1);
    glColor4f(0.58f, 0.42f, 0.24f, 1.0f); glVertex2f(0, 1);
    glEnd();

    glColor4f(1.0f, 0.84f, 0.52f, 0.10f + sGlimmer * 0.06f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.72f, 0.72f);
    for (int i = 0; i <= 40; i++) {
        float a = TWO_PI * i / 40.0f;
        glVertex2f(0.72f + cosf(a) * 0.28f, 0.72f + sinf(a) * 0.20f);
    }
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
}

static void drawSaturnRingSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef(sCamX, sCamY + 62.0f, sCamZ + 118.0f);
    glRotatef(-18.0f + 2.0f * sinf(sRingPhase), 1.0f, 0.0f, 0.0f);
    glRotatef(4.0f, 0.0f, 1.0f, 0.0f);

    for (int band = 0; band < 4; band++) {
        float inner = 72.0f + band * 8.0f;
        float outer = inner + 5.2f;
        float alpha = 0.16f - band * 0.025f + sGlimmer * 0.015f;
        glBegin(GL_QUAD_STRIP);
        for (int i = -46; i <= 46; i++) {
            float a = (-160.0f + i * 3.5f) * DEG2RAD;
            float ca = cosf(a), sa = sinf(a);
            float tone = 0.86f + 0.10f * sinf(a * 8.0f + band * 1.7f);
            glColor4f(1.0f * tone, 0.84f * tone, 0.58f * tone, alpha);
            glVertex3f(ca * inner, sa * 6.0f, sa * inner);
            glVertex3f(ca * outer, sa * 6.0f, sa * outer);
        }
        glEnd();
    }

    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void drawSaturnCloudLayer(float layerY, float layerId, float alpha, float widthScale) {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    const int sx = 26;
    const int sz = 46;
    const float stepX = 7.5f * widthScale;
    const float stepZ = 7.5f;
    float baseX = floorf(sCamX / stepX) * stepX;
    float baseZ = floorf(sCamZ / stepZ) * stepZ;

    for (int z = -4; z < sz; z++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = -sx; x <= sx; x++) {
            for (int row = 0; row < 2; row++) {
                int zz = z + row;
                float wx = baseX + x * stepX;
                float wz = baseZ + zz * stepZ;
                float lift = getSaturnCloudLift(wx, wz, layerId);
                float ribbon = 0.5f + 0.5f * sinf(wz * 0.031f + layerId * 2.0f + sCloudPhase * 0.22f);
                float pearl = fbmNoise(wx * 0.010f + layerId * 17.0f, wz * 0.008f + sCloudPhase * 0.04f, 3, 0.52f);

                float cr = lerpf(0.72f, 1.00f, ribbon) + pearl * 0.045f;
                float cg = lerpf(0.58f, 0.83f, ribbon) + pearl * 0.035f;
                float cb = lerpf(0.36f, 0.58f, ribbon) + pearl * 0.030f;
                float fade = 1.0f - clampf((float)(zz) / (float)sz, 0.0f, 1.0f);
                glColor4f(clampf(cr, 0, 1), clampf(cg, 0, 1), clampf(cb, 0, 1), alpha * (0.35f + 0.65f * fade));
                glVertex3f(wx, layerY + lift, wz);
            }
        }
        glEnd();
    }
}

static void drawSaturnMotes() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < SATURN_MOTE_COUNT; i++) {
        float dx = sMotes[i].x - sCamX;
        float dz = sMotes[i].z - sCamZ;
        float dist = sqrtf(dx * dx + dz * dz);
        float a = clampf(1.0f - dist / 210.0f, 0.0f, 1.0f) * 0.42f;
        glColor4f(1.0f, 0.86f, 0.58f, a);
        glVertex3f(sMotes[i].x, sMotes[i].y, sMotes[i].z);
    }
    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
}

static void drawSaturnHUD() {
    if (!showSaturnHUD) return;
    int w, h;
    begin2DOverlay(w, h);
    drawGlassPanel(14.0f, h - 116.0f, 455.0f, 92.0f, 0.50f);
    glColor4f(1.0f, 0.86f, 0.52f, 0.95f);
    drawString(GLUT_BITMAP_HELVETICA_18, 28.0f, h - 48.0f, "SATURN RING-SKY EXPLORATION");
    glColor4f(0.98f, 0.78f, 0.45f, 0.80f);
    drawString(GLUT_BITMAP_HELVETICA_12, 28.0f, h - 72.0f, "Pale golden atmosphere: smooth cloud rivers + visible ring arc in the sky.");
    glColor4f(1.0f, 0.88f, 0.58f, 0.72f);
    drawString(GLUT_BITMAP_HELVETICA_12, 28.0f, h - 94.0f, "WASD: Float  |  Mouse: Look  |  Wheel: Rise/Sink  |  6/ESC/R: Exit Saturn  |  H: Toggle HUD");
    end2DOverlay();
}

static void displaySaturnScene() {
    glClearColor(0.34f, 0.23f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    applyPerspectiveIfNeeded(58.0f, 0.15f, 1050.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float yawR = sCamYaw * DEG2RAD;
    float pitR = sCamPitch * DEG2RAD;
    float lookX = sCamX + cosf(pitR) * sinf(yawR);
    float lookY = sCamY + sinf(pitR);
    float lookZ = sCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(sCamX, sCamY, sCamZ, lookX, lookY, lookZ, 0, 1, 0);

    glEnable(GL_FOG);
    GLfloat fogCol[] = { 0.56f, 0.40f, 0.20f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.0042f + 0.0022f * (1.0f - clampf(sCamY / 70.0f, 0.0f, 1.0f)));

    glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
    GLfloat sunDir[] = { 0.52f, 0.75f, 0.20f, 0.0f };
    GLfloat sunAmb[] = { 0.30f, 0.22f, 0.12f, 1.0f };
    GLfloat sunDif[] = { 1.05f, 0.82f, 0.50f, 1.0f };
    GLfloat sunSpc[] = { 0.20f, 0.18f, 0.12f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);
    GLfloat ringDir[] = { -0.30f, 0.35f, 0.80f, 0.0f };
    GLfloat ringAmb[] = { 0.12f, 0.09f, 0.04f, 1.0f };
    GLfloat ringDif[] = { 0.48f + sGlimmer * 0.10f, 0.36f + sGlimmer * 0.08f, 0.18f + sGlimmer * 0.05f, 1.0f };
    GLfloat ringSpc[] = { 0.18f, 0.15f, 0.08f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, ringDir);
    glLightfv(GL_LIGHT1, GL_AMBIENT, ringAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, ringDif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, ringSpc);

    drawSaturnSkyGradient();
    drawSaturnRingSky();
    drawSaturnCloudLayer(-10.0f, 3.0f, 0.24f, 1.08f);
    drawSaturnCloudLayer(2.0f, 2.0f, 0.32f, 1.00f);
    drawSaturnCloudLayer(14.0f, 1.0f, 0.26f, 0.94f);
    drawSaturnMotes();
    glDisable(GL_LIGHT1);
    glDisable(GL_FOG);
    drawVignette(0.26f);
    drawSaturnHUD();
}

// =========================
// Uranus Exploration Mode — Quiet Methane Haze Atmosphere
// =========================
static void initUranusHaze() {
    for (int i = 0; i < URANUS_HAZE_COUNT; i++) {
        uHaze[i].x = randRange(-155.0f, 155.0f);
        uHaze[i].y = randRange(-22.0f, 66.0f);
        uHaze[i].z = randRange(-155.0f, 155.0f);
        uHaze[i].speed = randRange(0.20f, 1.15f);
        uHaze[i].drift = randRange(-0.18f, 0.18f);
        uHaze[i].size = randRange(0.45f, 1.75f);
        uHaze[i].phase = randRange(0.0f, TWO_PI);
    }
    uHazeInit = true;
}

static void resetUranusCamera() {
    uCamX = targetUCamX = 0.0f;
    uCamY = targetUCamY = 14.0f;
    uCamZ = targetUCamZ = 0.0f;
    uCamYaw = targetUCamYaw = -12.0f;
    uCamPitch = targetUCamPitch = -3.0f;
    uMoveSpeed = 0.0f;
    uHazePhase = 0.0f;
    uAuroraPhase = 0.0f;
}

static void updateUranusCamera() {
    float yawRad = targetUCamYaw * DEG2RAD;
    float forwardX = sinf(yawRad);
    float forwardZ = cosf(yawRad);
    float rightX = cosf(yawRad);
    float rightZ = -sinf(yawRad);

    float desiredX = 0.0f, desiredZ = 0.0f;
    if (keyW) { desiredX += forwardX; desiredZ += forwardZ; }
    if (keyS) { desiredX -= forwardX; desiredZ -= forwardZ; }
    if (keyD) { desiredX += rightX; desiredZ += rightZ; }
    if (keyA) { desiredX -= rightX; desiredZ -= rightZ; }
    float len = sqrtf(desiredX * desiredX + desiredZ * desiredZ);
    if (len > 0.0001f) { desiredX /= len; desiredZ /= len; }

    float targetSpeed = len > 0.0f ? 15.0f : 0.0f;
    uMoveSpeed += (targetSpeed - uMoveSpeed) * clampf((len > 0.0f ? 1.4f : 0.8f) * deltaTime, 0.0f, 1.0f);
    targetUCamX += desiredX * uMoveSpeed * deltaTime;
    targetUCamZ += desiredZ * uMoveSpeed * deltaTime;

    // Uranus is intentionally smoother and more silent than Neptune.
    uCamX += (targetUCamX - uCamX) * 0.040f;
    uCamY += (targetUCamY - uCamY) * 0.040f;
    uCamZ += (targetUCamZ - uCamZ) * 0.040f;
    uCamYaw += (targetUCamYaw - uCamYaw) * 0.060f;
    uCamPitch += (targetUCamPitch - uCamPitch) * 0.060f;

    targetUCamY = clampf(targetUCamY, -8.0f, 74.0f);
    targetUCamPitch = clampf(targetUCamPitch, -78.0f, 76.0f);
    uCamPitch = clampf(uCamPitch, -78.0f, 76.0f);

    uHazePhase += deltaTime * 0.11f;
    uAuroraPhase += deltaTime * 0.17f;
}

static void updateUranusHaze() {
    for (int i = 0; i < URANUS_HAZE_COUNT; i++) {
        uHaze[i].x += (uHaze[i].drift + 0.06f * sinf(uHazePhase + uHaze[i].phase)) * deltaTime;
        uHaze[i].z -= uHaze[i].speed * deltaTime;
        uHaze[i].y += sinf(uHazePhase * 1.5f + uHaze[i].phase) * 0.010f;
        if (uHaze[i].z < uCamZ - 165.0f) {
            uHaze[i].z = uCamZ + 165.0f;
            uHaze[i].x = uCamX + randRange(-155.0f, 155.0f);
            uHaze[i].y = randRange(-20.0f, 66.0f);
        }
        if (uHaze[i].x < uCamX - 170.0f) uHaze[i].x = uCamX + 170.0f;
        if (uHaze[i].x > uCamX + 170.0f) uHaze[i].x = uCamX - 170.0f;
    }
}

static float getUranusHazeLift(float wx, float wz, float layer) {
    float n = fbmNoise(wx * 0.0035f + layer * 33.0f + uHazePhase * 0.025f,
        wz * 0.0040f + layer * 19.0f - uHazePhase * 0.018f, 4, 0.60f);
    float softBand = sinf(wz * 0.013f + wx * 0.002f + layer + n * 0.8f + uHazePhase * 0.10f);
    return softBand * 0.75f + n * 1.75f;
}

static void drawUranusSkyGradient() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glBegin(GL_QUADS);
    glColor4f(0.010f, 0.070f, 0.090f, 1.0f); glVertex2f(0, 0);
    glColor4f(0.010f, 0.080f, 0.100f, 1.0f); glVertex2f(1, 0);
    glColor4f(0.120f, 0.460f, 0.520f, 1.0f); glVertex2f(1, 1);
    glColor4f(0.060f, 0.330f, 0.390f, 1.0f); glVertex2f(0, 1);
    glEnd();

    // Minimal tilted aurora ribbons: soft, far, almost silent.
    for (int r = 0; r < 3; r++) {
        glBegin(GL_LINE_STRIP);
        glColor4f(0.45f, 1.0f, 0.90f, 0.06f - r * 0.010f);
        for (int i = 0; i <= 72; i++) {
            float x = (float)i / 72.0f;
            float y = 0.68f + r * 0.055f + sinf(x * TWO_PI * 1.2f + uAuroraPhase + r) * 0.025f;
            glVertex2f(x, y);
        }
        glEnd();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

static void drawUranusHazeLayer(float layerY, float layerId, float alpha, float spread) {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    const int sx = 22;
    const int sz = 42;
    const float stepX = 8.0f * spread;
    const float stepZ = 8.0f;
    float baseX = floorf(uCamX / stepX) * stepX;
    float baseZ = floorf(uCamZ / stepZ) * stepZ;

    for (int z = -5; z < sz; z++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = -sx; x <= sx; x++) {
            for (int row = 0; row < 2; row++) {
                int zz = z + row;
                float wx = baseX + x * stepX;
                float wz = baseZ + zz * stepZ;
                float lift = getUranusHazeLift(wx, wz, layerId);
                float methane = fbmNoise(wx * 0.007f + layerId * 10.0f, wz * 0.006f + uHazePhase * 0.04f, 3, 0.58f);
                float stripe = 0.5f + 0.5f * sinf(wz * 0.018f + layerId * 1.4f + methane * 0.35f);
                float cr = lerpf(0.035f, 0.14f, stripe) + methane * 0.025f;
                float cg = lerpf(0.34f, 0.74f, stripe) + methane * 0.030f;
                float cb = lerpf(0.42f, 0.82f, stripe) + methane * 0.030f;
                float fade = 1.0f - clampf((float)(zz) / (float)sz, 0.0f, 1.0f);
                glColor4f(clampf(cr, 0, 1), clampf(cg, 0, 1), clampf(cb, 0, 1), alpha * (0.32f + 0.68f * fade));
                glVertex3f(wx, layerY + lift, wz);
            }
        }
        glEnd();
    }
}

static void drawUranusHazeParticles() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < URANUS_HAZE_COUNT; i++) {
        float dx = uHaze[i].x - uCamX;
        float dz = uHaze[i].z - uCamZ;
        float dist = sqrtf(dx * dx + dz * dz);
        float a = clampf(1.0f - dist / 180.0f, 0.0f, 1.0f) * 0.34f;
        float pulse = 0.75f + 0.25f * sinf(uHazePhase * 3.0f + uHaze[i].phase);
        glColor4f(0.44f * pulse, 1.0f * pulse, 0.92f * pulse, a);
        glVertex3f(uHaze[i].x, uHaze[i].y, uHaze[i].z);
    }
    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
}

static void drawUranusHUD() {
    if (!showUranusHUD) return;
    int w, h;
    begin2DOverlay(w, h);
    drawGlassPanel(14.0f, h - 116.0f, 455.0f, 92.0f, 0.50f);
    glColor4f(0.58f, 1.0f, 0.96f, 0.95f);
    drawString(GLUT_BITMAP_HELVETICA_18, 28.0f, h - 48.0f, "URANUS METHANE HAZE EXPLORATION");
    glColor4f(0.48f, 0.96f, 0.92f, 0.80f);
    drawString(GLUT_BITMAP_HELVETICA_12, 28.0f, h - 72.0f, "Cold cyan gas world: quiet fog decks, minimal aurora, soft alien silence.");
    glColor4f(0.58f, 1.0f, 0.96f, 0.72f);
    drawString(GLUT_BITMAP_HELVETICA_12, 28.0f, h - 94.0f, "WASD: Glide  |  Mouse: Look  |  Wheel: Rise/Sink  |  7/ESC/R: Exit Uranus  |  H: Toggle HUD");
    end2DOverlay();
}

static void displayUranusScene() {
    glClearColor(0.015f, 0.09f, 0.105f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    applyPerspectiveIfNeeded(57.0f, 0.15f, 950.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float yawR = uCamYaw * DEG2RAD;
    float pitR = uCamPitch * DEG2RAD;
    float lookX = uCamX + cosf(pitR) * sinf(yawR);
    float lookY = uCamY + sinf(pitR);
    float lookZ = uCamZ + cosf(pitR) * cosf(yawR);
    gluLookAt(uCamX, uCamY, uCamZ, lookX, lookY, lookZ, 0, 1, 0);

    glEnable(GL_FOG);
    GLfloat fogCol[] = { 0.10f, 0.38f, 0.42f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.0072f + 0.0020f * (1.0f - clampf(uCamY / 72.0f, 0.0f, 1.0f)));

    glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
    GLfloat sunDir[] = { 0.26f, 0.82f, 0.44f, 0.0f };
    GLfloat sunAmb[] = { 0.10f, 0.22f, 0.24f, 1.0f };
    GLfloat sunDif[] = { 0.38f, 0.88f, 0.86f, 1.0f };
    GLfloat sunSpc[] = { 0.12f, 0.30f, 0.34f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpc);
    GLfloat hazeDir[] = { -0.35f, 0.18f, 0.80f, 0.0f };
    GLfloat hazeAmb[] = { 0.03f, 0.10f, 0.12f, 1.0f };
    GLfloat hazeDif[] = { 0.14f, 0.46f, 0.52f, 1.0f };
    GLfloat hazeSpc[] = { 0.06f, 0.20f, 0.23f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, hazeDir);
    glLightfv(GL_LIGHT1, GL_AMBIENT, hazeAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, hazeDif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, hazeSpc);

    drawUranusSkyGradient();
    drawUranusHazeLayer(-11.0f, 3.0f, 0.24f, 1.08f);
    drawUranusHazeLayer(2.5f, 2.0f, 0.30f, 1.00f);
    drawUranusHazeLayer(15.5f, 1.0f, 0.22f, 0.92f);
    drawUranusHazeParticles();
    glDisable(GL_LIGHT1);
    glDisable(GL_FOG);
    drawVignette(0.34f);
    drawUranusHUD();
}

// =========================
// Scene Init
// =========================


void initAsteroids() {
    asteroids.clear();
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid a;
        a.angle = (float)(rand() % 360);
        a.dist = 26.0f + ((rand() % 40) / 10.0f);
        a.height = ((rand() % 24) / 10.0f) - 1.2f;
        a.size = 0.08f + ((rand() % 14) / 100.0f);
        a.rot = (float)(rand() % 360);
        a.rotSpeed = 0.5f + ((rand() % 140) / 100.0f);
        a.scaleX = 0.55f + ((rand() % 95) / 100.0f);
        a.scaleY = 0.55f + ((rand() % 95) / 100.0f);
        a.scaleZ = 0.55f + ((rand() % 95) / 100.0f);
        a.shapeType = rand() % 4;
        asteroids.push_back(a);
    }
}

void initOrbits() {
    orbitsList = glGenLists(1);
    glNewList(orbitsList, GL_COMPILE);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    for (const auto& p : planets) {
        glBegin(GL_LINE_LOOP);
        glColor4f(p.r * 0.5f, p.g * 0.5f, p.b * 0.5f, 0.4f);
        for (int i = 0; i < 360; ++i) {
            float rad = (float)i * PI / 180.0f;
            float x = p.distance * cosf(rad);
            float z = p.distance * sinf(rad);
            float y = z * sinf(p.inclination * PI / 180.0f);
            z = z * cosf(p.inclination * PI / 180.0f);
            glVertex3f(x, y, z);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);
    glEndList();
}


static void spawnMeteor(Meteor& m, bool initial = false) {
    m.id = ++meteorCounter;

    char mName[32];
    sprintf(mName, "Meteor-%03d", m.id);
    m.name = mName;

    // Side-spawn meteor:
    // Starts from the left side of the solar system and moves across the scene.
    float spawnX = -randRange(150.0f, 210.0f);
    float spawnY = randRange(18.0f, 55.0f);
    float spawnZ = randRange(-85.0f, 85.0f);

    m.x = spawnX;
    m.y = spawnY;
    m.z = spawnZ;

    // Direction: mostly from left to right, slightly downward, with small Z variation.
    float dirX = 1.0f;
    float dirY = randRange(-0.25f, -0.05f);
    float dirZ = randRange(-0.18f, 0.18f);
    normalize3(dirX, dirY, dirZ);

    // Speed: visible but not too chaotic since count is now only 5.
    float speed = randRange(75.0f, 115.0f);

    m.vx = dirX * speed;
    m.vy = dirY * speed;
    m.vz = dirZ * speed;

    // Mostly medium meteors, with rare larger fireballs.
    float typeRoll = randRange(0.0f, 1.0f);
    if (typeRoll < 0.75f) {
        m.scale = randRange(0.14f, 0.24f);
    }
    else {
        m.scale = randRange(0.28f, 0.42f);
    }

    m.life = initial ? randRange(0.35f, 1.0f) : 1.0f;
    m.rot = randRange(0.0f, 360.0f);

    m.rotAxis[0] = randRange(-1.0f, 1.0f);
    m.rotAxis[1] = randRange(-1.0f, 1.0f);
    m.rotAxis[2] = randRange(-1.0f, 1.0f);
    normalize3(m.rotAxis[0], m.rotAxis[1], m.rotAxis[2]);

    // Initialize the trail behind the meteor, opposite to movement direction.
    for (int i = 0; i < METEOR_TRAIL_POINTS; i++) {
        float back = (float)i * m.scale * 18.0f;
        m.trail[i][0] = m.x - dirX * back;
        m.trail[i][1] = m.y - dirY * back;
        m.trail[i][2] = m.z - dirZ * back;
    }
}
static void initMeteors() {
    // Fire-only meteor material: every meteor uses the same flame texture.
    // No meteor1/meteor2/meteor3/meteor4 textures are needed anymore.
    fireTexture = loadTexture("meteor1.jpg");
    if (fireTexture == 0) {
        printf("Warning: fire texture failed to load. Meteor glow will use vertex colors.\n");
    }

    if (!meteorQuadric) {
        meteorQuadric = gluNewQuadric();
        gluQuadricNormals(meteorQuadric, GLU_SMOOTH);
        gluQuadricTexture(meteorQuadric, GL_TRUE);
    }

    meteors.clear();
    meteors.resize(METEOR_COUNT);
    for (int i = 0; i < METEOR_COUNT; i++) {
        spawnMeteor(meteors[i], true);
    }
}

static void updateMeteors() {
    if (!showMeteorShower || meteors.empty()) return;

    float dt = clampf(deltaTime, 0.0f, 0.05f) * timeSpeed;
    if (dt <= 0.0f) return;

    for (Meteor& m : meteors) {
        float oldX = m.x;
        float oldY = m.y;
        float oldZ = m.z;

        m.x += m.vx * dt;
        m.y += m.vy * dt;
        m.z += m.vz * dt;
        m.rot += 130.0f * dt;
        m.life -= 0.45f * dt;

        m.vy -= 1.4f * dt;

        float dx = m.x - m.trail[0][0];
        float dy = m.y - m.trail[0][1];
        float dz = m.z - m.trail[0][2];
        float moved2 = dx * dx + dy * dy + dz * dz;

        if (moved2 > 0.18f) {
            for (int t = METEOR_TRAIL_POINTS - 1; t > 0; t--) {
                m.trail[t][0] = m.trail[t - 1][0];
                m.trail[t][1] = m.trail[t - 1][1];
                m.trail[t][2] = m.trail[t - 1][2];
            }
            m.trail[0][0] = oldX;
            m.trail[0][1] = oldY;
            m.trail[0][2] = oldZ;
        }

        float dist2 = m.x * m.x + m.y * m.y + m.z * m.z;
        if (m.life <= 0.0f || dist2 > METEOR_RESPAWN_RADIUS * METEOR_RESPAWN_RADIUS || m.y < -45.0f) {
            spawnMeteor(m, false);
        }
    }
}



void init() {
    srand((unsigned int)time(NULL));
    glClearColor(0.01f, 0.01f, 0.03f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat globalAmbient[] = { 0.02f, 0.02f, 0.03f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    GLfloat ambient[] = { 0.03f, 0.03f, 0.03f, 1.0f };
    GLfloat diffuse[] = { 1.95f, 1.85f, 1.70f, 1.0f };
    GLfloat specular[] = { 1.50f, 1.40f, 1.20f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0012f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00002f);

    buildPrimitiveLists();

    initAsteroids();
    initOrbits();

    asteroidTexture = loadTexture("asteroid.jpeg");

    if (asteroidTexture == 0) {
        printf("Warning: asteroid texture failed to load.\n");
    }

    LoadPlanetTextures();
    LoadSkyboxTextures();
    initMeteors();

    // Procedural texture used only inside Mars terrain atmosphere/cloud layer.
    createMarsGasTexture();

    for (int i = 0; i < (int)planets.size(); i++) {
        planets[i].textureID = planetTextures[i];
    }
}

// =========================
// Drawing helpers
// =========================
void drawImpactCraterOnPlanet(float planetRadius, float alphaScale = 1.0f) {
    glPushMatrix();
    glRotatef(38.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-18.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, planetRadius * 0.99f, 0.0f);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float darkAlpha = 0.65f * alphaScale + craterFlash * 0.20f;
    glColor4f(0.08f, 0.05f, 0.03f, darkAlpha);
    glPushMatrix();
    glScalef(1.0f, 0.18f, 1.0f);
    glutSolidSphere(planetRadius * 0.19f, 18, 18);
    glPopMatrix();

    glColor4f(0.30f, 0.18f, 0.09f, 0.28f * alphaScale + craterFlash * 0.12f);
    glutSolidTorus(planetRadius * 0.012f, planetRadius * 0.18f, 12, 28);

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}
void drawSkybox(float size = 1200.0f) {
    if (!skyboxLoaded) return;

    float s = size / 2.0f;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glColor3f(1.0f, 1.0f, 1.0f);

    // Face 1: Front
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glEnd();

    // Face 2: Back
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, s);
    glEnd();

    // Face 3: Left
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glEnd();

    // Face 4: Right
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);
    glEnd();

    // Face 5: Top
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glEnd();

    // Face 6: Bottom
    glBindTexture(GL_TEXTURE_2D, skyboxTextures[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, -s, -s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, -s);
    glEnd();

    glDepthMask(GL_TRUE);

    glPopAttrib();
}
void drawSun() {
    glPushMatrix();

    if (sunTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, sunTexture);
        glEnable(GL_LIGHTING);

        GLfloat matColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat matShininess[] = { 100.0f };
        GLfloat matEmission[] = { 4.0f, 3.0f, 2.0f, 1.0f };
        GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matColor);
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);
        glMaterialfv(GL_FRONT, GL_EMISSION, matEmission);

        glColor3f(1.0f, 1.0f, 1.0f);
        glRotatef(currentTime * 5.0f, 0.0f, 1.0f, 0.0f);

        GLUquadricObj* sunSphere = gluNewQuadric();
        gluQuadricTexture(sunSphere, GL_TRUE);
        gluQuadricNormals(sunSphere, GLU_SMOOTH);
        gluSphere(sunSphere, 3.5f, 50, 50);
        gluDeleteQuadric(sunSphere);

        glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
        glDisable(GL_TEXTURE_2D);
    }
    else {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.8f, 0.2f);
        glutSolidSphere(3.5f, 50, 50);
        glEnable(GL_LIGHTING);

        GLfloat emission[] = { 1.0f, 0.5f, 0.0f, 1.0f };
        GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, emission);
        glutSolidSphere(3.5f, 50, 50);
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
    }

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = 1; i <= 6; i++) {
        glColor4f(1.0f, 0.6f, 0.1f, 0.25f - (i * 0.04f));
        glutSolidSphere(3.5f + (i * 0.4f), 40, 40);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawTexturedPlanet(const CelestialBody& p, bool highlighted, int idx) {
    glPushMatrix();

    bool hasTexture = (p.textureID != 0);
    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, p.textureID);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(p.r, p.g, p.b);
    }

    applyPlanetMaterial(p, idx, highlighted, hasTexture);

    GLUquadricObj* sphere = gluNewQuadric();
    gluQuadricTexture(sphere, GL_TRUE);
    gluQuadricNormals(sphere, GLU_SMOOTH);

    float drawRadius = p.radius;
    if (highlighted) drawRadius *= 1.10f;

    gluSphere(sphere, drawRadius, 46, 46);
    gluDeleteQuadric(sphere);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

void drawSaturnRings(float radius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    const int SEGMENTS = 120;
    const int BANDS = 5;
    float startR = radius * 1.34f;

    for (int b = 0; b < BANDS; ++b) {
        float inner = startR + b * radius * 0.15f;
        float outer = inner + radius * (0.11f + 0.015f * (b % 2));

        float r1 = 0.92f - b * 0.05f;
        float g1 = 0.84f - b * 0.04f;
        float b1 = 0.70f - b * 0.03f;

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= SEGMENTS; ++i) {
            float a = 2.0f * PI * i / SEGMENTS;
            float ca = cosf(a);
            float sa = sinf(a);

            glColor4f(r1 * 0.88f, g1 * 0.88f, b1 * 0.88f, 0.26f);
            glVertex3f(inner * ca, 0.0f, inner * sa);

            glColor4f(r1, g1, b1, 0.50f);
            glVertex3f(outer * ca, 0.0f, outer * sa);
        }
        glEnd();
    }

    for (int b = 0; b < 3; ++b) {
        float inner = radius * (1.50f + b * 0.23f);
        float outer = inner + radius * 0.03f;

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= SEGMENTS; ++i) {
            float a = 2.0f * PI * i / SEGMENTS;
            float ca = cosf(a);
            float sa = sinf(a);
            glColor4f(0.20f, 0.18f, 0.15f, 0.18f);
            glVertex3f(inner * ca, 0.0f, inner * sa);
            glColor4f(0.08f, 0.08f, 0.08f, 0.03f);
            glVertex3f(outer * ca, 0.0f, outer * sa);
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawMoonForEarth() {
    float moonAngle = currentTime * 3.0f;
    float moonDist = 2.0f;
    float moonX = moonDist * cosf(moonAngle);
    float moonZ = moonDist * sinf(moonAngle);

    glPushMatrix();
    glTranslatef(moonX, 0.0f, moonZ);

    GLfloat ambient[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
    GLfloat diffuse[4] = { 0.78f, 0.78f, 0.78f, 1.0f };
    GLfloat specular[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
    setMaterialColor(ambient, diffuse, specular, 8.0f);

    glColor3f(0.82f, 0.82f, 0.80f);
    glutSolidSphere(0.2f, 20, 20);

    glPopMatrix();
}

void drawAsteroidShape(const Asteroid& a, GLUquadric* quad) {
    glPushMatrix();
    glScalef(a.scaleX, a.scaleY, a.scaleZ);

    switch (a.shapeType) {
    case 0:
        gluSphere(quad, a.size, 10, 10);
        break;
    case 1:
        glRotatef(25.0f, 1, 0, 0);
        gluSphere(quad, a.size, 9, 8);
        glTranslatef(a.size * 0.35f, a.size * 0.05f, -a.size * 0.12f);
        gluSphere(quad, a.size * 0.35f, 8, 8);
        break;
    case 2:
        glRotatef(35.0f, 0, 0, 1);
        gluSphere(quad, a.size, 7, 9);
        glTranslatef(-a.size * 0.28f, a.size * 0.18f, a.size * 0.08f);
        gluSphere(quad, a.size * 0.26f, 7, 7);
        break;
    default:
        glRotatef(18.0f, 1, 1, 0);
        gluSphere(quad, a.size, 8, 8);
        glTranslatef(a.size * 0.30f, -a.size * 0.10f, a.size * 0.22f);
        gluSphere(quad, a.size * 0.22f, 7, 7);
        glTranslatef(-a.size * 0.55f, a.size * 0.16f, -a.size * 0.05f);
        gluSphere(quad, a.size * 0.16f, 6, 6);
        break;
    }

    glPopMatrix();
}


static void drawMeteorRock(float scale, float sx, float sy, float sz) {
    glPushMatrix();
    glScalef(scale * sx, scale * sy, scale * sz);

    const int slices = 8;
    const int stacks = 8;

    for (int i = 0; i < stacks; i++) {
        float phi1 = PI * (float)i / stacks;
        float phi2 = PI * (float)(i + 1) / stacks;

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = TWO_PI * (float)j / slices;

            for (int p = 0; p < 2; p++) {
                float phi = (p == 0) ? phi1 : phi2;
                float jitter = 1.0f + 0.15f * sinf(phi * 7.0f + theta * 5.0f);

                float x = jitter * sinf(phi) * cosf(theta);
                float y = jitter * cosf(phi);
                float z = jitter * sinf(phi) * sinf(theta);

                glNormal3f(x, y, z);
                glTexCoord2f((float)j / slices, (float)i / stacks);
                glVertex3f(x, y, z);
            }
        }
        glEnd();
    }

    glPopMatrix();
}

static void drawMeteorTrailRibbon(const Meteor& m, float widthMul, float alphaMul, bool innerCore) {
    float vx = m.vx, vy = m.vy, vz = m.vz;
    normalize3(vx, vy, vz);

    // Ribbon side vector. It stays stable in world-space and avoids the old diagonal X/Z trail look.
    float sideX = vz;
    float sideY = 0.0f;
    float sideZ = -vx;
    normalize3(sideX, sideY, sideZ);

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < METEOR_TRAIL_POINTS; i++) {
        float t = (float)i / (float)(METEOR_TRAIL_POINTS - 1);
        float fade = (1.0f - t);
        float alpha = fade * fade * m.life * alphaMul;
        float width = m.scale * widthMul * (1.0f - t * 0.92f);

        if (innerCore) {
            // Thin white/yellow core.
            glColor4f(1.0f, 0.98f - t * 0.20f, 0.78f - t * 0.55f, alpha);
        }
        else {
            // Wider orange/red atmospheric tail.
            glColor4f(1.0f, 0.48f - t * 0.28f, 0.08f * fade, alpha);
        }

        float ox = m.trail[i][0] - m.x;
        float oy = m.trail[i][1] - m.y;
        float oz = m.trail[i][2] - m.z;

        glVertex3f(ox + sideX * width, oy + sideY * width, oz + sideZ * width);
        glVertex3f(ox - sideX * width, oy - sideY * width, oz - sideZ * width);
    }
    glEnd();
}

static void drawMeteorSparks(const Meteor& m) {
    glDisable(GL_TEXTURE_2D);
    glPointSize(clampf(m.scale * 18.0f, 1.2f, 4.0f));

    glBegin(GL_POINTS);
    for (int s = 0; s < 7; s++) {
        int idx = 1 + (s % 5);
        float seed = (float)(m.id * 37 + s * 19);
        float jx = sinf(seed * 1.31f + currentTime * 2.1f) * m.scale * 2.2f;
        float jy = cosf(seed * 1.73f + currentTime * 1.7f) * m.scale * 1.5f;
        float jz = sinf(seed * 2.17f + currentTime * 2.4f) * m.scale * 2.2f;
        float alpha = (1.0f - (float)s / 7.0f) * m.life * 0.45f;

        glColor4f(1.0f, 0.62f, 0.16f, alpha);
        glVertex3f(
            m.trail[idx][0] - m.x + jx,
            m.trail[idx][1] - m.y + jy,
            m.trail[idx][2] - m.z + jz
        );
    }
    glEnd();

    glPointSize(1.0f);
}

static void drawSingleMeteor(const Meteor& m) {
    if (m.life <= 0.0f || meteorQuadric == nullptr) return;

    glPushMatrix();
    glTranslatef(m.x, m.y, m.z);

    // Competition-polish choice: no text labels on meteors.
    // They should read as natural shooting stars, not named UI objects.

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    if (fireTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, fireTexture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    // Bigger fire head: still cinematic, but more visible from the solar-system camera.
    glColor4f(1.0f, 0.20f, 0.03f, 0.24f * m.life);
    gluSphere(meteorQuadric, m.scale * 4.2f, 16, 10);

    glColor4f(1.0f, 0.68f, 0.08f, 0.68f * m.life);
    gluSphere(meteorQuadric, m.scale * 3.75f, 14, 10);

    glColor4f(1.0f, 1.0f, 0.88f, 1.00f * m.life);
    gluSphere(meteorQuadric, m.scale * 0.82f, 12, 8);

    // Keep fire texture available for the rock/core pass as well.

    // Cinematic two-layer tail: wide glow + thin hot core.
    drawMeteorTrailRibbon(m, 3.2f, 0.62f, false);
    drawMeteorTrailRibbon(m, 0.80f, 1.00f, true);
    drawMeteorSparks(m);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // Fire-textured jagged core: all meteors rely on fire.jpg only.
    if (fireTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, fireTexture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    glRotatef(m.rot, m.rotAxis[0], m.rotAxis[1], m.rotAxis[2]);

    GLfloat matAmb[] = { 0.10f, 0.035f, 0.012f, 1.0f };
    GLfloat matDif[] = { 0.95f, 0.32f, 0.07f, 1.0f };
    GLfloat matSpc[] = { 0.55f, 0.28f, 0.08f, 1.0f };
    GLfloat matEmi[] = { m.life * 0.45f, m.life * 0.14f, 0.02f, 1.0f };
    GLfloat matSh[] = { 14.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDif);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpc);
    glMaterialfv(GL_FRONT, GL_EMISSION, matEmi);
    glMaterialfv(GL_FRONT, GL_SHININESS, matSh);

    drawMeteorRock(m.scale * 0.85f, 1.35f, 0.70f, 1.55f);

    GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, black);

    glPopMatrix();
}
static void drawMeteorShower() {
    if (!showMeteorShower || meteors.empty()) return;
    for (const Meteor& m : meteors) {
        drawSingleMeteor(m);
    }
}

void drawAsteroidBelt() {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    for (auto& a : asteroids) {
        glPushMatrix();

        float ang = (a.angle + currentTime * 2.0f) * PI / 180.0f;
        float x = a.dist * cosf(ang);
        float z = a.dist * sinf(ang);
        float y = a.height;

        glTranslatef(x, y, z);
        glRotatef(a.rot + currentTime * a.rotSpeed * 20.0f, 1, 1, 0);

        applyAsteroidMaterial();

        if (asteroidTexture != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, asteroidTexture);
            glColor3f(1, 1, 1);
        }
        else {
            glDisable(GL_TEXTURE_2D);
            glColor3f(0.7f, 0.7f, 0.7f);
        }

        drawAsteroidShape(a, quad);
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
    gluDeleteQuadric(quad);
}

void drawImpactAsteroid() {
    if (!launchImpactAsteroid && !asteroidHitEarth) return;

    glPushMatrix();
    glTranslatef(impactAsteroidX, impactAsteroidY, impactAsteroidZ);
    glRotatef(currentTime * 200.0f, 1, 1, 0);

    applyAsteroidMaterial();

    if (asteroidTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, asteroidTexture);
        glColor3f(1, 1, 1);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.7f, 0.6f, 0.5f);
    }

    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);
    glScalef(1.25f, 0.85f, 1.0f);
    gluSphere(quad, 0.35f, 14, 14);
    gluDeleteQuadric(quad);

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void drawImpactExplosion() {
    if (!showImpactExplosion) return;

    float t = explosionProgress / explosionDuration;
    if (t > 1.0f) t = 1.0f;

    float flashRadius = 0.25f + t * 1.05f;
    float fireRadius = 0.18f + t * 1.55f;
    float smokeRadius = 0.20f + t * 2.10f;

    float flashAlpha = clampf(0.95f * (1.0f - t * 2.4f), 0, 1);
    float fireAlpha = 0.78f * (1.0f - t);
    float coreAlpha = clampf(0.50f * (1.0f - t * 1.15f), 0, 1);
    float smokeAlpha = clampf(0.38f * (1.0f - t * 0.85f), 0, 1);

    glPushMatrix();
    glTranslatef(explosionX, explosionY, explosionZ);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1, 1, 0.85f, flashAlpha);        glutSolidSphere(flashRadius, 20, 20);
    glColor4f(1, 0.45f, 0.05f, fireAlpha);     glutSolidSphere(fireRadius, 24, 24);
    glColor4f(1, 0.85f, 0.25f, coreAlpha);     glutSolidSphere(fireRadius * 0.55f, 20, 20);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(0, t * 0.35f, 0);
    glColor4f(0.18f, 0.18f, 0.18f, smokeAlpha);
    glutSolidSphere(smokeRadius, 24, 24);
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}


void drawDetailedSatellite(float scale, float panelR, float panelG, float panelB, int variant) {
    glPushMatrix();

    applySatelliteMaterial(1.0f);
    glColor3f(0.80f, 0.82f, 0.86f);

    glPushMatrix();
    glScalef(1.1f, 0.7f, 0.8f);
    glutSolidCube(scale);
    glPopMatrix();

    glColor3f(panelR, panelG, panelB);
    applySatelliteMaterial(0.95f);

    glPushMatrix();
    glTranslatef(scale * 1.15f, 0.0f, 0.0f);
    glScalef(2.8f, 0.10f, 1.1f);
    glutSolidCube(scale);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-scale * 1.15f, 0.0f, 0.0f);
    glScalef(2.8f, 0.10f, 1.1f);
    glutSolidCube(scale);
    glPopMatrix();

    glColor3f(0.70f, 0.72f, 0.75f);
    applySatelliteMaterial(0.85f);
    glPushMatrix();
    glTranslatef(0.0f, scale * 0.55f, 0.0f);
    glScalef(0.18f, 1.0f, 0.18f);
    glutSolidCube(scale);
    glPopMatrix();

    if (variant == 0) {
        glPushMatrix();
        glTranslatef(0.0f, scale * 0.95f, 0.0f);
        glRotatef(-90.0f, 1, 0, 0);
        glutSolidCone(scale * 0.22f, scale * 0.35f, 16, 8);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, -scale * 0.35f, scale * 0.35f);
        glutSolidSphere(scale * 0.16f, 12, 12);
        glPopMatrix();
    }
    else if (variant == 1) {
        glPushMatrix();
        glTranslatef(0.0f, scale * 0.82f, 0.0f);
        glRotatef(90.0f, 0, 0, 1);
        glutSolidCone(scale * 0.18f, scale * 0.42f, 14, 8);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, -scale * 0.30f, -scale * 0.35f);
        glScalef(0.45f, 0.20f, 0.45f);
        glutSolidSphere(scale, 10, 10);
        glPopMatrix();
    }
    else {
        glPushMatrix();
        glTranslatef(scale * 0.30f, scale * 0.80f, 0.0f);
        glutSolidSphere(scale * 0.12f, 10, 10);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-scale * 0.35f, scale * 0.75f, 0.0f);
        glRotatef(-90.0f, 1, 0, 0);
        glutSolidCone(scale * 0.12f, scale * 0.28f, 12, 8);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, -scale * 0.35f, 0.0f);
        glScalef(0.25f, 0.80f, 0.25f);
        glutSolidCube(scale);
        glPopMatrix();
    }

    glPopMatrix();
}

void drawEarthSatellites() {
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    float satAngle1 = currentTime * satelliteOrbitSpeed;
    float sx1 = satelliteDistance * cosf(satAngle1);
    float sz1 = satelliteDistance * sinf(satAngle1);
    glTranslatef(sx1, 0, sz1);
    glRotatef(currentTime * 50, 0, 1, 0);
    drawDetailedSatellite(satelliteSize, 0.25f, 0.45f, 1.0f, 0);
    glPopMatrix();

    glPushMatrix();
    float satAngle2 = currentTime * satelliteOrbitSpeed * 0.9f;
    float sy2 = (satelliteDistance + 0.8f) * cosf(satAngle2);
    float sz2 = (satelliteDistance + 0.8f) * sinf(satAngle2);
    glTranslatef(0, sy2, sz2);
    glRotatef(currentTime * 70, 1, 0, 0);
    drawDetailedSatellite(satelliteSize * 0.95f, 1.0f, 0.78f, 0.22f, 1);
    glPopMatrix();

    glPushMatrix();
    float satAngle3 = currentTime * satelliteOrbitSpeed * 1.2f;
    float sx3 = (satelliteDistance + 1.4f) * cosf(satAngle3);
    float sy3 = (satelliteDistance + 1.4f) * sinf(satAngle3);
    glTranslatef(sx3, sy3, 0);
    glRotatef(currentTime * 60, 0, 0, 1);
    drawDetailedSatellite(satelliteSize * 0.90f, 0.35f, 0.85f, 1.0f, 2);
    glPopMatrix();
}

void drawPlanets() {
    for (int idx = 0; idx < (int)planets.size(); ++idx) {
        CelestialBody& p = planets[idx];
        glPushMatrix();

        float angle = currentTime * p.orbitSpeed * 0.1f;
        float x = p.distance * cosf(angle);
        float z = p.distance * sinf(angle);
        float y = z * sinf(p.inclination * PI / 180.0f);
        z = z * cosf(p.inclination * PI / 180.0f);
        p.curX = x; p.curY = y; p.curZ = z;

        glTranslatef(x, y, z);
        bool highlighted = (idx == hoveredPlanetIndex) || (idx == focusedPlanetIndex);

        if (showLabels && !isPlanetSelectedView) {
            glDisable(GL_LIGHTING);
            glColor3f(1.0f, 1.0f, 1.0f);
            glRasterPos3f(0, p.radius + 1.6f, 0);
            for (char c : p.name) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            glEnable(GL_LIGHTING);
        }

        glRotatef(currentTime * p.rotationSpeed, 0, 1, 0);
        drawTexturedPlanet(p, highlighted, idx);

        if (idx == 2 && earthHasCrater)
            drawImpactCraterOnPlanet(p.radius, 1.0f);

        if (idx == 2) {
            drawMoonForEarth();
            drawEarthSatellites();
        }

        if (idx == 5) {
            glRotatef(25, 1, 0, 0);
            drawSaturnRings(p.radius);
        }

        glPopMatrix();
    }
}

void drawSelectedPlanetView() {
    if (selectedPlanetIndex == -1) return;

    CelestialBody& p = planets[selectedPlanetIndex];
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    bool hasTexture = (p.textureID != 0);
    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, p.textureID);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(p.r, p.g, p.b);
    }

    applyPlanetMaterial(p, selectedPlanetIndex, false, hasTexture);

    glPushMatrix();
    glRotatef(selectedViewCamAngleY, 0, 1, 0);
    glRotatef(selectedViewCamAngleX, 1, 0, 0);
    glRotatef(currentTime * p.rotationSpeed * 8.0f, 0, 1, 0);

    const float FIXED_PLANET_SIZE = (selectedPlanetIndex == 4 || selectedPlanetIndex == 5) ? 2.0f : 1.5f;

    GLUquadricObj* sphere = gluNewQuadric();
    gluQuadricTexture(sphere, GL_TRUE);
    gluQuadricNormals(sphere, GLU_SMOOTH);
    gluSphere(sphere, FIXED_PLANET_SIZE, 50, 50);
    gluDeleteQuadric(sphere);

    if (selectedPlanetIndex == 2 && earthHasCrater)
        drawImpactCraterOnPlanet(FIXED_PLANET_SIZE, 0.9f);

    if (selectedPlanetIndex == 5) {
        glRotatef(25, 1, 0, 0);
        drawSaturnRings(FIXED_PLANET_SIZE);
    }

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

// =========================
// Info Panels
// =========================
void drawInfoPanel() {
    if (focusedPlanetIndex == -1 || isPlanetSelectedView) return;

    CelestialBody& p = planets[focusedPlanetIndex];
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    int boxWidth = 430, boxHeight = 178;
    int boxX = w - boxWidth - 20, boxY = h - boxHeight - 20;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor4f(0.03f, 0.05f, 0.1f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f((float)boxX, (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)(boxY + boxHeight));
    glVertex2f((float)boxX, (float)(boxY + boxHeight));
    glEnd();

    glColor3f(0.5f, 0.85f, 1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)boxX, (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)(boxY + boxHeight));
    glVertex2f((float)boxX, (float)(boxY + boxHeight));
    glEnd();

    glColor3f(1, 1, 0);
    drawString(GLUT_BITMAP_HELVETICA_18, (float)(boxX + 15), (float)(boxY + boxHeight - 25), p.name.c_str());

    glColor3f(1, 1, 1);
    char line1[128];
    sprintf(line1, "Radius: %.2f  Distance: %.1f  Moons: %d", p.radius, p.distance, p.moonCount);

    const char* modeText = "Mode: Focus";
    if (cameraMode == CAMERA_INSPECT) modeText = "Mode: Inspect";
    else if (cameraMode == CAMERA_EXPLORE) modeText = "Mode: Explore";

    drawString(GLUT_BITMAP_HELVETICA_12, (float)(boxX + 15), (float)(boxY + boxHeight - 55), line1);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(boxX + 15), (float)(boxY + boxHeight - 72), modeText);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(boxX + 15), (float)(boxY + 22), p.info.c_str());

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawInfoPopupSelectedView() {
    if (selectedPlanetIndex == -1 || !isPlanetSelectedView) return;

    CelestialBody& p = planets[selectedPlanetIndex];
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    int boxHeight = 250, boxWidth = 450;
    int boxX = (w - boxWidth) / 2, boxY = 40;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.05f, 0.05f, 0.1f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f((float)boxX, (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)(boxY + boxHeight));
    glVertex2f((float)boxX, (float)(boxY + boxHeight));
    glEnd();

    glColor3f(0.5f, 0.8f, 1);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f((float)boxX, (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)boxY);
    glVertex2f((float)(boxX + boxWidth), (float)(boxY + boxHeight));
    glVertex2f((float)boxX, (float)(boxY + boxHeight));
    glEnd();
    glLineWidth(1);

    glColor3f(1, 1, 0);
    drawString(GLUT_BITMAP_HELVETICA_18, (float)(boxX + 20), (float)(boxY + boxHeight - 30), ("Planet: " + p.name).c_str());

    glColor3f(0.9f, 0.9f, 1);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(boxX + 20), (float)(boxY + boxHeight - 60), "Left Drag: Rotate | Wheel: Zoom | ESC: Back");

    char buffer[128];
    sprintf(buffer, "Distance: %.1f  Radius: %.2f  Moons: %d", p.distance, p.radius, p.moonCount);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(boxX + 20), (float)(boxY + boxHeight - 85), buffer);
    drawString(GLUT_BITMAP_HELVETICA_12, (float)(boxX + 20), (float)(boxY + 25), p.info.c_str());

    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawHUD() {
    if (!showHUD || isPlanetSelectedView) return;

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 0.8f);
    drawString(GLUT_BITMAP_HELVETICA_18, 20, (float)(h - 25), "Solar System Final");

    glColor3f(1, 1, 1);
    drawString(GLUT_BITMAP_HELVETICA_12, 20, (float)(h - 48), "Double Click Planet: Focus -> Explore -> Inspect");
    drawString(GLUT_BITMAP_HELVETICA_12, 20, (float)(h - 64), "Explore: Left Drag Look 360 | Right Drag Move On Surface | Wheel Fine Zoom | Arrows Look");
    drawString(GLUT_BITMAP_HELVETICA_12, 20, (float)(h - 80), "Free/Focus: Left Drag Orbit | Right Drag Pan | Double Click Empty: Free Mode");
    drawString(GLUT_BITMAP_HELVETICA_12, 20, (float)(h - 96), "ESC: Back | P: Planet View | R: Reset | I: Asteroid  | F: Fullscreen");

    if (hoveredPlanetIndex != -1 && hoveredPlanetIndex < (int)planets.size()) {
        std::string hoverText = "Hover: " + planets[hoveredPlanetIndex].name;
        glColor3f(0.6f, 1, 0.8f);
        drawString(GLUT_BITMAP_HELVETICA_18, (float)(w / 2 - 60), (float)(h - 28), hoverText.c_str());
    }

    if (cameraMode == CAMERA_EXPLORE && focusedPlanetIndex != -1) {
        std::string modeText = "EXPLORE MODE: " + planets[focusedPlanetIndex].name;
        glColor3f(1.0f, 0.85f, 0.3f);
        drawString(GLUT_BITMAP_HELVETICA_18, 20, (float)(h - 120), modeText.c_str());
    }

    glColor3f(0.7f, 1, 0.7f);
    float cx = w / 2.0f, cy = h / 2.0f;
    glBegin(GL_LINES);
    glVertex2f(cx - 8, cy); glVertex2f(cx + 8, cy);
    glVertex2f(cx, cy - 8); glVertex2f(cx, cy + 8);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}


static const char* getCurrentModeName() {
    if (showStartScreen) return "Start Screen";
    if (inMercuryMode) return "Mercury Surface";
    if (inTerrainMode) return "Earth Terrain";
    if (inVenusTerrainMode) return "Venus Terrain";
    if (inMarsMode) return "Mars Terrain";
    if (inJupiterMode) return "Jupiter Storm Deck";
    if (inSaturnMode) return "Saturn Ring Sky";
    if (inUranusMode) return "Uranus Methane Haze";
    if (inNeptuneMode) return "Neptune Interior";
    if (isPlanetSelectedView) return "Planet View";
    if (cameraMode == CAMERA_FOCUS) return "Planet Focus";
    if (cameraMode == CAMERA_INSPECT) return "Inspect";
    if (cameraMode == CAMERA_EXPLORE) return "Spherical Explore";
    return "Space";
}

static void begin2DOverlay(int& w, int& h) {
    w = glutGet(GLUT_WINDOW_WIDTH);
    h = glutGet(GLUT_WINDOW_HEIGHT);
    if (h <= 0) h = 1;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

static void end2DOverlay() {
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void drawPanelRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor4f(0.45f, 0.75f, 1.0f, 0.55f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);
}

void drawStartScreen() {
    if (!showStartScreen) return;

    int w, h;
    begin2DOverlay(w, h);

    glDisable(GL_BLEND);
    glColor3f(0.005f, 0.008f, 0.025f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f((float)w, 0); glVertex2f((float)w, (float)h); glVertex2f(0, (float)h);
    glEnd();
    glEnable(GL_BLEND);

    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 220; i++) {
        float sx = fmodf((float)(i * 97), (float)w);
        float sy = fmodf((float)(i * 181), (float)h);
        float tw = 0.55f + 0.45f * sinf(currentTime * 2.2f + i * 0.37f);
        glColor4f(0.60f + tw * 0.35f, 0.72f + tw * 0.20f, 1.0f, 0.45f + tw * 0.45f);
        glVertex2f(sx, sy);
    }
    glEnd();

    float boxW = 620.0f;
    float boxH = 300.0f;
    float x = (w - boxW) * 0.5f;
    float y = (h - boxH) * 0.5f;
    drawPanelRect(x, y, boxW, boxH, 0.02f, 0.04f, 0.11f, 0.86f);

    glColor3f(1.0f, 0.86f, 0.25f);
    drawString(GLUT_BITMAP_TIMES_ROMAN_24, x + 130.0f, y + boxH - 60.0f, "SOLAR SYSTEM INTERACTIVE");

    glColor3f(0.70f, 0.90f, 1.0f);
    drawString(GLUT_BITMAP_HELVETICA_18, x + 165.0f, y + boxH - 98.0f, "OpenGL C++");

    glColor3f(0.92f, 0.96f, 1.0f);
    drawString(GLUT_BITMAP_HELVETICA_18, x + 130.0f, y + boxH - 150.0f, "ENTER / SPACE  Start Exploration");
    drawString(GLUT_BITMAP_HELVETICA_18, x + 130.0f, y + boxH - 180.0f, "C              Cinematic Tour");
    drawString(GLUT_BITMAP_HELVETICA_18, x + 130.0f, y + boxH - 210.0f, "F1 or ?        Controls Panel");

    glColor3f(0.55f, 0.75f, 1.0f);
    drawString(GLUT_BITMAP_HELVETICA_12, x + 115.0f, y + 35.0f, "All 8 planet exploration modes + meteors + impact");

    end2DOverlay();
}

void drawControlsPanelOverlay() {
    if (!showControlsPanel) return;

    int w, h;
    begin2DOverlay(w, h);

    float boxW = 360.0f;
    float boxH = 300.0f;
    float x = (float)w - boxW - 24.0f;
    float y = (float)h - boxH - 24.0f;

    drawPanelRect(x, y, boxW, boxH, 0.015f, 0.020f, 0.055f, 0.84f);

    glColor3f(1.0f, 0.86f, 0.28f);
    drawString(GLUT_BITMAP_HELVETICA_18, x + 18.0f, y + boxH - 32.0f, "Controls");

    glColor3f(0.90f, 0.96f, 1.0f);
    float yy = y + boxH - 62.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "1 Mercury surface      2 Venus terrain"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "3 Earth terrain        4 Mars terrain"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "5 Jupiter storm        6 Saturn ring-sky"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "7 Uranus haze          8 Neptune interior"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "WASD Move              Mouse Look"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "ESC Back / Exit         F Fullscreen"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "T Meteor Shower         I Impact Asteroid"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "P Planet View           O Orbits / L Labels"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "Earth mode: 1 Clouds  2 Rain  3 Thunder");

    glColor3f(0.55f, 0.78f, 1.0f);
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, y + 22.0f, "Tip: press C during presentation for an automatic demo.");

    end2DOverlay();
}

void drawPerformanceHUD() {
    if (!showPerformanceHUD || showStartScreen) return;

    int w, h;
    begin2DOverlay(w, h);

    float boxW = 270.0f;
    float boxH = 118.0f;
    float x = (float)w - boxW - 24.0f;
    float y = 24.0f;

    drawPanelRect(x, y, boxW, boxH, 0.010f, 0.015f, 0.040f, 0.70f);

    char line[160];
    glColor3f(0.70f, 1.0f, 0.72f);
    sprintf(line, "FPS: %.0f", fpsValue);
    drawString(GLUT_BITMAP_HELVETICA_12, x + 14.0f, y + boxH - 24.0f, line);

    glColor3f(0.90f, 0.96f, 1.0f);
    sprintf(line, "Mode: %s", getCurrentModeName());
    drawString(GLUT_BITMAP_HELVETICA_12, x + 14.0f, y + boxH - 44.0f, line);

    sprintf(line, "Chunks  E:%d  V:%d  M:%d", (int)activeEarthChunks.size(), (int)activeVenusChunks.size(), (int)activeMarsChunks.size());
    drawString(GLUT_BITMAP_HELVETICA_12, x + 14.0f, y + boxH - 64.0f, line);

    sprintf(line, "Meteors: %s (%d)   Speed: %.1fx", showMeteorShower ? "ON" : "OFF", (int)meteors.size(), timeSpeed);
    drawString(GLUT_BITMAP_HELVETICA_12, x + 14.0f, y + boxH - 84.0f, line);

    glColor3f(1.0f, 0.80f, 0.35f);
    drawString(GLUT_BITMAP_HELVETICA_12, x + 14.0f, y + 14.0f, cinematicTourActive ? "Cinematic Tour: RUNNING" : "Cinematic Tour: C to start");

    end2DOverlay();
}

void drawCompetitionOverlays() {
    drawPerformanceHUD();
    drawControlsPanelOverlay();
}

static void stopCinematicTour() {
    cinematicTourActive = false;
    cinematicStep = 0;
    cinematicTimer = 0.0f;
    keyW = keyA = keyS = keyD = false;
}

static void resetCinematicSpaceView(float dist, float yaw, float pitch) {
    isPlanetSelectedView = false;
    selectedPlanetIndex = -1;
    isPaused = false;
    cameraMode = CAMERA_FREE;
    focusedPlanetIndex = -1;
    targetLookX = targetLookY = targetLookZ = 0.0f;
    targetFocusOffsetX = targetFocusOffsetY = targetFocusOffsetZ = 0.0f;
    targetCamDistance = dist;
    targetCamYaw = yaw;
    targetCamPitch = pitch;
    targetFOV = 55.0f;
}

static void startCinematicTour() {
    showStartScreen = false;
    showControlsPanel = false;
    showMeteorShower = true;
    isPaused = false;
    stopCinematicTour();
    cinematicTourActive = true;
    cinematicStep = 0;
    cinematicTimer = 0.0f;
    resetCinematicSpaceView(120.0f, -48.0f, 38.0f);
}

static void cinematicSurfaceDrift() {
    float dt = clampf(deltaTime, 0.0f, 0.05f);
    if (inMercuryMode || inTerrainMode || inVenusTerrainMode) {
        targetTCamYaw += 7.0f * dt;
        float yawRad = targetTCamYaw * DEG2RAD;
        targetTCamX += sinf(yawRad) * 10.0f * dt;
        targetTCamZ += cosf(yawRad) * 10.0f * dt;
    }
    else if (inMarsMode) {
        targetMarsCamYaw += 7.0f * dt;
        float yawRad = targetMarsCamYaw * DEG2RAD;
        targetMarsCamX += sinf(yawRad) * 18.0f * dt;
        targetMarsCamZ += cosf(yawRad) * 18.0f * dt;
    }
    else if (inNeptuneMode) {
        targetNCamYaw += 8.0f * dt;
        float yawRad = targetNCamYaw * DEG2RAD;
        targetNCamX += sinf(yawRad) * 8.0f * dt;
        targetNCamZ += cosf(yawRad) * 8.0f * dt;
    }
}

static void updateCinematicTour() {
    if (!cinematicTourActive) return;

    if (transActive) return;

    cinematicTimer += deltaTime;
    cinematicSurfaceDrift();

    switch (cinematicStep) {
    case 0:
        resetCinematicSpaceView(118.0f, -45.0f, 36.0f);
        if (cinematicTimer > 3.0f) { cinematicStep++; cinematicTimer = 0.0f; focusPlanet(2); }
        break;
    case 1:
        if (cinematicTimer > 2.5f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToTerrain(); }
        break;
    case 2:
        if (inTerrainMode && cinematicTimer > 5.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToSpace(); }
        break;
    case 3:
        if (!inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inNeptuneMode && cinematicTimer > 1.5f) {
            focusPlanet(1); cinematicStep++; cinematicTimer = 0.0f;
        }
        break;
    case 4:
        if (cinematicTimer > 2.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToVenusTerrain(); }
        break;
    case 5:
        if (inVenusTerrainMode && cinematicTimer > 5.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToSpace(); }
        break;
    case 6:
        if (!inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inNeptuneMode && cinematicTimer > 1.5f) {
            focusPlanet(3); cinematicStep++; cinematicTimer = 0.0f;
        }
        break;
    case 7:
        if (cinematicTimer > 2.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToMars(); }
        break;
    case 8:
        if (inMarsMode && cinematicTimer > 5.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToSpace(); }
        break;
    case 9:
        if (!inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inNeptuneMode && cinematicTimer > 1.5f) {
            focusPlanet(7); cinematicStep++; cinematicTimer = 0.0f;
        }
        break;
    case 10:
        if (cinematicTimer > 2.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToNeptune(); }
        break;
    case 11:
        if (inNeptuneMode && cinematicTimer > 5.0f) { cinematicStep++; cinematicTimer = 0.0f; startTransitionToSpace(); }
        break;
    case 12:
        if (!inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inNeptuneMode) {
            resetCinematicSpaceView(125.0f, -38.0f, 36.0f);
            if (cinematicTimer > 6.0f) stopCinematicTour();
        }
        break;
    default:
        stopCinematicTour();
        break;
    }
}

// =========================
// Picking
// =========================
void checkPlanetHover(int x, int y) {
    if (isPlanetSelectedView || cameraMode == CAMERA_EXPLORE) return;

    GLint viewport[4];
    GLdouble modelview[16], projection[16];
    GLdouble winX, winY, winZ;

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    float mouseX = (float)x;
    float mouseY = (float)viewport[3] - (float)y;

    hoveredPlanetIndex = -1;
    for (int i = 0; i < (int)planets.size(); ++i) {
        gluProject(planets[i].curX, planets[i].curY, planets[i].curZ,
            modelview, projection, viewport, &winX, &winY, &winZ);

        float dx = mouseX - (float)winX;
        float dy = mouseY - (float)winY;
        if (sqrtf(dx * dx + dy * dy) < 22.0f) {
            hoveredPlanetIndex = i;
            break;
        }
    }
}

// =========================
// Update
// =========================
void updateImpactAsteroid() {
    // Convert deltaTime to a 60 FPS-style multiplier.
    // Example: at 60 FPS, deltaTime ~= 0.016, so frameScale ~= 1.0
    // Clamp prevents huge jumps if the game freezes/lags for a moment.
    float frameScale = deltaTime * 60.0f;
    frameScale = clampf(frameScale, 0.0f, 3.0f);

    if (launchImpactAsteroid) {
        CelestialBody& earth = planets[2];

        impactProgress += impactSpeed * timeSpeed * frameScale;

        impactAsteroidX = impactStartX + (earth.curX - impactStartX) * impactProgress;
        impactAsteroidY = impactStartY + (earth.curY - impactStartY) * impactProgress;
        impactAsteroidZ = impactStartZ + (earth.curZ - impactStartZ) * impactProgress;

        bool hit = checkSphereCollision(
            impactAsteroidX, impactAsteroidY, impactAsteroidZ, IMPACT_ASTEROID_RADIUS,
            earth.curX, earth.curY, earth.curZ, earth.radius * 0.98f
        );

        if (hit || impactProgress >= 1.2f) {
            float dx = impactAsteroidX - earth.curX;
            float dy = impactAsteroidY - earth.curY;
            float dz = impactAsteroidZ - earth.curZ;

            float len = sqrtf(dx * dx + dy * dy + dz * dz);

            if (len < 0.0001f) {
                dx = 0.3f;
                dy = 0.1f;
                dz = 0.2f;
                len = sqrtf(dx * dx + dy * dy + dz * dz);
            }

            dx /= len;
            dy /= len;
            dz /= len;

            explosionX = earth.curX + dx * earth.radius * 0.95f;
            explosionY = earth.curY + dy * earth.radius * 0.95f;
            explosionZ = earth.curZ + dz * earth.radius * 0.95f;

            launchImpactAsteroid = false;
            asteroidHitEarth = true;
            showImpactExplosion = true;
            explosionProgress = 0.0f;
            earthHasCrater = true;
            craterFlash = 1.0f;
        }
    }

    if (showImpactExplosion) {
        explosionProgress += 0.01f * timeSpeed * frameScale;

        if (explosionProgress >= explosionDuration) {
            showImpactExplosion = false;
            asteroidHitEarth = false;
        }
    }

    if (craterFlash > 0.0f) {
        craterFlash -= 0.02f * timeSpeed * frameScale;

        if (craterFlash < 0.0f) {
            craterFlash = 0.0f;
        }
    }
}

static void resetSpaceLighting() {
    GLfloat globalAmbient[] = { 0.02f, 0.02f, 0.03f, 1.0f };
    GLfloat ambient[] = { 0.03f, 0.03f, 0.03f, 1.0f };
    GLfloat diffuse[] = { 1.95f, 1.85f, 1.70f, 1.0f };
    GLfloat specular[] = { 1.50f, 1.40f, 1.20f, 1.0f };

    glDisable(GL_FOG);
    glDisable(GL_LIGHT1);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0012f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00002f);
}



static void cleanupMeteors() {
    if (fireTexture != 0) {
        glDeleteTextures(1, &fireTexture);
        fireTexture = 0;
    }

    if (meteorQuadric != nullptr) {
        gluDeleteQuadric(meteorQuadric);
        meteorQuadric = nullptr;
    }

    meteors.clear();
}

static void cleanupAllGLResources() {
    cleanupMercuryTerrain();
    cleanupEarthTerrain();
    cleanupVenusTerrain();
    cleanupMarsTerrain();
    cleanupMeteors();

    if (orbitsList != 0) {
        glDeleteLists(orbitsList, 1);
        orbitsList = 0;
    }

    if (coneList != 0) {
        glDeleteLists(coneList, 1);
        coneList = 0;
    }

    if (sphereList != 0) {
        glDeleteLists(sphereList, 1);
        sphereList = 0;
    }

    if (sunTexture != 0) { glDeleteTextures(1, &sunTexture); sunTexture = 0; }
    if (asteroidTexture != 0) { glDeleteTextures(1, &asteroidTexture); asteroidTexture = 0; }
    if (marsGasTexture != 0) { glDeleteTextures(1, &marsGasTexture); marsGasTexture = 0; }
    if (earthNightTexture != 0) { glDeleteTextures(1, &earthNightTexture); earthNightTexture = 0; }
    if (earthCloudTexture != 0) { glDeleteTextures(1, &earthCloudTexture); earthCloudTexture = 0; }

    for (int i = 0; i < 8; i++) {
        if (planetTextures[i] != 0) {
            glDeleteTextures(1, &planetTextures[i]);
            planetTextures[i] = 0;
        }
    }

    for (int i = 0; i < 6; i++) {
        if (skyboxTextures[i] != 0) {
            glDeleteTextures(1, &skyboxTextures[i]);
            skyboxTextures[i] = 0;
        }
    }
}

static void applyRenderSceneState(RenderScene scene) {
    if (scene == appliedRenderScene) return;

    appliedRenderScene = scene;

    switch (scene) {
    case SCENE_SPACE:
        resetSpaceLighting();
        glDisable(GL_FOG);
        glDisable(GL_LIGHT1);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_TEXTURE_2D);
        break;

    case SCENE_MERCURY:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glDisable(GL_FOG);
        glEnable(GL_TEXTURE_2D);
        break;

    case SCENE_EARTH:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glEnable(GL_FOG);
        break;

    case SCENE_VENUS:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_FOG);
        break;

    case SCENE_MARS:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_FOG);
        break;

    case SCENE_JUPITER:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_FOG);
        glEnable(GL_BLEND);
        break;

    case SCENE_SATURN:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_FOG);
        glEnable(GL_BLEND);
        break;

    case SCENE_URANUS:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_FOG);
        glEnable(GL_BLEND);
        break;

    case SCENE_NEPTUNE:
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_FOG);
        break;
    }
}

// =========================
// Display / Idle
// =========================
void display() {
    if (showStartScreen) {
        applyRenderSceneState(SCENE_SPACE);
        glClearColor(0.005f, 0.008f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawStartScreen();
        drawControlsPanelOverlay();
        glutSwapBuffers();
        return;
    }

    if (inMercuryMode) {
        applyRenderSceneState(SCENE_MERCURY);
        displayMercuryScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inTerrainMode) {
        applyRenderSceneState(SCENE_EARTH);
        displayTerrainScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inVenusTerrainMode) {
        applyRenderSceneState(SCENE_VENUS);
        displayVenusTerrainScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inMarsMode) {
        applyRenderSceneState(SCENE_MARS);
        displayMarsScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inJupiterMode) {
        applyRenderSceneState(SCENE_JUPITER);
        displayJupiterScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inSaturnMode) {
        applyRenderSceneState(SCENE_SATURN);
        displaySaturnScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inUranusMode) {
        applyRenderSceneState(SCENE_URANUS);
        displayUranusScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    if (inNeptuneMode) {
        applyRenderSceneState(SCENE_NEPTUNE);
        displayNeptuneScene();
        drawTransitionOverlay();
        drawCompetitionOverlays();
        glutSwapBuffers();
        return;
    }

    applyRenderSceneState(SCENE_SPACE);
    glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float nearPlane = 1.0f;
    float farPlane = 1000.0f;

    if (cameraMode == CAMERA_EXPLORE) {
        nearPlane = 0.03f;
        farPlane = 500.0f;
    }

    applyPerspectiveIfNeeded(cameraFOV, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (isPlanetSelectedView && selectedPlanetIndex != -1) {
        float radY = selectedViewCamAngleY * PI / 180.0f;
        float radX = selectedViewCamAngleX * PI / 180.0f;

        float eyeX = selectedViewCamDistance * sinf(radY) * cosf(radX);
        float eyeZ = selectedViewCamDistance * cosf(radY) * cosf(radX);
        float eyeY = selectedViewCamDistance * sinf(radX);

        gluLookAt(
            eyeX, eyeY, eyeZ,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f
        );

        GLfloat localLightPos[] = { -8.0f, 3.0f, 7.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, localLightPos);

        drawSkybox();

        drawSelectedPlanetView();
        drawInfoPopupSelectedView();
    }
    else {
        if (cameraMode == CAMERA_EXPLORE) {
            applyExploreCamera();
        }
        else {
            applyCamera();
        }

        GLfloat sunPosView[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, sunPosView);

        drawSkybox();

        if (showOrbits && cameraMode != CAMERA_EXPLORE) {
            glCallList(orbitsList);
        }

        drawMeteorShower();
        drawAsteroidBelt();
        drawImpactAsteroid();
        drawSun();
        drawPlanets();
        drawImpactExplosion();
        drawInfoPanel();
        drawHUD();
    }

    drawTransitionOverlay();
    drawCompetitionOverlays();

    glutSwapBuffers();
}

void idle() {
    // Delta time calculation for terrain movement/weather.
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (lastFrameTime == 0) lastFrameTime = now;
    int dtMs = now - lastFrameTime;
    lastFrameTime = now;
    if (dtMs < 0) dtMs = 0;
    if (dtMs > 100) dtMs = 100;
    deltaTime = dtMs / 1000.0f;
    smoothDeltaTime += (deltaTime - smoothDeltaTime) * 0.3f;

    fpsFrameCount++;
    fpsAccumTime += deltaTime;
    if (fpsAccumTime >= 0.50f) {
        fpsValue = (fpsAccumTime > 0.0f) ? ((float)fpsFrameCount / fpsAccumTime) : 0.0f;
        fpsFrameCount = 0;
        fpsAccumTime = 0.0f;
    }

    updateTransition();
    updateCinematicTour();

    if (inMercuryMode || inTerrainMode || inVenusTerrainMode) {
        updateTerrainCamera();
    }
    else if (inMarsMode) {
        updateMarsCamera();
        updateMarsParticles();
        if ((rand() % 200) < 2) marsLightning = 1.0f;
        else marsLightning *= 0.85f;
    }
    else if (inJupiterMode) {
        updateJupiterCamera();
        updateJupiterParticles();
    }
    else if (inSaturnMode) {
        updateSaturnCamera();
        updateSaturnMotes();
    }
    else if (inUranusMode) {
        updateUranusCamera();
        updateUranusHaze();
    }
    else if (inNeptuneMode) {
        updateNeptuneCamera();
        updateNeptuneSnow();
    }
    else {
        updateCamera();
    }

    if (!isPaused) {
        currentTime += 0.6f * deltaTime * timeSpeed;
        updateImpactAsteroid();
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode) {
            updateMeteors();
        }
    }

    glutPostRedisplay();
}

// =========================
// Input
// =========================
void handleDoubleClickAction(int clickedPlanet) {
    if (clickedPlanet != -1) {
        toggleFocusExploreInspect(clickedPlanet);
    }
    else {
        if (cameraMode == CAMERA_FOCUS || cameraMode == CAMERA_INSPECT || cameraMode == CAMERA_EXPLORE) {
            resetCamera();
        }
    }
}

void mouse(int button, int state, int x, int y) {
    if (transActive) {
        glutPostRedisplay();
        return;
    }

    if (inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) {
        // Mouse wheel changes altitude/height in exploration modes.
        if (button == 3 && state == GLUT_DOWN) {
            if (inMercuryMode || inTerrainMode || inVenusTerrainMode) {
                float minAlt = inMercuryMode ? 1.8f : 2.0f;
                float maxAlt = inMercuryMode ? 90.0f : 75.0f;
                targetTCamY = clampf(targetTCamY * 0.88f, minAlt, maxAlt);
            }
            else if (inMarsMode) {
                targetMarsCamY = clampf(targetMarsCamY - 20.0f, 4.0f, 350.0f);
            }
            else if (inJupiterMode) {
                targetJCamY = clampf(targetJCamY - 4.0f, -8.0f, 60.0f);
            }
            else if (inSaturnMode) {
                targetSCamY = clampf(targetSCamY - 3.5f, -10.0f, 72.0f);
            }
            else if (inUranusMode) {
                targetUCamY = clampf(targetUCamY - 3.5f, -8.0f, 74.0f);
            }
            else {
                targetNCamY = clampf(targetNCamY - 4.0f, 1.8f, 70.0f);
            }
            glutPostRedisplay();
            return;
        }
        if (button == 4 && state == GLUT_DOWN) {
            if (inMercuryMode || inTerrainMode || inVenusTerrainMode) {
                float minAlt = inMercuryMode ? 1.8f : 2.0f;
                float maxAlt = inMercuryMode ? 90.0f : 75.0f;
                targetTCamY = clampf(targetTCamY * 1.14f, minAlt, maxAlt);
            }
            else if (inMarsMode) {
                targetMarsCamY = clampf(targetMarsCamY + 20.0f, 4.0f, 350.0f);
            }
            else if (inJupiterMode) {
                targetJCamY = clampf(targetJCamY + 4.0f, -8.0f, 60.0f);
            }
            else if (inSaturnMode) {
                targetSCamY = clampf(targetSCamY + 3.5f, -10.0f, 72.0f);
            }
            else if (inUranusMode) {
                targetUCamY = clampf(targetUCamY + 3.5f, -8.0f, 74.0f);
            }
            else {
                targetNCamY = clampf(targetNCamY + 4.0f, 1.8f, 70.0f);
            }
            glutPostRedisplay();
            return;
        }

        if (button == GLUT_LEFT_BUTTON) leftMouseDown = (state == GLUT_DOWN);
        if (button == GLUT_RIGHT_BUTTON) rightMouseDown = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
        return;
    }

    if (isPlanetSelectedView) {
        if (button == 3 && state == GLUT_DOWN) {
            selectedViewCamDistance -= 0.8f;
            if (selectedViewCamDistance < 3) selectedViewCamDistance = 3;
        }
        if (button == 4 && state == GLUT_DOWN) {
            selectedViewCamDistance += 0.8f;
            if (selectedViewCamDistance > 30) selectedViewCamDistance = 30;
        }
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            leftMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
            leftMouseDown = false;
        }
        glutPostRedisplay();
        return;
    }

    if (cameraMode != CAMERA_EXPLORE)
        checkPlanetHover(x, y);

    int clickedPlanet = (cameraMode == CAMERA_EXPLORE) ? focusedPlanetIndex : hoveredPlanetIndex;

    if (button == 3 && state == GLUT_DOWN) {
        if (cameraMode == CAMERA_EXPLORE && focusedPlanetIndex != -1) {
            float mn = getExploreMinDistance(focusedPlanetIndex);
            float mx = getExploreMaxDistance(focusedPlanetIndex);
            targetExploreDistance = clampf(targetExploreDistance - 0.05f, mn, mx);
            targetFOV = clampf(targetFOV - 0.5f, 24.0f, 40.0f);
        }
        else if (focusedPlanetIndex != -1) {
            float mn = getPlanetMinDistance(focusedPlanetIndex);
            float mx = getPlanetMaxDistance(focusedPlanetIndex);
            float step = (cameraMode == CAMERA_INSPECT ? 0.45f : 1.0f);
            targetCamDistance = clampf(targetCamDistance - step, mn, mx);
            targetFOV = clampf(targetFOV - 1.0f, 20.0f, 70.0f);
        }
        else {
            targetCamDistance = clampf(targetCamDistance - 2.0f, 4.0f, 160.0f);
            targetFOV = clampf(targetFOV - 1.0f, 20.0f, 70.0f);
        }
        glutPostRedisplay();
        return;
    }
    else if (button == 4 && state == GLUT_DOWN) {
        if (cameraMode == CAMERA_EXPLORE && focusedPlanetIndex != -1) {
            float mn = getExploreMinDistance(focusedPlanetIndex);
            float mx = getExploreMaxDistance(focusedPlanetIndex);
            targetExploreDistance = clampf(targetExploreDistance + 0.05f, mn, mx);
            targetFOV = clampf(targetFOV + 0.5f, 24.0f, 40.0f);
        }
        else if (focusedPlanetIndex != -1) {
            float mn = getPlanetMinDistance(focusedPlanetIndex);
            float mx = getPlanetMaxDistance(focusedPlanetIndex);
            float step = (cameraMode == CAMERA_INSPECT ? 0.45f : 1.0f);
            targetCamDistance = clampf(targetCamDistance + step, mn, mx);
            targetFOV = clampf(targetFOV + 1.0f, 20.0f, 70.0f);
        }
        else {
            targetCamDistance = clampf(targetCamDistance + 2.0f, 4.0f, 160.0f);
            targetFOV = clampf(targetFOV + 1.0f, 20.0f, 70.0f);
        }
        glutPostRedisplay();
        return;
    }

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            int now = glutGet(GLUT_ELAPSED_TIME);
            int dx = abs(x - lastClickX);
            int dy = abs(y - lastClickY);
            int dt = now - lastClickTime;

            bool isDoubleClick =
                (dt <= DOUBLE_CLICK_MS) &&
                (dx < 8) && (dy < 8) &&
                (lastClickPlanetIndex == clickedPlanet);

            if (isDoubleClick) {
                handleDoubleClickAction(clickedPlanet);
                lastClickTime = 0;
                lastClickPlanetIndex = -999;
            }
            else {
                lastClickTime = now;
                lastClickPlanetIndex = clickedPlanet;
                lastClickX = x;
                lastClickY = y;
            }

            leftMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else if (state == GLUT_UP) {
            leftMouseDown = false;
        }
    }
    else if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            rightMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else if (state == GLUT_UP) {
            rightMouseDown = false;
        }
    }
    else if (button == GLUT_MIDDLE_BUTTON) {
        if (state == GLUT_DOWN) {
            middleMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else if (state == GLUT_UP) {
            middleMouseDown = false;
        }
    }

    glutPostRedisplay();
}

void motion(int x, int y) {
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    if (fpsMouseActive && (inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) && !transActive) {
        int cx = glutGet(GLUT_WINDOW_WIDTH) / 2;
        int cy = glutGet(GLUT_WINDOW_HEIGHT) / 2;

        if (fpsFirstFrame) {
            fpsFirstFrame = false;
            glutWarpPointer(cx, cy);
            lastMouseX = cx;
            lastMouseY = cy;
            return;
        }

        int fdx = x - cx;
        int fdy = y - cy;

        if (fdx != 0 || fdy != 0) {
            if (inMercuryMode || inTerrainMode || inVenusTerrainMode) {
                float rawYaw = -fdx * mouseSensitivity;
                float rawPitch = -fdy * mouseSensitivity * 0.85f;
                mouseSmoothX[mouseSmoothIdx % MOUSE_SMOOTH_SAMPLES] = rawYaw;
                mouseSmoothY[mouseSmoothIdx % MOUSE_SMOOTH_SAMPLES] = rawPitch;
                mouseSmoothIdx++;
                float avgYaw = 0.0f, avgPitch = 0.0f;
                for (int i = 0; i < MOUSE_SMOOTH_SAMPLES; i++) {
                    avgYaw += mouseSmoothX[i];
                    avgPitch += mouseSmoothY[i];
                }
                avgYaw /= MOUSE_SMOOTH_SAMPLES;
                avgPitch /= MOUSE_SMOOTH_SAMPLES;
                targetTCamYaw += avgYaw;
                targetTCamPitch += avgPitch;
                targetTCamPitch = clampf(targetTCamPitch, -85.0f, 85.0f);
            }
            else if (inMarsMode) {
                targetMarsCamYaw += fdx * 0.14f;
                targetMarsCamPitch -= fdy * 0.10f;
                targetMarsCamPitch = clampf(targetMarsCamPitch, -85.0f, 85.0f);
            }
            else if (inJupiterMode) {
                targetJCamYaw += fdx * 0.13f;
                targetJCamPitch -= fdy * 0.095f;
                targetJCamPitch = clampf(targetJCamPitch, -82.0f, 82.0f);
            }
            else if (inSaturnMode) {
                targetSCamYaw += fdx * 0.105f;
                targetSCamPitch -= fdy * 0.078f;
                targetSCamPitch = clampf(targetSCamPitch, -75.0f, 72.0f);
            }
            else if (inUranusMode) {
                targetUCamYaw += fdx * 0.095f;
                targetUCamPitch -= fdy * 0.070f;
                targetUCamPitch = clampf(targetUCamPitch, -78.0f, 76.0f);
            }
            else {
                targetNCamYaw += fdx * 0.14f;
                targetNCamPitch -= fdy * 0.10f;
                targetNCamPitch = clampf(targetNCamPitch, -85.0f, 85.0f);
            }
            glutWarpPointer(cx, cy);
        }
        return;
    }

    if (isPlanetSelectedView) {
        if (leftMouseDown) {
            selectedViewCamAngleY += dx * 0.5f;
            selectedViewCamAngleX += dy * 0.35f;
            selectedViewCamAngleX = clampf(selectedViewCamAngleX, -89.0f, 89.0f);
        }
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
        return;
    }

    if (cameraMode == CAMERA_EXPLORE && focusedPlanetIndex != -1) {
        if (leftMouseDown) {
            targetExploreLookYaw += dx * 0.45f;
            targetExploreLookPitch += dy * 0.28f;
            targetExploreLookPitch = clampf(targetExploreLookPitch, -89.0f, 89.0f);
        }

        if (rightMouseDown || middleMouseDown) {
            float headingRad = targetExploreLookYaw * PI / 180.0f;
            float moveScale = 0.18f;

            float moveForward = -dy * moveScale;
            float moveSide = -dx * moveScale;

            targetExploreAnchorLat += moveForward * cosf(headingRad) + moveSide * sinf(headingRad) * 0.55f;
            targetExploreAnchorLon += moveForward * sinf(headingRad) - moveSide * cosf(headingRad);

            targetExploreAnchorLat = clampf(targetExploreAnchorLat, -85.0f, 85.0f);
            targetExploreAnchorLon = wrapAngle360(targetExploreAnchorLon);
        }

        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
        return;
    }

    if (leftMouseDown) {
        float orbitSpeed = (cameraMode == CAMERA_INSPECT) ? 0.28f : 0.35f;
        targetCamYaw += dx * orbitSpeed;
        targetCamPitch += dy * orbitSpeed * 0.75f;
        targetCamPitch = clampf(targetCamPitch, -85.0f, 85.0f);
    }

    if (rightMouseDown || middleMouseDown) {
        float yawRad = targetCamYaw * PI / 180.0f;
        float pitchRad = targetCamPitch * PI / 180.0f;

        float rightX = cosf(yawRad);
        float rightY = 0.0f;
        float rightZ = -sinf(yawRad);

        float upX = -sinf(pitchRad) * sinf(yawRad);
        float upY = cosf(pitchRad);
        float upZ = -sinf(pitchRad) * cosf(yawRad);

        if (focusedPlanetIndex != -1) {
            float panSpeed = (cameraMode == CAMERA_INSPECT ? 0.0045f : 0.0075f) * camDistance;

            targetFocusOffsetX += (-dx * panSpeed * rightX) + (dy * panSpeed * upX);
            targetFocusOffsetY += (-dx * panSpeed * rightY) + (dy * panSpeed * upY);
            targetFocusOffsetZ += (-dx * panSpeed * rightZ) + (dy * panSpeed * upZ);

            float lim = getFocusOffsetLimit(focusedPlanetIndex);
            targetFocusOffsetX = clampf(targetFocusOffsetX, -lim, lim);
            targetFocusOffsetY = clampf(targetFocusOffsetY, -lim, lim);
            targetFocusOffsetZ = clampf(targetFocusOffsetZ, -lim, lim);
        }
        else {
            float panSpeed = 0.025f * camDistance;
            targetLookX += (-dx * panSpeed * rightX) + (dy * panSpeed * upX);
            targetLookY += (-dx * panSpeed * rightY) + (dy * panSpeed * upY);
            targetLookZ += (-dx * panSpeed * rightZ) + (dy * panSpeed * upZ);
        }
    }

    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

void passiveMotion(int x, int y) {
    if (fpsMouseActive && (inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) && !transActive) {
        int cx = glutGet(GLUT_WINDOW_WIDTH) / 2;
        int cy = glutGet(GLUT_WINDOW_HEIGHT) / 2;

        if (fpsFirstFrame) {
            fpsFirstFrame = false;
            glutWarpPointer(cx, cy);
            return;
        }

        int fdx = x - cx;
        int fdy = y - cy;

        if (fdx != 0 || fdy != 0) {
            if (inMercuryMode || inTerrainMode || inVenusTerrainMode) {
                float rawYaw = -fdx * mouseSensitivity;
                float rawPitch = -fdy * mouseSensitivity * 0.85f;
                mouseSmoothX[mouseSmoothIdx % MOUSE_SMOOTH_SAMPLES] = rawYaw;
                mouseSmoothY[mouseSmoothIdx % MOUSE_SMOOTH_SAMPLES] = rawPitch;
                mouseSmoothIdx++;
                float avgYaw = 0.0f, avgPitch = 0.0f;
                for (int i = 0; i < MOUSE_SMOOTH_SAMPLES; i++) {
                    avgYaw += mouseSmoothX[i];
                    avgPitch += mouseSmoothY[i];
                }
                avgYaw /= MOUSE_SMOOTH_SAMPLES;
                avgPitch /= MOUSE_SMOOTH_SAMPLES;
                targetTCamYaw += avgYaw;
                targetTCamPitch += avgPitch;
                targetTCamPitch = clampf(targetTCamPitch, -85.0f, 85.0f);
            }
            else if (inMarsMode) {
                targetMarsCamYaw += fdx * 0.14f;
                targetMarsCamPitch -= fdy * 0.10f;
                targetMarsCamPitch = clampf(targetMarsCamPitch, -85.0f, 85.0f);
            }
            else if (inJupiterMode) {
                targetJCamYaw += fdx * 0.13f;
                targetJCamPitch -= fdy * 0.095f;
                targetJCamPitch = clampf(targetJCamPitch, -82.0f, 82.0f);
            }
            else if (inSaturnMode) {
                targetSCamYaw += fdx * 0.105f;
                targetSCamPitch -= fdy * 0.078f;
                targetSCamPitch = clampf(targetSCamPitch, -75.0f, 72.0f);
            }
            else if (inUranusMode) {
                targetUCamYaw += fdx * 0.095f;
                targetUCamPitch -= fdy * 0.070f;
                targetUCamPitch = clampf(targetUCamPitch, -78.0f, 76.0f);
            }
            else {
                targetNCamYaw += fdx * 0.14f;
                targetNCamPitch -= fdy * 0.10f;
                targetNCamPitch = clampf(targetNCamPitch, -85.0f, 85.0f);
            }
            glutWarpPointer(cx, cy);
        }
        return;
    }

    if (cameraMode != CAMERA_EXPLORE)
        checkPlanetHover(x, y);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (showStartScreen) {
        switch (key) {
        case 13: case ' ':
            showStartScreen = false;
            break;
        case 'c': case 'C':
            startCinematicTour();
            break;
        case '?':
            showControlsPanel = !showControlsPanel;
            break;
        case 'f': case 'F':
            toggleFullscreen();
            break;
        case 27:
            cleanupAllGLResources();
            exit(0);
            break;
        default:
            break;
        }
        glutPostRedisplay();
        return;
    }

    if ((inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) && !transActive) {
        switch (key) {
        case 'w': case 'W': keyW = true; return;
        case 's': case 'S': keyS = true; return;
        case 'a': case 'A': keyA = true; return;
        case 'd': case 'D': keyD = true; return;
        }
    }

    // Avoid state corruption by ignoring regular keyboard commands while a fade transition is running.
    if (transActive) {
        glutPostRedisplay();
        return;
    }

    switch (key) {
    case 27:
        if (cinematicTourActive) {
            stopCinematicTour();
            resetCinematicSpaceView(95.0f, -45.0f, 35.0f);
            break;
        }
        if ((inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) && !transActive) {
            startTransitionToSpace();
        }
        else if (isPlanetSelectedView) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            selectedViewCamDistance = 10;
            selectedViewCamAngleY = 0;
            selectedViewCamAngleX = 15;
            cameraMode = (focusedPlanetIndex != -1) ? CAMERA_FOCUS : CAMERA_FREE;
        }
        else if (cameraMode == CAMERA_EXPLORE) {
            if (focusedPlanetIndex != -1) focusPlanet(focusedPlanetIndex);
            else resetCamera();
        }
        else if (cameraMode == CAMERA_INSPECT || cameraMode == CAMERA_FOCUS) {
            resetCamera();
        }
        else {
            cleanupAllGLResources();
            exit(0);
        }
        break;

    case ' ':
        isPaused = !isPaused;
        break;

    case 'o': case 'O':
        showOrbits = !showOrbits;
        break;

    case 'l': case 'L':
        showLabels = !showLabels;
        break;

    case 't': case 'T':
        showMeteorShower = !showMeteorShower;
        break;

    case 'c': case 'C':
        if (cinematicTourActive) stopCinematicTour();
        else startCinematicTour();
        break;

    case '?':
        showControlsPanel = !showControlsPanel;
        break;



    case '+': case '=':
        timeSpeed *= 1.2f;
        break;

    case '-': case '_':
        timeSpeed /= 1.2f;
        if (timeSpeed < 0.2f) timeSpeed = 0.2f;
        break;

    case 'r': case 'R':
        if (inTerrainMode) {
            earthNightMode = !earthNightMode;
        }
        else if ((inMercuryMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) && !transActive) {
            startTransitionToSpace();
        }
        else {
            resetCamera();
        }
        break;

    case 'f': case 'F':
        toggleFullscreen();
        break;

    case 'i': case 'I':
        if (!launchImpactAsteroid && !inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode) {
            impactStartX = planets[2].curX + 15.0f;
            impactStartY = planets[2].curY + 0.5f;
            impactStartZ = planets[2].curZ;

            impactAsteroidX = impactStartX;
            impactAsteroidY = impactStartY;
            impactAsteroidZ = impactStartZ;
            impactProgress = 0;

            launchImpactAsteroid = true;
            asteroidHitEarth = false;
            showImpactExplosion = false;
        }
        break;


    case 'v': case 'V':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(1);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToVenusTerrain();
        }
        else if (inVenusTerrainMode && !transActive) {
            startTransitionToSpace();
        }
        break;

    case 'e': case 'E':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(2);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToTerrain();
        }
        break;

    case 'm': case 'M':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(3);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToMars();
        }
        else if (inMarsMode && !transActive) {
            startTransitionToSpace();
        }
        break;

    case 'n': case 'N':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(7);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToNeptune();
        }
        else if (inNeptuneMode && !transActive) {
            startTransitionToSpace();
        }
        break;

    case 'h': case 'H':
        if (inMercuryMode) showMercuryHUD = !showMercuryHUD;
        else if (inVenusTerrainMode) showVenusTerrainHUD = !showVenusTerrainHUD;
        else if (inJupiterMode) showJupiterHUD = !showJupiterHUD;
        else if (inSaturnMode) showSaturnHUD = !showSaturnHUD;
        else if (inUranusMode) showUranusHUD = !showUranusHUD;
        else if (inNeptuneMode) showNeptuneHUD = !showNeptuneHUD;
        else if (inTerrainMode || inMarsMode) showTerrainHUD = !showTerrainHUD;
        else showHUD = !showHUD;
        break;

    case '1':
        if (inTerrainMode) weatherClouds = !weatherClouds;
        else if (inMercuryMode && !transActive) startTransitionToSpace();
        else if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(0);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToMercury();
        }
        break;

    case '2':
        if (inTerrainMode) { weatherRain = !weatherRain; if (weatherRain) weatherClouds = true; }
        else if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(1);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToVenusTerrain();
        }
        break;

    case '3':
        if (inTerrainMode) { weatherThunder = !weatherThunder; if (weatherThunder) { weatherClouds = true; weatherRain = true; } }
        else if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(2);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToTerrain();
        }
        break;

    case '4':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(3);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToMars();
        }
        break;

    case '5':
        if (inJupiterMode && !transActive) startTransitionToSpace();
        else if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(4);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToJupiter();
        }
        break;

    case '6':
        if (inSaturnMode && !transActive) startTransitionToSpace();
        else if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(5);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToSaturn();
        }
        break;

    case '7':
        if (inUranusMode && !transActive) startTransitionToSpace();
        else if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(6);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToUranus();
        }
        break;

    case '8':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && !transActive) {
            isPlanetSelectedView = false;
            selectedPlanetIndex = -1;
            isPaused = false;
            focusPlanet(7);
            cameraMode = CAMERA_EXPLORE;
            startTransitionToNeptune();
        }
        break;

    case 'p': case 'P':
        if (!inMercuryMode && !inTerrainMode && !inVenusTerrainMode && !inMarsMode && !inJupiterMode && !inSaturnMode && !inUranusMode && !inNeptuneMode && (hoveredPlanetIndex != -1 || focusedPlanetIndex != -1)) {
            selectedPlanetIndex = (hoveredPlanetIndex != -1) ? hoveredPlanetIndex : focusedPlanetIndex;
            isPlanetSelectedView = true;
            isPaused = true;
            cameraMode = CAMERA_PLANET_VIEW;
            selectedViewCamDistance = 10;
            selectedViewCamAngleY = 0;
            selectedViewCamAngleX = 15;
        }
        break;
    }

    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': case 'W': keyW = false; break;
    case 's': case 'S': keyS = false; break;
    case 'a': case 'A': keyA = false; break;
    case 'd': case 'D': keyD = false; break;
    }
}

void special(int key, int x, int y) {
    if (key == GLUT_KEY_F1) {
        showControlsPanel = !showControlsPanel;
        glutPostRedisplay();
        return;
    }

    if (inMercuryMode || inTerrainMode || inVenusTerrainMode || inMarsMode || inJupiterMode || inSaturnMode || inUranusMode || inNeptuneMode) {
        glutPostRedisplay();
        return;
    }

    if (isPlanetSelectedView) {
        switch (key) {
        case GLUT_KEY_LEFT:  selectedViewCamAngleY -= 5.0f; break;
        case GLUT_KEY_RIGHT: selectedViewCamAngleY += 5.0f; break;
        case GLUT_KEY_UP:    selectedViewCamAngleX += 5.0f; break;
        case GLUT_KEY_DOWN:  selectedViewCamAngleX -= 5.0f; break;
        }
        selectedViewCamAngleX = clampf(selectedViewCamAngleX, -89.0f, 89.0f);
        glutPostRedisplay();
        return;
    }

    if (cameraMode == CAMERA_EXPLORE) {
        switch (key) {
        case GLUT_KEY_LEFT:  targetExploreLookYaw -= 5.0f; break;
        case GLUT_KEY_RIGHT: targetExploreLookYaw += 5.0f; break;
        case GLUT_KEY_UP:    targetExploreLookPitch += 4.0f; break;
        case GLUT_KEY_DOWN:  targetExploreLookPitch -= 4.0f; break;
        }
        targetExploreLookPitch = clampf(targetExploreLookPitch, -89.0f, 89.0f);
        glutPostRedisplay();
        return;
    }

    switch (key) {
    case GLUT_KEY_LEFT:
        targetCamYaw -= 5.0f;
        break;
    case GLUT_KEY_RIGHT:
        targetCamYaw += 5.0f;
        break;
    case GLUT_KEY_UP:
        targetCamPitch += 4.0f;
        break;
    case GLUT_KEY_DOWN:
        targetCamPitch -= 4.0f;
        break;
    }

    targetCamPitch = clampf(targetCamPitch, -85.0f, 85.0f);
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h <= 0) h = 1;

    cachedWindowW = w;
    cachedWindowH = h;
    cachedAspect = (float)w / (float)h;
    projectionDirty = true;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
}

// =========================
// Main
// =========================

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Solar System ");

    init();

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passiveMotion);

    glutMainLoop();
    return 0;
}