#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aRgba;

out vec2 fUV;
out vec4 fRgba;

uniform mat4 proj;

void main() {
    fUV = aUV;
    fRgba = aRgba;
    gl_Position = proj * vec4(aPos, 1.0);
}
