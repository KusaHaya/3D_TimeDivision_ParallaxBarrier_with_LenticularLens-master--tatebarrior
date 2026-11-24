#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include "ShaderLoader.h"

namespace {

bool ReadTextFile(const char* path, std::string& text)
{
	std::ifstream stream(path, std::ios::in);
	if (!stream.is_open()) {
		printf("[ERROR] Failed to open shader file: %s\n", path);
		return false;
	}
	std::stringstream sstr;
	sstr << stream.rdbuf();
	text = sstr.str();
	return true;
}

// シェーダーを1つコンパイルする。失敗したらログを表示して0を返す。
GLuint CompileShader(GLenum type, const std::string& source, const char* path)
{
	printf("Compiling shader : %s\n", path);
	GLuint shader = glCreateShader(type);
	const char* sourcePointer = source.c_str();
	glShaderSource(shader, 1, &sourcePointer, NULL);
	glCompileShader(shader);

	GLint result = GL_FALSE;
	int infoLogLength = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 1) {
		std::vector<char> message(infoLogLength + 1);
		glGetShaderInfoLog(shader, infoLogLength, NULL, &message[0]);
		printf("%s\n", &message[0]);
	}
	if (result != GL_TRUE) {
		printf("[ERROR] Shader compilation failed: %s\n", path);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

}

// シェーダーファイルを読み込み、コンパイルしてリンクする。
// ファイルがない・コンパイルやリンクに失敗した場合は、理由を表示して0を返す。
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
	std::string vertexCode, fragmentCode;
	if (!ReadTextFile(vertex_file_path, vertexCode)) return 0;
	if (!ReadTextFile(fragment_file_path, fragmentCode)) return 0;

	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode, vertex_file_path);
	if (vertexShader == 0) return 0;
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode, fragment_file_path);
	if (fragmentShader == 0) {
		glDeleteShader(vertexShader);
		return 0;
	}

	// シェーダープログラムを作成してリンクする。
	printf("Linking program\n");
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint result = GL_FALSE;
	int infoLogLength = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 1) {
		std::vector<char> message(infoLogLength + 1);
		glGetProgramInfoLog(program, infoLogLength, NULL, &message[0]);
		printf("%s\n", &message[0]);
	}

	// リンク後に個々のシェーダーオブジェクトを解放する。
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	if (result != GL_TRUE) {
		printf("[ERROR] Shader link failed: %s + %s\n", vertex_file_path, fragment_file_path);
		glDeleteProgram(program);
		return 0;
	}
	return program;
}
