#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 projection;

uniform vec2 spriteSize;
uniform vec2 spriteOffset;

void main()
{
    TexCoords = (vertex.zw * spriteSize) + spriteOffset;
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
}