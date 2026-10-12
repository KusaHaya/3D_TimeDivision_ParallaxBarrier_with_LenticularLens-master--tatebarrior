#ifndef PARALLAX_OPTICAL_RAYS
#define PARALLAX_OPTICAL_RAYS
float4 _EyeL, _EyeR, _Panel;
float _Gap, _Period, _Width, _Step, _Offset, _Slope, _Phase, _Direction;
float sourceCoord(float2 p, float3 e) {
    float r=_Gap/e.z;
    return (1+r)*(p.x+_Slope*p.y)-r*(e.x+_Slope*e.y);
}
bool visible(float s,int k) {
    float d=s-_Offset-_Direction*k*_Step;
    d-=_Period*floor(d/_Period+0.5);
    return abs(d)<=_Width*0.5;
}
int classify(float a,float b,int k) { return (visible(a,k)?1:0)+(visible(b,k)?2:0); }
#endif
