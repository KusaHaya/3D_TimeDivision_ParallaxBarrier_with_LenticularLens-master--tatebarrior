#version 330 core

in vec2 UV;

// 受け取るテクスチャは1枚だけにする
uniform sampler2D displayTexture;

out vec4 color;

void main(){
    // 受け取ったテクスチャの色をそのまま出力する
    color = texture(displayTexture, UV);
}