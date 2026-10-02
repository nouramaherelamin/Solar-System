#pragma once
#include "Utils.h"

// ============================================================
// TextureManager — loads and manages all OpenGL textures
// ============================================================
class TextureManager {
public:
    // Planet & sun textures
    static GLuint planetTextures[8];
    static GLuint sunTexture;
    static GLuint asteroidTexture;
    static GLuint skyboxTextures[6];
    static GLuint fireTexture;
    static GLuint earthNightTexture;
    static GLuint earthCloudTexture;
    static GLuint marsGasTexture;

    static bool texturesLoaded;
    static bool skyboxLoaded;

    static GLuint loadTexture(const char* filename);
    static bool   loadPlanetTextures();
    static bool   loadSkyboxTextures();
    static void   createMarsGasTexture();
    static void   cleanup();
};
