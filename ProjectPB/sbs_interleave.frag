// advanced_parallax_sbs.frag

#version 330 core

in vec2 UV;

// C++から受け取るuniform変数
uniform sampler2D sbsTexture; // サイドバイサイド動画テクスチャ

uniform float haba;
uniform float totalShift;
uniform int   timeStep;
uniform float manualShift;

out vec4 color;

// 従来方式の計算式をGLSLで再現したヘルパー関数
// サブピクセル座標を受け取り、左目用ならtrueを返す
bool shouldShowLeftEye(float W) {
    float value = (W - (W - totalShift) / haba) + (2.0 * float(timeStep)) + manualShift;
    return mod(value, 12.0) < 6.0;
}

void main() {
    vec4 finalColor;
    finalColor.a = 1.0;

    float subpixel_coord_x = gl_FragCoord.x * 3.0;

    // --- R, G, Bの各成分ごとに、表示すべき視点とテクスチャ座標を決定 ---

    // 【R成分の決定】
    if (shouldShowLeftEye(subpixel_coord_x + 0.0)) {
        vec2 leftUV = vec2(UV.x * 0.5, UV.y); // テクスチャの左半分
        finalColor.r = texture(sbsTexture, leftUV).r;
    } else {
        vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y); // テクスチャの右半分
        finalColor.r = texture(sbsTexture, rightUV).r;
    }

    // 【G成分の決定】
    if (shouldShowLeftEye(subpixel_coord_x + 1.0)) {
        vec2 leftUV = vec2(UV.x * 0.5, UV.y);
        finalColor.g = texture(sbsTexture, leftUV).g;
    } else {
        vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);
        finalColor.g = texture(sbsTexture, rightUV).g;
    }

    // 【B成分の決定】
    if (shouldShowLeftEye(subpixel_coord_x + 2.0)) {
        vec2 leftUV = vec2(UV.x * 0.5, UV.y);
        finalColor.b = texture(sbsTexture, leftUV).b;
    } else {
        vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);
        finalColor.b = texture(sbsTexture, rightUV).b;
    }

    color = finalColor;
}