#version 430 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec4 inColor;

out vec4 vColor;

uniform mat4 projection;

void main() {

    gl_Position = projection * vec4(inPos.xy, 1.0, 1.0);

    vColor = inColor;
}
