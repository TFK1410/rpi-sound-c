#ifndef LOOPS_H
#define LOOPS_H

#include "led-matrix-c.h"
#include "utils.h"
#include "configuration.h"

#define COLOR(x) x.r, x.g, x.b
#define COLOR_PTR(x) x->r, x->g, x->b

typedef enum {STD_WAVE, RIPPLE_WAVE, MIRROR_WAVE, QUAD_WAVE, QUAD_WAVE_INV, QUAD_WAVE_MIRR, LAST_WAVE} wave_type;
typedef struct LedOutData LedOutData;

typedef struct WaveData{
  void (*call_wave) (LedOutData *out);
  LedColor ***bass_color_to_matrix;
  double *col_barriers;

  int data_height;
  double white_dot_height_step;
} WaveData;

typedef struct LedOutData{
    struct LedCanvas *offscreen_canvas;
    struct RGBLedMatrix *matrix;

    int matrix_width;
    int matrix_height;
    int data_width;

    double *out_matrix;

    double *white_dot_arr;
    int add_white_dot;

    int max_radii;
    LedColor *bass_wave_color;

    LedColor *fft_colors;

    size_t chunk_size;

    WaveData *waves;
    wave_type my_wave_type;
} LedOutData;

void set_color_vector(LedOutData *out, LedColor fft_color_base);
double get_white_dot_step(LedOutData *out);

void center_distance(LedOutData *out, float center_x, float center_y, LedColor ***bass_dist);

void init_data(LedOutData *out);
void init_waves(LedOutData *out);
void delete_waves(LedOutData *out);
void delete_data(LedOutData *out);

void call_loop(LedOutData *out);

LedColor change_brightness(LedColor x, double brightness);

void std_wave(LedOutData *out);
void ripple_wave(LedOutData *out);
void mirror_wave(LedOutData *out);
void quad_wave(LedOutData *out);
void quad_wave_inv(LedOutData *out);
void quad_wave_mirr(LedOutData *out);

#endif
