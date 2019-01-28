#include "configuration.h"
#include "../inih/ini.h"

#include <string.h>
#include <stdlib.h>

void config_defaults(){
    config.led_matrix_rows = 32;
    config.led_matrix_cols = 64;
    config.led_matrix_chains = 4;
    config.led_matrix_pixel_mapper = "UInv-Mapper";
    config.led_matrix_brightness = 50;

    config.fft_core = 1;
    config.main_core = 2;

    config.sample_rate = 44100;
    config.chunk_power = 13;
    config.fft_update_rate = 100;

    config.min_hz = 36;
    config.max_hz = 20000;
    config.min_val = 110;
    config.max_val = 155;
    config.fft_curve = 0.65;
    config.darker_mult = 0.4;

    config.wave_speed = 500; // in ms
    config.subbands_count = 64;
    config.subbands_history = 43;
    config.subbands_constant = 1.1;

    config.white_dot_hang = 20;
    config.white_dot_fall = 0.5;
    config.add_white_dot = 1;

    config.disp_change_sec = 0;
    config.starting_wave = 4;

    config.wave_types = 5; /* this should be a constant and unchangeable by the ini */

    config.fft_color_base = (LedColor){0xff, 0xff, 0x00};
    config.bass_color = (LedColor){0xb0, 0x60, 0xd0};
    config.clear_color = (LedColor){0x00, 0x00, 0x00};
    config.white_dot = (LedColor){0xff, 0xff, 0xff};
}

static int handler(void* user, const char* section, const char* name, const char* value){
    Configuration *pconfig = (Configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    #define COLOR_SET(c, r, g, b) c.r = r; c.g = g; c.b = b;
    if (MATCH("led_matrix", "led_matrix_rows"))
        pconfig->led_matrix_rows = atoi(value);
    else if (MATCH("led_matrix", "led_matrix_cols"))
        pconfig->led_matrix_cols = atoi(value);
    else if (MATCH("led_matrix", "led_matrix_chains"))
        pconfig->led_matrix_chains = atoi(value);
    else if (MATCH("led_matrix", "led_matrix_pixel_mapper"))
        pconfig->led_matrix_pixel_mapper = strdup(value);
    else if (MATCH("led_matrix", "led_matrix_brightness"))
        pconfig->led_matrix_brightness = atoi(value);
    else if (MATCH("cores", "fft_core"))
        pconfig->fft_core = atoi(value);
    else if (MATCH("cores", "main_core"))
        pconfig->main_core = atoi(value);
    else if (MATCH("fft_sound", "sample_rate"))
        pconfig->sample_rate = atoi(value);
    else if (MATCH("fft_sound", "chunk_power"))
        pconfig->chunk_power = atoi(value);
    else if (MATCH("fft_sound", "fft_update_rate"))
        pconfig->fft_update_rate = atoi(value);
    else if (MATCH("display", "min_hz"))
        pconfig->min_hz = atoi(value);
    else if (MATCH("display", "max_hz"))
        pconfig->max_hz = atoi(value);
    else if (MATCH("display", "min_val"))
        pconfig->min_val = atoi(value);
    else if (MATCH("display", "max_val"))
        pconfig->max_val = atoi(value);
    else if (MATCH("display", "fft_curve"))
        pconfig->fft_curve = atof(value);
    else if (MATCH("display", "darker_mult"))
        pconfig->darker_mult = atof(value);
    else if (MATCH("beats", "wave_speed"))
        pconfig->wave_speed = atoi(value);
    else if (MATCH("beats", "subbands_count"))
        pconfig->subbands_count = atoi(value);
    else if (MATCH("beats", "subbands_history"))
        pconfig->subbands_history = atoi(value);
    else if (MATCH("beats", "subbands_constant"))
        pconfig->subbands_constant = atof(value);
    else if (MATCH("white_dot", "white_dot_hang"))
        pconfig->white_dot_hang = atoi(value);
    else if (MATCH("white_dot", "white_dot_fall"))
        pconfig->white_dot_fall = atof(value);
    else if (MATCH("white_dot", "add_white_dot"))
        pconfig->add_white_dot = atoi(value);
    else if (MATCH("wave_switch", "disp_change_sec"))
        pconfig->disp_change_sec = atoi(value);
    else if (MATCH("wave_switch", "starting_wave"))
        pconfig->starting_wave = atoi(value);
    else if (MATCH("colors", "fft_color_base")){
        int r, g, b;
        sscanf(value, "%x, %x, %x", &r, &g, &b);
        COLOR_SET(pconfig->fft_color_base, r, g, b)
    }
    else if (MATCH("colors", "bass_color")){
        int r, g, b;
        sscanf(value, "%x, %x, %x", &r, &g, &b);
        COLOR_SET(pconfig->bass_color, r, g, b)
    }
    else if (MATCH("colors", "clear_color")){
        int r, g, b;
        sscanf(value, "%x, %x, %x", &r, &g, &b);
        COLOR_SET(pconfig->clear_color, r, g, b)
    }
    else if (MATCH("colors", "white_dot")){
        int r, g, b;
        sscanf(value, "%x, %x, %x", &r, &g, &b);
        COLOR_SET(pconfig->white_dot, r, g, b)
    }
    else
        return 0;  /* unknown section/name, error */
    return 1;
}


void parse(const char *file){
    if (ini_parse(file, handler, &config) < 0) {
        printf("Can't load %s using the defaults\n", file);
        return;
    }
    printf("Config loaded from %s\n", file);
}
