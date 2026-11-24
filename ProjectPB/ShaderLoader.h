#pragma once
#include <GL/glew.h>

// シェーダーファイルを読み込み、コンパイルしてリンクする。
// 失敗したら理由を表示して0を返す。
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);
