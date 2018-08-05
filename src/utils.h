#ifndef UTILS_H
#define UTILS_H

#include <math.h>

#define CLIP(x, min, max)   (((x) < (min)) ? (min) : \
                            (((x) > (max)) ? (max) : (x)))

void calculate_barriers(int height, double min_val, double max_val, double *col_barriers);
void calculate_bands(double min_hz, double max_hz, int width, double *freq_bands);
void calculate_bins(double min_hz, double max_hz, int width, int sample_rate, int chunk_size, int *fft_bins);

double* logspace(double a, double b, int n, double u[]);
double* linspace(double a, double b, int n, double u[]);
double max_from_range(int start, int end, double* arr);

#endif
