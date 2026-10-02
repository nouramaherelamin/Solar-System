#include "Camera.h"

// ============================================================
// Static Member Definitions
// ============================================================
float Camera::camYaw = -45.0f, Camera::camPitch = 45.0f, Camera::camDistance = 90.0f;
float Camera::targetCamYaw = -45.0f, Camera::targetCamPitch = 45.0f, Camera::targetCamDistance = 90.0f;
float Camera::camTargetX = 0, Camera::camTargetY = 0, Camera::camTargetZ = 0;
float Camera::targetLookX = 0, Camera::targetLookY = 0, Camera::targetLookZ = 0;
float Camera::cameraFOV = 55.0f, Camera::targetFOV = 55.0f;
float Camera::focusOffsetX = 0, Camera::focusOffsetY = 0, Camera::focusOffsetZ = 0;
float Camera::targetFocusOffsetX = 0, Camera::targetFocusOffsetY = 0, Camera::targetFocusOffsetZ = 0;
float Camera::exploreAnchorLat = 0, Camera::exploreAnchorLon = 0;
float Camera::targetExploreAnchorLat = 0, Camera::targetExploreAnchorLon = 0;
float Camera::exploreLookYaw = 0, Camera::exploreLookPitch = -10.0f;
float Camera::targetExploreLookYaw = 0, Camera::targetExploreLookPitch = -10.0f;
float Camera::exploreDistance = 0.35f, Camera::targetExploreDistance = 0.35f;
bool  Camera::isPlanetSelectedView = false;
float Camera::selectedViewCamDistance = 10.0f;
float Camera::selectedViewCamAngleY = 0, Camera::selectedViewCamAngleX = 15.0f;
int   Camera::selectedPlanetIndex = -1;
int   Camera::lastMouseX = 0, Camera::lastMouseY = 0;
bool  Camera::leftMouseDown = false, Camera::rightMouseDown = false, Camera::middleMouseDown = false;
int   Camera::lastClickTime = 0, Camera::lastClickPlanetIndex = -2, Camera::lastClickX = 0, Camera::lastClickY = 0;
int   Camera::hoveredPlanetIndex = -1, Camera::focusedPlanetIndex = -1;
bool  Camera::isFullscreen = false;
CameraMode Camera::cameraMode = CAMERA_FREE;
float Camera::tCamX = 0, Camera::tCamY = 25.0f, Camera::tCamZ = 0;
float Camera::tCamYaw = 0, Camera::tCamPitch = -20.0f;
float Camera::targetTCamX = 0, Camera::targetTCamY = 25.0f, Camera::targetTCamZ = 0;
float Camera::targetTCamYaw = 0, Camera::targetTCamPitch = -20.0f;
float Camera::tCamVelX = 0, Camera::tCamVelZ = 0, Camera::tCamVelY = 0;
float Camera::marsCamX = 0, Camera::marsCamY = 8.0f, Camera::marsCamZ = 0;
float Camera::marsCamYaw = 0, Camera::marsCamPitch = -8.0f;
float Camera::targetMarsCamX = 0, Camera::targetMarsCamY = 8.0f, Camera::targetMarsCamZ = 0;
float Camera::targetMarsCamYaw = 0, Camera::targetMarsCamPitch = -8.0f;
float Camera::marsCamShakeX = 0, Camera::marsCamShakeY = 0;
float Camera::marsMoveSpeed = 0;
float Camera::marsLightning = 0, Camera::marsLightningTimer = 0;
float Camera::nCamX = 0, Camera::nCamY = 3.0f, Camera::nCamZ = 0;
float Camera::nCamYaw = 0, Camera::nCamPitch = -5.0f;
float Camera::targetNCamX = 0, Camera::targetNCamY = 3.0f, Camera::targetNCamZ = 0;
float Camera::targetNCamYaw = 0, Camera::targetNCamPitch = -5.0f;

