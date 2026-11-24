#version 330 core

// 静止画（左右2枚のテクスチャ）と動画（左右分割の1枚）で共通の合成シェーダー。
// 以前の interleave.frag と sbs_interleave.frag を1つにまとめた。

in vec2 UV;

uniform sampler2D leftTexture;  // 静止画: 左目画像 / 動画: 左右分割テクスチャ
uniform sampler2D rightTexture; // 静止画: 右目画像 / 動画: 未使用
uniform int   sideBySide = 0;   // 1: leftTexture の左半分を左目、右半分を右目として使う

uniform float haba;         // バリア幅（サブピクセル単位）。
uniform float totalShift;   // C++側で MiddleLine - 48 * haba を渡す。
uniform int   timeStep;     // 時分割位相（0～3）。旧方式の kk。
uniform float manualShift;  // 手動の位相補正。旧方式の SHIFT。
uniform float slantY;       // 斜めバリアの縦方向係数。旧方式の W + H 相当なら 1.0。

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

// true なら左目、false なら右目
bool shouldShowLeftEye(float Wsub, float Ypx)
{
    // バリアと同じ斜め座標（旧方式の W + H に相当）
    float S = Wsub + slantY * Ypx;

    // 旧方式の (W - totalShift) / haba。指定した境界値以上なら次の整数へ進める。
    float correction = quantizeBarrierCorrection((S - totalShift) / max(haba, 1.0));

    // 旧方式の ((W + H) - correction) + 2 * kk + SHIFT
    float value = S
                - correction
                + 2.0 * float(timeStep)
                + manualShift;

    // 旧方式では %8 < 4 がステンシル1（右目）だったので、>= 4 を左目とする。
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
        result = vec4(1.0, 0.0, 0.0, 1.0);
    return result;
}

void main()
{
    vec4 leftColor;
    vec4 rightColor;
    if (sideBySide != 0) {
        leftColor  = texture(leftTexture, vec2(UV.x * 0.5,       UV.y));
        rightColor = texture(leftTexture, vec2(UV.x * 0.5 + 0.5, UV.y));
    }
    else {
        leftColor  = texture(leftTexture, UV);
        rightColor = texture(rightTexture, UV);
    }

    vec4 finalColor;
    finalColor.a = 1.0;

    float Wsub = floor(gl_FragCoord.x) * 3.0;
    float Ypx  = floor(gl_FragCoord.y);

    // RGBのサブピクセルごとに左右どちらを表示するかを決める。
    bool rLeft = shouldShowLeftEye(Wsub + 0.0, Ypx);
    bool gLeft = shouldShowLeftEye(Wsub + 1.0, Ypx);
    bool bLeft = shouldShowLeftEye(Wsub + 2.0, Ypx);

    finalColor.r = rLeft ? leftColor.r : rightColor.r;
    finalColor.g = gLeft ? leftColor.g : rightColor.g;
    finalColor.b = bLeft ? leftColor.b : rightColor.b;

    // Apply the eye mask to the final output so OFF is exactly black.
    if ((rLeft ? leftEnabled : rightEnabled) == 0) finalColor.r = 0.0;
    if ((gLeft ? leftEnabled : rightEnabled) == 0) finalColor.g = 0.0;
    if ((bLeft ? leftEnabled : rightEnabled) == 0) finalColor.b = 0.0;

    color = applyAlignmentCenterGuide(finalColor);
}
