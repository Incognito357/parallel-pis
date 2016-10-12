#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef MASTER

#define LISTENPORT 31415

#else

#include <Mandelbrot.h>

#endif

using namespace std;

int main()
{
    #ifdef MASTER

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        printf("Could not create socket.");
        return -1;
    }

    struct sockaddr_in addr
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTENPORT);

    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        printf("Error binding address to socket");
        return -1;
    }

    listen(fd,

    #else

    Mandelbrot m;

    #endif

    double *vals = new double[1200 * 900];

    while (true)
    {
        #ifdef MASTER



        #else

        m.Update(vals);

        #endif
    }
}
