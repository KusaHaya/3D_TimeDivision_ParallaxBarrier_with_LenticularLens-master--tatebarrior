#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <algorithm>
#include <iostream>
#include <fstream>

#include <iostream>
#include<fstream>
#include "TCPClient.h"
#include <boost/asio.hpp>
#include "boost/format.hpp"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <thread>
#include "DrawVideo.h"
#include "Texture.h"
#include <vector>
#include <sstream>

#pragma comment(lib, "winmm.lib")


FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}


#define IM_W 1920 // image width
#define IM_H 1080 // image height

#define haba_first 330 // default 500
#define WID 528
#define HGT 297

#define REFRESH_RATE 120

// PC->Arduino commands
#define ENABLE_TIMEDIVISION 12
#define DISABLE_TIMEDIVISION 13
#define RESET_SYNC 20
#define NEXT_TIMEDIVISION 21

#define LIGHT_EXIT 110

#define TIME_DIV_0 200
#define TIME_DIV_1 201
#define TIME_DIV_2 202
#define TIME_DIV_3 203



// �Î~��Ɠ���̃p�^�[���̊ԂŐ�����ʒu���̕␳�i�e�N�X�`���}�b�s���O�ƃX�e���V���}�X�N�j
// �p�l�����Ƃɐݒ肷��K�v����
int SHIFT = 1;

#define SIM_W 1920 // calibration image width
#define SIM_H 1080 // calibration image height

#define SIN 0.139
#define COS 0.99

// teapot�ɓ\��
//texture�̏c���i2^n 2^m�łȂ��Ƃ����Ȃ��j

#define TEAPOT_HEIGHT 512
#define TEAPOT_WIDTH  512

#define SHGT 12 // �X�e���V��1�P�ʂ̏c��
#define MIM_W 1920 + 1008  //1968 // �X�e���V���}�[�W���t������


float eyeposx[2];
float eyeposy[2];
float eyeposz[2];
float zfar = 3000;
float zsft = 0;
float alphax = 1.0;
int habat = 0;

float crosstalkFactor = 0.15f; // �N���X�g�[�N�W���̏����l (15%)
float headTrackShift = 0.0f; // �w�b�h�g���b�L���O�ɂ��V�t�g�ʁi�T�u�s�N�Z���P�ʁj

// �F�␳�p�����[�^�i���@�Œ������₷���悤�ɃO���[�o�����j
float colorGainR = 1.00f;
float colorGainG = 1.00f;
float colorGainB = 1.00f;
float colorGamma = 1.00f;
bool colorCorrectionEnabled = true;


std::unique_ptr<vmlab::DrawVideo> VideoMode;

int running = 1; // 0:test mode on, 1:off(time-division running)
int ht = 0; // head-tracking mode 0:OFF, 1:ON
int mode = 2;// system mode 
// 0:pixel barrier by pixel units
// 1:sub-pixel barrier by pixel units
// 2:sub-pixel barrier by sub-pixel units
char anmode = 0; // anaglyph mode

char filePath[32]; // store filename

float delta = 0.0;
float DotPixel = 0.27;
float DotSubPixel = 0.09;// DotSubPixel=DotPixel(=0.27)/3
float theta = 30; // teapot�̌������ς��

int flag = 0; // an auto parameter acting as a switch signal
int img = 1; // image number: 1~10
int qq = 1; // an auto parameter acting as a switch signal
int haba = haba_first; // phase width with 1/3 pixel (= sub-pixel)
int kk = 0; // �������̐���p�����[�^
int cali_flag = 0; // calibation mode
// int phaba = 0;
// int pMiddleLine = 0;
bool LiverMode = false;

float caliX = 0; // head tracking base X
float caliY = 0; // head tracking base Y
float caliZ = 0; // head tracking base Z
float tmpX; // head tracking current X
float tmpY; // head tracking current Y
float tmpZ; // head tracking current Z
float PosX = 0;
float PosY = -0.18;
float PosZ = 0.11;

// �摜�\���p�e�N�X�`��
unsigned char image_texture[2][SIM_H][SIM_W][3];
GLuint imageL, imageR;

