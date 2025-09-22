#pragma once
#include "Vector.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include "Media.h"
namespace vmlab
{
	class DrawVideo
	{
	public:
		DrawVideo();
		void dispose();
		void Initialize();
		void DrawLeftImage(int w, int h, float scale);
		void DrawRightImage(int w, int h, float scale);
		void DrawLeftImageLiver(int w, int h);
		void DrawRightImageLiver(int w, int h);

		void Reshape(int x, int y);
		void Keyboard(unsigned char key, int x, int y);
		void Special(int key, int x, int y);
		void Update(int totalMilliSeconds);
		void drawBlackCover();
		bool Mode3D = true;
		bool video_flag;
		bool printflag=true;
		void SetSpeed(int speed);
		GLuint getVideoTextureID() {
			if (m_Texture) {
				return m_Texture->GetID();
			}
			return 0;
		}

	private:
		int speed = 12000;
		bool m_end;
		std::thread m_updatethread;
		std::mutex m_mutex;
		// 	std::shared_ptr<Media> m_spMedia;
		std::shared_ptr<Texture> m_Texture;
		std::shared_ptr<Texture> m_TextureR;
	};
}