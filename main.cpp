#include <Mandelbrot.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
