#include <filesystem>
#include "DrawVideo.h"
#include "Viewport.hpp"
#include "Setting.h"
#include"Timer.h"
#define S_UP		0.0f
#define S_DOWN		1010.0f
#define S_LEFT		60.0f
#define S_RIGHT		1020.0f
#define S_WIDTH		960
#define S_HEIGHT	1010
namespace vmlab
{
	
	bool audio_flag;

	Timer tA,tB;
	int next = 16;
	const int width = 1920;
	const int height = 1080;
	const int aspect = 1080 * width / height;

	std::unique_ptr<Capture>	m_capture(new Capture());
	bool Started;

	DrawVideo::DrawVideo() :

		// 	m_spMedia(new Media()),
		m_Texture(nullptr)
	{
		m_Texture = std::shared_ptr<vmlab::Texture>(new Texture());
	}
	void DrawVideo::dispose()
	{
		m_end = true;
		if(Started) m_updatethread.join();
		else video_flag = true;
		//m_capture = nullptr;
		//m_Texture = nullptr;
	}

	void DrawVideo::Initialize()
	{
		if (m_capture->FindDevice() < 0)
		{
			m_capture->~Capture();
		}
		else
		{
			m_capture->OpenComponent();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			m_end = false;
			Started = true;

			m_updatethread = std::thread([&]()
			{
				while (!m_end)
				{
					m_capture->CreateBitmap();
					std::this_thread::sleep_for(std::chrono::microseconds(speed));
				}
				video_flag = true;
			});
		}
	}

	void DrawVideo::SetSpeed(int m_speed)
	{
		speed = m_speed;
	}
	int S = 1080;
	void DrawVideo::DrawLeftImage(int w, int h, float scale)
	{

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDisable(GL_DEPTH_TEST);
		float tmax = scale / 2.0f + 0.50f;
		float tmin = 0.50f - scale / 2.0f;
		//tmax = 1.0f;
		//tmin = 0.0f;
		glBindTexture(GL_TEXTURE_2D, m_Texture->GetID());
		{
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);


			glTexCoord2f(tmin, tmax); glVertex3d(0, 0, 0);
			glTexCoord2f(tmin, tmin);		glVertex3d(0, h, 0);
			glTexCoord2f(0.50f, tmin);		glVertex3d(w, h, 0);
			glTexCoord2f(0.50f, tmax);	glVertex3d(w, 0, 0);

			glEnd();
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		//glFlush();
		//glClear(GL_COLOR_BUFFER_BIT);
	}


	void DrawVideo::DrawRightImage(int w, int h, float scale)
	{

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDisable(GL_DEPTH_TEST);
		float tmax = scale / 2.0f + 0.50f;
		float tmin = 0.50f - scale / 2.0f;
		//tmax = 1.0f;
		//tmin = 0.0f;
		glBindTexture(GL_TEXTURE_2D, m_Texture->GetID());
		{
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);


			glTexCoord2d(0.50f, tmax);	glVertex3d(0, 0, 0);
			glTexCoord2d(0.50f, tmin);		glVertex3d(0, h, 0);
			glTexCoord2d(tmax, tmin);		glVertex3d(w, h, 0);
			glTexCoord2d(tmax, tmax);	glVertex3d(w, 0, 0);

			glEnd();
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		//glFlush();
		//glClear(GL_COLOR_BUFFER_BIT);
	}

	void DrawVideo::DrawLeftImageLiver(int w, int h)
	{

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDisable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, m_Texture->GetID());
		{
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);


			glTexCoord2f(0.00f, 1.00f); glVertex3d(-100, -37, 0);
			glTexCoord2f(0.00f, 0.00f);	glVertex3d(-100, 1124, 0);
			glTexCoord2f(0.175f, 0.00f);	glVertex3d(268.2, 1124, 0);/*284 / w, 0.00f*/
			glTexCoord2f(0.175f, 1.00f);	glVertex3d(268.2, -37, 0);/*284 / w, 1.00f*/
			glEnd();



			glBegin(GL_QUADS);

			glTexCoord2f(0.175f, 0.118f); glVertex3d(268.2, 987, 0);/*284 / w, 95 / h*/
			glTexCoord2f(0.175f, 0.00f);		glVertex3d(268.2, 1124, 0);/*284 / w, 0.00f*/
			glTexCoord2f(1.00f, 0.00f);		glVertex3d(2004, 1124, 0);
			glTexCoord2f(1.00f, 0.118f);	glVertex3d(2004, 987, 0);/*1.00f, 95 / h*/
			glEnd();

