#include "fft.h"

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
    for (j = 1; j <= log2(N); j++) {
        if (n & (1 << (log2(N) - j)))
            p |= 1 << (j - 1);
    }
    return p;
}

void ordina(std::complex<double>* f1, int N) //using the reverse order in the array
{
    std::complex<double> f2[N_FFT];
    for (int i = 0; i < N; i++)
        f2[i] = f1[reverse(N, i)];
    for (int j = 0; j < N; j++)
        f1[j] = f2[j];
}

void fft(std::complex<double>* f, int N) //
{
    ordina(f, N);    //first: reverse order
    std::complex<double>* W;
    W = (std::complex<double> *)malloc(N / 2 * sizeof(std::complex<double>));
    W[1] = std::polar(1., -2. * M_PI / N);
    W[0] = 1;
    for (int i = 2; i < N / 2; i++)
        W[i] = pow(W[1], i);
    int n = 1;
    int a = N / 2;
    for (int j = 0; j < log2(N); j++) {
        for (int i = 0; i < N; i++) {
            if (!(i & n)) {
                std::complex<double> temp = f[i];
                std::complex<double> Temp = W[(i * a) % (n * a)] * f[i + n];
                f[i] = temp + Temp;
                f[i + n] = temp - Temp;
            }
        }
        n *= 2;
        a = a / 2;
    }
    free(W);
}
