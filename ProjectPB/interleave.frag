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
uniform float crosstalk;    // クロストーク補償量
uniform int   enableColorCorrection; // 1:有効, 0:無効
uniform float middleLinePx; // ピクセル単位（C++側で MiddleLine/3 を渡す）

// 追加: 角度調整用
uniform float slantY;       // 1.0 で tan^-1(-3)

out vec4 color;

float positiveMod(float x, float m)
{
    return mod(mod(x, m) + m, m);
}

bool shouldShowLeftEye(float Wsub, float Ypx)
{
    float middleLineSub = middleLinePx * 3.0;

    // middleLine補正
    float distSub = abs(Wsub - middleLineSub);
    float skipCount = floor(distSub / haba);
    float dir = (Wsub >= middleLineSub) ? -1.0 : 1.0;
    float skipSub = dir * skipCount;

    // 右肩下がり
    // slantY = 1.0 のとき tan^-1(-3)
    float slantedCoord = Wsub + slantY * Ypx;

    float divTerm = floor((Wsub - totalShift) / haba);

    float value = slantedCoord
                - divTerm
                + 2.0 * float(timeStep)
                + manualShift
                + skipSub;

    // 最初の2相(0,1)を右、後ろの2相(2,3)を左
    return positiveMod(value, 8.0) >= 4.0;
}

void main()
{
    vec4 leftColor = texture(leftTexture, UV);
    vec4 rightColor = texture(rightTexture, UV);

    vec4 finalColor;
    finalColor.a = 1.0;

    float WsubBase = floor(gl_FragCoord.x) * 3.0;
    float Ypx      = floor(gl_FragCoord.y);

    float appliedCrosstalk = (enableColorCorrection != 0) ? crosstalk : 0.0;
    vec3 appliedGain = (enableColorCorrection != 0) ? rgbGain : vec3(1.0);
    float appliedGamma = (enableColorCorrection != 0) ? gammaValue : 1.0;
    float leakSafe = max(1.0 - appliedCrosstalk, 0.0001);

    float rPrimary, rLeak;
    if (shouldShowLeftEye(WsubBase + 0.0, Ypx)) {
        rPrimary = leftColor.r;
        rLeak    = rightColor.r;
    } else {
        rPrimary = rightColor.r;
        rLeak    = leftColor.r;
    }
    finalColor.r = clamp((rPrimary - appliedCrosstalk * rLeak) / leakSafe, 0.0, 1.0);

    float gPrimary, gLeak;
    if (shouldShowLeftEye(WsubBase + 1.0, Ypx)) {
        gPrimary = leftColor.g;
        gLeak    = rightColor.g;
    } else {
        gPrimary = rightColor.g;
        gLeak    = leftColor.g;
    }
    finalColor.g = clamp((gPrimary - appliedCrosstalk * gLeak) / leakSafe, 0.0, 1.0);

    float bPrimary, bLeak;
    if (shouldShowLeftEye(WsubBase + 2.0, Ypx)) {
        bPrimary = leftColor.b;
        bLeak    = rightColor.b;
    } else {
        bPrimary = rightColor.b;
        bLeak    = leftColor.b;
    }
    finalColor.b = clamp((bPrimary - appliedCrosstalk * bLeak) / leakSafe, 0.0, 1.0);

    finalColor.rgb = clamp(finalColor.rgb * appliedGain, 0.0, 1.0);
    finalColor.rgb = pow(finalColor.rgb, vec3(1.0 / max(appliedGamma, 0.01)));

    color = finalColor;
}