			glBegin(GL_QUADS);

			glTexCoord2f(0.175f, 1.00f); glVertex3d(268.2 + (-26.6), -37, 0);/*284 / w, 1.00f*/
			glTexCoord2f(0.175f, 0.118f);		glVertex3d(268.2 + (-26.6), 987, 0);/*284 / w, 95 / h*/
			glTexCoord2f(0.5662f, 0.118f);		glVertex3d(2004 + (-26.6), 987, 0);/*1102 / w, 95 / h*/
			glTexCoord2f(0.5662f, 1.00f);	glVertex3d(2004 + (-26.6), -37, 0);/*1102 / w, 1.00f*/
			glEnd();
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		//glFlush();
		//glClear(GL_COLOR_BUFFER_BIT);
	}


	void DrawVideo::DrawRightImageLiver(int w, int h)
	{

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDisable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, m_Texture->GetID());
		{
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);

			glTexCoord2f(0.00f, 1.00f); glVertex3d(-100, -37, 0);
			glTexCoord2f(0.00f, 0.00f);	glVertex3d(-100, 1124, 0);
			glTexCoord2f(0.175f, 0.00f);	glVertex3d(268.2, 1124, 0);/*284 / w, 0.00f*/
			glTexCoord2f(0.175f, 1.00f);	glVertex3d(268.2, -37, 0);/*284 / w, 1.00f*/
			glEnd();



			glBegin(GL_QUADS);

			glTexCoord2f(0.175f, 0.118f); glVertex3d(268.2, 987, 0);/*284 / w, 95 / h*/
			glTexCoord2f(0.175f, 0.00f);		glVertex3d(268.2, 1124, 0);/*284 / w, 0.00f*/
			glTexCoord2f(1.00f, 0.00f);		glVertex3d(2004, 1124, 0);
			glTexCoord2f(1.00f, 0.118f);	glVertex3d(2004, 987, 0);/*1.00f, 95 / h*/
			glEnd();
			glBegin(GL_QUADS);

			glTexCoord2f(0.5662f, 1.00f); glVertex3d(268.2 - (-26.6), -37, 0);/*1102 / w, 1.00f*/
			glTexCoord2f(0.5662f, 0.118f);		glVertex3d(268.2 - (-26.6), 987, 0);/*1102 / w, 95 / h*/
			glTexCoord2f(1.00f, 0.118f);		glVertex3d(2090 - (-26.6), 987, 0);/*1.00f, 95 / h*//*2004*/
			glTexCoord2f(1.00f, 1.00f);	glVertex3d(2090 - (-26.6), -37, 0);/*2004*/
			glEnd();

			glBegin(GL_QUADS);

			glTexCoord2f(0.5662f, 1.00f); glVertex3d(268.2, -37, 0);/*1102 / w, 1.00f*/
			glTexCoord2f(0.5662f, 0.118f);		glVertex3d(268.2, 987, 0);/*1102 / w, 95 / h*/
			/*0.60f‚ª‰¼*/glTexCoord2f(0.60f, 0.118f);		glVertex3d(268.2 - (-26.6), 987, 0);/*1.00f, 95 / h*//*2004*/
			/*0.60f‚ª‰¼*/glTexCoord2f(0.60f, 1.00f);	glVertex3d(268.2 - (-26.6), -37, 0);/*2004*/
			glEnd();
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		//glFlush();
		//glClear(GL_COLOR_BUFFER_BIT);
	}
	void DrawVideo::Reshape(int x, int y)
	{
	}
	void DrawVideo::Keyboard(unsigned char key, int x, int y)
	{
	}

	void DrawVideo::Update(int totalMilliSeconds)
	{
		//auto lock = std::unique_lock<std::mutex>(m_mutex);
		m_capture->SetBitmap(*m_Texture.get());
		//return next;
	}


	void DrawVideo::Special(int key, int x, int y)
	{
	}

	/*
	int video_thread(void *opaque)
	{
		//auto m_spMedia = static_cast<std::shared_ptr<DrawStereoAviMode>>(opaque);

		while (!video_flag)
		{
			m_capture->CreateBitmap();
		}

		return 0;
	}
	*/
}