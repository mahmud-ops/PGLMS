#version 330 core
in vec2 TexCoord;
uniform sampler2D uTex;
uniform float uAlpha;
uniform vec4 uColor;
uniform int uUseTexture;
out vec4 FragColor;
void main(){
    if(uUseTexture == 1){
        vec4 c = texture(uTex, TexCoord);
        FragColor = vec4(c.rgb, c.a * uAlpha);
    } else {
        // for circles, discard outside radius
        if(distance(TexCoord, vec2(0.5)) > 0.5) discard;
        FragColor = uColor * uAlpha;
    }
}