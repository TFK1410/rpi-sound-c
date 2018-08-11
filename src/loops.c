#include "loops.h"
#include <math.h>
#include <stdlib.h>


LedColor fft_color_base = {0xff, 0xff, 0x00};
LedColor bass_color = {0xb0, 0x60, 0xd0};
LedColor clear_color = {0x00, 0x00, 0x00};
LedColor white_dot = {0xff, 0xff, 0xff};

LedColor change_brightness(LedColor x, double brightness){
    LedColor y;
    y.r = (unsigned char)(round(x.r * brightness));
    y.g = (unsigned char)(round(x.g * brightness));
    y.b = (unsigned char)(round(x.b * brightness));
    return y;
}

void set_color_vector(LedOutData *out, LedColor fft_color_base){
    LedColor clr;
    for (int y = 0; y < out->matrix_height; ++y) {
        clr = fft_color_base;
        if (y < out->matrix_height / 2) {
            clr.g = (unsigned char)(1.0f * y / (out->matrix_height / 2) * clr.g);
        } else {
            clr.r = (unsigned char)((1.0f * (out->matrix_height - y - 1) / (out->matrix_height / 2)) * clr.r);
        }
        out->fft_colors[y] = clr;
    }
}

double get_white_dot_step(LedOutData *out){
    return out->waves[out->my_wave_type].white_dot_height_step;
}

void center_distance(LedOutData *out, float center_x, float center_y, LedColor ***bass_dist){
    int xx, yy, dist;
    for (int x = 0; x < out->matrix_width; ++x) {
        for (int y = 0; y < out->matrix_height; ++y) {
            xx = abs(x - center_x);
            yy = abs(y - center_y);
            dist = round(hypot(xx, yy));
            bass_dist[x][y] = &out->bass_wave_color[dist];
        }
    }
}

void init_data(LedOutData *out){
    led_canvas_get_size(out->offscreen_canvas, &out->matrix_width, &out->matrix_height);

    out->data_width = out->matrix_width / 2;

    out->out_matrix = calloc(out->data_width, sizeof(double));

    out->white_dot_arr = calloc(out->data_width, sizeof(double));

    out->max_radii = round(hypot(out->matrix_width, out->matrix_height));

    out->bass_wave_color = calloc(out->max_radii, sizeof(LedColor));

    out->fft_colors = malloc(out->matrix_height * sizeof(LedColor));

    set_color_vector(out, fft_color_base);

    init_waves(out);
}

void init_waves(LedOutData *out){
    for(int i = 0; i < WAVE_TYPES; i++) {
          out->waves[i].bass_color_to_matrix = malloc(out->matrix_width * sizeof(LedColor**));
          for (int j = 0; j < out->matrix_width; j++)
              out->waves[i].bass_color_to_matrix[j] = malloc(out->matrix_height * sizeof(LedColor*));

          if (i == RIPPLE_WAVE)
              out->waves[i].data_height = 3 * out->matrix_height / 2;
          else if (i == QUAD_WAVE || i == QUAD_WAVE_INV)
              out->waves[i].data_height = out->matrix_height / 2;
          else
              out->waves[i].data_height = out->matrix_height;

          if (i == MIRROR_WAVE)
              center_distance(out, out->matrix_width / 2 + 0.5, 0, out->waves[i].bass_color_to_matrix);
          else if (i == QUAD_WAVE || i == QUAD_WAVE_INV)
              center_distance(out, out->matrix_width / 2 + 0.5, out->matrix_height / 2 + 0.5, out->waves[i].bass_color_to_matrix);
          else
              center_distance(out, 0, 0, out->waves[i].bass_color_to_matrix);

          out->waves[i].col_barriers = malloc(out->waves[i].data_height * sizeof(double));
          calculate_barriers(out->waves[i].data_height, MIN_VAL, MAX_VAL, out->waves[i].col_barriers);

          out->waves[i].white_dot_height_step = 1.0f * WHITE_DOT_FALL * (MAX_VAL - MIN_VAL) / out->waves[i].data_height;
      }

      out->waves[STD_WAVE].call_wave = std_wave;
      out->waves[RIPPLE_WAVE].call_wave = ripple_wave;
      out->waves[MIRROR_WAVE].call_wave = mirror_wave;
      out->waves[QUAD_WAVE].call_wave = quad_wave;
      out->waves[QUAD_WAVE_INV].call_wave = quad_wave_inv;
}

void delete_waves(LedOutData *out){

    for (int i = 0; i < WAVE_TYPES; i++){
        free(out->waves[i].col_barriers);
        for (int j = 0; j < out->matrix_width; j++)
            free(out->waves[i].bass_color_to_matrix[j]);
        free(out->waves[i].bass_color_to_matrix);
    }
}

void delete_data(LedOutData *out){

    free(out->white_dot_arr);
    free(out->fft_colors);
    free(out->bass_wave_color);
    delete_waves(out);

    led_matrix_delete(out->matrix);
}

void call_loop(LedOutData *out){
    out->waves[out->my_wave_type].call_wave(out);
}


void led_canvas_set_two_pixels(struct LedCanvas *canv, int x1, int x2, int y,
                               LedColor clr){
    led_canvas_set_pixel(canv, x1, y, COLOR(clr));
    led_canvas_set_pixel(canv, x2, y, COLOR(clr));
}