// ============================================================
// Helpers
// ============================================================
void Camera::clearFocusOffsets() {
    focusOffsetX = targetFocusOffsetX = 0;
    focusOffsetY = targetFocusOffsetY = 0;
    focusOffsetZ = targetFocusOffsetZ = 0;
}
void Camera::clearExploreState() {
    exploreAnchorLat = targetExploreAnchorLat = 0;
    exploreAnchorLon = targetExploreAnchorLon = 0;
    exploreLookYaw = targetExploreLookYaw = 0;
    exploreLookPitch = targetExploreLookPitch = -10.0f;
    exploreDistance = targetExploreDistance = 0.35f;
}

float Camera::getPlanetMinDistance(int idx) {
    if (idx<0||idx>=(int)planets.size()) return 4.0f;
    return planets[idx].radius * 3.0f + 1.2f;
}
float Camera::getPlanetMaxDistance(int idx) {
    if (idx<0||idx>=(int)planets.size()) return 160.0f;
    return planets[idx].radius * 22.0f + 20.0f;
}
float Camera::getFocusOffsetLimit(int idx) {
    if (idx<0||idx>=(int)planets.size()) return 2.0f;
    float base = planets[idx].radius * 2.6f;
    if (cameraMode == CAMERA_INSPECT) base = planets[idx].radius * 1.5f;
    return clampf(base, 0.8f, 8.0f);
}
float Camera::getExploreMinDistance(int idx) {
    if (idx<0||idx>=(int)planets.size()) return 0.2f;
    return clampf(planets[idx].radius*0.08f+0.05f, 0.08f, 0.45f);
}
float Camera::getExploreMaxDistance(int idx) {
    if (idx<0||idx>=(int)planets.size()) return 2.0f;
    return clampf(planets[idx].radius*1.8f, 0.7f, 4.0f);
}

// ============================================================
// reset / focus / inspect / explore
// ============================================================
void Camera::reset() {
    camYaw=targetCamYaw=-45.0f; camPitch=targetCamPitch=45.0f; camDistance=targetCamDistance=90.0f;
    camTargetX=targetLookX=0; camTargetY=targetLookY=0; camTargetZ=targetLookZ=0;
    cameraFOV=targetFOV=55.0f;
    focusedPlanetIndex=-1; cameraMode=CAMERA_FREE;
    clearFocusOffsets(); clearExploreState();
}

void Camera::focusPlanet(int index) {
    if (index<0||index>=(int)planets.size()) return;
    bool same = (focusedPlanetIndex==index);
    focusedPlanetIndex=index; cameraMode=CAMERA_FOCUS;
    if (!same) { clearFocusOffsets(); clearExploreState(); }
    targetLookX=planets[index].curX; targetLookY=planets[index].curY; targetLookZ=planets[index].curZ;
    if (!same) {
        if (index==2) { targetCamDistance=9.0f; targetFOV=28.0f; targetCamYaw=-25.0f; targetCamPitch=25.0f; }
        else if (index==4||index==5) { targetCamDistance=18.0f; targetFOV=32.0f; targetCamYaw=-35.0f; targetCamPitch=25.0f; }
        else { targetCamDistance=12.0f; targetFOV=30.0f; targetCamYaw=-35.0f; targetCamPitch=22.0f; }
    }
}

void Camera::inspectPlanet(int index) {
    if (index<0||index>=(int)planets.size()) return;
    bool same=(focusedPlanetIndex==index);
    focusedPlanetIndex=index; cameraMode=CAMERA_INSPECT;
    if (!same) { clearFocusOffsets(); clearExploreState(); }
    targetLookX=planets[index].curX; targetLookY=planets[index].curY; targetLookZ=planets[index].curZ;
    targetCamDistance=clampf(getPlanetMinDistance(index)+0.8f,getPlanetMinDistance(index),getPlanetMaxDistance(index));
    targetFOV=22.0f; targetCamYaw=-20.0f; targetCamPitch=15.0f;
}

// Forward declarations for transition starters (defined in Earth/Mars/Venus/Neptune)
extern void startTransitionToTerrain();
extern void startTransitionToMars();
extern void startTransitionToVenusTerrain();
extern void startTransitionToNeptune();

