#pragma once
// 左右画像をサブピクセル単位の斜めバリアで合成するシェーダー描画。
// interleave.frag（静止画）と sbs_interleave.frag（左右分割動画）を使う。
#include <GL/glew.h>

// シェーダーを読み込み、uniform位置と全画面四角形を準備する。
void InitInterleaveRenderer();
// 左右の静止画をサブピクセル単位で交互に描画する。
void renderInterleavedImage();
// 左右分割動画テクスチャをサブピクセル単位で交互に描画する。
void renderInterleavedVideo(GLuint videoTexID);
