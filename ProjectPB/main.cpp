#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include<fstream>
#include "TCPClient.h"
#include <boost/asio.hpp>
#include "boost/format.hpp"
#include <windows.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <thread>
#include "DrawVideo.h"
#include "Texture.h"
#include <vector>
#include <sstream>


FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}


#define IM_W 1920 // image width
#define IM_H 1080 // image height

#define haba_first 750 // default 500
#define WID 528
#define HGT 297

#define LIGHT_CON_0 100
#define LIGHT_CON_1 101
#define LIGHT_CON_2 102
#define LIGHT_CON_3 103



// 静止画と動画のパターンの間で生じる位置差の補正（テクスチャマッピングとステンシルマスク）
// パネルごとに設定する必要あり
int SHIFT = 1;

#define SIM_W 1920 // calibration image width
#define SIM_H 1080 // calibration image height

#define SIN 0.139
#define COS 0.99

// teapotに貼る
//textureの縦横（2^n 2^mでないといけない）

#define TEAPOT_HEIGHT 512
#define TEAPOT_WIDTH  512

#define SHGT 12 // ステンシル1単位の縦幅
#define MIM_W 1920 + 1008  //1968 // ステンシルマージン付き横幅


float eyeposx[2];
float eyeposy[2];
float eyeposz[2];
float zfar = 3000;
float zsft = 0;
float alphax = 0.0;
int habat = 0;

float crosstalkFactor = 0.15f; // クロストーク係数の初期値 (15%)
float headTrackShift = 0.0f; // ヘッドトラッキングによるシフト量（サブピクセル単位）


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
float theta = 30; // teapotの向きが変わる

int flag = 0; // an auto parameter acting as a switch signal
int img = 1; // image number: 1~10
int qq = 1; // an auto parameter acting as a switch signal
int haba = haba_first; // phase width with 1/3 pixel (= sub-pixel)
int kk = 0; // 時分割の制御パラメータ
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

// 画像表示用テクスチャ
unsigned char image_texture[2][SIM_H][SIM_W][3];
GLuint imageL, imageR;

const int MiddleDefault = 3420;
//const int MiddleDefault = 2340;
int MiddleLine = MiddleDefault;
int mrk = 0;
int eyeright = 1; // 1: 右目画像を表示, 0: 右目画像非表示
int eyeleft = 1; // 1: 左目画像を表示, 0: 左目画像非表示
const float DefaultScale = 1.0f;
float videoscale = 1.0f;//0.923f
bool VideoSwitch = false;

// random はkeyboardFucの中でしか使っていない
int random = 0; // random mode for test

int list;
unsigned char sbuf2[3][SHGT][MIM_W];
int VideoFlag = 1;

GLuint Tex_Name;
GLuint RenderBuffer;
GLuint FrameBuffer;
GLuint shaderProgram;
GLuint videoshaderProgram;

bool ReserveLR = true;

boost::asio::io_service io;
boost::asio::serial_port arduinoSerial(io);

// teapotテクスチャマッピング
static GLubyte Teapotimage[TEAPOT_HEIGHT][TEAPOT_WIDTH][4];
GLuint teapot;

// teapot回転
int preX = 0, preY = 0, dx, dy, IntegralX = 0, IntegralY = 0;

// Teapotにはるテクスチャ(TGAファイル)の読み込みor生成
void TeapotInitTexture(void){

	FILE *fp;
	errno_t error;
	int x, z;

	// texture file open 
	if ((error = fopen_s(&fp, "test.tga", "rb")) != 0){//fopenだとビルド時に怒られたのでfopen_s使用
		//	if ((error = fopen_s(&fp, "TeapotBackground.tga", "rb")) != 0){
		fprintf(stderr, "texture file cannot open\n");
		return;
	}
	fseek(fp, 18, SEEK_SET);//TGAファイルの最初18byteはRGBAではなく画像の大きさとかそういうデータだから飛ばす。
	for (x = 0; x < TEAPOT_HEIGHT; x++){
		for (z = 0; z < TEAPOT_WIDTH; z++){
			Teapotimage[x][z][2] = fgetc(fp);// B 
			Teapotimage[x][z][1] = fgetc(fp);// G 
			Teapotimage[x][z][0] = fgetc(fp);// R 
			Teapotimage[x][z][3] = fgetc(fp);// alpha 
		}
	}
	fclose(fp);
}

