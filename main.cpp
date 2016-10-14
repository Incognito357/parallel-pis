#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define HOSTPORT 31415
#define HOSTADDR "10.3.14.15"

#ifdef MASTER

#include <vector>

#else

#include <Mandelbrot.h>

#endif

using namespace std;

int main()
{
    #ifdef MASTER



    #else



    #endif

    while (true)
    {
        #ifdef MASTER



        #else



        #endif
    }

    #ifdef MASTER



    #endif
}
