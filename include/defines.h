#ifndef CONSTANTS_H
#define CONSTANTS_H

#define INIT_SCREEN_WIDTH 1200
#define INIT_SCREEN_HEIGHT 900
#define INIT_ITER 150
#define MAX_ITER 5000
#define MAXBUF

enum MessageType {
    Recalc, OffX, OffY, Zoom, Iter, Vals, ResX, ResY, Connections, Text
};

struct Message {
    MessageType type;
    int size;
};

#ifdef CLIENT

#define RETRYATTEMPTS 5

#elif defined MASTER

#include <sys/time.h>
#include <vector>
#define HOSTPORT 31415
#define MAXCLIENTS 10

#else

#include <Mandelbrot.h>
#define RETRYATTEMPTS 9999

#endif

#ifndef MASTER

#define HOSTPORT "31415"
#define HOSTADDR "10.3.14.5"

#endif

#endif
