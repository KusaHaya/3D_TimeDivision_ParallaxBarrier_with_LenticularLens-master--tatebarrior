// interleave.frag (更新後)

#version 330 core

in vec2 UV;

uniform sampler2D leftTexture;
uniform sampler2D rightTexture;
uniform float columnPitch;
uniform float screenWidth;
uniform float finalShift; // ★すべてのシフトを合計した最終的な値を受け取る

out vec4 color;

void main() {
    // 最終的なシフト量を適用するだけ
    float pixelX = UV.x * screenWidth + finalShift;
    float positionInCycle = mod(pixelX, columnPitch);

    if (positionInCycle < (columnPitch / 2.0)) {
        color = texture(leftTexture, UV);
    } else {
        color = texture(rightTexture, UV);
    }
}