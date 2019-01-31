#include "loops.h"
#include <math.h>
#include <stdlib.h>

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

float get_max(LedColor color){
    unsigned char mx = color.r;
    if (mx < color.g)
        mx = color.g;
    if (mx < color.b)
        mx = color.b;
    return mx / 255.0;
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

    set_color_vector(out, config.fft_color_base);

    config.wave_types = LAST_WAVE;
    if (config.starting_wave >= config.wave_types){
      config.starting_wave = 0;
    }

    init_waves(out);
}

void init_waves(LedOutData *out){
    out->waves = malloc(config.wave_types * sizeof(WaveData));
    for(int i = 0; i < config.wave_types; i++) {
          out->waves[i].bass_color_to_matrix = malloc(out->matrix_width * sizeof(LedColor**));
          for (int j = 0; j < out->matrix_width; j++)
              out->waves[i].bass_color_to_matrix[j] = malloc(out->matrix_height * sizeof(LedColor*));

          if (i == RIPPLE_WAVE)
              out->waves[i].data_height = 3 * out->matrix_height / 4;
          else if (i == QUAD_WAVE || i == QUAD_WAVE_INV || i == QUAD_WAVE_MIRR)
              out->waves[i].data_height = out->matrix_height / 2;
          else
              out->waves[i].data_height = out->matrix_height;

          if (i == MIRROR_WAVE)
              center_distance(out, out->matrix_width / 2 - 0.5, 0, out->waves[i].bass_color_to_matrix);
          else if (i == QUAD_WAVE || i == QUAD_WAVE_INV || i == QUAD_WAVE_MIRR)
              center_distance(out, out->matrix_width / 2 - 0.5, out->matrix_height / 2 - 0.5, out->waves[i].bass_color_to_matrix);
          else
              center_distance(out, 0, 0, out->waves[i].bass_color_to_matrix);

          out->waves[i].col_barriers = malloc(out->waves[i].data_height * sizeof(double));
          calculate_barriers(out->waves[i].data_height, config.min_val, config.max_val, out->waves[i].col_barriers);

          out->waves[i].white_dot_height_step = 1.0f * config.white_dot_fall * (config.max_val - config.min_val) / out->waves[i].data_height;
      }

      out->waves[STD_WAVE].call_wave = std_wave;
      out->waves[RIPPLE_WAVE].call_wave = ripple_wave;
      out->waves[MIRROR_WAVE].call_wave = mirror_wave;
      out->waves[QUAD_WAVE].call_wave = quad_wave;
      out->waves[QUAD_WAVE_INV].call_wave = quad_wave_inv;
      out->waves[QUAD_WAVE_MIRR].call_wave = quad_wave_mirr;
}

void delete_waves(LedOutData *out){

    for (int i = 0; i < config.wave_types; i++){
        free(out->waves[i].col_barriers);
        for (int j = 0; j < out->matrix_width; j++)
            free(out->waves[i].bass_color_to_matrix[j]);
        free(out->waves[i].bass_color_to_matrix);
    }
    free(out->waves);
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

void led_canvas_set_pixel_dmx_wave(struct LedCanvas *canv, int x, int y,
                                   LedColor bass_clr, LedColor dmx_color){
    float mx = get_max(bass_clr);
    LedColor clr = change_brightness(dmx_color, mx);
    led_canvas_set_pixel(canv, x, y, COLOR(clr));
}

void led_canvas_set_two_pixels_dmx_wave(struct LedCanvas *canv, int x1, int x2,
                                        int y, LedColor bass_clr, LedColor dmx_color){
    float mx = get_max(bass_clr);
    LedColor clr = change_brightness(dmx_color, mx);
    led_canvas_set_two_pixels(canv, x1, x2, y, clr);
}

void std_wave(LedOutData *out){
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = 2 * x;
        int x2 = 2 * x + 1;
        for (int y = 0; y < out->waves[STD_WAVE].data_height; ++y) {
            if(out->waves[STD_WAVE].col_barriers[y-1] > out->white_dot_arr[x] &&
               out->waves[STD_WAVE].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, config.white_dot);
            } else if(out->waves[STD_WAVE].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
                } else {
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->dmx_color);
                }
            } else {
                if(config.dmx_mode == 0){
                    led_canvas_set_pixel(out->offscreen_canvas, x1, y, COLOR_PTR(out->waves[STD_WAVE].bass_color_to_matrix[x1][y]));
                    led_canvas_set_pixel(out->offscreen_canvas, x2, y, COLOR_PTR(out->waves[STD_WAVE].bass_color_to_matrix[x2][y]));
                } else {
                    led_canvas_set_pixel_dmx_wave(out->offscreen_canvas, x1, y, *out->waves[STD_WAVE].bass_color_to_matrix[x1][y], out->dmx_color);
                    led_canvas_set_pixel_dmx_wave(out->offscreen_canvas, x2, y, *out->waves[STD_WAVE].bass_color_to_matrix[x2][y], out->dmx_color);
                }
            }
        }
    }
}