const int MiddleDefault = 3420;
//const int MiddleDefault = 2340;
int MiddleLine = MiddleDefault;
int mrk = 0;
int eyeright = 1; // 1: �E�ډ摜��\��, 0: �E�ډ摜��\��
int eyeleft = 1; // 1: ���ډ摜��\��, 0: ���ډ摜��\��
const float DefaultScale = 1.0f;
float videoscale = 1.0f;//0.923f
bool VideoSwitch = false;

// random ��keyboardFuc�̒��ł����g���Ă��Ȃ�
int random = 0; // random mode for test

GLuint shaderProgram;
GLuint videoshaderProgram;

// --- uniform location cache (image shader) ---
GLint u_img_leftTexture = -1;
GLint u_img_rightTexture = -1;
GLint u_img_haba = -1;
GLint u_img_totalShift = -1;
GLint u_img_timeStep = -1;
GLint u_img_manualShift = -1;
float barrierSlantY = 1.0f;   // 1.0 �� tan^-1(-3)

GLint u_img_slantY = -1;
GLint u_vid_slantY = -1;
//GLint u_img_middleLinePx = -1; // ���ǉ�����uniform������ꍇ
//GLint u_img_rgbGain = -1;
//GLint u_img_gamma = -1;
//GLint u_img_crosstalk = -1;
//GLint u_img_enableColorCorrection = -1;

// --- uniform location cache (video shader) ---
GLint u_vid_sbsTexture = -1;
GLint u_vid_haba = -1;
GLint u_vid_totalShift = -1;
GLint u_vid_timeStep = -1;
GLint u_vid_manualShift = -1;
//GLint u_vid_middleLinePx = -1; // ���ǉ�����uniform������ꍇ
//GLint u_vid_rgbGain = -1;
//GLint u_vid_gamma = -1;
//GLint u_vid_crosstalk = -1;
//GLint u_vid_enableColorCorrection = -1;

bool ReserveLR = true;

boost::asio::io_service io;
boost::asio::serial_port arduinoSerial(io);

// Arduino�ɃR�}���h�ԍ��iint�l�j��1�o�C�g�ő��M����֐�
void SendArduinoCommand(int command) {
	if (arduinoSerial.is_open()) {
		unsigned char cmd = static_cast<unsigned char>(command);
		boost::system::error_code ec;
		size_t bytes_written = boost::asio::write(arduinoSerial, boost::asio::buffer(&cmd, 1), ec);
		if (ec || bytes_written != 1) {
			printf("[ERROR] Arduino�ւ̑��M���s: %s\n", ec.message().c_str());
		}
	}
	else {
		printf("[WARN] Arduino�V���A���|�[�g���J���Ă��܂���\n");
	}
}

