#version 330 core

in vec2 UV;

// シェーダーの表示パラメーター。
uniform sampler2D leftTexture;
uniform sampler2D rightTexture;

uniform float haba;         // バリア幅（サブピクセル単位）。
uniform float totalShift;   // 追跡位置から計算した表示位相の補正。
uniform int   timeStep;     // 時分割位相（0～3）。
uniform float manualShift;  // 手動の位相補正。
uniform vec3  rgbGain;      // RGBごとの明るさ補正。
uniform float gammaValue;   // ガンマ補正値。
uniform float crosstalk;    // クロストーク補正係数。
uniform int   enableColorCorrection; // 1で色補正を有効にする。
uniform float middleLinePx; // 中心線のピクセル位置。

// シェーダーの表示パラメーター。
uniform float slantY;       // 斜めバリアの縦方向係数。

uniform float correctionThreshold = 0.8; // Runtime threshold; compatible default.
uniform int leftEnabled;
uniform int rightEnabled;

uniform int showAlignmentCenter = 0;
uniform float alignmentCenterSub;
uniform float alignmentViewportHeight;

out vec4 color;

float positiveMod(float x, float m)
{
    return mod(mod(x, m) + m, m);
}

// Advance at the configured fractional magnitude (default 0.8).
// Apply the same threshold to negative values, then restore the sign.
float quantizeBarrierCorrection(float q)
{
    float magnitude = abs(q);
    float base = floor(magnitude);
    float rounded = (magnitude >= base + clamp(correctionThreshold, 0.05, 1.0)) ? base + 1.0 : base;
    return (q < 0.0) ? -rounded : rounded;
}

bool shouldShowLeftEye(float Wsub, float Ypx)
{
    // バリアと同じ斜め座標
    float S = Wsub + slantY * Ypx;

    // C++側で渡した totalShift = MiddleLine - 48 * haba を使う
    float q = (S - totalShift) / max(haba, 1.0);

    // 指定した境界値以上なら、次の整数へ進める。
    float correction = quantizeBarrierCorrection(q);

    float value = S
                - correction
                + 2.0 * float(timeStep)
                + manualShift;

    return positiveMod(value, 8.0) >= 4.0;
}
// Calibration reference S = MiddleLine, not an optical slit or eye boundary.
vec4 applyAlignmentCenterGuide(vec4 inputColor)
{
    if (showAlignmentCenter == 0) return inputColor;
    vec2 p = gl_FragCoord.xy;
    float reference = 3.0 * p.x + slantY * p.y - alignmentCenterSub;
    float normalLength = length(vec2(3.0, slantY));
    vec4 result = inputColor;
    if (abs(reference) <= normalLength)
        result = vec4(1.0, 0.0, 1.0, 1.0);
    float centerY = alignmentViewportHeight * 0.5;
    float centerX = (alignmentCenterSub - slantY * centerY) / 3.0;
    vec2 distanceToCenter = abs(p - vec2(centerX, centerY));
    if ((distanceToCenter.x <= 10.0 && distanceToCenter.y <= 1.0) ||
        (distanceToCenter.y <= 10.0 && distanceToCenter.x <= 1.0))
        result = vec4(1.0);
    return result;
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


    // Apply the eye mask to the final output so OFF is exactly black.
    float maskBase = floor(gl_FragCoord.x) * 3.0;
    float maskY = floor(gl_FragCoord.y);
    if ((shouldShowLeftEye(maskBase, maskY) ? leftEnabled : rightEnabled) == 0) finalColor.r = 0.0;
    if ((shouldShowLeftEye(maskBase + 1.0, maskY) ? leftEnabled : rightEnabled) == 0) finalColor.g = 0.0;
    if ((shouldShowLeftEye(maskBase + 2.0, maskY) ? leftEnabled : rightEnabled) == 0) finalColor.b = 0.0;

    color = applyAlignmentCenterGuide(finalColor);
}
