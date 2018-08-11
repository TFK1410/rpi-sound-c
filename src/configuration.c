#include "configuration.h"
#include "../inih/ini.h"

void config_defaults(){
  config.led_matrix_rows = 32;
  config.led_matrix_cols = 64;
  config.led_matrix_chains = 4;
  config.led_matrix_pixel_mapper = "UInv-Mapper";
  config.sample_rate = 44100;
  config.fft_core = 1;
  config.main_core = 2;

  config.chunk_power = 13;
  config.fft_update_rate = 100;
  config.min_hz = 36;
  config.max_hz = 20000;
  config.fft_curve = 0.65;
  config.led_matrix_brightness = 50;

  config.wave_speed = 500; // in ms
  config.subbands_count = 64;
  config.subbands_history = 43;
  config.subbands_constant = 1.1;

  config.white_dot_hang = 20;

  config.disp_change_sec = 0;

  config.darker_mult = 0.4;
  config.min_val = 110;
  config.max_val = 155;
  config.white_dot_fall = 0.5;

  config.wave_types = 5;

  config.starting_wave = 4;
  config.add_white_dot = 1;

  config.fft_color_base = (LedColor){0xff, 0xff, 0x00};
  config.bass_color = (LedColor){0xb0, 0x60, 0xd0};
  config.clear_color = (LedColor){0x00, 0x00, 0x00};
  config.white_dot = (LedColor){0xff, 0xff, 0xff};
}
