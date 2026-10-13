#ifndef PARALLAX_OPTICAL_RAYS
#define PARALLAX_OPTICAL_RAYS
float4 _EyeL, _EyeR, _Panel;
float _Gap, _Period, _Width, _Step, _Offset, _Slope, _Phase, _Direction;
// Shared by visibility and BOTH received-eye passes. Distances referenced to convex vertex.
float _LensDistance, _ObjectDistance, _LensPitch, _LensFocal, _LedWidth, _LedPitch, _LedGroups;
float sourceCoord(float2 p, float3 e) {
    float panel=p.x+_Slope*p.y;
    float theta=(e.x+_Slope*e.y-panel)/e.z;
    float exit=panel-_LensDistance*theta;
    float local=exit-_LensPitch*floor(exit/_LensPitch+.5);
    return exit-_ObjectDistance*(theta+local/_LensFocal);
}
bool visible(float s,int k) {
    float group=floor((s-k*_LedPitch)/(4*_LedPitch)+.5);
    return abs(group)<=_LedGroups && abs(s-(k+4*group)*_LedPitch)<=_LedWidth*.5;
}
int classify(float a,float b,int k) { return (visible(a,k)?1:0)+(visible(b,k)?2:0); }
#endif
