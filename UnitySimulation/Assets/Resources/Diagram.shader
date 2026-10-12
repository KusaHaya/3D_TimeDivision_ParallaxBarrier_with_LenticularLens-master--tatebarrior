Shader "Parallax/Diagram" {
    Properties { _Color("Color",Color)=(1,1,1,1) }
    SubShader { Tags { "Queue"="Transparent" } Blend SrcAlpha OneMinusSrcAlpha Cull Off ZWrite Off
        Pass { CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            float4 _Color;
            struct output { float4 pos:SV_POSITION; };
            output vert(float4 p:POSITION) { output o;o.pos=UnityObjectToClipPos(p);return o; }
            fixed4 frag(output i):SV_Target { return _Color; }
        ENDCG }
    }
}
