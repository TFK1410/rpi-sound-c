#include "utils.h"
#include <stdlib.h>

double* logspace(double a, double b, int n, double u[])
{
    double c;
    int i;

    /* make sure number of points and array are valid */
    if(n < 2 || u == 0)
        return (void*)0;

    /* step size */
    c = (b - a)/(n - 1);

    /* fill vector */
    for(i = 0; i < n - 1; ++i)
        u[i] = pow(10., a + i*c);

    /* fix last entry to 10^b */
    u[n - 1] = pow(10., b);

    /* done */
    return u;
}

double* linspace(double a, double b, int n, double u[])
{
    double c;
    int i;

    /* make sure number of points and array are valid */
    if(n < 2 || u == 0)
        return (void*)0;

    /* step size */
    c = (b - a)/(n - 1);

    /* fill vector */
    for(i = 0; i < n - 1; ++i)
        u[i] = a + i*c;

    /* fix last entry to b */
    u[n - 1] = b;

    /* done */
    return u;
}

double max_from_range(int start, int end, double* arr){
    double mx = 0;
    for (int i = start; i < end; i++){
        if (mx < abs(arr[i])){
            mx = abs(arr[i]);
        }
    }
    return mx;
}

void calculate_bands(double min_hz, double max_hz, int width, double *freq_bands){
    logspace(log10(min_hz), log10(max_hz), width + 1, freq_bands);
}

void calculate_bins(double min_hz, double max_hz, int width, int sample_rate, int chunk_size, int *fft_bins){
    double *freq_bands = malloc((width + 1) * sizeof(double));
    calculate_bands(min_hz, max_hz, width, freq_bands);

    for (int i = 0; i < width + 1; i++){
        fft_bins[i] = (int)round(1.0f * chunk_size * freq_bands[i] / sample_rate);
        fft_bins[i] = CLIP(fft_bins[i], 1, chunk_size>>1);
    }
#ifdef DEBUG_BINS
    for (int i = 0; i < width + 1; i++){
        printf("%d ", fft_bins[i]);
    }
    printf("\n");
#endif
    free(freq_bands);
}

void calculate_barriers(int height, double min_val, double max_val, double *col_barriers){
    double *space = malloc(height * sizeof(double));
    linspace(min_val, max_val, height, space);

    for (int i = 0; i < height; i++){
        col_barriers[i] = space[height - i - 1];
    }
    free(space);
}
