#include "Texture.h"
#include <memory>
namespace vmlab
{
	const int Texture::GetWidth()
	{
		return m_width;
	}

	const int Texture::GetHeight()
	{
		return m_height;
	}

	const GLuint Texture::GetID()
	{
		return m_ID;
	}

	Texture::Texture() :
		m_ID(0),
		m_width(0),
		m_height(0)
	{
	}

	Texture::~Texture()
	{
		if (!m_ID)
			glDeleteTextures(1, &m_ID);
	}

	const bool Texture::Load(LPCSTR file)
	{
		auto format = FreeImage_GetFileType(file);
		std::unique_ptr<FIBITMAP, decltype(&FreeImage_Unload)> imagen(FreeImage_Load(format, file), FreeImage_Unload);

		bool result(imagen.get());
		if (result)
		{
			imagen.reset(FreeImage_ConvertTo32Bits(imagen.get()));
			m_width = FreeImage_GetWidth(imagen.get());
			m_height = FreeImage_GetHeight(imagen.get());

			glGenTextures(1, &m_ID);
			glBindTexture(GL_TEXTURE_2D, m_ID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_BGRA,
				m_width,
				m_height,
				0,
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				FreeImage_GetBits(imagen.get())
				);
		}
		return result;
	}
	const bool Texture::Load(GLint internalFormat, int width, int height, GLenum format, BYTE *pData)
	{
		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);//  {Texture blends with object background}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);// { only first two can be used }
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);// { all of the above can be used }
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pData);

		if (!m_ID)
			return false;

		m_width = width;
		m_height = height;

		return true;
	}
	const bool Texture::ReloadSub(int xoffset, int yoffset, int width, int height, GLenum format, BYTE *pData)
	{
		if (!m_ID)
		{
			if (xoffset == 0 && yoffset == 0)
				return Load(format, width, height, format, pData);
			else
			{
				auto result = Load(format, xoffset + width, yoffset + height, format, pData);
				if (result)
					ReloadSub(xoffset, yoffset, width, height, format, pData);
			}
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_ID);
			glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, format, GL_UNSIGNED_BYTE, pData);
			return true;
		}
	}
}