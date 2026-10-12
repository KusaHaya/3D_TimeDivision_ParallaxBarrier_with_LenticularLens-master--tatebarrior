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
            float4 _TargetL, _TargetR;
            // Output: 0=left received,1=right received,2=LCD phase,3=left input,4=right input.
            float _Output, _Pattern, _HasLeft, _HasRight, _Conflict, _Cycle, _Gain;
            float3 inputColor(float2 uv,bool right) {
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
            float4 frag(v2f_img i):SV_Target {
                float2 pixel=min(floor(i.uv*_Panel.zw),_Panel.zw-1);
                float2 uv=(pixel+.5)/_Panel.zw;
                float3 l=inputColor(uv,false),r=inputColor(uv,true);
                if(_Output>2.5) return float4(LinearToGammaSpace(_Output<3.5?l:r),1);
                float3 result=0;
                [unroll] for(int channel=0;channel<3;channel++) {
                    float2 subuv=(pixel+float2((channel+.5)/3.0,.5))/_Panel.zw;
                    float2 p=(subuv-.5)*_Panel.xy;
                    float a=sourceCoord(p,_TargetL.xyz), b=sourceCoord(p,_TargetR.xyz);
                    float seen=sourceCoord(p,_Output<.5?_EyeL.xyz:_EyeR.xyz);
                    float sum=0;
                    [unroll] for(int k=0;k<4;k++) {
                        bool include=(_Output>1.5 || _Cycle<.5)?k==(int)_Phase:true;
                        if(include) {
                            float d=drive(classify(a,b,k),l[channel],r[channel]);
                            sum+=(_Output>1.5 || visible(seen,k))?d:0;
                        }
                    }
                    float gain=_Output>1.5?1:_Gain;
                    float divisor=(_Output<1.5 && _Cycle>.5)?4:1;
                    result[channel]=sum*gain/divisor;
                }
                return float4(LinearToGammaSpace(saturate(result)),1);
            }
            ENDCG
        }
    }
}
