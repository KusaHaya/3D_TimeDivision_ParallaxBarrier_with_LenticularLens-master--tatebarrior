Shader "Parallax/BacklightMura"
{
    Properties { _MainTex("Unused", 2D)="white" {} }
    SubShader {
        Cull Off ZWrite Off ZTest Always
        Pass {
            CGPROGRAM
            #pragma vertex vert_img
            #pragma fragment frag
            #pragma target 3.0
            #include "UnityCG.cginc"
            #include "OpticalRays.cginc"

            float _Observer, _PupilDiameter, _Cycle, _Gain, _Output;
            float4 _Region;

            float imagePlaneCoord(float2 p, float3 eye)
            {
                float panel = p.x + _Slope * p.y;
                float eyeCoord = eye.x + _Slope * eye.y;
                float theta = (eyeCoord - panel) / eye.z;
                return panel - _Gap * theta;
            }

            bool imagePlaneVisible(float coordinate, int k)
            {
                float centre = _Offset + _Direction * k * _Step;
                float delta = coordinate - centre;
                delta -= _Period * floor(delta / _Period + .5);
                return abs(delta) <= _Width * .5;
            }

            float oneRay(float2 p, float3 eye, int k, bool diffuser)
            {
                if(diffuser) return imagePlaneVisible(imagePlaneCoord(p, eye), k) ? 1 : 0;
                return visible(sourceCoord(p, eye), k) ? 1 : 0;
            }

            float pupilBrightness(float2 p, float3 eye, int k, bool diffuser)
            {
                float r = _PupilDiameter * .5;
                float q = r * .70710678;
                float sum = 0;
                sum += oneRay(p, eye, k, diffuser);
                sum += oneRay(p, eye + float3( q, 0, 0), k, diffuser);
                sum += oneRay(p, eye + float3(-q, 0, 0), k, diffuser);
                sum += oneRay(p, eye + float3(0,  q, 0), k, diffuser);
                sum += oneRay(p, eye + float3(0, -q, 0), k, diffuser);
                sum += oneRay(p, eye + float3( q,  q, 0), k, diffuser);
                sum += oneRay(p, eye + float3(-q,  q, 0), k, diffuser);
                sum += oneRay(p, eye + float3( q, -q, 0), k, diffuser);
                sum += oneRay(p, eye + float3(-q, -q, 0), k, diffuser);
                return sum / 9.0;
            }

            float integratedBrightness(float2 p, float3 eye, bool diffuser)
            {
                float sum = 0;
                [unroll] for(int k = 0; k < 4; k++)
                {
                    if(_Cycle > .5 || k == (int)_Phase)
                        sum += pupilBrightness(p, eye, k, diffuser);
                }
                return sum / (_Cycle > .5 ? 4.0 : 1.0);
            }

            fixed4 frag(v2f_img i) : SV_Target
            {
                float2 p = (i.uv - .5) * _Region.zw + _Region.xy;
                float3 eye = _Observer < .5 ? _EyeL.xyz :
                    (_Observer > 1.5 ? _EyeR.xyz : (_EyeL.xyz + _EyeR.xyz) * .5);
                float direct = integratedBrightness(p, eye, false);
                float diffuser = integratedBrightness(p, eye, true);
                if(_Output > 1.5)
                {
                    float difference = (direct - diffuser) * _Gain;
                    float common = min(direct, diffuser) * .12;
                    float3 heat = common + float3(max(difference, 0), abs(difference) * .18,
                        max(-difference, 0));
                    return fixed4(LinearToGammaSpace(saturate(heat)), 1);
                }
                float value = (_Output < .5 ? direct : diffuser) * _Gain;
                return fixed4(LinearToGammaSpace(saturate(value.xxx)), 1);
            }
            ENDCG
        }
    }
}