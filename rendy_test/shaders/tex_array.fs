#version 430 core

in vec2 texCoords;
flat in uint layerIndex;

out vec4 fragColor;

uniform sampler2DArray tex_array;

void main() {
    fragColor = texture(tex_array, vec3(texCoords.xy, layerIndex));
}