void Receive(TCPClient& client, const std::function<void(boost::system::error_code, std::size_t)>& callback)
{
	auto buffer = std::make_shared<boost::asio::streambuf>(sizeof(float) * 6);
	client.RecieveAsync(buffer, [&, buffer](boost::system::error_code e, size_t l)
	{
		const float* datas = boost::asio::buffer_cast<const float*>(buffer->data());
		if (cali_flag == 1) {
			caliX = (datas[3 * 0 + 0] + datas[3 * 1 + 0]) / 2 + PosX;
			caliY = COS * (datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + SIN * (datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 + PosY;
			caliZ = COS * (datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 - SIN * (datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + PosZ;
			ht = 1;
			printf("head-tracking: ON\n");
			cali_flag = 0;
		}
		else {
			tmpX = (datas[3 * 0 + 0] + datas[3 * 1 + 0]) / 2 + PosX;
			tmpY = COS * (datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + SIN * (datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 + PosY;
			tmpZ = COS * (datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 - SIN * (datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + PosZ;
			eyeposx[0] = 1000 * (datas[3 * 1 + 0] + PosX);
			eyeposx[1] = 1000 * (datas[3 * 0 + 0] + PosX);
			eyeposy[0] = 1000 * (COS*datas[3 * 1 + 1] + SIN * datas[3 * 1 + 2] + PosY);
			eyeposy[1] = 1000 * (COS*datas[3 * 0 + 1] + SIN * datas[3 * 0 + 2] + PosY);
			eyeposz[0] = 1000 * (COS*datas[3 * 1 + 2] + SIN * datas[3 * 1 + 1] + PosZ);
			eyeposz[1] = 1000 * (COS*datas[3 * 0 + 2] + SIN * datas[3 * 0 + 1] + PosZ);
		}

		if (ht == 1) {
			// haba, delta�̕ω��ʂɂ��Ă͎����I�ɋ��߂�
			haba = haba_first + (int)((caliZ - tmpZ) * 1000 / 2.0);
			// haba = int(haba_first * caliZ / tmpZ);
			delta = 0.895 / (tmpZ - 0.3245) * ((tmpX - caliX)) * 1000 / DotSubPixel;//face moves by sub-pixel units

			headTrackShift = delta;
			//�f�o�b�O�p
			//printf("Calibrated Z=%.3f, Current Z=%.3f -> Calculated Shift=%.2f\n", caliZ, tmpZ, headTrackShift);
			printf("CALIB(X:%.2f, Z:%.2f), CURRENT(X:%.2f, Z:%.2f) ---> DELTA: %.2f\n",
				caliX, caliZ, tmpX, tmpZ, delta);

			int move = 0;
			if (delta < 0.0)move = (int)(delta - 0.5);
			else move = (int)(delta + 0.5);
			//			move = (move / 3) * 3;
			MiddleLine = MiddleDefault - move;
			//			if (abs(pMiddleLine - MiddleLine) < 10){
			//				MiddleLine = pMiddleLine;
			//			}
			//			pMiddleLine = MiddleLine;
		}
		else {
			headTrackShift = 0;
			MiddleLine = MiddleDefault;
		}
		buffer->consume(sizeof(float) * 6);
		Receive(client, callback);
	});
}

int ppm_reader(char *filename, unsigned char *pimage)
{
	char buff[16];
	FILE *fp;
	int c, rgb_comp_color;
	//open PPM file for reading
	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		exit(1);
	}

	//check the image format
	if (buff[0] != 'P' || buff[1] != '6') {
		fprintf(stderr, "Invalid image format (must be 'P6')\n");
		exit(1);
	}

	//alloc memory form image
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//check for comments
	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n');
		c = getc(fp);
	}
	int width, height;
	ungetc(c, fp);
	//read image size information
	if (fscanf(fp, "%d %d", &width, &height) != 2) {
		fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
		exit(1);
	}

	//read rgb component
	if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
		fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
		exit(1);
	}

	while (fgetc(fp) != '\n');
	//memory allocation for pixel data

	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//read pixel data from file
	if (fread(pimage, 3 * width, height, fp) != height) {
		fprintf(stderr, "Error loading image '%s'\n", filename);
		exit(1);
	}
	fclose(fp);
}

// �V�F�[�_�[�t�@�C����ǂݍ��݁A�R���p�C�����āA�v���O�������쐬����w���p�[�֐�
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
	// 1. �V�F�[�_�[�I�u�W�F�N�g���쐬
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// 2. ���_�V�F�[�_�[�̃\�[�X�R�[�h���t�@�C������ǂݍ���
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("Failed to open %s\n", vertex_file_path);
		// getchar(); // �R���\�[������u�ŕ��Ȃ��悤�ɂ���
		return 0;
	}

	// 3. �t���O�����g�V�F�[�_�[�̃\�[�X�R�[�h���t�@�C������ǂݍ���
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// 4. ���_�V�F�[�_�[���R���p�C��
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// ���_�V�F�[�_�[�̃R���p�C�����ʂ��`�F�b�N
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// 5. �t���O�����g�V�F�[�_�[���R���p�C��
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// �t���O�����g�V�F�[�_�[�̃R���p�C�����ʂ��`�F�b�N
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// 6. �V�F�[�_�[�v���O�������쐬���A2�̃V�F�[�_�[�������N����
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// �����N���ʂ��`�F�b�N
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	// 7. �����N��͌X�̃V�F�[�_�[�I�u�W�F�N�g�͕s�v�Ȃ̂ō폜����
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	// 8. ���������v���O������ID��Ԃ�
	return ProgramID;
}

// �`��p�̎l�p�`(Quad)���������邽�߂̕ϐ��Ɗ֐�
GLuint quadVBO, quadVAO;
GLfloat quadVertices[] = {
	// �ʒu(x,y,z)      // �e�N�X�`�����W(u,v)
	-1.0f,  1.0f, 0.0f,  0.0f, 0.0f, // V���W�� 1.0 �� 0.0 �ɕύX
	-1.0f, -1.0f, 0.0f,  0.0f, 1.0f, // V���W�� 0.0 �� 1.0 �ɕύX
	 1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // V���W�� 0.0 �� 1.0 �ɕύX

	-1.0f,  1.0f, 0.0f,  0.0f, 0.0f, // V���W�� 1.0 �� 0.0 �ɕύX
	 1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // V���W�� 0.0 �� 1.0 �ɕύX
	 1.0f,  1.0f, 0.0f,  1.0f, 0.0f  // V���W�� 1.0 �� 0.0 �ɕύX
};

void setupQuad() {
	// VAO (Vertex Array Object) ���쐬���ăo�C���h
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	// VBO (Vertex Buffer Object) ���쐬���ăo�C���h
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	// ���_�f�[�^��VBO�ɏ�������
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	// ���_�����|�C���^�[��ݒ� (���_�V�F�[�_�[�� layout(location = 0) �ɑΉ�)
	// ����0: ���_�ʒu
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);

	// ���_�����|�C���^�[��ݒ� (���_�V�F�[�_�[�� layout(location = 1) �ɑΉ�)
	// ����1: �e�N�X�`��UV���W
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	// VAO�̃o�C���h������
	glBindVertexArray(0);
}

void init(void) {
	glewInit();
	typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);

	PFNWGLSWAPINTERVALEXTPROC pWglSwapIntervalEXT =
		(PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (pWglSwapIntervalEXT) {
		pWglSwapIntervalEXT(1);
	}
	else {
		printf("[WARN] V-Sync extension not available.\n");
	}
	anmode = 0;

	glGenTextures(1, &imageL);
	glGenTextures(1, &imageR);


	shaderProgram = LoadShaders("passthrough.vert", "interleave.frag");
	videoshaderProgram = LoadShaders("passthrough.vert", "sbs_interleave.frag");

	// --- cache uniform locations (image) ---
	u_img_leftTexture = glGetUniformLocation(shaderProgram, "leftTexture");
	u_img_rightTexture = glGetUniformLocation(shaderProgram, "rightTexture");
	u_img_haba = glGetUniformLocation(shaderProgram, "haba");
	u_img_totalShift = glGetUniformLocation(shaderProgram, "totalShift");
	u_img_timeStep = glGetUniformLocation(shaderProgram, "timeStep");
	u_img_manualShift = glGetUniformLocation(shaderProgram, "manualShift");
	u_img_slantY = glGetUniformLocation(shaderProgram, "slantY");
	/*u_img_middleLinePx = glGetUniformLocation(shaderProgram, "middleLinePx");
	u_img_rgbGain = glGetUniformLocation(shaderProgram, "rgbGain");
	u_img_gamma = glGetUniformLocation(shaderProgram, "gammaValue");
	u_img_crosstalk = glGetUniformLocation(shaderProgram, "crosstalk");
	u_img_enableColorCorrection = glGetUniformLocation(shaderProgram, "enableColorCorrection");*/

	// --- cache uniform locations (video) ---
	u_vid_sbsTexture = glGetUniformLocation(videoshaderProgram, "sbsTexture");
	u_vid_haba = glGetUniformLocation(videoshaderProgram, "haba");
	u_vid_totalShift = glGetUniformLocation(videoshaderProgram, "totalShift");
	u_vid_timeStep = glGetUniformLocation(videoshaderProgram, "timeStep");
	u_vid_manualShift = glGetUniformLocation(videoshaderProgram, "manualShift");
	u_vid_slantY = glGetUniformLocation(videoshaderProgram, "slantY");
	/*u_vid_middleLinePx = glGetUniformLocation(videoshaderProgram, "middleLinePx");
	u_vid_rgbGain = glGetUniformLocation(videoshaderProgram, "rgbGain");
	u_vid_gamma = glGetUniformLocation(videoshaderProgram, "gammaValue");
	u_vid_crosstalk = glGetUniformLocation(videoshaderProgram, "crosstalk");
	u_vid_enableColorCorrection = glGetUniformLocation(videoshaderProgram, "enableColorCorrection");*/

	auto warnIfMissing = [](const char* name, GLint loc) {
		if (loc < 0) printf("[WARN] uniform not found: %s\n", name);
	};
	warnIfMissing("leftTexture", u_img_leftTexture);
	warnIfMissing("rightTexture", u_img_rightTexture);
	warnIfMissing("haba", u_img_haba);
	warnIfMissing("totalShift", u_img_totalShift);
	warnIfMissing("timeStep", u_img_timeStep);
	warnIfMissing("manualShift", u_img_manualShift);
	/*warnIfMissing("middleLinePx", u_img_middleLinePx);
	warnIfMissing("rgbGain", u_img_rgbGain);
	warnIfMissing("gammaValue", u_img_gamma);
	warnIfMissing("crosstalk", u_img_crosstalk);
	warnIfMissing("enableColorCorrection", u_img_enableColorCorrection);*/

	warnIfMissing("sbsTexture", u_vid_sbsTexture);
	warnIfMissing("haba", u_vid_haba);
	warnIfMissing("totalShift", u_vid_totalShift);
	warnIfMissing("timeStep", u_vid_timeStep);
	warnIfMissing("manualShift", u_vid_manualShift);
	warnIfMissing("video slantY", u_vid_slantY);
	/*warnIfMissing("middleLinePx", u_vid_middleLinePx);
	warnIfMissing("rgbGain", u_vid_rgbGain);
	warnIfMissing("gammaValue", u_vid_gamma);
	warnIfMissing("crosstalk", u_vid_crosstalk);
	warnIfMissing("enableColorCorrection", u_vid_enableColorCorrection);
*/
	setupQuad();
	ht = 0;
	printf("head-tracking: OFF\n");
	printf("mode : SS\n");
	printf("haba = %d\n", haba);
}

// RGBCG_image() �̑���ƂȂ�V�����֐�
// renderInterleavedImage() ���C��
void renderInterleavedImage() {
	glUseProgram(shaderProgram); // ���̐V�����V�F�[�_�[��ǂݍ��ނ悤��init()���ύX
	glBindVertexArray(quadVAO);

	// --- �]�������̃p�����[�^���v�Z ---
	// calculate_stencil()�̖`���ɂ������v�Z�������Ɏ����Ă���
	int totalShift = MiddleLine - 48 * haba;

	// --- uniform�ϐ����V�F�[�_�[�ɑ��� ---
	// �e�N�X�`���̐ݒ�
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageL);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, imageR);
	glUniform1i(u_img_leftTexture, 0);
	glUniform1i(u_img_rightTexture, 1);

	glUniform1f(u_img_haba, (float)haba);
	glUniform1f(u_img_totalShift, (float)totalShift);
	glUniform1i(u_img_timeStep, kk);
	glUniform1f(u_img_manualShift, (float)SHIFT);
	if (u_img_slantY >= 0)
		glUniform1f(u_img_slantY, barrierSlantY);
	//// �ǉ��ς݂Ȃ�
	//glUniform1f(u_img_middleLinePx, (float)MiddleLine / 3.0f);
	//glUniform3f(u_img_rgbGain, colorGainR, colorGainG, colorGainB);
	//glUniform1f(u_img_gamma, colorGamma);
	//glUniform1f(u_img_crosstalk, crosstalkFactor);
	//glUniform1i(u_img_enableColorCorrection, colorCorrectionEnabled ? 1 : 0);
	// �`��
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// ��Еt��
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);
}

