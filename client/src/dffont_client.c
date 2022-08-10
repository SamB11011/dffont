#include <stdio.h>

#include "dffont_client.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int dffont_client_init(DFFont_Client* client, const char* filepath, const char* atlaspath) {
    {
        #define DFFONT_CLIENT_FSCANF(numArgs, format, ...)\
        {                                                \
            int res = fscanf(file, format, __VA_ARGS__); \
            if (res != numArgs) {                        \
                fclose(file);                            \
                return 0;                                \
            }                                            \
        }
        
        FILE* file = fopen(filepath, "r");
        if (file == NULL) {
            return 0;
        }
        
        DFFONT_CLIENT_FSCANF(1, "num_glyphs=%d\n", &client->numGlyphs);
        DFFONT_CLIENT_FSCANF(1, "ppem=%d\n", &client->ppemInitial);
        DFFONT_CLIENT_FSCANF(1, "line_gap=%d\n", &client->lineGap);
        
        if (client->numGlyphs != DFFONT_NUM_GLYPHS) {
            fclose(file);
            return 0;
        }
        
        for (int i = 0; i < numGlyphs; i++) {
            Glyph* glyph = client->glyphs + i;
            DFFONT_CLIENT_FSCANF(
                9, file, "char=%d, x=%d, y=%d, w=%d, h=%d, xoff=%d, yoff=%d, xadv=%d, yadv=%d\n",
                &glyph->codepoint, &glyph->x, &glyph->y, &glyph->w, &glyph->h, 
                &glyph->xoff, &glyph->yoff, &glyph->xadv, &glyph->yadv);
        }
        
        fclose(file);
    }
    
    {
        int comp;
        client->atlasPixels = stbi_load(atlaspath, &client->atlasWidth, &client->atlasHeight, &comp, 0);
        if (client->atlasPixels == NULL) {
            return 0;
        }
        if (comp != 1) {
            dffont_client_free(client);
            return 0;
        }
    }
    
    return 1;
}

void dffont_client_free(DFFont_Client* client) {
    stbi_image_free(client->atlasPixels);
    client->atlasPixels = NULL;
}
