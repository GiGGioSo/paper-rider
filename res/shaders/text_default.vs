#version 430 core

layout (location = 0) in vec4 vertex;
layout (location = 1) in vec4 inColor;

out vec2 TexCoords;
out vec4 vColor;

uniform mat4 projection;

void main() {
    vec2 pos = vertex.xy;
    gl_Position = projection * vec4(pos, 1.0, 1.0);
    
    TexCoords = vertex.zw;
    vColor = inColor;
}
