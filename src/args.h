#ifndef DFFONT_ARGS_H
#define DFFONT_ARGS_H

typedef struct {
    char* ttf_path;
    char* out_image_path;
    char* out_font_path;
    int   padding[4]; /* left, right, top, bottom */
    int   ppem;
    int   out_image_w;
    int   out_image_h;
    int   spread;
    int   scale;
} Args;

void parse_args(Args* args, int argc, char** argv);

#endif