void Camera::enterExploreMode(int index) {
    if (index<0||index>=(int)planets.size()) return;
    if (index==1) { focusedPlanetIndex=index; cameraMode=CAMERA_EXPLORE; startTransitionToVenusTerrain(); return; }
    if (index==2) { focusedPlanetIndex=index; cameraMode=CAMERA_EXPLORE; startTransitionToTerrain();      return; }
    if (index==3) { focusedPlanetIndex=index; cameraMode=CAMERA_EXPLORE; startTransitionToMars();         return; }
    if (index==7) { focusedPlanetIndex=index; cameraMode=CAMERA_EXPLORE; startTransitionToNeptune();      return; }
    focusedPlanetIndex=index; cameraMode=CAMERA_EXPLORE; clearFocusOffsets();
    targetExploreAnchorLat=exploreAnchorLat=0; targetExploreAnchorLon=exploreAnchorLon=0;
    targetExploreLookYaw=exploreLookYaw=0; targetExploreLookPitch=exploreLookPitch=-8.0f;
    float sd=clampf(planets[index].radius*0.25f,getExploreMinDistance(index),getExploreMaxDistance(index));
    targetExploreDistance=exploreDistance=sd; targetFOV=28.0f;
}

void Camera::toggleFocusExploreInspect(int index) {
    if (index<0||index>=(int)planets.size()) return;
    if (focusedPlanetIndex!=index) { focusPlanet(index); return; }
    if (cameraMode==CAMERA_FOCUS)   { enterExploreMode(index); }
    else if (cameraMode==CAMERA_EXPLORE) { inspectPlanet(index); }
    else if (cameraMode==CAMERA_INSPECT) { focusPlanet(index); }
    else { focusPlanet(index); }
}

void Camera::toggleFullscreen() {
    if (!isFullscreen) { glutFullScreen(); isFullscreen=true; }
    else { glutReshapeWindow(1280,720); glutPositionWindow(100,100); isFullscreen=false; }
}

// ============================================================
// update — smooth space camera
// ============================================================
void Camera::update() {
    if (cameraMode==CAMERA_EXPLORE && focusedPlanetIndex!=-1 && focusedPlanetIndex<(int)planets.size()) {
        exploreAnchorLat  += (targetExploreAnchorLat -exploreAnchorLat )*0.14f;
        exploreAnchorLon  += (targetExploreAnchorLon -exploreAnchorLon )*0.14f;
        exploreLookYaw    += (targetExploreLookYaw   -exploreLookYaw   )*0.14f;
        exploreLookPitch  += (targetExploreLookPitch -exploreLookPitch )*0.14f;
        exploreDistance   += (targetExploreDistance  -exploreDistance  )*0.14f;
        exploreAnchorLon  = wrapAngle360(exploreAnchorLon);
        targetExploreAnchorLon = wrapAngle360(targetExploreAnchorLon);
        exploreAnchorLat  = clampf(exploreAnchorLat,-85,85); targetExploreAnchorLat=clampf(targetExploreAnchorLat,-85,85);
        exploreLookPitch  = clampf(exploreLookPitch,-89,89); targetExploreLookPitch=clampf(targetExploreLookPitch,-89,89);
        float mn=getExploreMinDistance(focusedPlanetIndex), mx=getExploreMaxDistance(focusedPlanetIndex);
        exploreDistance=clampf(exploreDistance,mn,mx); targetExploreDistance=clampf(targetExploreDistance,mn,mx);
        cameraFOV+=(targetFOV-cameraFOV)*0.10f; return;
    }
    focusOffsetX+=(targetFocusOffsetX-focusOffsetX)*0.12f;
    focusOffsetY+=(targetFocusOffsetY-focusOffsetY)*0.12f;
    focusOffsetZ+=(targetFocusOffsetZ-focusOffsetZ)*0.12f;
    if (focusedPlanetIndex!=-1 && focusedPlanetIndex<(int)planets.size() && !isPlanetSelectedView) {
        float ts=(cameraMode==CAMERA_INSPECT)?0.06f:0.12f;
        float dx=planets[focusedPlanetIndex].curX+focusOffsetX, dy=planets[focusedPlanetIndex].curY+focusOffsetY, dz=planets[focusedPlanetIndex].curZ+focusOffsetZ;
        targetLookX+=(dx-targetLookX)*ts; targetLookY+=(dy-targetLookY)*ts; targetLookZ+=(dz-targetLookZ)*ts;
        float mn=getPlanetMinDistance(focusedPlanetIndex), mx=getPlanetMaxDistance(focusedPlanetIndex);
        targetCamDistance=clampf(targetCamDistance,mn,mx);
    }
    float MAX_PAN=85.0f;
    targetLookX=clampf(targetLookX,-MAX_PAN,MAX_PAN); targetLookZ=clampf(targetLookZ,-MAX_PAN,MAX_PAN); targetLookY=clampf(targetLookY,-20,20);
    float ld=sqrtf((targetLookX-camTargetX)*(targetLookX-camTargetX)+(targetLookZ-camTargetZ)*(targetLookZ-camTargetZ));
    float ps=(ld>20.0f)?0.055f:0.12f;
    camYaw+=(targetCamYaw-camYaw)*0.10f; camPitch+=(targetCamPitch-camPitch)*0.10f; camDistance+=(targetCamDistance-camDistance)*0.10f;
    camTargetX+=(targetLookX-camTargetX)*ps; camTargetY+=(targetLookY-camTargetY)*ps; camTargetZ+=(targetLookZ-camTargetZ)*ps;
    cameraFOV+=(targetFOV-cameraFOV)*0.10f;
    camPitch=clampf(camPitch,-85,85); camDistance=clampf(camDistance,2,200);
}

