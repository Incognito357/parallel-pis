#include "Mandelbrot.h"

Mandelbrot::Mandelbrot()
{

}

void Mandelbrot::Update(double *vals) const
{
    int step = SCREEN_HEIGHT / thread::hardware_concurrency();
    vector<thread> threads;
    for (int i = 0; i < SCREEN_HEIGHT; i += step)
        threads.push_back(thread(&Mandelbrot::Slice, *this, ref(vals), i, min(i + step, SCREEN_HEIGHT)));
    for (auto &t : threads) t.join();
}

void Mandelbrot::Slice(double *vals, int minY, int maxY) const
{
    long double real = 0 * zoom - SCREEN_WIDTH / 2.0 * zoom + offx;
    long double imags = minY * zoom - SCREEN_HEIGHT / 2.0 * zoom + offy;
    for (int x = 0; x < SCREEN_WIDTH; x++, real += zoom)
    {
        long double imag = imags;
        for (int y = minY; y < maxY; y++, imag += zoom)
            vals[y * SCREEN_WIDTH + x] = Calculate(real, imag);
    }
}

double Mandelbrot::Calculate(long double r, long double i) const
{
    long double zReal = r, zImag = i;

    for (int c = 0; c < iter; c++)
    {
        long double r2 = zReal * zReal, i2 = zImag * zImag;
        if (r2 + i2 > 4.0) return c + 1 - (log(log(r2 + i2) / 2) / log(2));
        zImag = 2.0 * zReal * zImag + i;
        zReal = r2 - i2 + r;
    }
    return -1;
}