// ����p�̕`��֐�
// renderInterleavedVideo() ���C��
void renderInterleavedVideo() {
	GLuint videoTexID = VideoMode->getVideoTextureID();
	if (videoTexID == 0) return;

	glUseProgram(videoshaderProgram);
	glBindVertexArray(quadVAO);

	// --- �]�������̃p�����[�^���v�Z ---
	int totalShift = MiddleLine - 48 * haba;

	// --- uniform�ϐ����V�F�[�_�[�ɑ��� ---
	// �e�N�X�`���̐ݒ�
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, videoTexID);
	glUniform1i(u_vid_sbsTexture, 0);

	glUniform1f(u_vid_haba, (float)haba);
	glUniform1f(u_vid_totalShift, (float)totalShift);
	glUniform1i(u_vid_timeStep, kk);
	glUniform1f(u_vid_manualShift, (float)SHIFT);

	// 旧方式の W + H 相当
	if (u_vid_slantY >= 0)
		glUniform1f(u_vid_slantY, barrierSlantY);

	//// �ǉ��ς݂Ȃ�
	//glUniform1f(u_vid_middleLinePx, (float)MiddleLine / 3.0f);
	//glUniform3f(u_vid_rgbGain, colorGainR, colorGainG, colorGainB);
	//glUniform1f(u_vid_gamma, colorGamma);
	//glUniform1f(u_vid_crosstalk, crosstalkFactor);
	//glUniform1i(u_vid_enableColorCorrection, colorCorrectionEnabled ? 1 : 0);
	// �`��
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// ��Еt��
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(0);
}

