#pragma once
#include "Utils.h"

// ============================================================
// Camera — all camera modes, mouse look, transitions
// ============================================================
enum CameraMode {
    CAMERA_FREE,
    CAMERA_FOCUS,
    CAMERA_INSPECT,
    CAMERA_EXPLORE,
    CAMERA_PLANET_VIEW
};

class Camera {
public:
    // ---- Space camera state ----
    static float camYaw, camPitch, camDistance;
    static float targetCamYaw, targetCamPitch, targetCamDistance;
    static float camTargetX, camTargetY, camTargetZ;
    static float targetLookX, targetLookY, targetLookZ;
    static float cameraFOV, targetFOV;

    // Focus pan offsets
    static float focusOffsetX, focusOffsetY, focusOffsetZ;
    static float targetFocusOffsetX, targetFocusOffsetY, targetFocusOffsetZ;

    // Explore mode (orbital surface camera)
    static float exploreAnchorLat, exploreAnchorLon;
    static float targetExploreAnchorLat, targetExploreAnchorLon;
    static float exploreLookYaw, exploreLookPitch;
    static float targetExploreLookYaw, targetExploreLookPitch;
    static float exploreDistance, targetExploreDistance;

    // Planet-selected cinematic view
    static bool  isPlanetSelectedView;
    static float selectedViewCamDistance;
    static float selectedViewCamAngleY;
    static float selectedViewCamAngleX;
    static int   selectedPlanetIndex;

    // Mouse state
    static int  lastMouseX, lastMouseY;
    static bool leftMouseDown, rightMouseDown, middleMouseDown;

    // Double-click tracking
    static int  lastClickTime, lastClickPlanetIndex, lastClickX, lastClickY;
    static const int DOUBLE_CLICK_MS = 300;

    // Planet interaction
    static int hoveredPlanetIndex;
    static int focusedPlanetIndex;

    // Fullscreen
    static bool isFullscreen;

    // Camera mode
    static CameraMode cameraMode;

    // Earth / Venus terrain camera
    static float tCamX, tCamY, tCamZ;
    static float tCamYaw, tCamPitch;
    static float targetTCamX, targetTCamY, targetTCamZ;
    static float targetTCamYaw, targetTCamPitch;
    static float tCamVelX, tCamVelZ, tCamVelY;

    // Mars camera
    static float marsCamX, marsCamY, marsCamZ;
    static float marsCamYaw, marsCamPitch;
    static float targetMarsCamX, targetMarsCamY, targetMarsCamZ;
    static float targetMarsCamYaw, targetMarsCamPitch;
    static float marsCamShakeX, marsCamShakeY;
    static float marsMoveSpeed;
    static float marsLightning, marsLightningTimer;

    // Neptune camera
    static float nCamX, nCamY, nCamZ;
    static float nCamYaw, nCamPitch;
    static float targetNCamX, targetNCamY, targetNCamZ;
    static float targetNCamYaw, targetNCamPitch;

    // ---- Methods ----
    static void reset();
    static void focusPlanet(int index);
    static void inspectPlanet(int index);
    static void enterExploreMode(int index);
    static void toggleFocusExploreInspect(int index);
    static void toggleFullscreen();

    static void update();
    static void apply();
    static void applyExplore();

    static void resetTerrainCamera();
    static void updateTerrainCamera(bool isVenus);
    static void updateMarsCamera();
    static void resetNeptuneCamera();
    static void updateNeptuneCamera();

    // Planet distance helpers
    static float getPlanetMinDistance(int idx);
    static float getPlanetMaxDistance(int idx);
    static float getFocusOffsetLimit(int idx);
    static float getExploreMinDistance(int idx);
    static float getExploreMaxDistance(int idx);

    static void clearFocusOffsets();
    static void clearExploreState();
};