void Camera::apply() {
    float yr=camYaw*PI/180.0f, pr=camPitch*PI/180.0f;
    float ex=camTargetX+camDistance*cosf(pr)*sinf(yr);
    float ey=camTargetY+camDistance*sinf(pr);
    float ez=camTargetZ+camDistance*cosf(pr)*cosf(yr);
    gluLookAt(ex,ey,ez,camTargetX,camTargetY,camTargetZ,0,1,0);
}

void Camera::applyExplore() {
    if (focusedPlanetIndex<0||focusedPlanetIndex>=(int)planets.size()) { apply(); return; }
    CelestialBody& p=planets[focusedPlanetIndex];
    float latR=exploreAnchorLat*PI/180.0f, lonR=(exploreAnchorLon+currentTime*p.rotationSpeed)*PI/180.0f;
    float nx=cosf(latR)*cosf(lonR), ny=sinf(latR), nz=cosf(latR)*sinf(lonR);
    float upX=nx,upY=ny,upZ=nz;
    float eastX=-sinf(lonR),eastY=0,eastZ=cosf(lonR);
    float northX=-sinf(latR)*cosf(lonR),northY=cosf(latR),northZ=-sinf(latR)*sinf(lonR);
    float hR=exploreLookYaw*PI/180.0f, pR=exploreLookPitch*PI/180.0f;
    float fbX=sinf(hR)*eastX+cosf(hR)*northX, fbY=sinf(hR)*eastY+cosf(hR)*northY, fbZ=sinf(hR)*eastZ+cosf(hR)*northZ;
    float fwdX=cosf(pR)*fbX+sinf(pR)*upX, fwdY=cosf(pR)*fbY+sinf(pR)*upY, fwdZ=cosf(pR)*fbZ+sinf(pR)*upZ;
    float eyeX=p.curX+nx*(p.radius+exploreDistance), eyeY=p.curY+ny*(p.radius+exploreDistance), eyeZ=p.curZ+nz*(p.radius+exploreDistance);
    gluLookAt(eyeX,eyeY,eyeZ,eyeX+fwdX,eyeY+fwdY,eyeZ+fwdZ,upX,upY,upZ);
}

// ============================================================
// Terrain (Earth/Venus) camera
// ============================================================
void Camera::resetTerrainCamera() {
    tCamX=targetTCamX=0; tCamZ=targetTCamZ=0; tCamY=targetTCamY=25.0f;
    tCamYaw=targetTCamYaw=0; tCamPitch=targetTCamPitch=-15.0f;
    keyW=keyA=keyS=keyD=false; terrainMoveSpeed=0; tCamVelX=tCamVelZ=tCamVelY=0;
    for (int i=0;i<MOUSE_SMOOTH_SAMPLES;i++) { mouseSmoothX[i]=0; mouseSmoothY[i]=0; }
    mouseSmoothIdx=0;
}

