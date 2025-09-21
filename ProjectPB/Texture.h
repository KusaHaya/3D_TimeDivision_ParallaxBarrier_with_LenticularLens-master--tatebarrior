#pragma once

#include <iostream>
#include <Windows.h>
#include "FreeImage.h"
#include "GL/glew.h"
namespace vmlab
{
	class Texture
	{
	private:
		GLuint m_ID;
		int m_width;
		int m_height;

	public:
		const int GetWidth();
		const int GetHeight();
		const GLuint GetID();
		Texture();
		~Texture();
		const bool Load(LPCSTR file);
		const bool Load(GLint internalFormat, int width, int height, GLenum format, BYTE *pData);
		const bool ReloadSub(int xoffset, int yoffset, int width, int height, GLenum format, BYTE *pData);
	};
}