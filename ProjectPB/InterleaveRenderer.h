#pragma once
// 左右画像をサブピクセル単位の斜めバリアで合成するシェーダー描画。
// 静止画と左右分割動画は同じ interleave.frag を使う。
#include <GL/glew.h>

// シェーダーを読み込み、uniform位置と全画面四角形を準備する。
// シェーダーの読み込みに失敗したら false を返す。
bool InitInterleaveRenderer();
// 左右の静止画（imageL / imageR）をサブピクセル単位で交互に描画する。
void renderInterleavedImage();
// 左右分割動画テクスチャをサブピクセル単位で交互に描画する。
void renderInterleavedVideo(GLuint videoTexID);