// Forward declare height samplers (defined in Earth/Venus)
extern float sampleTerrainHeightV2(float wx, float wz);
extern float sampleVenusTerrainHeight(float wx, float wz);
// Forward declare streaming updaters
extern void updateEarthTerrainStreaming();
extern void updateVenusTerrainStreaming();

void Camera::updateTerrainCamera(bool isVenus) {
    float moveX=0,moveZ=0;
    if (keyW||keyA||keyS||keyD) {
        float speed=45.0f*timeSpeed*deltaTime;
        float yr=tCamYaw*DEG2RAD;
        float fx=sinf(yr),fz=cosf(yr),rx=cosf(yr),rz=-sinf(yr);
        if (keyW){moveX+=fx*speed;moveZ+=fz*speed;} if (keyS){moveX-=fx*speed;moveZ-=fz*speed;}
        if (keyA){moveX-=rx*speed;moveZ-=rz*speed;} if (keyD){moveX+=rx*speed;moveZ+=rz*speed;}
        targetTCamX+=moveX; targetTCamZ+=moveZ;
    }
    float isp=(deltaTime>0.0001f)?sqrtf(moveX*moveX+moveZ*moveZ)/deltaTime:0.0f;
    terrainMoveSpeed+=(isp-terrainMoveSpeed)*(1.0f-expf(-8.0f*deltaTime));
    tCamVelX=(targetTCamX-tCamX)*6.0f; tCamVelZ=(targetTCamZ-tCamZ)*6.0f;
    float ps=1.0f-expf(-9.0f*deltaTime), ls=1.0f-expf(-12.0f*deltaTime);
    tCamX+=(targetTCamX-tCamX)*ps; tCamY+=(targetTCamY-tCamY)*ps; tCamZ+=(targetTCamZ-tCamZ)*ps;
    tCamYaw+=(targetTCamYaw-tCamYaw)*ls; tCamPitch+=(targetTCamPitch-tCamPitch)*ls;
    targetTCamPitch=clampf(targetTCamPitch,-85,85); tCamPitch=clampf(tCamPitch,-85,85);
    if (inTerrainMode) updateEarthTerrainStreaming();
    else if (inVenusTerrainMode) updateVenusTerrainStreaming();
    float groundH = isVenus ? sampleVenusTerrainHeight(tCamX,tCamZ) : sampleTerrainHeightV2(tCamX,tCamZ);
    float minH=groundH+(isVenus?1.6f:2.0f); if(minH<2.0f)minH=2.0f;
    if(tCamY<minH)tCamY=minH; if(targetTCamY<minH)targetTCamY=minH;
    targetTCamY=clampf(targetTCamY,2.0f,75.0f);
}

// Forward declare Mars height sampler
extern float sampleMarsTerrainHeight(float wx, float wz);
extern void  updateMarsTerrainStreaming();

