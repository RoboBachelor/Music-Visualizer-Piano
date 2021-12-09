#pragma once

#include <iostream>
#include <complex>
#define N_FFT 4096	/* N-point FFT, iFFT */
#define M_PI 3.1415926535897932384

void fft(std::complex<double>* f, int N);