const double TARGET_HZ = 120.0;
const auto TARGET_PERIOD = std::chrono::duration<double>(1.0 / TARGET_HZ);
bool syncClockInitialized = false;
std::chrono::steady_clock::time_point nextFrameDeadline;

void DTimer(int totalMilliSeconds)
{
	using steady_clock = std::chrono::steady_clock;
	const auto targetPeriod = std::chrono::duration_cast<steady_clock::duration>(TARGET_PERIOD);
	auto now = steady_clock::now();

	if (!syncClockInitialized) {
		nextFrameDeadline = now;
		syncClockInitialized = true;
	}

	bool shouldRender = false;
	int tickCount = 0;
	while (now >= nextFrameDeadline) {
		shouldRender = true;
		tickCount++;
		nextFrameDeadline += targetPeriod;
	}

	if (shouldRender) {
		if (VideoSwitch) VideoMode->Update(0);
		if (arduinoSerial.is_open()) {
			if ( kk == 0) {
				SendArduinoCommand(TIME_DIV_0);
			}
		}
		glutPostRedisplay();
		if (running == 1 && tickCount > 0) {
			kk = (kk + (tickCount % 4)) % 4;
		}
	}

	auto remain = nextFrameDeadline - steady_clock::now();
	auto remainMs = std::chrono::duration_cast<std::chrono::milliseconds>(remain).count();
	unsigned int nextCallMs = (remainMs > 1) ? static_cast<unsigned int>(remainMs) : 1;
	glutTimerFunc(nextCallMs, DTimer, 0);
}

