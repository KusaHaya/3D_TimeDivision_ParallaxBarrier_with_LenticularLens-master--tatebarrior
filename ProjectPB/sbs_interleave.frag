#version 330 core



in vec2 UV;



uniform sampler2D sbsTexture;



// 旧方式と対応させるuniform

uniform float haba;         // サブピクセル単位

uniform float totalShift;   // C++側で MiddleLine - 48.0 * haba を渡す

uniform int   timeStep;     // 旧方式の kk

uniform float manualShift;  // 旧方式の SHIFT

uniform float slantY;       // 旧方式の W + H 相当なら 1.0



uniform float correctionThreshold = 0.8; // Runtime threshold; compatible default.
uniform int leftEnabled;
uniform int rightEnabled;

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

    // 旧方式の W + H に相当

    float W = Wsub + slantY * Ypx;



    // 旧方式の (W - totalShift) / haba

    float correction = quantizeBarrierCorrection((W - totalShift) / max(haba, 1.0));



    // 旧方式の ((W + H) - correction) + 2 * kk + SHIFT

    float value = W

                - correction

                + 2.0 * float(timeStep)

                + manualShift;



    // 旧方式では %8 < 4 がステンシル1

    // ステンシル1側には右目を描いていたので、

    // shaderで true=左目 にするなら >=4

    return positiveMod(value, 8.0) >= 4.0;

}



void main()

{

    vec2 leftUV  = vec2(UV.x * 0.5,       UV.y);

    vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);



    vec4 leftColor  = texture(sbsTexture, leftUV);

    vec4 rightColor = texture(sbsTexture, rightUV);



    vec4 finalColor;

    finalColor.a = 1.0;



    float Wsub = floor(gl_FragCoord.x) * 3.0;

    float Ypx  = floor(gl_FragCoord.y);



    bool rLeft = shouldShowLeftEye(Wsub + 0.0, Ypx);

    bool gLeft = shouldShowLeftEye(Wsub + 1.0, Ypx);

    bool bLeft = shouldShowLeftEye(Wsub + 2.0, Ypx);



    finalColor.r = rLeft ? leftColor.r : rightColor.r;

    finalColor.g = gLeft ? leftColor.g : rightColor.g;

    finalColor.b = bLeft ? leftColor.b : rightColor.b;




    // Apply the eye mask to the final output so OFF is exactly black.
    float maskBase = floor(gl_FragCoord.x) * 3.0;
    float maskY = floor(gl_FragCoord.y);
    if ((shouldShowLeftEye(maskBase, maskY) ? leftEnabled : rightEnabled) == 0) finalColor.r = 0.0;
    if ((shouldShowLeftEye(maskBase + 1.0, maskY) ? leftEnabled : rightEnabled) == 0) finalColor.g = 0.0;
    if ((shouldShowLeftEye(maskBase + 2.0, maskY) ? leftEnabled : rightEnabled) == 0) finalColor.b = 0.0;

    color = finalColor;

}