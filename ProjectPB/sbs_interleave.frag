// advanced_parallax_sbs.frag

#version 330 core

in vec2 UV;

// C++??????uniform??
uniform sampler2D sbsTexture; // ???????????????

uniform float haba;
uniform float totalShift;
uniform int   timeStep;
uniform float manualShift;
uniform vec3  rgbGain;
uniform float gammaValue;
uniform float crosstalk;
uniform int   enableColorCorrection;

// ???MiddleLine????
uniform float middleLinePx; // C++?? MiddleLine/3 ???

out vec4 color;

// ?????????GLSL???
bool shouldShowLeftEye(float W) {
    float middleLineSub = middleLinePx * 3.0;
    float distSub = abs(W - middleLineSub);
    float skipCount = floor(distSub / haba);
    float dir = (W >= middleLineSub) ? -1.0 : 1.0;
    float skipSub = dir * skipCount;

    float value = (W - (W - totalShift) / haba)
                + (3.0 * float(timeStep))
                + manualShift
                + skipSub;

    return mod(value, 12.0) < 6.0;
}

void main() {
    vec4 finalColor;
    finalColor.a = 1.0;

    float subpixel_coord_x = gl_FragCoord.x * 3.0;
    float appliedCrosstalk = (enableColorCorrection != 0) ? crosstalk : 0.0;
    vec3 appliedGain = (enableColorCorrection != 0) ? rgbGain : vec3(1.0);
    float appliedGamma = (enableColorCorrection != 0) ? gammaValue : 1.0;
    float leakSafe = max(1.0 - appliedCrosstalk, 0.0001);

    vec2 leftUV = vec2(UV.x * 0.5, UV.y);
    vec2 rightUV = vec2(UV.x * 0.5 + 0.5, UV.y);
    vec3 leftColor = texture(sbsTexture, leftUV).rgb;
    vec3 rightColor = texture(sbsTexture, rightUV).rgb;

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