// advanced_parallax.frag

#version 330 core

in vec2 UV;

// C++から受け取るuniform変数
uniform sampler2D leftTexture;
uniform sampler2D rightTexture;

uniform float haba;         // 視聴距離(Z)から計算された値
uniform float totalShift;   // 視点位置(X)から計算された値
uniform int   timeStep;     // 時分割のステップ (kk)
uniform float manualShift;  // 手動調整用のシフト (SHIFT)

out vec4 color;

// 従来方式の計算式をGLSLで再現したヘルパー関数
// サブピクセル座標を受け取り、左目用ならtrueを返す
bool shouldShowLeftEye(float W) {
    // ((W - (W - totalShift) / haba) + 2 * kk + SHIFT)
    float value = (W - (W - totalShift) / haba) + (2.0 * float(timeStep)) + manualShift;
    
    // % 12 < 6
    return mod(value, 12.0) < 6.0;
}

void main() {
    vec4 leftColor = texture(leftTexture, UV);
    vec4 rightColor = texture(rightTexture, UV);

    vec4 finalColor;
    finalColor.a = 1.0;

    // 物理的なサブピクセル座標の基準値
    float subpixel_coord_x = gl_FragCoord.x * 3.0;

    // R, G, Bの各成分ごとに、従来方式の計算式で左右を判断
    if (shouldShowLeftEye(subpixel_coord_x + 0.0)) {
        finalColor.r = leftColor.r;
    } else {
        finalColor.r = rightColor.r;
    }

    if (shouldShowLeftEye(subpixel_coord_x + 1.0)) {
        finalColor.g = leftColor.g;
    } else {
        finalColor.g = rightColor.g;
    }

    if (shouldShowLeftEye(subpixel_coord_x + 2.0)) {
        finalColor.b = leftColor.b;
    } else {
        finalColor.b = rightColor.b;
    }

    color = finalColor;
}