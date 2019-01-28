#ifndef CONFIGURATION_H
#define CONFIGURATION_H

typedef struct{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} LedColor;

typedef struct Configuration{
    int led_matrix_rows;
    int led_matrix_cols;
    int led_matrix_chains;
    const char* led_matrix_pixel_mapper;
    int led_matrix_brightness;

    int fft_core;
    int main_core;

    int sample_rate;
    int chunk_power;
    int fft_update_rate;

    int min_hz;
    int max_hz;
    int min_val;
    int max_val;
    float fft_curve;
    float darker_mult;

    int wave_speed;
    int subbands_count;
    int subbands_history;
    float subbands_constant;

    int white_dot_hang;
    float white_dot_fall;
    int add_white_dot;

    int disp_change_sec;
    int starting_wave;

    int wave_types;

    LedColor fft_color_base;
    LedColor bass_color;
    LedColor clear_color;
    LedColor white_dot;
} Configuration;

Configuration config;

void config_defaults();
void parse(const char *file);

#endif
