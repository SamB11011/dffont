#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "stb_image.h"
#include "../../src/dffont_client.h"


/* ------- */
/* globals */
/* ------- */
#define VBO_STRIDE      9
#define VBO_STRIDE_SIZE (VBO_STRIDE * sizeof(float))

/* Can render up to MAX_GLYPHS glyphs at a time */
#define MAX_GLYPHS        100
#define MAX_VERTEX_VALUES (MAX_GLYPHS * 4 * VBO_STRIDE)
#define MAX_INDICES       (MAX_GLYPHS * 6)

#define ATLAS_PATH    "./fonts/roboto-regular-df.png"
#define INFO_PATH     "./fonts/roboto-regular-info"

static GLFWwindow*   g_window;
static float         g_values[MAX_VERTEX_VALUES];
static GLuint        g_indices[MAX_INDICES];
static DFFont_Client g_client;
static GLuint        g_numValues;
static GLuint        g_numIndices;
static GLuint        g_program;
static GLuint        g_vbo;
static GLuint        g_ebo;
static GLuint        g_vao;
static GLuint        g_texture;
static int           g_texw;
static int           g_texh;


/* ---------- */
/* GL helpers */
/* ---------- */
static GLuint create_shader(GLenum type, const char* path) {
    static char shaderCode[1000];
    GLuint shader = glCreateShader(type);
    
    // Read shader file
    {
        FILE* file = fopen(path, "r");
        if (file == NULL) {
            fprintf(stderr, "Failed to open shader %s\n", path);
            exit(1);
        }

        size_t numBytes = fread(shaderCode, 1, sizeof(shaderCode) - 1, file);
        if (ferror(file) != 0) {
            fprintf(stderr, "Failed to read shader %s\n", path);
            fclose(file);
            exit(1);
        }

        shaderCode[numBytes] = '\0';
        fclose(file);
    }
    
    // Compile shader
    {
        const char* constCode = shaderCode;
        glShaderSource(shader, 1, &constCode, NULL);
        glCompileShader(shader);
    }
    
    // Check for compilation errors
    {
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled) {
            return shader;
        }
    }
    
    // Print compilation errors
    {
        GLint infoLogLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        fprintf(stderr, "Failed to compile shader %s\n", path);

        if (infoLogLength > 0) {
            char* infoLog = malloc(infoLogLength);
            
            if (infoLog != NULL) {
                glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
                fprintf(stderr, infoLog);
            }
        }
        exit(1);
    }
}

static GLuint create_gl_program_object(const char* vspath, const char* fspath) {
    GLuint program = glCreateProgram();
    
    // Link shaders
    {
        GLuint vs = create_shader(GL_VERTEX_SHADER, vspath);
        GLuint fs = create_shader(GL_FRAGMENT_SHADER, fspath);
        
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        
        glDetachShader(program, vs);
        glDetachShader(program, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }
    
    // Check for linking errors
    {
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked) {            
            return program;
        }
    }
    
    // Print linking errors
    {
        fprintf(stderr, "Failed to link shader program\n");

        GLint infoLogLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 0) {
            char* infoLog = malloc(infoLogLength);
            
            if (infoLog != NULL) {
                glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
                fprintf(stderr, infoLog);
            }
        }
        exit(1);
    }
}

static GLuint create_gl_buffer_object(GLuint target, GLsizeiptr size, void* data, GLenum usage) {
    GLuint buf;
    glGenBuffers(1, &buf);
    glBindBuffer(target, buf);
    glBufferData(target, size, data, usage);
    glBindBuffer(target, 0);
    return buf;
}


/* ------------ */
/* GLFW helpers */
/* ------------ */
GLFWwindow* create_glfw_window(const char* title, int w, int h) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW.\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    
    g_window = glfwCreateWindow(w, h, title, NULL, NULL);
    if (g_window == NULL) {
        fprintf(stderr, "Failed to create GLFW window.\n");
        exit(1);
    }
    
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to load OpenGL with glad.\n");
        exit(1);
    }

    return g_window;
}


