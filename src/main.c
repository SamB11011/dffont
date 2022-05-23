#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "truety.h"
#include "args.h"
#include "df.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define FIRST_CHAR '!'
#define LAST_CHAR  '~'
#define NUM_CHARS  (LAST_CHAR - FIRST_CHAR + 1)

int main(int argc, char** argv) {
    Args args;
    parse_args(&args, argc, argv);

    TTY_Font font;
    if (tty_font_init(&font, args.ttf_path)) {
        fprintf(stderr, "error: '%s': Failed to load\n", args.ttf_path);
        exit(1);
    }

    TTY_Instance instance;
    if (tty_instance_init(&font, &instance, args.ppem * args.scale, TTY_INSTANCE_NO_HINTING)) {
        goto internal_font_error;
    }

    FILE* font_info_file;
    {
        const char* path = args.out_font_path == NULL ? "./dffont_info" : args.out_font_path;
        font_info_file = fopen(path, "w");
        if (font_info_file == NULL) {
            fprintf(stderr, "error: '%s': failed to create font info file", path);
            exit(1);
        }
    }
    fprintf(font_info_file, "num_chars=%d\n", NUM_CHARS);
    fprintf(font_info_file, "ppem=%d\n", args.ppem);
    fprintf(font_info_file, "line_gap=%d\n", instance.lineGap);

    int spread_size = 2 * args.spread;

    DF df = {0};
    df.w = instance.maxGlyphSize.x + args.scale * spread_size;
    df.h = instance.maxGlyphSize.y + args.scale * spread_size;
    df.spread = args.scale * args.spread;

    uint8_t* down_pixels = NULL; // The destination buffer for stb_image_resize
    int down_w = instance.maxGlyphSize.x / args.scale + spread_size;
    int down_h = instance.maxGlyphSize.y / args.scale + spread_size;

    uint8_t* out_pixels = NULL; // Size for the output image is stored in args

    {
        int off              = 0;
        int dim              = df.w > df.h ? df.w : df.h;
        int dists_size       = df.w * df.h * sizeof(float);
        int xinters_size     = dim * sizeof(float);
        int verts_size       = dim * sizeof(Vec2);
        int df_pixels_size   = df.w * df.h;
        int down_pixels_size = down_w * down_h;
        int out_pixels_size  = args.out_image_w * args.out_image_h;
        uint8_t* mem = calloc(dists_size + xinters_size + verts_size + df_pixels_size + down_pixels_size + out_pixels_size, 1);
        if (mem == NULL) {
            goto out_of_memory;
        }
        df.dists    = (float*)(mem);
        df.xinters  = (float*)(mem + (off += dists_size));
        df.verts    = (Vec2*) (mem + (off += xinters_size));
        df.pixels   =         (mem + (off += verts_size));
        down_pixels =         (mem + (off += df_pixels_size));
        out_pixels  =         (mem + (off += down_pixels_size));
    }

    TTY_U32 x = args.padding[0];
    TTY_U32 y = args.padding[2];
    TTY_U32 largest_h = 0;

    for (char c = FIRST_CHAR; c <= LAST_CHAR; c++) {
        TTY_U32   glyphIdx;
        TTY_Glyph glyph;

        if (tty_get_glyph_index(&font, c, &glyphIdx) ||
            tty_glyph_init(&font, &glyph, glyphIdx))
        {
            goto internal_font_error;
        }
        
        {
            TTY_Image image = {
                .pixels = df.pixels, 
                .size = {.x = df.w, .y = df.h}
            };

            if (tty_render_glyph_to_existing_image(&font, &instance, &glyph, &image, df.spread, df.spread)) {
                goto internal_font_error;
            }
        }

        calc_df(&df);

        if (!stbir_resize_uint8(df.pixels, df.w, df.h, df.w, 
                                down_pixels, down_w, down_h, down_w, 
                                1))
        {
            goto out_of_memory;
        }

        memset(df.pixels, 0, df.w * df.h);

        {
            int glyph_h = glyph.size.y / args.scale + spread_size;
            int glyph_w = glyph.size.x / args.scale + spread_size;

            if (y + glyph_h > args.out_image_h) {
                // There isn't enough room in the output image for the glyph
                break;
            }

            if (x + glyph_w > args.out_image_w) {
                x = args.padding[0];
                y += largest_h + args.padding[2] + args.padding[3];
                largest_h = 0;
            }

            for (int yi = 0; yi < glyph_h; yi++) {
                uint8_t* out = out_pixels + (x + (yi + y) * args.out_image_w);
                uint8_t* down = down_pixels + (yi * down_w);
                memcpy(out, down, glyph_w);
            }

            if (glyph_h > largest_h) {
                largest_h = glyph_h;
            }
            
            // TODO: Add spread to offset and advance?
            fprintf(font_info_file, "char=%d, x=%d, y=%d, w=%d, h=%d, xoff=%d, yoff=%d, xadv=%d, yadv=%d\n",
                    (int)c, (int)x, (int)y, glyph_w, glyph_h, 
                    (int)glyph.offset.x / args.scale, (int)glyph.offset.y / args.scale, 
                    (int)glyph.advance.x / args.scale, (int)glyph.advance.y / args.scale);

            x += glyph_w + args.padding[0] + args.padding[1];
        }
    }

    stbi_write_png(
        args.out_image_path == NULL ? "./dffont_image.png" : args.out_image_path, 
        args.out_image_w, args.out_image_h, 1, out_pixels, args.out_image_w);

    return 0;

internal_font_error:
    fprintf(stderr, "error: an internal font error occurred");
    exit(1);

out_of_memory:
    fprintf(stderr, "error: failed to allocate memory");
    exit(1);
}
