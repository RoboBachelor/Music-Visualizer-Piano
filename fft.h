#pragma once

#include <iostream>
#include <complex>
#define Q_FFT 11
#define N_FFT (1 << Q_FFT)	/* N-point FFT, iFFT */
#define M_PI 3.1415926535897932384

typedef std::complex<float> fft_complex_t;

void fft(fft_complex_t* f, int N);
