#version 430 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inTexCoords;
layout (location = 2) in uint inLayerIndex;

out vec2 texCoords;
flat out uint layerIndex;

void main() {
    gl_Position = vec4(inPos, 1.0f, 1.0f);

    texCoords = inTexCoords;
    layerIndex = inLayerIndex;
}
