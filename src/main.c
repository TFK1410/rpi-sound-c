#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <portaudio.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "led-matrix-c.h"
#include "utils.h"
#include "loops.h"

// code is constructed with below values in mind
#define LED_MATRIX_ROWS 32
#define LED_MATRIX_COLS 64
#define LED_MATRIX_CHAINS 4
#define LED_MATRIX_PIXEL_MAPPER "UInv-Mapper"
#define DEVICE 2
#define SAMPLE_RATE 44100
#define FFT_CORE 1
#define MAIN_CORE 2

#define CHUNK_POWER 13
#define FFT_UPDATE_RATE 100
#define MIN_HZ 36
#define MAX_HZ 20000
#define FFT_CURVE 0.7
#define DISPLAY_CURVE 0.7
#define LED_MATRIX_BRIGHTNESS 50

#define WAVE_SPEED 500 // in ms
#define SUBBANDS_COUNT 64
#define SUBBANDS_HISTORY 43
#define SUBBANDS_CONSTANT 1.1

#define WHITE_DOT_HANG 20

#define DISP_CHANGE_SEC 60

LedOutData out;
short *audio_data;

int stream_read_frames;

bool record;
bool stop_app;

void *fft_func();
pthread_t fft_thread;

void catch_signals(int signo){
    if(signo == SIGUSR2){
        record = true;
    }
    else if (signo == SIGINT){
        stop_app = true;
    }

}

static int callback( const void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags,
                     void *userData )
{

    size_t dataSize = sizeof(audio_data[0]);

    memmove(audio_data, audio_data + framesPerBuffer, (out.chunk_size - framesPerBuffer) * dataSize);
    memcpy(audio_data + out.chunk_size - framesPerBuffer, inputBuffer, framesPerBuffer * dataSize);

    //record
    if (record){
        printf("Recording\n");
        FILE *fp = fopen("raw_wav","w+");
        fwrite(audio_data, out.chunk_size, sizeof(short), fp);
        fclose(fp);

        printf("Recorded\n");
        record = false;
    }

    return paContinue;
}



int main(int argc, char* argv[]){
    PaError err;
    PaStreamParameters inputParameters;
    PaStream* stream;

    stream_read_frames = SAMPLE_RATE / FFT_UPDATE_RATE;
    out.chunk_size = 1<<CHUNK_POWER;

    record = false;
    stop_app = false;
    if (signal(SIGUSR2, catch_signals) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit(1);
    }

    if (signal(SIGINT, catch_signals) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit(1);
    }

    if(geteuid() != 0)
    {
        printf("RUN AS ROOT, USE SUDO\n");
        exit(1);
    }

    if(argc != 3)
    {
        printf("Incorrect number of parameters\n");
        exit(1);
    }

    out.my_wave_type = atoi(argv[1]) - 1;
    out.add_white_dot = atoi(argv[2]);
    if(out.add_white_dot > 1 || out.add_white_dot < 0) out.add_white_dot = 0;
    if(out.my_wave_type > 3 || out.my_wave_type < 1) out.my_wave_type = STD_WAVE;

    err = Pa_Initialize();
    if( err != paNoError ) {
        printf("PortAudio init error\n");
        exit(1);
    }

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default input device.\n");
        exit(1);
    }
    inputParameters.channelCount = 1;                    /* mono input FOR NOW */
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Record some audio. -------------------------------------------- */
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              NULL,
              SAMPLE_RATE,
              stream_read_frames,
              paClipOff,
              callback,
              NULL );

    if( err != paNoError ) {
        printf("Error when creating the stream\n");
        exit(1);
    }

    // initialize LED matrix
    struct RGBLedMatrixOptions options;

    memset(&options, 0, sizeof(options));
    options.rows = LED_MATRIX_ROWS;
    options.chain_length = LED_MATRIX_CHAINS;
    options.cols = LED_MATRIX_COLS;
    options.brightness = LED_MATRIX_BRIGHTNESS;
    options.pixel_mapper_config = LED_MATRIX_PIXEL_MAPPER;
#ifdef DEBUG_LED_REFRESH
    options.show_refresh_rate = true;
#endif

    out.matrix = led_matrix_create_from_options(&options, 0, NULL);
    if (out.matrix == NULL){
        printf("Error when creating the LED matrix\n");
        exit(1);
    }

    // create offscreen canvas for vsync switching
    out.offscreen_canvas = led_matrix_create_offscreen_canvas(out.matrix);
    audio_data = calloc(out.chunk_size, sizeof(short));
    init_data(&out);


    printf("FFT samples: %zu. Samples per callback: %d. Size: %dx%d. Hardware gpio mapping: %s\n",
            out.chunk_size, stream_read_frames, out.matrix_width, out.matrix_height, options.hardware_mapping);

    err = Pa_StartStream(stream);
    if( err != paNoError ) {
        printf("Error when starting the stream\n");
        exit(1);
    }

    pthread_create(&fft_thread, NULL, fft_func, NULL);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(FFT_CORE, &cpuset);
    pthread_setaffinity_np(fft_thread, sizeof(cpu_set_t), &cpuset);
    CPU_ZERO(&cpuset);
    CPU_SET(MAIN_CORE, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    clock_t rotation_clock = clock();
    while(!stop_app){
      led_canvas_clear(out.offscreen_canvas);

      if ((clock() - rotation_clock) / CLOCKS_PER_SEC > DISP_CHANGE_SEC){
          rotation_clock = clock();
          out.my_wave_type++;
          if (out.my_wave_type == 3)
            out.my_wave_type = STD_WAVE;
      }
      call_loop(&out);

      out.offscreen_canvas = led_matrix_swap_on_vsync(out.matrix, out.offscreen_canvas);
    }

    printf("Stopping\n");

    Pa_AbortStream(stream);

    pthread_join(fft_thread, NULL);

    free(audio_data);

    delete_data(&out);

    printf("Exiting\n");
}

