// sbs_interleave.frag
#version 330 core
in vec2 UV;
uniform sampler2D sbsTexture;
uniform float columnPitch;
uniform float screenWidth;
uniform float finalShift;
out vec4 color;

void main() {
    float pixelX = UV.x * screenWidth + finalShift;
    float positionInCycle = mod(pixelX, columnPitch);
    vec2 sourceUV;
    if (positionInCycle < (columnPitch / 2.0)) {
        sourceUV = vec2(UV.x * 0.5, UV.y); // テクスチャの左半分から取得
    } else {
        sourceUV = vec2(UV.x * 0.5 + 0.5, UV.y); // テクスチャの右半分から取得
    }
    color = texture(sbsTexture, sourceUV);
}