int frame = 0;
bool GetPic = false;
int prt = 0;


void disp(void) {
	int i, e;
	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	if (flag == 1) {


		glDisable(GL_TEXTURE_2D);

		if (mrk == 1) {
			// ���惂�[�h



			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedVideo(); // ���V��������`��֐����Ăяo��
		//	glUniform1f(glGetUniformLocation(videoshaderProgram, "middleLinePx"), (float)MiddleLine / 3.0f);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		}
		else {
			// �摜���[�h

			// �ύX��F�V�����֐�����x�Ăяo������
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // FBO���N���A
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedImage();
			//	glUniform1f(glGetUniformLocation(shaderProgram, "middleLinePx"), (float)MiddleLine / 3.0f);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		// kk��DTimer()����120Hz�����X�V����
	}

	else {
		sprintf(filePath, "./images/IDW/%03dl.ppm", img);
		ppm_reader(filePath, &image_texture[0][0][0][0]);
		sprintf(filePath, "./images/IDW/%03dr.ppm", img);//�{����r
		ppm_reader(filePath, &image_texture[1][0][0][0]);

		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, imageL);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIM_W, SIM_H, 0, GL_RGB, GL_UNSIGNED_BYTE, image_texture[0]);

		glBindTexture(GL_TEXTURE_2D, imageR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIM_W, SIM_H, 0, GL_RGB, GL_UNSIGNED_BYTE, image_texture[1]);

		qq = 0;
		flag = 1;
	}
	glutSwapBuffers();
}

