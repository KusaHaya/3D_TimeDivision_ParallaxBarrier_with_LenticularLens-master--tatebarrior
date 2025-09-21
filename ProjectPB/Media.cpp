#include "Media.h"

#include <iostream>
#include <string>
#include <fstream>


Uint8*	gAudioChunk;
Uint32	gAudioLength;
Uint8*	gAudioPosition;
namespace vmlab
{


	Capture::Capture() :pFormatCtx(nullptr), pCodecCtx(nullptr), pCodec(nullptr)
	{
		//av_register_all();
		avformat_network_init();
		pFormatCtx = avformat_alloc_context();
		//Register Device		
		avdevice_register_all();
	}
	bool HaveSws = false;
	int  Capture::FindWindow(char *name)
	{
		//Windows
		//Use gdigrab
		AVDictionary* options = NULL;
		//Set some options
		//grabbing frame rate
		//av_dict_set(&options,"framerate","5",0);
		//The distance from the left edge of the screen or desktop
		//av_dict_set(&options,"offset_x","20",0);
		//The distance from the top edge of the screen or desktop
		//av_dict_set(&options,"offset_y","40",0);
		//Video frame size. The default is to capture the full screen
		//av_dict_set(&options,"video_size","640x480",0);
		AVInputFormat *ifmt = av_find_input_format("gdigrab");
		if (avformat_open_input(&pFormatCtx, "title=MonsterX", ifmt, &options) != 0) {
			printf("Couldn't open input stream.\n");

			return -1;
		}
	}

	int  Capture::FindDevice()
	{
		AVDictionary* options = NULL;
		//Windows
		//Set some options
		//grabbing frame rate
		//av_dict_set(&options,"framerate","5",0);
		//The distance from the left edge of the screen or desktop
		//av_dict_set(&options,"offset_x","20",0);
		//The distance from the top edge of the screen or desktop
		//av_dict_set(&options,"offset_y","40",0);
		//Video frame size. The default is to capture the full screen
		//av_dict_set(&options,"video_size","1280x720",0);
		AVInputFormat *ifmt = av_find_input_format("dshow");

		//av_dict_set(&options, "framerate", "29.97", 0);
		//av_dict_set(&options, "video_size", "1920x1080", 0);
		//av_dict_set(&options, "framerate", "29.97", 0);
		av_dict_set(&options, "rtbufsize", "2000000000", NULL);
		printf("Start Capture!");
		bool GetDevice = false;

		if (avformat_open_input(&pFormatCtx, "C:/TestHD.mp4", NULL, NULL) != 0) {
			//		if (avformat_open_input(&pFormatCtx, "C:/All_Movie.mp4", NULL, NULL) != 0){
			//		if (avformat_open_input(&pFormatCtx, "C:/bigt.mp4", NULL, NULL) != 0){
			//		if (avformat_open_input(&pFormatCtx, "C:/tombraider_origin.mp4", NULL, NULL) != 0){
			printf("Couldn't open input stream.\n");
		}
		else GetDevice = true;
		if (!GetDevice) {

			printf("Couldn't open input stream.\n");
			return -1;
		}
		return 0;
	}

	int Capture::OpenComponent()
	{
		if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		{
			printf("Couldn't find stream information.\n");
			return -1;
		}

		videoindex = -1;
		for (i = 0; i < pFormatCtx->nb_streams; i++)
			if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				videoindex = i;
				break;
			}
		if (videoindex == -1)
		{
			printf("Didn't find a video stream.\n");
			return -1;
		}
		//pCodecCtx = pFormatCtx->streams[videoindex]->codec;
		//pCodecCtx->rc_buffer_size *= 10;
		pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
		/*’Ç‹L*/
		pCodecCtx = avcodec_alloc_context3(pCodec);
		pCodecCtx->rc_buffer_size *= 10;
		if (pCodec == NULL)
		{
			printf("Codec not found.\n");
		}
		/*’Ç‹L*/
		if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar) < 0) {
			printf("avcodec_parameters_to_context failed\n");
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
			printf("Could not open codec.\n");
			return -1;
		}

		//Output Information-----------------------------
		printf("File Information---------------------------------\n");
		av_dump_format(pFormatCtx, 0, NULL, 0);
		printf("-------------------------------------------------\n");

		pFrame = av_frame_alloc();
		pFrameRGB = av_frame_alloc();

		int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 16);

		uint8_t *video_buffer = (uint8_t*)av_malloc(bytes * sizeof(uint8_t));
		//avpicture_fill((AVPicture *)pFrameRGB, video_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
		av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, video_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

		sws_ctx = sws_getCachedContext(nullptr, pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24,
			SWS_BICUBIC, NULL, NULL, NULL);
		HaveSws = true;
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);


	}

	int Capture::CreateBitmap()
	{
		int ret;
		int got_picture;
		if (av_read_frame(pFormatCtx, &packet) >= 0)
		{
			if (packet.stream_index == videoindex)
			{
				//ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);
				ret = avcodec_send_packet(pCodecCtx, &packet);
				got_picture = avcodec_receive_frame(pCodecCtx, pFrame);
				if (ret < 0)
				{
					printf("Decode Error.\n");
					return -1;
				}
				if (got_picture == 0)
				{
					sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
						pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
						pFrameRGB->linesize);
				}

			}

			av_packet_unref(&packet);
		}
		else
		{
			av_seek_frame(pFormatCtx, videoindex, 0, AVSEEK_FLAG_FRAME);
		}
	}

	void Capture::SetBitmap(Texture& const LRTexture)
	{
		LRTexture.ReloadSub(0, 0, pCodecCtx->width, pCodecCtx->height, GL_RGB, pFrameRGB->data[0]);
	}

	Capture::~Capture()
	{
		if (HaveSws) sws_freeContext(sws_ctx);
		SDL_Quit();
		//av_free(out_buffer);
		if (HaveSws)
		{
			av_free(pFrameRGB);
			avcodec_close(pCodecCtx);
			avformat_close_input(&pFormatCtx);
		}
	}
}