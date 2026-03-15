// advanced_parallax.frag

#version 330 core

in vec2 UV;

// C++から受け取るuniform変数
uniform sampler2D leftTexture;
uniform sampler2D rightTexture;

uniform float haba;         // 視聴距離(Z)から計算された値（サブピクセル幅）
uniform float totalShift;   // 従来方式の基準シフト
uniform int   timeStep;     // 時分割のステップ (kk)
uniform float manualShift;  // 手動調整用のシフト (SHIFT)
uniform vec3  rgbGain;      // RGBゲイン補正
uniform float gammaValue;   // ガンマ補正
uniform float crosstalk;    // クロストーク補償量 (0.0-0.5程度)
uniform int   enableColorCorrection; // 1:有効, 0:無効


// ★追加：中心（MiddleLine）に追従する補正用
uniform float middleLinePx; // ピクセル単位（C++側で MiddleLine/3 を渡す）

out vec4 color;

// 従来方式の計算式をGLSLで再現したヘルパー関数
// サブピクセル座標を受け取り、左目用ならtrueを返す
bool shouldShowLeftEye(float W) {
    // --- 中心基準・左右対称の「1subスキップ」補正 ---
    float middleLineSub = middleLinePx * 3.0;          // ピクセル→サブピクセル
    float distSub = abs(W - middleLineSub);
    float skipCount = floor(distSub / haba);           // habaごとに1回スキップ
    float dir = (W >= middleLineSub) ? -1.0 : 1.0;     // 中心へ寄せる（左右対称）
    float skipSub = dir * skipCount;                   // 1回=1sub

    // ((W - (W - totalShift) / haba) + 2 * kk + SHIFT) + skipSub
    float value = (W - (W - totalShift) / haba)
                + (3.0 * float(timeStep))
                + manualShift
                + skipSub;

    return mod(value, 12.0) < 6.0;
}

void main() {
    vec4 leftColor = texture(leftTexture, UV);
    vec4 rightColor = texture(rightTexture, UV);

    vec4 finalColor;
    finalColor.a = 1.0;

    float subpixel_coord_x = gl_FragCoord.x * 3.0;

    float appliedCrosstalk = (enableColorCorrection != 0) ? crosstalk : 0.0;
    vec3 appliedGain = (enableColorCorrection != 0) ? rgbGain : vec3(1.0);
    float appliedGamma = (enableColorCorrection != 0) ? gammaValue : 1.0;
    float leakSafe = max(1.0 - appliedCrosstalk, 0.0001);

    float rPrimary;
    float rLeak;
    if (shouldShowLeftEye(subpixel_coord_x + 0.0)) {
        rPrimary = leftColor.r;
        rLeak = rightColor.r;
    } else {
        rPrimary = rightColor.r;
        rLeak = leftColor.r;
    }
    finalColor.r = clamp((rPrimary - appliedCrosstalk * rLeak) / leakSafe, 0.0, 1.0);

    float gPrimary;
    float gLeak;
    if (shouldShowLeftEye(subpixel_coord_x + 1.0)) {
        gPrimary = leftColor.g;
        gLeak = rightColor.g;
    } else {
        gPrimary = rightColor.g;
        gLeak = leftColor.g;
    }
    finalColor.g = clamp((gPrimary - appliedCrosstalk * gLeak) / leakSafe, 0.0, 1.0);

    float bPrimary;
    float bLeak;
    if (shouldShowLeftEye(subpixel_coord_x + 2.0)) {
        bPrimary = leftColor.b;
        bLeak = rightColor.b;
    } else {
        bPrimary = rightColor.b;
        bLeak = leftColor.b;
    }
    finalColor.b = clamp((bPrimary - appliedCrosstalk * bLeak) / leakSafe, 0.0, 1.0);

    finalColor.rgb = clamp(finalColor.rgb * appliedGain, 0.0, 1.0);
    finalColor.rgb = pow(finalColor.rgb, vec3(1.0 / max(appliedGamma, 0.01)));

    color = finalColor;
}