/* --------- */
/* rendering */
/* --------- */
static void draw_text(const char* text, int x, int y, float scale) {
    for (int i = 0; text[i] != '\0'; i++) {
        DFFont_Glyph* glyph = g_client.glyphs + (text[i] - ' ');
        if (glyph->codepoint == ' ') {
            x += glyph->xadv * scale;
            continue;
        }
        
        {
            GLuint off = g_numValues / VBO_STRIDE;
            
            GLuint indices[] = {
                off + 0, off + 1, off + 2,
                off + 1, off + 3, off + 2,
            };
            
            memcpy(g_indices + g_numIndices, indices, sizeof(indices));
            g_numIndices += sizeof(indices) / sizeof(GLuint);
        }
        
        {
            float x0 = x + glyph->xoff * scale;
            float y0 = y + glyph->yoff * scale;
            float x1 = x0 + glyph->w * scale;
            float y1 = y0 + glyph->h * scale;
            
            float u0 = (float)glyph->x / g_texw;
            float v0 = (float)glyph->y / g_texh;
            float u1 = (float)(glyph->x + glyph->w) / g_texw;
            float v1 = (float)(glyph->y + glyph->h) / g_texh;
            
            float values[] = {
                x0, y0, 0.0f, u0, v0, 1.0f, 1.0f, 1.0f, 1.0f,
                x1, y0, 0.0f, u1, v0, 1.0f, 1.0f, 1.0f, 1.0f,
                x0, y1, 0.0f, u0, v1, 1.0f, 1.0f, 1.0f, 1.0f,
                x1, y1, 0.0f, u1, v1, 1.0f, 1.0f, 1.0f, 1.0f,
            };
            
            memcpy(g_values + g_numValues, values, sizeof(values));
            g_numValues += sizeof(values) / sizeof(float);
        }
        
        x += glyph->xadv * scale;
    }
}

static void draw_frame() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    draw_text("The quick brown fox jumps over the lazy dog.", 30, 30, 0.3);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, g_numValues * sizeof(float), g_values);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
        
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, g_numIndices * sizeof(GLuint), g_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glBindVertexArray(g_vao);
    glUseProgram(g_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glUniform1i(glGetUniformLocation(g_program, "tex"), 0);
    
    {
        int winw, winh;
        glfwGetWindowSize(g_window, &winw, &winh);

        float m00 =  2.0f / (float)winw;
        float m11 = -2.0f / (float)winh;
        float proj[] = {
             m00 , 0.0f, 0.0f, 0.0f,
             0.0f, m11 , 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 1.0f,
        };

        glUniformMatrix4fv(glGetUniformLocation(g_program, "proj"), 1, GL_FALSE, proj);
    }
    
    glDrawElements(GL_TRIANGLES, g_numIndices, GL_UNSIGNED_INT, NULL);
    glfwSwapBuffers(g_window);
    
    g_numValues = 0;
    g_numIndices = 0;

    glDisable(GL_BLEND);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    draw_frame();
}


int main() {
    create_glfw_window("DFFont Client Example", 800, 600);
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
    
    if (!dffont_client_init(&g_client, INFO_PATH, ATLAS_PATH)) {
        assert(0);
    }
    g_texw = g_client.atlasWidth;
    g_texh = g_client.atlasHeight;
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, g_texw, g_texh, 0, GL_RED, GL_UNSIGNED_BYTE, g_client.atlasPixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    g_program = create_gl_program_object("./shaders/vs.glsl", "./shaders/fs.glsl");
    
    {
        g_vbo = create_gl_buffer_object(GL_ARRAY_BUFFER        , sizeof(g_values) , NULL, GL_DYNAMIC_DRAW);
        g_ebo = create_gl_buffer_object(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_indices), NULL, GL_DYNAMIC_DRAW);
        
        glGenVertexArrays(1, &g_vao);
        glBindVertexArray(g_vao);

        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VBO_STRIDE_SIZE, (void*)(0));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VBO_STRIDE_SIZE, (void*)(3 * sizeof(float)));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, VBO_STRIDE_SIZE, (void*)(5 * sizeof(float)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    while (!glfwWindowShouldClose(g_window)) {
        draw_frame();
        // glfwPollEvents();
        glfwWaitEvents();
    }
    
    return 0;
}
