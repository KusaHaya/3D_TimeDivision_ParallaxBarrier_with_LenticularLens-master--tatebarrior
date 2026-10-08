#version 330 core

in vec2 UV;

// C++?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｯ趣ｿｽ?ｿｽuniform?ｿｽﾏ撰ｿｽ
uniform sampler2D leftTexture;
uniform sampler2D rightTexture;

uniform float haba;         // ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ(Z)?ｿｽ?ｿｽ?ｿｽ?ｿｽv?ｿｽZ?ｿｽ?ｿｽ?ｿｽ黷ｽ?ｿｽl?ｿｽi?ｿｽT?ｿｽu?ｿｽs?ｿｽN?ｿｽZ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽj
uniform float totalShift;   // ?ｿｽ]?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽﾌ基準?ｿｽV?ｿｽt?ｿｽg
uniform int   timeStep;     // ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽﾌス?ｿｽe?ｿｽb?ｿｽv (kk)
uniform float manualShift;  // ?ｿｽ闢ｮ?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽp?ｿｽﾌシ?ｿｽt?ｿｽg (SHIFT)
uniform vec3  rgbGain;      // RGB?ｿｽQ?ｿｽC?ｿｽ?ｿｽ?ｿｽ竦ｳ
uniform float gammaValue;   // ?ｿｽK?ｿｽ?ｿｽ?ｿｽ}?ｿｽ竦ｳ
uniform float crosstalk;    // ?ｿｽN?ｿｽ?ｿｽ?ｿｽX?ｿｽg?ｿｽ[?ｿｽN?ｿｽ竢橸ｿｽ?ｿｽ
uniform int   enableColorCorrection; // 1:?ｿｽL?ｿｽ?ｿｽ, 0:?ｿｽ?ｿｽ?ｿｽ?ｿｽ
uniform float middleLinePx; // ?ｿｽs?ｿｽN?ｿｽZ?ｿｽ?ｿｽ?ｿｽP?ｿｽﾊ（C++?ｿｽ?ｿｽ?ｿｽ?ｿｽ MiddleLine/3 ?ｿｽ?ｿｽn?ｿｽ?ｿｽ?ｿｽj

// ?ｿｽﾇ会ｿｽ: ?ｿｽp?ｿｽx?ｿｽ?ｿｽ?ｿｽ?ｿｽ?ｿｽp
uniform float slantY;       // 1.0 ?ｿｽ?ｿｽ tan^-1(-3)

out vec4 color;

float positiveMod(float x, float m)
{
    return mod(mod(x, m) + m, m);
}

bool shouldShowLeftEye(float Wsub, float Ypx)
{
    // バリアと同じ斜め座標
    float S = Wsub + slantY * Ypx;

    // C++側で渡した totalShift = MiddleLine - 48 * haba を使う
    float q = (S - totalShift) / max(haba, 1.0);

    // floorを使わず、連続的な幅補正にする
    float correction = q;

    float value = S
                - correction
                + 2.0 * float(timeStep)
                + manualShift;

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