static void KeyEvent(unsigned char key, int x, int y) {
	switch (key) {
	case 27:

		while (!VideoMode->video_flag) VideoMode->dispose();
		if (arduinoSerial.is_open()) {
			SendArduinoCommand(DISABLE_TIMEDIVISION);
			SendArduinoCommand(LIGHT_EXIT);
		}
		timeEndPeriod(1);
		exit(0);
		break;
	case 'Z':
		anmode = 0;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'A':
		anmode = 1;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'Q':
		anmode = 2;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'q':
		kk = 0;
		glutDisplayFunc(disp);
		break;
	case 'w':
		kk = 1;
		glutDisplayFunc(disp);
		break;
	case 'e':
		kk = 2;
		glutDisplayFunc(disp);
		break;
	case 'r':
		kk = 3;
		glutDisplayFunc(disp);
		break;
	case 't':
		if (running) {
			running = 0;
			SendArduinoCommand(DISABLE_TIMEDIVISION);
		}
		else {
			running = 1;
			kk = 0;
			SendArduinoCommand(RESET_SYNC);
			SendArduinoCommand(ENABLE_TIMEDIVISION);
		}
		glutDisplayFunc(disp);
		break;
	case 'o':
		SHIFT += 1;
		glutDisplayFunc(disp);
		break;
	case 'h':
		if (ht == 0) {
			Sleep(100);
			cali_flag = 1;
		}
		else {
			Sleep(100);
			ht = 0;
			//		haba=haba_first;
			//		move=0;
			printf("head-tracking: OFF\n");
			haba = haba_first;
		}
		MiddleLine = MiddleDefault;
		glutDisplayFunc(disp);
		break;
	case 'a':
		haba = haba + 1;
		printf("haba = %d\n", haba);
		glutDisplayFunc(disp);
		break;
	case 's':
		VideoMode->SetSpeed(200);
		break;
	case'z':
		if (random == 0) {
			mode = 0;
			printf("mode : PP\n");
		}
		else if (random == 1) {
			mode = 0;
			printf("mode : PP\n");
		}
		else if (random == 2) {
			mode = 2;
		}
		else {
			mode = 1;
			printf("mode : SP\n");
		}
		glutDisplayFunc(disp);
		break;
	case'x':
		if (random == 0) {
			mode = 1;
			printf("mode : SP\n");
		}
		else if (random == 1) {
			mode = 2;
			printf("mode : SS\n");
		}
		else if (random == 2) {
			mode = 0;
			printf("mode : PP\n");
		}
		else {
			mode = 2;
			printf("mode : SS\n");
		}
		glutDisplayFunc(disp);
		break;
	case'c':
		if (random == 0) {
			mode = 2;
			printf("mode : SS\n");
		}
		else if (random == 1) {
			mode = 1;
			printf("mode : SP\n");
		}
		else if (random == 2) {
			mode = 1;
			printf("mode : SP\n");
		}
		else {
			mode = 0;
			printf("mode : PP\n");
		}
		glutDisplayFunc(disp);
		break;
	case 'L':
		LiverMode = true;
		break;
	case 'l':
		LiverMode = false;
		break;
	case '1':
		img = 2;
		random = 0;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '2':
		img = 3;
		random = 1;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '3':
		img = 4;
		random = 2;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '4':
		img = 5;
		random = 3;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '5':
		img = 6;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '6':
		img = 7;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '7':
		img = 8;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '0':
		img = 1;
		random = 0;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'm':
		mrk = 1;
		flag = 1;

		if (!VideoSwitch)
		{
			VideoMode->Initialize();

			// Initialize直後に少し待つ
			Sleep(100);

			VideoMode->SetSpeed(16000);

			// ここで初めて動画更新を許可
			VideoSwitch = true;
		}

		glutPostRedisplay();
		break;
	case 'M':
		mrk = 0;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'b':
		eyeright = 1;
		glutDisplayFunc(disp);
		break;
	case 'B':
		eyeright = 0;
		glutDisplayFunc(disp);
		break;
	case 'v':
		eyeleft = 1;
		glutDisplayFunc(disp);
		break;
	case 'V':
		eyeleft = 0;
		glutDisplayFunc(disp);
		break;
	case 'S':
		VideoMode->Mode3D = !VideoMode->Mode3D;
		glutDisplayFunc(disp);
		break;
		/*case 'p':
			VideoMode->printflag = false;
			glutDisplayFunc(disp);
			break;*/
	case 'R':
		ReserveLR = !ReserveLR;
		if (ReserveLR) printf("RL\n");
		else printf("LR\n");
		glutDisplayFunc(disp);
		break;
		//case 'f':
		//	zsft += 5;
		//	glutDisplayFunc(disp);
		//	break;
		//case 'F':
		//	zsft -= 5;
		//	glutDisplayFunc(disp);
		//	break;
	case '[':
		haba++;
		//printf("%d\n", haba);
		glutDisplayFunc(disp);
		break;
	case ']':
		haba--;
		//printf("%d\n", haba);
		glutDisplayFunc(disp);
		break;
	case 'k':
		kk = (kk + 1) % 4;
		break;
	case 'K':
		printf("z: %f, haba: %d\n", tmpZ, haba);
		break;
	case'd':
		alphax += 0.1;
		if (alphax > 1.1)
			alphax = 0;
		break;
	case'D':
		habat += 1;
		break;
		// KeyEvent() �֐��ɃL�[��ǉ�
	case 'f':
		barrierSlantY -= 0.02f;
		if (barrierSlantY < 0.1f) barrierSlantY = 0.1f;
		printf("barrierSlantY = %.3f, angle = %.3f deg\n",
			barrierSlantY,
			atan(-3.0f / barrierSlantY) * 180.0f / 3.14159265f);
		glutPostRedisplay();
		break;

	case 'F':
		barrierSlantY += 0.02f;
		printf("barrierSlantY = %.3f, angle = %.3f deg\n",
			barrierSlantY,
			atan(-3.0f / barrierSlantY) * 180.0f / 3.14159265f);
		glutPostRedisplay();
		break;

	}
}
static void KeyUp(unsigned
	char key, int x, int y) {
	switch (key) {
	case 's':
		VideoMode->SetSpeed(9000);
		break;
	}
}
static void KeySpecialEvent(int key, int x, int y) {
	if (key == GLUT_KEY_LEFT) {
		theta = (int)(theta + 1) % 360;
		glutDisplayFunc(disp);
	}
	if (key == GLUT_KEY_RIGHT) {
		theta = (int)(theta - 1 + 360) % 360;
		glutDisplayFunc(disp);
	}

}



