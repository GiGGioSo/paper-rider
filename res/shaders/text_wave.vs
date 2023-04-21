#version 460 core

layout (location = 0) in vec4 vertex;
layout (location = 1) in vec4 inColor;

out vec2 TexCoords;
out vec4 vColor;

uniform mat4 projection;
uniform float time;

void main() {
    vec2 pos = vertex.xy;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
    TexCoords = vertex.zw;

    float strength = 0.03;
    gl_Position.y += sin(time*5 + gl_Position.x * 5.0) * strength;

    vColor = inColor;
}
