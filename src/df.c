#include <math.h>
#include <float.h>
#include <assert.h>
#include "df.h"

static void march_parabolas(DF* df, int y, int transpose) {
    int i = 0;
    int wt = transpose ? df->h : df->w;
    for (int x = 0; x < wt; x++) {
        while (i < df->n_xinters && x > df->xinters[i]) {
            // Once an x-intersection is reached, transition to the next parabola
            i++;
        }
        int idx = transpose ? y + x * df->w : x + y * df->w;
        assert(idx < df->w * df->h);
        float dx = x - df->verts[i].x;
        float dist = dx * dx + df->verts[i].y;
        if (dist < df->dists[idx]) {
            df->dists[idx] = dist;
        }
    }
}

static float calc_x_intersection(Vec2 p, Vec2 q) {
    assert(p.y != FLT_MAX && q.y != FLT_MAX);
    return ((q.y + q.x*q.x) - (p.y + p.x*p.x)) / (2.0f*q.x - 2.0f*p.x);
}

static void calc_df_pass(DF* df, int transpose) {
    int ht = transpose ? df->w : df->h;
    int wt = transpose ? df->h : df->w;
    
    for (int y = 0; y < ht; y++) {
        df->n_xinters = 0;
        df->n_verts = 0;

        for (int x = 0; x < wt; x++) {
            int idx = transpose ? y + x * df->w : x + y * df->w;
            assert(idx < df->w * df->h);

            Vec2 vert = {x, df->dists[idx]};
            if (vert.y == FLT_MAX) {
                // A parabola at y = infinity will never be a part of the lower envelope
                continue;
            }

            if (df->n_verts > 0) {
                float xinter = calc_x_intersection(df->verts[df->n_verts - 1], vert);

                // If current x-intersection is < prev x-intersection, the prev 
                // vertex is not a part of the lower envelope
                //
                // Note: This condition will never occur in the first pass since 
                // all parabolas are at y = 0
                while (df->n_xinters > 0 && xinter < df->xinters[df->n_xinters - 1]) {
                    df->n_xinters--;
                    df->n_verts--;
                    xinter = calc_x_intersection(df->verts[df->n_verts - 1], vert);
                }

                assert(df->n_xinters < wt);
                df->xinters[df->n_xinters++] = xinter;
            }

            assert(df->n_verts < wt);
            df->verts[df->n_verts++] = vert;
        }

        if (df->n_verts > 0) {
            // Number of vertices should equal the number of x-intersections + 1
            assert(df->n_verts == df->n_xinters + 1);
            march_parabolas(df, y, transpose);
        }
    }
}

static float linear_map(float x, float x0, float y0, float x1, float y1) {
    float m = (y1 - y0) / (x1 - x0);
    float b = y0 - m * x0;
    return m * x + b;
}

void calc_df(DF* df) {
    float maxdist = 0.0f;

    // Calculate distances for on pixels
    {
        for (int i = 0; i < df->w * df->h; i++) {
            df->dists[i] = df->pixels[i] > 0 ? FLT_MAX : 0;
        }

        calc_df_pass(df, 0);
        calc_df_pass(df, 1);

        for (int i = 0; i < df->w * df->h; i++) {
            if (df->pixels[i] > 0) {
                df->dists[i] = sqrtf(df->dists[i]) + df->spread;
                if (df->dists[i] > maxdist) {
                    maxdist = df->dists[i];
                }
            }
        }

        for (int i = 0; i < df->w * df->h; i++) {
            if (df->pixels[i] > 0) {
                df->pixels[i] = (uint8_t)(255.0f * (df->dists[i] / maxdist));
            }
        }
    }

    // Calculate distances for off pixels
    {
        for (int i = 0; i < df->w * df->h; i++) {
            df->dists[i] = df->pixels[i] == 0 ? FLT_MAX : 0;
        }

        calc_df_pass(df, 0);
        calc_df_pass(df, 1);

        for (int i = 0; i < df->w * df->h; i++) {
            if (df->pixels[i] == 0) {
                float dist = sqrtf(df->dists[i]);
                if (dist <= df->spread) {
                    float d = linear_map(dist, 1.0f, df->spread, df->spread, 1.0f); // Smaller distances should give bigger values since they are closer to on-pixels
                    df->pixels[i] = (uint8_t)(255.0f * (d / maxdist));
                }
            }
        }
    }
}