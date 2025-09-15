#version 330 core

// Position attribute
layout (location = 0) in vec3 aPos;

// New attribute for texture coordinates
layout (location = 1) in vec2 aTexCoord; 

// Uniforms for scaling and offsetting the model
uniform vec3 uOffset;
uniform float uScale;

// Output variable for texture coordinates, passed to the fragment shader
out vec2 TexCoord;

void main()
{
    // Apply scaling and offsetting to the position
    vec3 scaledPos = aPos * uScale + uOffset;

    // Set the final position of the vertex
    gl_Position = vec4(scaledPos, 1.0);

    // Pass the texture coordinates to the fragment shader
    TexCoord = aTexCoord;
}
