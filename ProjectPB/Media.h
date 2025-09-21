#pragma once
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "avdevice.lib")
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include"libavutil/imgutils.h"
}
#include "Texture.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"
#define MAX_AUDIO_FRAME_SIZE 192000 

namespace vmlab
{

	class Capture
	{
	private:
		AVFormatContext	*pFormatCtx;
		int				i, videoindex;
		AVCodecContext	*pCodecCtx;
		AVCodec			*pCodec;
		AVFrame			*pFrame, *pFrameRGB;
		SwsContext		*sws_ctx;
		AVPacket		packet;
	public:
		Capture();
		~Capture();

		int FindWindow(char *name);

		int FindDevice();

		int OpenComponent();

		int CreateBitmap();

		void SetBitmap(Texture& const texture);


	};
}