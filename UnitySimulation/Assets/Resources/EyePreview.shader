Shader "Parallax/EyePreview"
{
    Properties {
        _MainTex("Unused",2D)="white" {}
        _LeftImage("Left view",2D)="white" {}
        _RightImage("Right view",2D)="white" {}
    }
    SubShader { Cull Off ZWrite Off ZTest Always
        Pass {
            CGPROGRAM
            #pragma vertex vert_img
            #pragma fragment frag
            #pragma target 3.0
            #include "UnityCG.cginc"
            #include "OpticalRays.cginc"
            sampler2D _LeftImage, _RightImage;
            // Each texture is a single physical LCD drive shared by BOTH eyes.
            // Stored in linear light, one texture for each illumination phase.
            sampler2D _Drive0, _Drive1, _Drive2, _Drive3;
            float4 _TargetL, _TargetR;
            // 0/1=received eyes, 2=linear LCD drive, 3/4=inputs, 5=cached LCD, 6=live LCD.
            float _AllocationMode, _LegacyHaba, _LegacyMiddle, _LegacyShift, _LegacySlant;
            float _Output, _Pattern, _HasLeft, _HasRight, _Conflict, _Cycle, _Gain;
            float _SwapEyes;
            float3 inputColor(float2 uv,bool right) {
                if(_SwapEyes>.5)right=!right;
                if(_Pattern>0.5 && _Pattern<1.5) return right?0:1;
                if(_Pattern>1.5 && _Pattern<2.5) return right?1:0;
                if(_Pattern>2.5) return 1;
                if(right && _HasRight>.5) return GammaToLinearSpace(tex2D(_RightImage,uv).rgb);
                if(!right && _HasLeft>.5) return GammaToLinearSpace(tex2D(_LeftImage,uv).rgb);
                // Distinct fallback targets with opposite disparity and L/R block glyphs.
                float2 p=uv-float2(right?.54:.46,.5);
                float checker=fmod(floor(uv.x*16)+floor(uv.y*9),2);
                float3 c=lerp(float3(.04,.04,.04),float3(.14,.14,.14),checker);
                bool disk=length(p*float2(1.77778,1))<.25;
                if(disk) c=right?float3(.05,.45,1):float3(1,.2,.04);
                float2 q=(p+float2(.07,.13))/float2(.14,.26);
                bool stroke=false;
                if(all(q>=0) && all(q<=1)) {
                    if(!right) stroke=q.x<.22 || q.y<.16;
                    else stroke=q.x<.20 || q.y>.84 || (q.y>.48 && q.y<.64)
                        || (q.x>.78 && q.y>.5) || (q.y<.5 && abs(q.x-(1-q.y))<.12);
                }
                if(stroke) c=1;
                return c; // generated targets are linear intensities
            }
            float drive(int c,float l,float r) {
                if(c==1) return l;
                if(c==2) return r;
                if(c!=3 || _Conflict<.5) return 0;
                if(_Conflict<1.5) return .5*(l+r);
                return l;
            }
            bool legacyLeft(float2 pixel,int channel,int phase) {
                // GLSL gl_FragCoord: bottom-left origin, integer RGB indices.
                float S=pixel.x*3+channel+_LegacySlant*pixel.y;
                float q=(S-(_LegacyMiddle-48*_LegacyHaba))/max(_LegacyHaba,1.0);
                float correction=_AllocationMode>1.5?(q>=0?floor(q):ceil(q)):q;
                float value=S-correction+2*phase+_LegacyShift;
                return value-8*floor(value/8)>=4;
            }
            float3 storedDrive(float2 uv,int k) {
                float3 value=tex2D(_Drive0,uv).rgb;
                if(k==1) value=tex2D(_Drive1,uv).rgb;
                if(k==2) value=tex2D(_Drive2,uv).rgb;
                if(k==3) value=tex2D(_Drive3,uv).rgb;
                return value;
            }
            float4 frag(v2f_img i):SV_Target {
                float2 pixel=min(floor(i.uv*_Panel.zw),_Panel.zw-1);
                float2 uv=(pixel+.5)/_Panel.zw;
                float3 l=inputColor(uv,false),r=inputColor(uv,true);
                if(_Output>4.5 && _Output<5.5) return float4(LinearToGammaSpace(storedDrive(uv,(int)_Phase)),1);
                if(_Output>2.5 && _Output<4.5) return float4(LinearToGammaSpace(_Output<3.5?l:r),1);
                float3 result=0;
                [unroll] for(int channel=0;channel<3;channel++) {
                    float2 subuv=(pixel+float2((channel+.5)/3.0,.5))/_Panel.zw;
                    float2 p=(subuv-.5)*_Panel.xy;
                    float a=sourceCoord(p,_TargetL.xyz), b=sourceCoord(p,_TargetR.xyz);
                    if(_Output>1.5) {
                        if(_AllocationMode>.5)
                            result[channel]=legacyLeft(pixel,channel,(int)_Phase)?l[channel]:r[channel];
                        else result[channel]=drive(classify(a,b,(int)_Phase),l[channel],r[channel]);
                    } else {
                    float seen=sourceCoord(p,_Output<.5?_EyeL.xyz:_EyeR.xyz);
                    float sum=0;
                    [unroll] for(int k=0;k<4;k++) {
                        bool include=_Cycle<.5?k==(int)_Phase:true;
                        if(include) {
                            float3 stored=storedDrive(uv,k);
                            float d=stored[channel];
                            sum+=visible(seen,k)?d:0;
                        }
                    }
                    float divisor=_Cycle>.5?4:1;
                    result[channel]=sum*_Gain/divisor;
                    }
                }
                if(_Output>5.5) return float4(LinearToGammaSpace(result),1); // native hardware output, no preview gain
                if(_Output>1.5) return float4(result,1); // linear-light drive buffer
                return float4(LinearToGammaSpace(saturate(result)),1);
            }
            ENDCG
        }
    }
}
