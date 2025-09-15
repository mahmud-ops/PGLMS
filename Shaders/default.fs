#version 330 core

// Input from the vertex shader for interpolated texture coordinates
in vec2 TexCoord;

// The texture sampler uniform, used to access the texture data
uniform sampler2D ourTexture;

// Uniform for solid color
uniform vec3 uColor;

// Uniform to choose between texture and color
uniform bool useTexture;

// The final output color of the fragment
out vec4 FragColor;

void main()
{
    if (useTexture) {
        // Sample the color from the texture using the interpolated TexCoord.
        FragColor = texture(ourTexture, TexCoord);
    } else {
        // Use solid color
        FragColor = vec4(uColor, 1.0);
    }
}
