#include "Mandelbrot.h"

Mandelbrot::Mandelbrot()
{

}

void Mandelbrot::Update(short *vals) const
{
    double stp = (double)parallel_height / thread::hardware_concurrency();
    int step = 0;
    if ((int)stp != stp) step = (int)stp + 1;
    else step = (int)stp;
    vector<thread> threads;
    for (int i = 0; i < parallel_height; i += step)
        threads.push_back(thread(&Mandelbrot::Slice, *this, ref(vals),
            (parallel_pos * parallel_height) + i,
            (parallel_pos * parallel_height) + min(i + step, parallel_height)));
    for (auto &t : threads) t.join();
}

void Mandelbrot::Slice(short *vals, int minY, int maxY) const
{
    long double real = 0L * zoom - width / 2.0L * zoom + offx;
    long double imags = minY * zoom - parallel_height / 2.0L * zoom + offy;
    for (int x = 0; x < width; x++, real += zoom)
    {
        long double imag = imags;
        for (int y = minY; y < maxY; y++, imag += zoom)
        {
            //printf("Accessing %d\n", (y - (parallel_pos * parallel_height)) * width + x);
            vals[(y - (parallel_pos * parallel_height)) * width + x] = (short)(Calculate(real, imag) * 100);
        }
    }
}

double Mandelbrot::Calculate(long double r, long double i) const
{
    long double zReal = r, zImag = i;

    for (int c = 0; c < iter; c++)
    {
        long double r2 = zReal * zReal, i2 = zImag * zImag;
        if (r2 + i2 > 4.0) return c + 1 - (log(log(r2 + i2) / 2) / log(2));
        zImag = 2.0L * zReal * zImag + i;
        zReal = r2 - i2 + r;
    }
    return -1;
}
