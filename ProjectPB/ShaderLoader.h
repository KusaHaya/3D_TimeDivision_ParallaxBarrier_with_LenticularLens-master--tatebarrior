#pragma once
#include <GL/glew.h>

// シェーダーファイルを読み込み、コンパイルしてリンクする。
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);
