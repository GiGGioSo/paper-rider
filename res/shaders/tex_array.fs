in vec3 texCoords;
out vec4 fragColor;

uniform sampler2DArray tex;

void main() {
    fragColor = texture(tex, texCoords);
}
