#include "fft.h"

static fft_complex_t f2[N_FFT];

int log2(int N)    /*function to calculate the log2(.) of int numbers*/
{
    int k = N, i = 0;
    while (k) {
        k >>= 1;
        i++;
    }
    return i - 1;
}

int check(int n)    //checking if the number of element is a power of 2
{
    return n > 0 && (n & (n - 1)) == 0;
}

int reverse(int N, int n)    //calculating revers number
{
    int j, p = 0;
    for (j = 1; j <= Q_FFT; j++) {
        if (n & (1 << (Q_FFT - j)))
            p |= 1 << (j - 1);
    }
    return p;
}

void ordina(fft_complex_t* f1, int N) //using the reverse order in the array
{
    for (int i = 0; i < N; i++)
        f2[i] = f1[reverse(N, i)];
    for (int j = 0; j < N; j++)
        f1[j] = f2[j];
}

void fft(fft_complex_t* f, int N) //
{
    ordina(f, N);    //first: reverse order
    fft_complex_t* W = new fft_complex_t[N / 2];
    W[1] = std::polar(1., -2. * M_PI / N);
    W[0] = 1;
    for (int i = 2; i < N / 2; i++)
        W[i] = pow(W[1], i);
    int n = 1;
    int a = N / 2;
    for (int j = 0; j < Q_FFT; j++) {
        for (int i = 0; i < N; i++) {
            if (!(i & n)) {
                fft_complex_t temp = f[i];
                fft_complex_t Temp = W[(i * a) % (n * a)] * f[i + n];
                f[i] = temp + Temp;
                f[i + n] = temp - Temp;
            }
        }
        n *= 2;
        a = a / 2;
    }
    free(W);
}
