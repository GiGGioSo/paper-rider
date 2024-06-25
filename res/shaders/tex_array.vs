layout (location = 0) in vec2 inPos;
layout (location = 1) in vec3 inTexCoords;

out vec3 texCoords;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(inPos, 1.0f, 1.0f);

    texCoords = inTexCoords;
}
