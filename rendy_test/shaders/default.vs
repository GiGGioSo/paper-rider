#version 430 core

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec4 in_color;

out vec4 fragment_color;

void main() {

    gl_Position = vec4(in_pos.xy, 0, 1);

    fragment_color = in_color;
}
