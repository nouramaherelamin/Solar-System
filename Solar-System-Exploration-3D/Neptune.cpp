#include "Neptune.h"
#include "Camera.h"

// Neptune global state
bool Neptune::nepSnowInit = false;
NeptuneSnowflake Neptune::nepSnow[NEPTUNE_SNOW_COUNT];
bool Neptune::showHUDFlag = true;

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


float getNeptuneTerrainHeight(float wx, float wz) { return Neptune::getTerrainHeight(wx, wz); }
