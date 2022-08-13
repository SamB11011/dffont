#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "args.h"

static void print_ttf_help() {
    printf(
        "usage:\n"
        "    dffont ttf <path> <glyph-size> <width,height>\n"
        "               [--spread=<value>] [--scale=<value>]\n"
        "               [--padding=<left,right,top,bottom>]\n"
        "               [--out-image=<path>] [--out-info=<path>]\n"
        "\n"
        "Description:\n"
        "    Generates distance fields for glyphs in a TrueType Font file.\n"
        "\n"
        "Arguments:\n"
        "    <path>\n"
        "        The path to a TTF file.\n"
        "    <glyph-size>\n"
        "        The ppem that each glyph will be rendered at.\n"
        "    <width,height>\n"
        "        The width and height of output image (in pixels).\n"
        "Options:\n"
        "    [--spread=<value>]\n"
        "        How far from the edge of a glyph the effect of a the distance field will be seen.\n"
        "        The default value of is glyph-size / 14.\n"
        "\n"
        "    [--scale=<value>]\n"
        "        The amount each glyph will be scaled before calculating its distance field. A larger scale\n"
        "        value will give better accuracy, but the calculations will take more time/ resources.\n"
        "        The default value is 5.\n"
        "    [--padding=<left,right,top,bottom>]\n"
        "            The amount of padding there will be between glyphs.\n"
        "            The default values are 0.\n"
        "    [--out-image=<path>]\n"
        "            The path of the output image.\n"
        "            The default path is './dffont_image.png'.\n"
        "    [--out-font=<path>]\n"
        "            The path of the output file that contains information about the font.\n"
        "            The default path is './dffont_info'.\n");
}

static void print_image_help() {

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

static int try_get_spread(Args* args, char* arg) {
    if (str_starts_with(arg, "--spread")) {
        char* value = get_option_value(arg);
        args->spread = parse_int(value, 0);
        if (args->spread < 0) {
            fprintf(stderr, "error: '%s': invalid spread value\n", value);
            exit(1);
        }
        return 1;
    }
    return 0;
}

static int try_get_scale(Args* args, char* arg) {
    if (str_starts_with(arg, "--scale")) {
        char* value = get_option_value(arg);
        args->scale = parse_int(value, 1);
        if (args->scale < 0) {
            fprintf(stderr, "error: '%s': invalid scale value\n", value);
            exit(1);
        }
        return 1;
    }
    return 0;
}

void parse_args(Args* args, int argc, char** argv) {
    // Check if no arguments were given or just --help was given
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        printf(
            "usage: dffont [--help] <command> [<args>]\n"
            "\n"
            "These are the two commands that can be used:\n"
            "    ttf      Generate distance fields for glyphs contained in a TrueType Font file.\n"
            "    image    Generate a distance field for a given image.\n"
        );
        exit(0);
    }

    // Check if a command was used without any arguments
    // If this is the case, it behaves the as if it was paired with --help
    if (argc == 2 && strcmp(argv[1], "ttf") == 0) {
        print_ttf_help();
        exit(0);
    }
    if (argc == 2 && strcmp(argv[1], "image") == 0) {
        print_image_help();
        exit(0);
    }

    // Check for --help option paired with a command
    if (argc == 3) {
        char* command_arg = NULL;
        if (strcmp(argv[1], "--help") == 0) {
            command_arg = argv[1];
        }
        if (strcmp(argv[2], "--help") == 0) {
            command_arg = argv[2];
        }
        if (command_arg != NULL) {
            if (strcmp(command_arg, "ttf") == 0) {
                print_ttf_help();
                exit(0);
            }
            else if (strcmp(command_arg, "image") == 0) {
                print_image_help();
                exit(0);
            }
            else {
                fprintf(stderr, "error: '%s': unknown command\n", command_arg);
                exit(1);
            }
        }
    }

    memset(args, 0, sizeof(Args));

    if (strcmp(argv[1], "ttf") == 0) {
        // Process arguments
        if (argc < 5) {
            fprintf(stderr, "error: too few arguments: use --help for more information\n");
            exit(1);
        }
        {
            args->ppem = parse_int(argv[3], 1);
            if (args->ppem < 0) {
                fprintf(stderr, "error: '%s': invalid glyph size\n", argv[3]);
                exit(1);
            }
        }
        {
            int values[2];
            if (!parse_comma_separated_ints(argv[4], values, 2, 1)) {
                fprintf(stderr, "error: '%s': invalid image size\n", argv[4]);
                exit(1);
            }
            args->out_image_w = values[0];
            args->out_image_h = values[1];
        }
        args->ttf_path = argv[2];

        // Set default values for spread and scale in case they are not given as options
        args->spread = roundf(args->ppem / 14.0f);
        args->scale = 5;

        // Process options
        for (int i = 5; i < argc; i++) {
            char* arg = argv[i];

            if (!try_get_spread(args, arg) && !try_get_scale(args, arg)) {
                if (str_starts_with(arg, "--padding")) {
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
                else if (strcmp(arg, "--help") == 0) {
                    fprintf(stderr, "error: --help is not valid in that context\n");
                    exit(1);
                }
                else {
                    fprintf(stderr, "error: '%s': unknown option\n", arg);
                    exit(1);
                }
            }
        }
    }
    else if (strcmp(argv[1], "image") == 0) {
        
    }
    else {
        fprintf(stderr, "error: '%s': unknown command\n", argv[1]);
        exit(1);
    }
}
