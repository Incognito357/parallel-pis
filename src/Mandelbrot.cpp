#include "Mandelbrot.h"

Mandelbrot::Mandelbrot()
{

}

void Mandelbrot::Update(double *vals) const
{
    int step = height / thread::hardware_concurrency();
    vector<thread> threads;
    for (int i = 0; i < height; i += step)
        threads.push_back(thread(&Mandelbrot::Slice, *this, ref(vals), (parallel_pos * height) + i, min((parallel_pos * height) + i + step, height)));
    for (auto &t : threads) t.join();
}

void Mandelbrot::Slice(double *vals, int minY, int maxY) const
{
    long double real = 0 * zoom - width / 2.0 * zoom + offx;
    long double imags = minY * zoom - height / 2.0 * zoom + offy;
    for (int x = 0; x < width; x++, real += zoom)
    {
        long double imag = imags;
        for (int y = minY; y < maxY; y++, imag += zoom)
            vals[(y - (parallel_pos * height)) * width + x] = Calculate(real, imag);
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
