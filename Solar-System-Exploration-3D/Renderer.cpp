#include "Renderer.h"
#include "TextureManager.h"
#include "Camera.h"
#include "Earth.h"
#include "Mars.h"
#include "Venus.h"
#include "Neptune.h"

// Renderer global state
GLuint Renderer::orbitsList = 0;
GLuint Renderer::coneList   = 0;
GLuint Renderer::sphereList = 0;
bool   Renderer::showMeteorShower = true;
// Meteor state
static GLUquadric* meteorQuadric = nullptr;
static std::vector<Meteor> meteors;
static int meteorCounter = 0;
static const int METEOR_COUNT = 5;
static const float METEOR_RESPAWN_RADIUS = 320.0f;
// Impact/explosion state
float satelliteOrbitSpeed = 8.0f;
float satelliteDistance   = 2.5f;
float satelliteSize       = 0.15f;
bool  launchImpactAsteroid = false;
bool  asteroidHitEarth     = false;
bool  earthHasCrater       = false;
float impactAsteroidX=0,impactAsteroidY=0,impactAsteroidZ=0;
float impactStartX=0,impactStartY=0,impactStartZ=0;
float impactProgress=0,impactSpeed=0.014f;
bool  showImpactExplosion=false;
float explosionProgress=0,explosionDuration=0.55f;
float explosionX=0,explosionY=0,explosionZ=0;
float craterFlash=0;
const float IMPACT_ASTEROID_RADIUS=0.32f;


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
    if (inTerrainMode) return "Earth Terrain";
    if (inVenusTerrainMode) return "Venus Terrain";
    if (inMarsMode) return "Mars Terrain";
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
    drawString(GLUT_BITMAP_HELVETICA_12, x + 115.0f, y + 35.0f, "Earth / Venus / Mars / Neptune terrains + meteor shower + asteroid impact");

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
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "E  Earth terrain        V  Venus terrain"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "M  Mars terrain         N  Neptune interior"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "WASD Move              Mouse Look"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "ESC Back / Exit         F Fullscreen"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "T Meteor Shower         I Impact Asteroid"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "P Planet View           O Orbits / L Labels"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "1 Clouds  2 Rain  3 Thunder on Earth"); yy -= 20.0f;
    drawString(GLUT_BITMAP_HELVETICA_12, x + 18.0f, yy, "C Cinematic Tour        ? Toggle this panel");

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
    if (inTerrainMode || inVenusTerrainMode) {
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
