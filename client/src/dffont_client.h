#ifndef DFFONT_CLIENT_H
#define DFFONT_CLIENT_H

#define DFFONT_FIRST_CHAR ' '
#define DFFONT_LAST_CHAR  '~'
#define DFFONT_NUM_CHARS  (DFFONT_LAST_CHAR - DFFONT_FIRST_CHAR + 1)

typedef struct {
    int codepoint;
    int x;
    int y;
    int w;
    int h;
    int xoff;
    int yoff;
    int xadv;
    int yadv;
} DFFont_Glyph;

typedef struct {
    DFFont_Glyph  glyphs[DFFONT_NUM_CHARS];
    char*         atlasPixels;
    int           atlasWidth;
    int           atlasHeight;
    int           numGlyphs; /* This should always equal DFFONT_NUM_CHARS */
    int           ppemInitial;
    int           lineGap;
} DFFont_Client;


int dffont_client_init(DFFont_Client* client, const char* filepath, const char* atlaspath);

void dffont_client_free(DFFont_Client* client);

#endif
