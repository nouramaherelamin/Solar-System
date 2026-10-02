#pragma once
#include "Utils.h"

enum RenderScene { SCENE_SPACE, SCENE_EARTH, SCENE_VENUS, SCENE_MARS, SCENE_NEPTUNE };

extern float satelliteOrbitSpeed, satelliteDistance, satelliteSize;
extern bool  launchImpactAsteroid, asteroidHitEarth, earthHasCrater;
extern float impactAsteroidX, impactAsteroidY, impactAsteroidZ;
extern float impactStartX, impactStartY, impactStartZ;
extern float impactProgress, impactSpeed;
extern bool  showImpactExplosion;
extern float explosionProgress, explosionDuration;
extern float explosionX, explosionY, explosionZ;
extern float craterFlash;
extern const float IMPACT_ASTEROID_RADIUS;

class Renderer {
public:
    static GLuint orbitsList, coneList, sphereList;
    static bool   showMeteorShower;

    static void init();
    static void buildPrimitiveLists();
    static void buildOrbitsList();

    static void drawSkybox(float size = 1200.0f);
    static void drawSun();
    static void drawPlanets();
    static void drawAsteroidBelt();
    static void drawMeteorShower();
    static void updateMeteors();
    static void initMeteors();
    static void cleanupMeteors();

    static void drawImpactAsteroid();
    static void drawImpactExplosion();
    static void updateImpactAsteroid();

    static void drawHUD();
    static void drawInfoPanel();
    static void drawStartScreen();
    static void drawControlsPanel();
    static void drawCompetitionOverlays();
    static void drawTransitionOverlay();
    static void drawVignette(float strength);
    static void drawGlassPanel(float x, float y, float w, float h, float a = 1.0f);

    static void startCinematicTour();
    static void stopCinematicTour();
    static void updateCinematicTour();

    static void startTransitionToTerrain();
    static void startTransitionToMars();
    static void startTransitionToVenusTerrain();
    static void startTransitionToNeptune();
    static void startTransitionToSpace();
    static void updateTransition();

    static void applyRenderSceneState(RenderScene scene);
    static void resetSpaceLighting();

    static void checkPlanetHover(int x, int y);
    static void display();
};

// Forward declares for Camera.cpp
void startTransitionToTerrain();
void startTransitionToMars();
void startTransitionToVenusTerrain();
void startTransitionToNeptune();
void startTransitionToSpace();