void ripple_wave(LedOutData *out){
    LedColor clr1, clr2;
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = 2 * x;
        int x2 = 2 * x + 1;
        for (int y = 0; y < out->waves[RIPPLE_WAVE].data_height; ++y) {
            int y_num = ceil(1.0f * y * 4 / 3);
            int y_col = out->matrix_height - y / 3 - 1;

            if(out->waves[RIPPLE_WAVE].col_barriers[y-1] > out->white_dot_arr[x] &&
               out->waves[RIPPLE_WAVE].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, config.white_dot);
            } else if(out->waves[RIPPLE_WAVE].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y_num]);
                } else {
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->dmx_color);
                }
            } else {
                if(config.dmx_mode == 0){
                    led_canvas_set_pixel(out->offscreen_canvas, x1, y, COLOR_PTR(out->waves[RIPPLE_WAVE].bass_color_to_matrix[x1][y]));
                    led_canvas_set_pixel(out->offscreen_canvas, x2, y, COLOR_PTR(out->waves[RIPPLE_WAVE].bass_color_to_matrix[x2][y]));
                } else {
                    led_canvas_set_pixel_dmx_wave(out->offscreen_canvas, x1, y, *out->waves[RIPPLE_WAVE].bass_color_to_matrix[x1][y], out->dmx_color);
                    led_canvas_set_pixel_dmx_wave(out->offscreen_canvas, x2, y, *out->waves[RIPPLE_WAVE].bass_color_to_matrix[x2][y], out->dmx_color);
                }
            }

            if (y % 3 == 0 &&
                out->waves[RIPPLE_WAVE].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    clr1 = change_brightness(out->fft_colors[y_num], config.darker_mult);
                } else {
                    clr1 = change_brightness(out->dmx_color, config.darker_mult);
                }
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y_col, clr1);
            } else if (y % 3 == 0){
                float darker = 1.0f * y / 3 * 4 / out->matrix_height;
                clr1 = change_brightness(*out->waves[RIPPLE_WAVE].bass_color_to_matrix[x1][y_col], darker);
                clr2 = change_brightness(*out->waves[RIPPLE_WAVE].bass_color_to_matrix[x2][y_col], darker);
                if(config.dmx_mode == 0){
                    led_canvas_set_pixel(out->offscreen_canvas, x1, y_col, COLOR(clr1));
                    led_canvas_set_pixel(out->offscreen_canvas, x2, y_col, COLOR(clr2));
                } else {
                    led_canvas_set_pixel_dmx_wave(out->offscreen_canvas, x1, y_col, clr1, out->dmx_color);
                    led_canvas_set_pixel_dmx_wave(out->offscreen_canvas, x2, y_col, clr2, out->dmx_color);
                }
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
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, config.white_dot);
            } else if(out->waves[MIRROR_WAVE].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
                } else {
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->dmx_color);
                }
            } else {
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, *out->waves[MIRROR_WAVE].bass_color_to_matrix[x1][y]);
                } else {
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, y, *out->waves[MIRROR_WAVE].bass_color_to_matrix[x1][y], out->dmx_color);
                }
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
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, config.white_dot);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, config.white_dot);
            } else if(out->waves[QUAD_WAVE].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, out->fft_colors[y]);
                } else {
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->dmx_color);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, out->dmx_color);
                }
            } else {
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, *out->waves[QUAD_WAVE].bass_color_to_matrix[x1][y]);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, *out->waves[QUAD_WAVE].bass_color_to_matrix[x1][yy]);
                } else {
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, y, *out->waves[QUAD_WAVE].bass_color_to_matrix[x1][y], out->dmx_color);
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, yy, *out->waves[QUAD_WAVE].bass_color_to_matrix[x1][yy], out->dmx_color);
                }
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
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, config.white_dot);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, config.white_dot);
            } else if(out->waves[QUAD_WAVE_INV].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, out->fft_colors[y]);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, out->fft_colors[y]);
                } else {
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, out->dmx_color);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, out->dmx_color);
                }
            } else {
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy1, *out->waves[QUAD_WAVE_INV].bass_color_to_matrix[x1][yy1]);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy2, *out->waves[QUAD_WAVE_INV].bass_color_to_matrix[x1][yy2]);
                } else {
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, yy1, *out->waves[QUAD_WAVE_INV].bass_color_to_matrix[x1][yy1], out->dmx_color);
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, yy2, *out->waves[QUAD_WAVE_INV].bass_color_to_matrix[x1][yy2], out->dmx_color);
                }
            }
        }
    }
}

void quad_wave_mirr(LedOutData *out){
    for (int x = 0; x < out->data_width; ++x) {
        int x1 = out->matrix_width - 1 - x;
        int x2 = x;
        for (int y = 0; y < out->waves[QUAD_WAVE_MIRR].data_height; ++y) {
            int yy = out->matrix_height - y - 1;
            if(out->waves[QUAD_WAVE_MIRR].col_barriers[y-1] > out->white_dot_arr[x] &&
               out->waves[QUAD_WAVE_MIRR].col_barriers[y] < out->white_dot_arr[x]){
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, config.white_dot);
                led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, config.white_dot);
            } else if(out->waves[QUAD_WAVE_MIRR].col_barriers[y] < out->out_matrix[x]){
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->fft_colors[y]);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, out->fft_colors[y]);
                } else {
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, out->dmx_color);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, out->dmx_color);
                }
            } else {
                if(config.dmx_mode == 0){
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, y, *out->waves[QUAD_WAVE_MIRR].bass_color_to_matrix[x1][y]);
                    led_canvas_set_two_pixels(out->offscreen_canvas, x1, x2, yy, *out->waves[QUAD_WAVE_MIRR].bass_color_to_matrix[x1][yy]);
                } else {
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, y, *out->waves[QUAD_WAVE_MIRR].bass_color_to_matrix[x1][y], out->dmx_color);
                    led_canvas_set_two_pixels_dmx_wave(out->offscreen_canvas, x1, x2, yy, *out->waves[QUAD_WAVE_MIRR].bass_color_to_matrix[x1][yy], out->dmx_color);
                }
            }
        }
    }
}
