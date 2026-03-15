#version 330 core

in vec2 UV;
uniform sampler2D leftTexture;
uniform sampler2D rightTexture;
uniform float haba;
uniform float totalShift;
uniform int timeStep;
uniform float manualShift;

out vec4 color;

// 元の C++ のステンシル計算と完全に一致させる
bool isRightEyeZone(float W) {
    // 元の式: ((W - (W - totalShift) / haba) + 3 * kk + SHIFT)
    // ※ 符号 (+ / -) は元の calculate_stencil 内の記述に厳密に合わせてください
    float value = (W + (W - totalShift) / haba) + (3.0 * float(timeStep)) + manualShift;
    
    // 元の式: % 12 < 6 
    // 真 (255) なら 右目、 偽 (0) なら 左目
    return mod(value, 12.0) < 6.0;
}

void main() {
    vec4 leftColor = texture(leftTexture, UV);
    vec4 rightColor = texture(rightTexture, UV);

    vec4 finalColor;
    finalColor.a = 1.0;

    float subpixel_coord_x = gl_FragCoord.x * 3.0;

    // R成分の判定
    if (isRightEyeZone(subpixel_coord_x + 0.0)) {
        finalColor.r = rightColor.r; // True = Right
    } else {
        finalColor.r = leftColor.r;  // False = Left
    }

    // G成分の判定
    if (isRightEyeZone(subpixel_coord_x + 1.0)) {
        finalColor.g = rightColor.g;
    } else {
        finalColor.g = leftColor.g;
    }

    // B成分の判定
    if (isRightEyeZone(subpixel_coord_x + 2.0)) {
        finalColor.b = rightColor.b;
    } else {
        finalColor.b = leftColor.b;
    }

    color = finalColor;
}