void Camera::updateMarsCamera() {
    float moveX=0,moveZ=0,moveY=0;
    if(keyW||keyA||keyS||keyD){
        float speed=55.0f*timeSpeed*deltaTime;
        float yr=marsCamYaw*DEG2RAD,pr=marsCamPitch*DEG2RAD;
        float fx=sinf(yr)*cosf(pr),fy=sinf(pr),fz=cosf(yr)*cosf(pr),rx=cosf(yr),rz=-sinf(yr);
        if(keyW){moveX+=fx*speed;moveY+=fy*speed;moveZ+=fz*speed;} if(keyS){moveX-=fx*speed;moveY-=fy*speed;moveZ-=fz*speed;}
        if(keyA){moveX-=rx*speed;moveZ-=rz*speed;} if(keyD){moveX+=rx*speed;moveZ+=rz*speed;}
        targetMarsCamX+=moveX; targetMarsCamY+=moveY; targetMarsCamZ+=moveZ;
    }
    float isp=(deltaTime>0.0001f)?sqrtf(moveX*moveX+moveZ*moveZ+moveY*moveY)/deltaTime:0.0f;
    float ss=1.0f-expf(-8.0f*deltaTime),ps=1.0f-expf(-9.0f*deltaTime),ls=1.0f-expf(-12.0f*deltaTime);
    marsMoveSpeed+=(isp-marsMoveSpeed)*ss;
    marsCamX+=(targetMarsCamX-marsCamX)*ps; marsCamY+=(targetMarsCamY-marsCamY)*ps; marsCamZ+=(targetMarsCamZ-marsCamZ)*ps;
    marsCamYaw+=(targetMarsCamYaw-marsCamYaw)*ls; marsCamPitch+=(targetMarsCamPitch-marsCamPitch)*ls;
    targetMarsCamPitch=clampf(targetMarsCamPitch,-85,85); marsCamPitch=clampf(marsCamPitch,-85,85);
    float gH=sampleMarsTerrainHeight(marsCamX,marsCamZ)+2.0f;
    if(marsCamY<gH)marsCamY=gH; if(targetMarsCamY<gH)targetMarsCamY=gH;
    if(marsLightning>0.1f){
        marsCamShakeX=(noiseHash(glutGet(GLUT_ELAPSED_TIME),0)*2.0f-1.0f)*marsLightning*0.3f;
        marsCamShakeY=(noiseHash(0,glutGet(GLUT_ELAPSED_TIME))*2.0f-1.0f)*marsLightning*0.3f;
    } else { marsCamShakeX=marsCamShakeY=0; }
    updateMarsTerrainStreaming();
}

// Forward declare Neptune height
extern float getNeptuneTerrainHeight(float wx, float wz);
static float nepWrapCam(float v,float half){float r=half*2.0f;v=fmodf(v+half,r);if(v<0)v+=r;return v-half;}

void Camera::resetNeptuneCamera() {
    nCamX=targetNCamX=0; nCamZ=targetNCamZ=0; nCamY=targetNCamY=3.2f;
    nCamYaw=targetNCamYaw=0; nCamPitch=targetNCamPitch=-5.0f;
    keyW=keyA=keyS=keyD=false; terrainMoveSpeed=0;
}

void Camera::updateNeptuneCamera() {
    float moveX=0,moveZ=0,speed=38.0f*timeSpeed*deltaTime;
    if(keyW||keyA||keyS||keyD){
        float yr=nCamYaw*DEG2RAD,fx=sinf(yr),fz=cosf(yr),rx=cosf(yr),rz=-sinf(yr);
        if(keyW){moveX+=fx*speed;moveZ+=fz*speed;} if(keyS){moveX-=fx*speed;moveZ-=fz*speed;}
        if(keyA){moveX-=rx*speed;moveZ-=rz*speed;} if(keyD){moveX+=rx*speed;moveZ+=rz*speed;}
        targetNCamX+=moveX; targetNCamZ+=moveZ;
    }
    float isp=(deltaTime>0.0001f)?sqrtf(moveX*moveX+moveZ*moveZ)/deltaTime:0.0f;
    float ss=1.0f-expf(-8.0f*deltaTime),ps=1.0f-expf(-9.0f*deltaTime),hs=1.0f-expf(-7.0f*deltaTime),ls=1.0f-expf(-10.0f*deltaTime);
    terrainMoveSpeed+=(isp-terrainMoveSpeed)*ss;
    nCamX+=(targetNCamX-nCamX)*ps; nCamZ+=(targetNCamZ-nCamZ)*ps; nCamY+=(targetNCamY-nCamY)*hs;
    nCamYaw+=(targetNCamYaw-nCamYaw)*ls; nCamPitch+=(targetNCamPitch-nCamPitch)*ls;
    targetNCamPitch=clampf(targetNCamPitch,-85,85); nCamPitch=clampf(nCamPitch,-85,85);
    float gH=getNeptuneTerrainHeight(nepWrapCam(nCamX,100.0f),nepWrapCam(nCamZ,100.0f))+1.8f;
    if(gH<1.8f)gH=1.8f;
    if(nCamY<gH)nCamY=gH; if(targetNCamY<gH)targetNCamY=gH;
    targetNCamY=clampf(targetNCamY,gH,70.0f);
}
