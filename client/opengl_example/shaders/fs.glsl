#version 330 core

out vec4 fragColor;

uniform sampler2D tex;

in vec2 fUV;
in vec4 fRgba;

void main() {
    float d = 1.0 - texture(tex, fUV).r;
    float w = fwidth(d);
    fragColor = fRgba;
    fragColor.a *= 1.0 - smoothstep(0.4 - w, 0.4 + w, d);;
}
