#ifndef DF_H
#define DF_H

#include <stdint.h>

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    uint8_t* pixels;
    float*   dists;
    float*   xinters;
    Vec2*    verts;
    int      w;
    int      h;
    int      n_xinters;
    int      n_verts;
    int      spread;
} DF;

void calc_df(DF* df);

#endif
