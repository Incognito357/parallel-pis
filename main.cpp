#include <Mandelbrot.h>

using namespace std;

int main()
{
    Mandelbrot m;

    double *vals = new double[1200 * 900];

    while (true)
    {
        #ifdef MASTER

        m.Update(vals);

        #endif
    }
}
