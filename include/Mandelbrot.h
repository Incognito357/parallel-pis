#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <thread>
#include <vector>
#include <math.h>

using namespace std;

class Mandelbrot
{
    public:
        Mandelbrot();
        void Update(double *vals) const;
        int iter = 150;
        long double zoom = 0.004;
        long double offx = 0, offy = 0;
        int width = 1200, height = 900;
    protected:
    private:
        void Slice(double* vals, int minY, int maxY) const;
        double Calculate(long double r, long double i) const;
};

#endif // MANDELBROT_H