//Teapodにテクスチャマッピングするための準備
void TeapotInit(void){
	TeapotInitTexture();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &teapot);
	glBindTexture(GL_TEXTURE_2D, teapot);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEAPOT_WIDTH, TEAPOT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, Teapotimage);
}

void mySetLight(void)
{
	GLfloat light_diffuse[] = { 0.9, 0.9, 0.9, 1.0 };	// 拡散反射光
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };	// 鏡面反射光
	GLfloat light_ambient[] = { 0.3, 0.3, 0.3, 0.1 };	// 環境光
	GLfloat light_position[] = { 0.0, 0.0, 100.0, 1.0 };	// 位置と種類

	// 光源の設定
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);	 // 拡散反射光の設定
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); // 鏡面反射光の設定
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);	 // 環境光の設定
	glLightfv(GL_LIGHT0, GL_POSITION, light_position); // 位置と種類の設定

	glShadeModel(GL_SMOOTH);	// シェーディングの種類の設定
	glEnable(GL_LIGHT0);		// 光源の有効化
}

void List()
{
	GLfloat nad[] = { 1.0, 1.0, 1.0, 1.0 };
	static GLfloat lightPos[] = { 0.4, 0.2, 0.1, 0.8 };

	list = glGenLists(1);
	glNewList(list, GL_COMPILE_AND_EXECUTE);
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, nad);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glRotatef(-theta, 0, 1, 0);
		glBindTexture(GL_TEXTURE_2D, teapot);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glEnable(GL_TEXTURE_2D);

		glBegin(GL_POLYGON);
		glTexCoord2d(0.0, 1.0);	 glVertex3f(-480.0, 270.0, -250.0); //glNormal3f(0, 0, 1);
		glTexCoord2d(0.0, 0.0);	glVertex3f(-480.0, -270.0, -250.0); //glNormal3f(0, 0, 1);
		glTexCoord2d(1.0, 0.0);	glVertex3f(480.0, -270.0, -250.0); //glNormal3f(0, 0, 1);
		glTexCoord2d(1.0, 1.0);	glVertex3f(480.0, 270.0, -250.0); //glNormal3f(0, 0, 1);
		glEnd();

		glDisable(GL_TEXTURE_2D);

		glRotatef(theta, 0, 1, 0);
		glBindTexture(GL_TEXTURE_2D, teapot);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glEnable(GL_TEXTURE_2D);

		glutSolidTeapot(100);

		//		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
	}
	glEndList();
}


