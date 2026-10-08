#version 330 core

// C++側から受け取る頂点属性 (位置とテクスチャUV座標)
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;

// フラグメントシェーダーに渡すための変数
out vec2 UV;

void main(){
    // 頂点位置は変更せずにそのまま出力
    gl_Position = vec4(vertexPosition, 1.0);
    
    // UV座標も変更せずにそのまま出力
    UV = vertexUV;
}