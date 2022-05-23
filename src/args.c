#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "args.h"

static void print_help_info() {
    const char* help = 
        "usage: dffont <path-to-ttf> <glyph-size> <width,height>\n"
        "    <path-to-ttf>\n"
        "            A path to a TTF file.\n"
        "    <glyph-size>\n"
        "            The ppem that each glyph will be rendered at.\n"
        "    <width,height>\n"
        "            The width and height of the output image.\n"
        "options:\n"
        "    --help\n"
        "            Displays this information.\n"
        "    --spread=value\n"
        "            How far from the edge of a glyph the effect of the distance field will be seen.\n"
        "            The default value is glyph-size / 14.\n"
        "    --scale=value,\n"
        "            Distance fields will be calculated using glyphs that are scale times bigger than glyph-size.\n"
        "            A larger scale value will give better accuracy, but calculations will take more time.\n"
        "            The default value is 5.\n"
        "    --padding=<left,right,top,bottom>,\n"
        "            The amount of padding there will be between glyphs.\n"
        "            The default values are 0.\n"
        "    --out-image=<path>\n"
        "            The path of the output image.\n"
        "            The default path is './dffont_image.png'.\n"
        "    --out-font=<path>\n"
        "            The path of the output file that contains information about the font.\n"
        "            The default path is './dffont_info'.";
    printf(help);
}

static int str_starts_with(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static int strtol_is_valid(int value, int nonzero) {
    return !(value < 0 || (errno != 0 && value == 0) || (nonzero && value == 0));
}

static int parse_int(char* input, int nonzero) {
    char* next;
    int value = (int)strtol(input, &next, 10);
    if (strtol_is_valid(value, nonzero) && *next == '\0') {
        return value;
    }
    return -1;
}

static int parse_comma_separated_ints(char* input, int* values, int n, int nonzero) {
    char* next;
    for (int i = 0; i < n; i++) {
        values[i] = (int)strtol(input, &next, 10);
        if (!strtol_is_valid(values[i], nonzero) || (i < n - 1 && *next != ',')) {
            return 0;
        }
        input = next + 1;
    }
    return *next == '\0';
}

static char* get_option_value(char* arg) {
    char* value = strstr(arg, "=");
    if (value == NULL) {
        fprintf(stderr, "error: option '%s' was not assigned a value", arg);
        exit(1);
    }
    return value + 1;
}

void parse_args(Args* args, int argc, char** argv) {
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        print_help_info();
        exit(0);
    }

    if (argc < 4) {
        fprintf(stderr, "error: invalid arguments: use --help for more information\n");
        exit(1);
    }

    {
        args->ppem = parse_int(argv[2], 1);
        if (args->ppem < 0) {
            fprintf(stderr, "error: '%s': invalid glyph size\n", argv[2]);
            exit(1);
        }
    }

    {
        int values[2];
        if (!parse_comma_separated_ints(argv[3], values, 2, 1)) {
            fprintf(stderr, "error: '%s': invalid image size\n", argv[3]);
            exit(1);
        }
        args->out_image_w = values[0];
        args->out_image_h = values[1];
    }

    args->ttf_path = argv[1];
    args->out_image_path = NULL;
    args->out_font_path = NULL;
    args->spread = ceilf(args->ppem / 14.0f);
    args->scale = 5;
    memset(args->padding, 0, sizeof(args->padding));

    for (int i = 4; i < argc; i++) {
        char* arg = argv[i];

        if (str_starts_with(arg, "--help") || str_starts_with(arg, "--help=")) {
            fprintf(stderr, "error: use of '--help' is not valid in this context\n");
            exit(1);
        }
        if (str_starts_with(arg, "--spread")) {
            char* value = get_option_value(arg);
            args->spread = parse_int(value, 0);
            if (args->spread < 0) {
                fprintf(stderr, "error: '%s': invalid value for spread\n", value);
                exit(1);
            }
        }
        else if (str_starts_with(arg, "--scale")) {
            char* value = get_option_value(arg);
            args->scale = parse_int(value, 1);
            if (args->scale < 0) {
                fprintf(stderr, "error: '%s': invalid value for scale\n", value);
                exit(1);
            }
        }
        else if (str_starts_with(arg, "--padding")) {
            char* value = get_option_value(arg);
            if (!parse_comma_separated_ints(value, args->padding, 4, 0)) {
                fprintf(stderr, "error: '%s': invalid value for padding\n", value);
                exit(1);
            }
        }
        else if (str_starts_with(arg, "--out-image")) {
            args->out_image_path = get_option_value(arg);
        }
        else if (str_starts_with(arg, "--out-font")) {
            args->out_font_path = get_option_value(arg);
        }
        else {
            for (int i = 0; arg[i] != '\0'; i++) {
                if (arg[i] == '=') {
                    arg[i] = '\0';
                    break;
                }
            }
            fprintf(stderr, "error: '%s': unknown option\n", arg);
            exit(1);
        }
    }
}
