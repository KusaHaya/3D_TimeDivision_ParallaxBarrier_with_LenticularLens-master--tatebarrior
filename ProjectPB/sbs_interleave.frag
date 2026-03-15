// advanced_parallax_sbs.frag

#version 330 core

in vec2 UV;

// C++から受け取るuniform変数
uniform sampler2D sbsTexture; // サイドバイサイド動画テクスチャ

uniform float haba;
uniform float totalShift;
uniform int   timeStep;
uniform float manualShift;

// ★追加
uniform float middleLinePx; // ピクセル単位（C++側で MiddleLine/3 を渡す）

out vec4 color;

// 従来方式の計算式をGLSLで再現したヘルパー関数
bool shouldShowLeftEye(float W) {
    // --- 中心基準・左右対称の「1subスキップ」補正 ---
    float middleLineSub = middleLinePx * 3.0;
    float distSub = abs(W - middleLineSub);
    float skipCount = floor(distSub / haba);
    float dir = (W >= middleLineSub) ? -1.0 : 1.0;
    float skipSub = dir * skipCount;

    float value = (W - (W - totalShift) / haba)
                + (2.0 * float(timeStep))
                + manualShift
                + skipSub;

    return mod(value, 12.0) < 6.0;
}

void main() {
    vec4 finalColor;
    finalColor.a = 1.0;

    float subpixel_coord_x = gl_FragCoord.x * 3.0;

    // 【R】
    if (shouldShowLeftEye(subpixel_coord_x + 0.0)) {
        vec2 leftUV = vec2(UV.x * 0.5, UV.y);
        finalColor.r = texture(sbsTexture, leftUV).r;
    } else {
        vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);
        finalColor.r = texture(sbsTexture, rightUV).r;
    }

    // 【G】
    if (shouldShowLeftEye(subpixel_coord_x + 1.0)) {
        vec2 leftUV = vec2(UV.x * 0.5, UV.y);
        finalColor.g = texture(sbsTexture, leftUV).g;
    } else {
        vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);
        finalColor.g = texture(sbsTexture, rightUV).g;
    }

    // 【B】
    if (shouldShowLeftEye(subpixel_coord_x + 2.0)) {
        vec2 leftUV = vec2(UV.x * 0.5, UV.y);
        finalColor.b = texture(sbsTexture, leftUV).b;
    } else {
        vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);
        finalColor.b = texture(sbsTexture, rightUV).b;
    }

    color = finalColor;
}