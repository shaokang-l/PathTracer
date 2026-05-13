#pragma once
#include "material/materialFlags.hpp"
#include "utils/matrix.hpp"

#define LIGHT_SAMPLE_X 1
#define LIGHT_SAMPLE_Y 1
#define MAX_RAY_DEPTH 20
#define SPP_X 16
#define SPP_Y 16
#define GAMMA 2.2
#define WIDTH 500
#define HEIGHT 500
#define BG_COLOR gl::vec3(0.0f, 0.0f, 0.0f)
#define useBVH false
// NOT IMPLEMENTED YET: // #define USE_MAXDEPTH_MIS
#define USE_MAXDEPTH_NEE
// #define USE_MAXDEPTH_NAIVE
// #define USE_ROULETTE_NAIVE
// #define USE_MAXDEPTH_RESERVOIR
// #define USE_MAXDEPTH_VOLUME
#define OVERRIDE_LOCAL_RENDER_VAL TRUE
#define NUM_THREADS 16
#define MODE TransportMode::Importance
const int LIGHT_SAMPLE_NUM = LIGHT_SAMPLE_X * LIGHT_SAMPLE_Y;