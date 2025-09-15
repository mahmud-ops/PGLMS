#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTex;

uniform vec2 uPos;   // center position
uniform vec2 uScale; // size
uniform float uAngle; // rotation in radians

out vec2 TexCoord;

void main()
{
    // Scale
    vec2 pos = aPos * uScale;

    // Rotate
    float cosA = cos(uAngle);
    float sinA = sin(uAngle);
    pos = vec2(
        pos.x * cosA - pos.y * sinA,
        pos.x * sinA + pos.y * cosA
    );

    // Translate
    pos += uPos;

    gl_Position = vec4(pos, 0.0, 1.0);
    TexCoord = aTex;
}
