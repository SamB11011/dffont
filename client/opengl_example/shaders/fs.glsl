#version 330 core

#define width 0.3
#define edge  0.12

out vec4 fragColor;

uniform sampler2D tex;

in vec2 fUV;
in vec4 fRgba;

void main() {
    float distance = 1.0 - texture(tex, fUV).r;
    float alpha = 1.0 - smoothstep(width, width + edge, distance);
    fragColor = fRgba;
    fragColor.a *= alpha;
}
