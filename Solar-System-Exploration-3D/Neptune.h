#pragma once
#include "Utils.h"

static const int NEPTUNE_SNOW_COUNT = 1200;
static const int NEP_GRID_N = 110;

struct NeptuneSnowflake { float x,y,z,speed,drift,size; };

class Neptune {
public:
    static NeptuneSnowflake nepSnow[NEPTUNE_SNOW_COUNT];
    static bool nepSnowInit;
    static bool showHUDFlag;

    static float getTerrainHeight(float wx, float wz);
    static void  initSnow();
    static void  updateSnow();
    static void  display();
    static void  drawHUD();
};

float getNeptuneTerrainHeight(float wx, float wz);
