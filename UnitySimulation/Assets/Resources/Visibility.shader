Shader "Parallax/Visibility"
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
            float4 _EyeL, _EyeR, _Region, _Panel;
            float _Gap, _Period, _Width, _Step, _Offset, _Slope, _Phase, _Mode, _Direction;
            float sourceCoord(float2 p, float3 e) {
                float r=_Gap/e.z;
                return (1+r)*(p.x+_Slope*p.y)-r*(e.x+_Slope*e.y);
            }
            bool visible(float s,int k) {
                float d=s-_Offset-_Direction*k*_Step;
                d-= _Period*floor(d/_Period+0.5);
                return abs(d)<=_Width*0.5;
            }
            int classify(float a,float b,int k) { return (visible(a,k)?1:0)+(visible(b,k)?2:0); }
            fixed4 frag(v2f_img i):SV_Target {
                float2 p=(i.uv-.5)*_Region.zw+_Region.xy;
                if(abs(p.x)>_Panel.x*.5 || abs(p.y)>_Panel.y*.5) return fixed4(.025,.03,.045,1);
                // Preview snaps to physical RGB subpixel centres. Full-panel display can alias;
                // the CPU scan is the authoritative centre-coverage result.
                float2 grid=_Panel.zw*float2(3,1);
                p=(floor((p/_Panel.xy+.5)*grid)+.5)/grid*_Panel.xy-_Panel.xy*.5;
                float a=sourceCoord(p,_EyeL.xyz), b=sourceCoord(p,_EyeR.xyz);
                int c=0;
                if(_Mode<.5) c=classify(a,b,(int)_Phase);
                else {
                    bool l=false,r=false;
                    for(int k=0;k<4;k++) { int v=classify(a,b,k); l=l||v==1; r=r||v==2; }
                    c=(l?1:0)+(r?2:0);
                }
                if(c==1) return fixed4(.22,.9,.5,1);
                if(c==2) return fixed4(.22,.52,1,1);
                if(c==3) return _Mode<.5?fixed4(1,.23,.63,1):fixed4(.85,.94,1,1);
                return fixed4(.045,.06,.09,1);
            }
            ENDCG
        }
    }
}
