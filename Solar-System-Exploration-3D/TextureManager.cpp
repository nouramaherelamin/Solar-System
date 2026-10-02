#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#include "TextureManager.h"

// ============================================================
// TextureManager — Static Member Definitions
// ============================================================
GLuint TextureManager::planetTextures[8] = { 0 };
GLuint TextureManager::sunTexture        = 0;
GLuint TextureManager::asteroidTexture   = 0;
GLuint TextureManager::skyboxTextures[6] = { 0 };
GLuint TextureManager::fireTexture       = 0;
GLuint TextureManager::earthNightTexture = 0;
GLuint TextureManager::earthCloudTexture = 0;
GLuint TextureManager::marsGasTexture    = 0;
bool   TextureManager::texturesLoaded    = false;
bool   TextureManager::skyboxLoaded      = false;

// ============================================================
// loadTexture
// ============================================================
GLuint TextureManager::loadTexture(const char* filename) {
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    printf("Loaded texture: %s\n", filename);
    return textureID;
}

// ============================================================
// loadPlanetTextures
// ============================================================
bool TextureManager::loadPlanetTextures() {
    bool ok = true;
    sunTexture = loadTexture("sun.jpg");
    if (sunTexture == 0) ok = false;

    const char* planetFiles[8] = {
        "mercury.jpg","venus.jpg","earth.jpg","mars.jpg",
        "jupiter.jpg","saturn.jpg","uranus.jpg","neptune.jpg"
    };
    for (int i = 0; i < 8; i++) {
        planetTextures[i] = loadTexture(planetFiles[i]);
        if (planetTextures[i] == 0) ok = false;
    }
    texturesLoaded = ok;
    if (!ok) printf("Warning: Some textures failed to load. Program will continue with fallback colors.\n");
    return ok;
}

// ============================================================
// loadSkyboxTextures
// ============================================================
bool TextureManager::loadSkyboxTextures() {
    bool ok = true;
    const char* skyboxFiles[6] = {
        "skybox1.jpg","skybox2.jpg","skybox3.jpg",
        "skybox4.jpg","skybox5.jpg","skybox6.jpg"
    };
    for (int i = 0; i < 6; i++) {
        skyboxTextures[i] = loadTexture(skyboxFiles[i]);
        if (skyboxTextures[i] == 0) ok = false;
    }
    skyboxLoaded = ok;
    if (!ok) printf("Warning: Some skybox textures failed to load.\n");
    return ok;
}

// ============================================================
// createMarsGasTexture — procedural atmosphere texture
// ============================================================
void TextureManager::createMarsGasTexture() {
    int w = 512, h = 256;
    unsigned char* data = new unsigned char[w * h * 3];

    for (int y = 0; y < h; y++) {
        float v  = (float)y / h;
        float py = (v - 0.5f) * 2.0f;
        for (int x = 0; x < w; x++) {
            float u  = (float)x / w;
            float n1 = seamlessNoise(u, v, 20.0f, 10.0f);
            float n2 = seamlessNoise(u, v, 40.0f, 20.0f);

            float bandFreq = 14.0f;
            float bandStr  = sinf(py * PI * bandFreq + n1 * 3.0f);
            bandStr = (bandStr + 1.0f) * 0.5f;

            float turb = seamlessNoise(u + n2*0.1f, v + n1*0.1f, 15.0f, 15.0f);

            float spotU  = fmodf(u + 0.3f, 1.0f);
            float spotDist = sqrtf(powf((spotU - 0.5f)*2.0f,2.0f) + powf((py-(-0.3f))*2.0f,2.0f));
            float spot = 1.0f - smoothstepf(0.0f, 0.3f, spotDist + turb*0.1f);

            float r = lerpf(0.85f, 0.95f, bandStr);
            float g = lerpf(0.40f, 0.65f, bandStr);
            float b = lerpf(0.20f, 0.35f, bandStr);
            r -= turb*0.15f; g -= turb*0.20f; b -= turb*0.15f;

            if (spot > 0) {
                float sr = lerpf(r, 0.95f, spot), sg = lerpf(g, 0.30f, spot), sb = lerpf(b, 0.10f, spot);
                float ring = smoothstepf(0.15f,0.25f,spotDist+turb*0.1f) - smoothstepf(0.25f,0.35f,spotDist+turb*0.1f);
                sr = lerpf(sr,0.9f,ring*0.5f); sg = lerpf(sg,0.7f,ring*0.5f); sb = lerpf(sb,0.4f,ring*0.5f);
                r=sr; g=sg; b=sb;
            }
            data[(y*w+x)*3+0] = (unsigned char)(clampf(r,0,1)*255);
            data[(y*w+x)*3+1] = (unsigned char)(clampf(g,0,1)*255);
            data[(y*w+x)*3+2] = (unsigned char)(clampf(b,0,1)*255);
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

// ============================================================
// cleanup — delete all OpenGL texture objects
// ============================================================
void TextureManager::cleanup() {
    if (sunTexture)      { glDeleteTextures(1, &sunTexture);      sunTexture = 0; }
    if (asteroidTexture) { glDeleteTextures(1, &asteroidTexture); asteroidTexture = 0; }
    if (marsGasTexture)  { glDeleteTextures(1, &marsGasTexture);  marsGasTexture = 0; }
    if (earthNightTexture){ glDeleteTextures(1, &earthNightTexture); earthNightTexture = 0; }
    if (earthCloudTexture){ glDeleteTextures(1, &earthCloudTexture); earthCloudTexture = 0; }
    if (fireTexture)     { glDeleteTextures(1, &fireTexture);     fireTexture = 0; }
    for (int i=0; i<8; i++) {
        if (planetTextures[i]) { glDeleteTextures(1,&planetTextures[i]); planetTextures[i]=0; }
    }
    for (int i=0; i<6; i++) {
        if (skyboxTextures[i]) { glDeleteTextures(1,&skyboxTextures[i]); skyboxTextures[i]=0; }
    }
}
