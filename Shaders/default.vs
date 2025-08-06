#version 330 core
layout (location = 0) in vec3 aPos;

uniform vec3 uOffset;
uniform float uScale;

void main()
{
    vec3 scaledPos = aPos * uScale + uOffset;
    gl_Position = vec4(scaledPos, 1.0);
}
 