void Receive(TCPClient& client, const std::function<void(boost::system::error_code, std::size_t)>& callback)
{
	auto buffer = std::make_shared<boost::asio::streambuf>(sizeof(float) * 6);
	client.RecieveAsync(buffer, [&, buffer](boost::system::error_code e, size_t l)
	{
		const float* datas = boost::asio::buffer_cast<const float*>(buffer->data());
		if (cali_flag == 1){
			caliX = (datas[3 * 0 + 0] + datas[3 * 1 + 0]) / 2 + PosX;
			caliY = COS*(datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + SIN*(datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 + PosY;
			caliZ = COS*(datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 - SIN*(datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + PosZ;
			ht = 1;
			printf("head-tracking: ON\n");
			cali_flag = 0;
		}
		else{
			tmpX = (datas[3 * 0 + 0] + datas[3 * 1 + 0]) / 2 + PosX;
			tmpY = COS*(datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + SIN*(datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 + PosY;
			tmpZ = COS*(datas[3 * 0 + 2] + datas[3 * 1 + 2]) / 2 - SIN*(datas[3 * 0 + 1] + datas[3 * 1 + 1]) / 2 + PosZ;
			eyeposx[0] = 1000 * (datas[3 * 1 + 0] + PosX);
			eyeposx[1] = 1000 * (datas[3 * 0 + 0] + PosX);
			eyeposy[0] = 1000 * (COS*datas[3 * 1 + 1] + SIN*datas[3 * 1 + 2] + PosY);
			eyeposy[1] = 1000 * (COS*datas[3 * 0 + 1] + SIN*datas[3 * 0 + 2] + PosY);
			eyeposz[0] = 1000 * (COS*datas[3 * 1 + 2] + SIN*datas[3 * 1 + 1] + PosZ);
			eyeposz[1] = 1000 * (COS*datas[3 * 0 + 2] + SIN*datas[3 * 0 + 1] + PosZ);
		}

		if (ht == 1){
			// haba, deltaの変化量については実験的に求めた
			haba = haba_first + (int)((caliZ - tmpZ) * 1000 / 2.0);
			// haba = int(haba_first * caliZ / tmpZ);
			delta = 0.895 / (tmpZ - 0.3245) * ((tmpX - caliX)) * 1000 / DotSubPixel;//face moves by sub-pixel units

			headTrackShift = delta;
			//デバッグ用
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

bool checkFramebufferStatus()
{
	// check FBO status
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch (status)
	{
	case GL_FRAMEBUFFER_COMPLETE_EXT:
		//std::cout << "Framebuffer complete.\n";
		return true;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
		std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete.\n";
		return false;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
		std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO.\n";
		return false;

	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions.\n";
		return false;

	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats.\n";
		return false;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
		std::cout << "[ERROR] Framebuffer incomplete: Draw buffer.\n";
		return false;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
		std::cout << "[ERROR] Framebuffer incomplete: Read buffer.\n";
		return false;

	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		std::cout << "[ERROR] Unsupported by FBO implementation.\n";
		return false;

	default:
		std::cout << "[ERROR] Unknow error.\n";
		return false;
	}
}

void Frame_Buffer_Sets(){

	//テクスチャ
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &Tex_Name);
	glBindTexture(GL_TEXTURE_2D, Tex_Name);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MIM_W, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	//レンダーバッファ
	glGenRenderbuffersEXT(1, &RenderBuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RenderBuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, MIM_W, 1080);

	//フレームバッファ
	glGenFramebuffersEXT(1, &FrameBuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FrameBuffer);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, Tex_Name, 0);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, RenderBuffer);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, RenderBuffer);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	if (checkFramebufferStatus() == false){
		exit(0);
	}

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

// シェーダーファイルを読み込み、コンパイルして、プログラムを作成するヘルパー関数
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
	// 1. シェーダーオブジェクトを作成
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// 2. 頂点シェーダーのソースコードをファイルから読み込む
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
		// getchar(); // コンソールが一瞬で閉じないようにする
		return 0;
	}

	// 3. フラグメントシェーダーのソースコードをファイルから読み込む
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

	// 4. 頂点シェーダーをコンパイル
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// 頂点シェーダーのコンパイル結果をチェック
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// 5. フラグメントシェーダーをコンパイル
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// フラグメントシェーダーのコンパイル結果をチェック
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// 6. シェーダープログラムを作成し、2つのシェーダーをリンクする
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// リンク結果をチェック
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	// 7. リンク後は個々のシェーダーオブジェクトは不要なので削除する
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	// 8. 完成したプログラムのIDを返す
	return ProgramID;
}

// 描画用の四角形(Quad)を準備するための変数と関数
GLuint quadVBO, quadVAO;
GLfloat quadVertices[] = {
    // 位置(x,y,z)      // テクスチャ座標(u,v)
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, // V座標を 1.0 → 0.0 に変更
    -1.0f, -1.0f, 0.0f,  0.0f, 1.0f, // V座標を 0.0 → 1.0 に変更
     1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // V座標を 0.0 → 1.0 に変更

    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, // V座標を 1.0 → 0.0 に変更
     1.0f, -1.0f, 0.0f,  1.0f, 1.0f, // V座標を 0.0 → 1.0 に変更
     1.0f,  1.0f, 0.0f,  1.0f, 0.0f  // V座標を 1.0 → 0.0 に変更
};

void setupQuad() {
	// VAO (Vertex Array Object) を作成してバインド
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	// VBO (Vertex Buffer Object) を作成してバインド
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	// 頂点データをVBOに書き込む
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	// 頂点属性ポインターを設定 (頂点シェーダーの layout(location = 0) に対応)
	// 属性0: 頂点位置
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);

	// 頂点属性ポインターを設定 (頂点シェーダーの layout(location = 1) に対応)
	// 属性1: テクスチャUV座標
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	// VAOのバインドを解除
	glBindVertexArray(0);
}

void init(void){
	glewInit();
	anmode = 0;

	glGenTextures(1, &imageL);
	glGenTextures(1, &imageR);

	Frame_Buffer_Sets();

	shaderProgram = LoadShaders("passthrough.vert", "interleave.frag");
	videoshaderProgram = LoadShaders("passthrough.vert", "sbs_interleave.frag");


	TeapotInit();//プラグラム実行中に m 1 の順で押すと表示されるteapotへのテクスチャマッピングの準備
	setupQuad();
	ht = 0;
	printf("head-tracking: OFF\n");
	printf("mode : SS\n");
	printf("haba = %d\n", haba);
}

void calculate_stencil() {
	int color, width;

	for (int H = 0; H < SHGT; H++) {
		for (int W = 0; W < MIM_W; W++) {
			for (int i = 0; i < 3; i++) {
				sbuf2[i][H][W] = 0;
			}
		}
	}

	/* CalStencil ステンシルの計算　*/
	int totalShift = MiddleLine - 48 * haba;
	for (int H = 0; H < SHGT; H++) {
		for (int W = 0; W < 3 * MIM_W; W++) {
			if (((W - (W - totalShift) / haba) + 2 * kk + SHIFT) % 12 < 6) {//3sub;3*4 = 12:6 = 3*2 2sub; 2*4 = 8:4 = 2*2
				color = W % 3;
				width = W / 3;
				sbuf2[color][H][width] = 255;
			}
		}
	}
}

void set_stencil_mask(int RGB) {
	int i;

	glEnable(GL_STENCIL_TEST);
	glColorMask(0, 0, 0, 0);
	glDepthMask(0);

	glStencilMask(1);
	glStencilFunc(GL_ALWAYS, 0x1, 0x1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	glViewport(0, 0, MIM_W, SHGT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRasterPos2i(-1, -1);

	glDrawPixels(MIM_W, SHGT, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, sbuf2[RGB]);

	for (i = 1; i * SHGT < IM_H  ; ++i) {
		glViewport(0, i*SHGT, MIM_W, SHGT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRasterPos2i(-1, -1);
		glCopyPixels(0, 0, MIM_W, SHGT, GL_STENCIL);
	}

	

	glColorMask(1, 1, 1, 1);
	glDepthMask(1);
	glStencilMask(0);
	glDisable(GL_STENCIL_TEST);
}

void RGBCG(int RGB)
{
	set_stencil_mask(RGB);

	if (RGB == 1)	glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
	if (RGB == 2)	glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);
	if (RGB == 0)	glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_STENCIL_TEST);

	if (VideoFlag == 1)
	{
		// 読み込んだ動画ファイルを表示
		glClear(GL_COLOR_BUFFER_BIT); // add for fbo
		if (eyeright == 1){
			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x1, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glOrtho(0.0, IM_W, 0.0, IM_H, 0.0, 1.0);
			glViewport(0, 0, IM_W, IM_H);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glEnable(GL_TEXTURE_2D);
			if (LiverMode) VideoMode->DrawRightImageLiver(IM_W, IM_H);
			else VideoMode->DrawRightImage(IM_W, IM_H, videoscale);
		}
		if (eyeleft == 1){
			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x0, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glOrtho(0.0, IM_W, 0.0, IM_H, 0.0, 1.0);
			glViewport(0, 0, IM_W, IM_H);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glEnable(GL_TEXTURE_2D);
			if (LiverMode) VideoMode->DrawLeftImageLiver(IM_W, IM_H);
			else VideoMode->DrawLeftImage(IM_W, IM_H, videoscale);
		}
	}
	else
	{
		// Teapotを表示
		if (eyeright == 1){
			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x1, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glViewport(0, 0, IM_W, IM_H);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glFrustum(0.5*(-0.5*WID - eyeposx[0]), 0.5*(0.5*WID - eyeposx[0]), 0.5*(-0.5*HGT - eyeposy[0]), 0.5*(0.5*HGT - eyeposy[0]), 0.5*eyeposz[0], zfar);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(eyeposx[0], eyeposy[0], eyeposz[0], eyeposx[0], eyeposy[0], 0, 0, 1, 0);
			glTranslatef(0, 0, zsft);
			glRotatef(theta, 0, 1, 0);
			glCallList(list);
		}

		if (eyeleft == 1){
			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x0, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glViewport(0, 0, IM_W, IM_H);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glFrustum(0.5*(-0.5*WID - eyeposx[1]), 0.5*(0.5*WID - eyeposx[1]), 0.5*(-0.5*HGT - eyeposy[1]), 0.5*(0.5*HGT - eyeposy[1]), 0.5*eyeposz[1], zfar);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(eyeposx[1], eyeposy[1], eyeposz[1], eyeposx[1], eyeposy[1], 0, 0, 1, 0);
			glTranslatef(0, 0, zsft);
			glRotatef(theta, 0, 1, 0);
			glCallList(list);
		}
	}
	glDisable(GL_STENCIL_TEST);
}

// グローバル変数に追加
float columnPitch = 4.0f;     // 物理バリアのピッチ（４ピクセル=3sub×４時分割）
float subpixelShift = 0.0f;   // キャリブレーション用の水平シフト量

// RGBCG_image() の代わりとなる新しい関数
// renderInterleavedImage() を修正
void renderInterleavedImage() {
	glUseProgram(shaderProgram); // この新しいシェーダーを読み込むようにinit()も変更
	glBindVertexArray(quadVAO);

	// --- 従来方式のパラメータを計算 ---
	// calculate_stencil()の冒頭にあった計算をここに持ってくる
	int totalShift = MiddleLine - 48 * haba;

	// --- uniform変数をシェーダーに送る ---
	// テクスチャの設定
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageL);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, imageR);
	glUniform1i(glGetUniformLocation(shaderProgram, "leftTexture"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram, "rightTexture"), 1);

	// 従来方式のパラメータをすべて送る
	glUniform1f(glGetUniformLocation(shaderProgram, "haba"), (float)haba);
	glUniform1f(glGetUniformLocation(shaderProgram, "totalShift"), (float)totalShift);
	glUniform1i(glGetUniformLocation(shaderProgram, "timeStep"), kk);
	glUniform1f(glGetUniformLocation(shaderProgram, "manualShift"), (float)SHIFT);

	// 描画
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// 後片付け
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
}

// 動画用の描画関数
// renderInterleavedVideo() を修正
void renderInterleavedVideo() {
	GLuint videoTexID = VideoMode->getVideoTextureID();
	if (videoTexID == 0) return;

	glUseProgram(videoshaderProgram);
	glBindVertexArray(quadVAO);

	// --- 従来方式のパラメータを計算 ---
	int totalShift = MiddleLine - 48 * haba;

	// --- uniform変数をシェーダーに送る ---
	// テクスチャの設定
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, videoTexID);
	glUniform1i(glGetUniformLocation(videoshaderProgram, "sbsTexture"), 0);

	// 従来方式のパラメータをすべて送る
	glUniform1f(glGetUniformLocation(videoshaderProgram, "haba"), (float)haba);
	glUniform1f(glGetUniformLocation(videoshaderProgram, "totalShift"), (float)totalShift);
	glUniform1i(glGetUniformLocation(videoshaderProgram, "timeStep"), kk);
	glUniform1f(glGetUniformLocation(videoshaderProgram, "manualShift"), (float)SHIFT);

	// 描画
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// 後片付け
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
}

int SPEED = 6;
void DTimer(int totalMilliSeconds)
{
	if (VideoSwitch) VideoMode->Update(0);
	if (arduinoSerial.is_open()) {
		char light_command;
		switch (kk) {
		case 0:light_command = LIGHT_CON_0; break;
		case 1:light_command = LIGHT_CON_1; break;
		case 2:light_command = LIGHT_CON_2; break;
		case 3:light_command = LIGHT_CON_3; break;
		}
		boost::asio::write(arduinoSerial, boost::asio::buffer(&light_command, 1));
	}
	glutTimerFunc(SPEED, DTimer, 0);
}

int frame = 0;
bool GetPic = false;
int prt = 0;


void disp(void){
	int i, e;
	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	if (flag == 1){

		glDisable(GL_TEXTURE_2D);

		if (mrk == 1){
			// 動画モード

			// FBOへの描画
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FrameBuffer);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedVideo(); // ★新しい動画描画関数を呼び出す

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			/////テクスチャを貼った、大きい板を描画
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, Tex_Name);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glPushMatrix();
			glViewport(0, 0, 1920, 1080);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glOrtho(0, 1920, 0, 1080, -1, 1);

			glBegin(GL_QUADS);
			glTexCoord2d(0, 0); glVertex2d(0, 0);
			glTexCoord2d(0, 1); glVertex2d(0, 1080);
			glTexCoord2d(1, 1); glVertex2d(1920 + 1008, 1080);
			glTexCoord2d(1, 0); glVertex2d(1920 + 1008, 0);
			glEnd();
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			
		}
		else{
			// 画像モード
			// FBOへの描画
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FrameBuffer);
			// 変更後：新しい関数を一度呼び出すだけ
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // FBOをクリア
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedImage();

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			/////テクスチャを貼った、大きい板を描画
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, Tex_Name);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glPushMatrix();
			glViewport(0, 0, 1920, 1080);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glOrtho(0, 1920, 0, 1080, -1, 1);

			glBegin(GL_QUADS);
			glTexCoord2d(0, 0); glVertex2d(0, 0);
			glTexCoord2d(0, 1); glVertex2d(0, 1080);
			glTexCoord2d(1, 1); glVertex2d(1920 + 1008, 1080);
			glTexCoord2d(1, 0); glVertex2d(1920 + 1008, 0);
			glEnd();
			glPopMatrix();
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if (running){
			kk++;
			if (kk == 4)kk = 0;
		}
	}

	else{
		sprintf(filePath, "./images/IDW/%03dl.ppm", img);
		ppm_reader(filePath, &image_texture[0][0][0][0]);
		sprintf(filePath, "./images/IDW/%03dr.ppm", img);//本来はr
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

static void KeyEvent(unsigned char key, int x, int y){
	switch (key){
	case 27:

		while (!VideoMode->video_flag) VideoMode->dispose();
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
		if (running) running = 0;
		else running = 1;
		glutDisplayFunc(disp);
		break;
	case 'o':
		SHIFT += 1;
		glutDisplayFunc(disp);
		break;
	case 'h':
		if (ht == 0){
			Sleep(100);
			cali_flag = 1;
		}
		else{
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
		if (random == 0){
			mode = 0;
			printf("mode : PP\n");
		}
		else if (random == 1){
			mode = 0;
			printf("mode : PP\n");
		}
		else if (random == 2){
			mode = 2;
		}
		else{
			mode = 1;
			printf("mode : SP\n");
		}
		glutDisplayFunc(disp);
		break;
	case'x':
		if (random == 0){
			mode = 1;
			printf("mode : SP\n");
		}
		else if (random == 1){
			mode = 2;
			printf("mode : SS\n");
		}
		else if (random == 2){
			mode = 0;
			printf("mode : PP\n");
		}
		else{
			mode = 2;
			printf("mode : SS\n");
		}
		glutDisplayFunc(disp);
		break;
	case'c':
		if (random == 0){
			mode = 2;
			printf("mode : SS\n");
		}
		else if (random == 1){
			mode = 1;
			printf("mode : SP\n");
		}
		else if (random == 2){
			mode = 1;
			printf("mode : SP\n");
		}
		else{
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
		if (mrk == 1)
		{
			VideoFlag = 0;
			flag = 0;
		}
		else
		{
			img = 2;
			random = 0;
			flag = 0;
		}
		glutDisplayFunc(disp);
		break;
	case '2':
		if (mrk == 1)
		{
			VideoFlag = 1;
			videoscale = DefaultScale;
			flag = 0;
		}
		else
		{
			img = 3;
			random = 1;
			flag = 0;
		}
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
		flag = 0;
		if (!VideoSwitch)
		{
			VideoMode->Initialize();
			VideoMode->SetSpeed(16000);
			VideoSwitch = true;
		}
		glutDisplayFunc(disp);
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
	//case '+':
	//	videoscale += 0.001;
	//	printf("%f\n", videoscale);
	//	glutDisplayFunc(disp);
	//	break;
	//case '-':
	//	videoscale -= 0.001;
	//	printf("%f\n", videoscale);
	//	glutDisplayFunc(disp);
	//	break;
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
	case 'f':
		zsft += 5;
		glutDisplayFunc(disp);
		break;
	case 'F':
		zsft -= 5;
		glutDisplayFunc(disp);
		break;
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
		// KeyEvent() 関数にキーを追加
	case 'p': // ピッチを広げる
		columnPitch += 0.01f;
		printf("Column Pitch: %f\n", columnPitch);
		break;
	case ';': // ピッチを狭める
		columnPitch -= 0.01f;
		printf("Column Pitch: %f\n", columnPitch);
		break;
	case '\'': // 右にシフト
		subpixelShift += 2.1f;
		printf("Subpixel Shift: %f\n", subpixelShift);
		break;
	case '/': // 左にシフト
		subpixelShift -= 0.1f;
		printf("Subpixel Shift: %f\n", subpixelShift);
		break;

	}
}
static void KeyUp(unsigned
	char key, int x, int y){
	switch (key){
	case 's':
		VideoMode->SetSpeed(9000);
		break;
	}
}
static void KeySpecialEvent(int key, int x, int y){
	if (key == GLUT_KEY_LEFT){
		theta = (int)(theta + 1) % 360;
		glutDisplayFunc(disp);
	}
	if (key == GLUT_KEY_RIGHT){
		theta = (int)(theta - 1 + 360) % 360;
		glutDisplayFunc(disp);
	}

}



int main(int argc, char ** argv){



	arduinoSerial.open("COM3"); // Arduinoのポート名に合わせて変更
	arduinoSerial.set_option(boost::asio::serial_port_base::baud_rate(9600));
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
	Receive(client, [](boost::system::error_code e, size_t){ std::cout << e.message() << std::endl; });
	List();
	glutDisplayFunc(disp);
	glutKeyboardFunc(KeyEvent);
	glutKeyboardUpFunc(KeyUp);
	glutSpecialFunc(KeySpecialEvent);
	glutIdleFunc(disp);

	glutMainLoop();
	client.Close();

	glDeleteTextures(1, &imageL);
	glDeleteTextures(1, &imageR);

	return 0;
}