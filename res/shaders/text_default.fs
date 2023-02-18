#version 330 core

in vec2 TexCoords;
in vec4 vColor;

out vec4 color;

uniform sampler2D tex;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(tex, TexCoords).r);
    color = vColor * sampled;
}
