#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <thread>
#include <vector>
#include <math.h>
#include <defines.h>

using namespace std;

class Mandelbrot
{
    public:
        Mandelbrot();
        #ifdef DEBUG
        void Update(double *vals) const;
        #else
        void Update(short *vals) const;
        #endif
        int iter = INIT_ITER;
        int parallel_pos = 0;
        long double zoom = 0.004L;
        long double offx = 0L, offy = 0L;
        int width = INIT_SCREEN_WIDTH, height = INIT_SCREEN_HEIGHT;
        int parallel_height = INIT_SCREEN_HEIGHT;
    protected:
    private:
        #ifdef DEBUG
        void Slice(double* vals, int minY, int maxY) const;
        #else
        void Slice(short* vals, int minY, int maxY) const;
        #endif
        double Calculate(long double r, long double i) const;
};

#endif // MANDELBROT_H
