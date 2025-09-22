// interleave.frag (更新後)

#version 330 core

in vec2 UV;

uniform sampler2D leftTexture;
uniform sampler2D rightTexture;
uniform float columnPitch;
uniform float screenWidth;
uniform float subpixelShift;
uniform int   timeStep;         // C++の 'kk' を受け取る (0, 1, 2, 3...)

out vec4 color;

void main() {
    // 1. 時分割によるシフト量を計算 (1ステップあたり3サブピクセル)
    float timeDivisionShift = float(timeStep) * 3.0;

    // 2. 微調整用のシフト量と時分割のシフト量を合計
    float totalShift = subpixelShift + timeDivisionShift;

    // 3. 適用
    float pixelX = UV.x * screenWidth + totalShift;
    float positionInCycle = mod(pixelX, columnPitch);

    if (positionInCycle < (columnPitch / 2.0)) {
        color = texture(leftTexture, UV);
    } else {
        color = texture(rightTexture, UV);
    }
}