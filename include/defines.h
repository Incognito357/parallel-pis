#ifndef CONSTANTS_H
#define CONSTANTS_H

#define INIT_SCREEN_WIDTH 1200
#define INIT_SCREEN_HEIGHT 900
#define INIT_ITER 150
#define INIT_ZOOM 0.004L
#define MAX_ITER 5000
#define MAXBUF

enum MessageType {
    NoEvent, Recalc, OffX, OffY, Zoom, Iter, Vals, ResX, ResY, Connections, Text
};

struct Message {
    MessageType type;
    int len;
};

/*
    Taken from Beej's guide to networking (beej.us/guide/bgnt/output/html/multipage/advanced.html)
    Convert long double to a format safe to transmit over the network
*/
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_64(i) (unpack754((i), 64, 11))

inline uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (f == 0.0) return 0; // get this special case out of the way

    // check sign and begin normalization
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }

    // get the normalized form of f and track the exponent
    shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL<<significandbits) + 0.5f);

    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

    // return the final answer
    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

inline long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0) return 0.0;

    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }

    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;

    return result;
}

//END Beej's Code

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
