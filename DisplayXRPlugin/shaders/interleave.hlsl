cbuffer Params : register(b0) {
    float haba; float middle; float manualShift; float slant;
    float threshold; uint phase; uint outputHeight; uint mono;
    float2 atlasSize; float2 viewSize;
};
Texture2D<float4> atlas : register(t0);
SamplerState pointClamp : register(s0);
struct Vertex { float4 pos : SV_Position; float2 uv : TEXCOORD0; };
Vertex VS(uint id : SV_VertexID) {
    Vertex v;
    v.uv = float2((id << 1) & 2, id & 2);
    v.pos = float4(v.uv.x * 2 - 1, 1 - v.uv.y * 2, 0, 1);
    return v;
}
float quantizeCorrection(float q) {
    float magnitude = abs(q), base = floor(magnitude);
    float rounded = (magnitude >= base + clamp(threshold, .05, 1)) ? base + 1 : base;
    return q < 0 ? -rounded : rounded;
}
bool leftAt(float subpixel, float y) {
    float s = subpixel + slant * y;
    float q = (s - (middle - 48 * haba)) / max(haba, 1);
    float value = s - quantizeCorrection(q) + 2 * phase + manualShift;
    // GLSL mod is floor-based; HLSL fmod has different negative semantics.
    float positive = value - 8 * floor(value / 8);
    return positive >= 4;
}
float4 PS(Vertex v) : SV_Target {
    float2 pixel = floor(v.pos.xy);
    float2 localUV = saturate(v.uv);
    float2 texel = clamp(localUV * viewSize, .5, viewSize - .5);
    float4 l = atlas.SampleLevel(pointClamp, texel / atlasSize, 0);
    if (mono != 0) return float4(l.rgb, 1);
    float4 r = atlas.SampleLevel(pointClamp, (texel + float2(viewSize.x, 0)) / atlasSize, 0);
    float yGL = outputHeight - 1 - pixel.y;
    float xSub = pixel.x * 3;
    return float4(leftAt(xSub, yGL) ? l.r : r.r,
                  leftAt(xSub + 1, yGL) ? l.g : r.g,
                  leftAt(xSub + 2, yGL) ? l.b : r.b, 1);
}