int main(int argc, char ** argv) {

	MMRESULT timerResult = timeBeginPeriod(1);
	if (timerResult != TIMERR_NOERROR) {
		printf("[WARN] Failed to set 1ms timer resolution.\n");
	}


	arduinoSerial.open("COM3"); // Arduino�̃|�[�g���ɍ��킹�ĕύXCOm1�̓f�o�b�O�p
	arduinoSerial.set_option(boost::asio::serial_port_base::baud_rate(115200));
	SendArduinoCommand(RESET_SYNC);
	SendArduinoCommand(ENABLE_TIMEDIVISION);
	TCPClient client("127.0.0.1", 30000);
	glutInit(&argc, argv);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(IM_W, IM_H);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL | GLUT_STEREO);
	glutCreateWindow("Test");

	VideoMode = std::unique_ptr<vmlab::DrawVideo>(new vmlab::DrawVideo);
	VideoSwitch = false;
	glutTimerFunc(0, DTimer, 0);

	init();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	Receive(client, [](boost::system::error_code e, size_t) { std::cout << e.message() << std::endl; });
	glutDisplayFunc(disp);
	glutKeyboardFunc(KeyEvent);
	glutKeyboardUpFunc(KeyUp);
	glutSpecialFunc(KeySpecialEvent);
	//glutIdleFunc(disp);

	glutMainLoop();
	client.Close();
	timeEndPeriod(1);

	glDeleteTextures(1, &imageL);
	glDeleteTextures(1, &imageR);

	return 0;
}