void beat_wave_subband_calc(double *average_subbands, double **subbands, double *cur_fft){
    for (int i = out.max_radii - 1; i > 0; i--){
        out.bass_wave_color[i] = change_brightness(out.bass_wave_color[i-1], 0.97);
    }

    out.bass_wave_color[0] = clear_color;
    for (int i = 0; i < SUBBANDS_COUNT; i++){
        average_subbands[i] = 0;
        for (int j = 0; j < SUBBANDS_HISTORY; j++){
            average_subbands[i] += subbands[i][j];
        }
        average_subbands[i] /= SUBBANDS_HISTORY;

        if (cur_fft[i] > MIN_VAL &&
            cur_fft[i] > SUBBANDS_CONSTANT*average_subbands[i]){
            if (i < 1.0f * SUBBANDS_COUNT / 3)
                out.bass_wave_color[0].b = bass_color.b;
            else if (i < 2.0f * SUBBANDS_COUNT / 3)
                out.bass_wave_color[0].g = bass_color.g;
            else
                out.bass_wave_color[0].r = bass_color.r;
        }

        memmove(&subbands[i][1], &subbands[i][0], (SUBBANDS_HISTORY - 1) * sizeof(double));
        subbands[i][0] = cur_fft[i];
    }
}

void fft_to_bins(int i, int *fft_bins, double *cur_fft, double *out_fft){
    double max_from_bins;

    if (fft_bins[i] != fft_bins[i+1])
        max_from_bins = max_from_range(fft_bins[i], fft_bins[i+1], out_fft);
    else
        max_from_bins = abs((out_fft[fft_bins[i]] + out_fft[fft_bins[i]+1]) / 2);

    double log10_max = 0;
    if (max_from_bins > 0){
        log10_max = log10(max_from_bins);
    }

    cur_fft[i] = FFT_CURVE * cur_fft[i] + (1 - FFT_CURVE) * 20 * log10_max;
    out.out_matrix[i] = (cur_fft[i] - out.out_matrix[i]) * DISPLAY_CURVE + out.out_matrix[i];
}

void white_dot_calc(int i, int *white_dot_arr_delay){
    if (out.add_white_dot == 1){
        if (out.white_dot_arr[i] < out.out_matrix[i]){
            out.white_dot_arr[i] = out.out_matrix[i];
            white_dot_arr_delay[i] = WHITE_DOT_HANG;
        }
        else if (white_dot_arr_delay[i] > 0){
            white_dot_arr_delay[i]--;
        }
        else if (white_dot_arr_delay[i] == 0){
            out.white_dot_arr[i] -= get_white_dot_step(&out);
        }
    }
}

void *fft_func(){

    double *in, *out_fft;
    int *fft_bins = malloc((out.data_width + 1) * sizeof(int));
    double *cur_fft = malloc(out.data_width * sizeof(double));
    double *average_subbands = calloc(SUBBANDS_COUNT, sizeof(double));
    double **subbands = calloc(SUBBANDS_COUNT, sizeof(double*));
    int *white_dot_arr_delay = calloc(out.data_width, sizeof(int));
    fftw_plan p;

    in = fftw_alloc_real(out.chunk_size);
    out_fft = fftw_alloc_real(out.chunk_size);

    p = fftw_plan_r2r_1d(out.chunk_size, in, out_fft, FFTW_FORWARD, FFTW_MEASURE);

    for (int i = 0; i < SUBBANDS_COUNT; i++){
        subbands[i] = malloc(SUBBANDS_HISTORY * sizeof(double));
        for (int j = 0; j < SUBBANDS_HISTORY; j++){
            subbands[i][j] = INFINITY;
        }
    }

    calculate_bins(MIN_HZ, MAX_HZ, out.data_width, SAMPLE_RATE, out.chunk_size, fft_bins);

    clock_t beat_clock = clock();
    clock_t beat_clock_interval = (clock_t)(1.0f * WAVE_SPEED / 1000 / out.max_radii * CLOCKS_PER_SEC);
    clock_t start;
    clock_t time_between_updates = (clock_t)(CLOCKS_PER_SEC / FFT_UPDATE_RATE);

    struct timespec tm;

    while(!stop_app) {
        start = clock();

        for (int i = 0; i < out.chunk_size; i++){
            in[i] = (double)audio_data[i];
        }
        fftw_execute(p); /* repeat as needed */

        for (int i = 0; i < out.data_width; i++){
            fft_to_bins(i, fft_bins, cur_fft, out_fft);

            white_dot_calc(i, white_dot_arr_delay);
        }

        if ((clock() - beat_clock) >= beat_clock_interval){
            beat_clock = clock();
            beat_wave_subband_calc(average_subbands, subbands, cur_fft);
        }

        if((clock() - start) < time_between_updates){
            tm.tv_nsec = (long)(1.0f * (clock() + time_between_updates - start) / CLOCKS_PER_SEC * 1000000000);
            nanosleep(&tm, NULL);
        }
    }
    printf("Closing fft thread\n");

    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out_fft);


    free(fft_bins);
    free(cur_fft);
    free(average_subbands);
    free(white_dot_arr_delay);
    for(int i = 0; i < SUBBANDS_COUNT; i++)
        free(subbands[i]);
    free(subbands);

    return NULL;
}