void std_wave(LedOutData *out){
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = 2 * x;
        int x2 = 2 * x + 1;
        for (int y = 0; y < out->waves[STD_WAVE].data_height; ++y) {
            if(out->waves[STD_WAVE].col_barriers[y-1] > out->white_dot_arr[x] &&
               out->waves[STD_WAVE].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, white_dot);
            } else if(out->waves[STD_WAVE].col_barriers[y] < out->out_matrix[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
            } else {
                led_canvas_set_pixel(out->offscreen_canvas, x1, y, COLOR_PTR(out->waves[STD_WAVE].bass_color_to_matrix[x1][y]));
                led_canvas_set_pixel(out->offscreen_canvas, x2, y, COLOR_PTR(out->waves[STD_WAVE].bass_color_to_matrix[x2][y]));
            }
        }
    }
}

void ripple_wave(LedOutData *out){
    LedColor clr;
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = 2 * x;
        int x2 = 2 * x + 1;
        for (int y = 0; y < out->waves[RIPPLE_WAVE].data_height; ++y) {
            int y_num = ceil(1.0f * y * 4 / 3);
            int y_col = out->matrix_height - y / 3 - 1;

            if(out->waves[RIPPLE_WAVE].col_barriers[y-1] > out->white_dot_arr[x] &&
               out->waves[RIPPLE_WAVE].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, white_dot);
            } else if(out->waves[RIPPLE_WAVE].col_barriers[y] < out->out_matrix[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y_num]);
            } else {
                led_canvas_set_pixel(out->offscreen_canvas, x1, y, COLOR_PTR(out->waves[RIPPLE_WAVE].bass_color_to_matrix[x1][y]));
                led_canvas_set_pixel(out->offscreen_canvas, x2, y, COLOR_PTR(out->waves[RIPPLE_WAVE].bass_color_to_matrix[x2][y]));
            }

            if (y % 3 == 0 &&
                out->waves[RIPPLE_WAVE].col_barriers[y] < out->out_matrix[x]){
                clr = change_brightness(out->fft_colors[y_num], DARKER_MULT);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y_col, clr);
            } else if (y % 3 == 0){
                float darker = 1.0f * y / 3 * 4 / out->matrix_height;
                clr = change_brightness(*out->waves[RIPPLE_WAVE].bass_color_to_matrix[x1][y_col], darker);
                led_canvas_set_pixel(out->offscreen_canvas, x1, y_col, COLOR(clr));
                clr = change_brightness(*out->waves[RIPPLE_WAVE].bass_color_to_matrix[x2][y_col], darker);
                led_canvas_set_pixel(out->offscreen_canvas, x2, y_col, COLOR(clr));
            }
        }
    }
}

void mirror_wave(LedOutData *out){
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = out->matrix_width / 2 - 1 - x;
        int x2 = out->matrix_width / 2 + x;
        for (int y = 0; y < out->waves[MIRROR_WAVE].data_height; ++y) {
            if(out->waves[MIRROR_WAVE].col_barriers[y-1] > out->white_dot_arr[x] &&
                out->waves[MIRROR_WAVE].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, white_dot);
            } else if(out->waves[MIRROR_WAVE].col_barriers[y] < out->out_matrix[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
            } else {
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, *out->waves[MIRROR_WAVE].bass_color_to_matrix[x1][y]);
            }
        }
    }
}

void quad_wave(LedOutData *out){
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = out->matrix_width / 2 - 1 - x;
        int x2 = out->matrix_width / 2 + x;
        for (int y = 0; y < out->waves[QUAD_WAVE].data_height; ++y) {
            int yy = out->matrix_height - y - 1;
            if(out->waves[QUAD_WAVE].col_barriers[y-1] > out->white_dot_arr[x] &&
                out->waves[QUAD_WAVE].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, white_dot);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, white_dot);
            } else if(out->waves[QUAD_WAVE].col_barriers[y] < out->out_matrix[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, out->fft_colors[y]);
            } else {
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, *out->waves[QUAD_WAVE].bass_color_to_matrix[x1][y]);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, *out->waves[QUAD_WAVE].bass_color_to_matrix[x1][yy]);
            }
        }
    }
}

void quad_wave_inv(LedOutData *out){
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = out->matrix_width / 2 - 1 - x;
        int x2 = out->matrix_width / 2 + x;
        for (int y = 0; y < out->waves[QUAD_WAVE_INV].data_height; ++y) {
            int yy1 = out->waves[QUAD_WAVE_INV].data_height - 1 - y;
            int yy2 = out->matrix_height - yy1 - 1;
            if(out->waves[QUAD_WAVE_INV].col_barriers[y-1] > out->white_dot_arr[x] &&
                out->waves[QUAD_WAVE_INV].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, white_dot);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, white_dot);
            } else if(out->waves[QUAD_WAVE_INV].col_barriers[y] < out->out_matrix[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, out->fft_colors[y]);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, out->fft_colors[y]);
            } else {
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, *out->waves[QUAD_WAVE_INV].bass_color_to_matrix[x1][yy1]);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, *out->waves[QUAD_WAVE_INV].bass_color_to_matrix[x1][yy2]);
            }
        }
    }
}
