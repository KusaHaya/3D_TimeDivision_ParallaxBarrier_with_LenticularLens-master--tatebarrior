#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include "TCPClient.h"
#include "boost/format.hpp"
#include <windows.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <thread>
#include "DrawVideo.h"
#include "Texture.h"

#define IM_W 1920 //image width
#define IM_H 1080 //image height
#define WIN_W 3840 //window width
#define WIN_H 1080 //window height
#define haba_first 180
#define WID 528
#define HGT 297
#define SHIFT 5//静止画と動画のパターンの間で生じる位置差の補正（テクスチャマッピングとステンシルマスク）//パネルごとに設定する必要あり
#define SFT 4//キャリブレーションの黒白の中心位置補正
#define ResolutionW 1920
#define ResolutionH 1080

#define INIX 120 
#define INIY 0
#define INIX2 2430 //1920+510
#define INIY2 160

//teapotに貼るtextureの縦横（2^n 2^mでないといけない）
#define TEAPOT_HEIGHT 512
#define TEAPOT_WIDTH  512

float eyeposx[2];
float eyeposy[2];
float eyeposz[2];
float zfar = 3000;
//#define NUM 3
//#define number 1365 //devide image pattern
#define DEMO 1 //demo:1,test:0h
float zsft=50;

GLuint Tex[4];//fixed barrier
GLuint TexS[4];//fixed image by sub-pixel units
GLuint TexP[4];//fixed image by pixel units
GLuint Stripe;

vmlab::Texture texL[4];
std::unique_ptr<vmlab::DrawVideo> VideoMode;

int k = 0;//frame switch signal,0~3
int off = 0;//mirror:450;//test:30;//barrier vertical offset
int off2 = 1920;//mirror:300;//test:1200;//imhhhhhhage vertical offset
int size = 40 + 470 * DEMO;//hideing area wide size
int mod;//an auto parameter
int running=1;//0:test mode on, 1:off(time-division running)
int mode=2;//system mode 0:pixel barrier by pixel units, 1:sub-pixel barrier by pixel units, 2:sub-pixel barrier by sub-pixel units
char filePath[32];//store filename
int ctrl=0;
int ht=0;//head-tracking mode 0:OFF, 1:ON
int move=0;
char anmode = 0;//anaglyph mode
int upt = 0;

float delta=0.0;
float DotPixel=0.27;
float DotSubPixel=0.09;//DotSubPixel=DotPixel(=0.27)/3
float theta = 30;

int wi=1;//image pattern width
int parawid=1;//barrier width*hight
int flag=0;//an auto parameter acting as a switch signal
int img=1;//image number: 1~10
int qq=1;//an auto parameter acting as a switch signal
int haba=haba_first;//phase width with 1/3 pixel (= sub-pixel)
int timg=2;//stores the number of the image to be shown (= 3 sub-pixel)
int tk=1;
int kk=0;
int cali_flag=0;//calibation mode
int phaba = 0;
int pMiddleLine = 0;
bool LiverMode = false;

float eyeRX;//right eye x
float eyeLX;//left eye x
float eyeRY;//right eye y
float eyeLY;//left eye y
float eyeRZ;//right eye z
float eyeLZ;//left eye z

float caliX=0;//head tracking base X
float caliY=0;//head tracking base Y
float caliZ=0;//head tracking base Z
float tmpX;//head tracking current X
float tmpY;//head tracking current Y
float tmpZ;//head tracking current Z
float PosX = 0;
float PosY = -0.18;
float PosZ = 0.11;

vmlab::Texture BlackView;
unsigned char view[4][IM_H][IM_W][3];//4-view source
unsigned char cgview[2][IM_H][IM_W][3];//4-view source
unsigned char viewadP[4][1920][1920][3];//4-view source changed into texture size
unsigned char viewadS[4][1920][1920][3];//4-view source changed into texture size
unsigned char barrierF[6][2290][3];//fixed barrier pattern:[H][W][RGB]

int MiddleDefault = 3420;
int MiddleLine=MiddleDefault;
float face=0.0;
float detX=0.0;
int mrk = 0;
int eyeright = 1;
int eyeleft = 1;
const float DefaultScale = 1.0f;
float videoscale = 1.0f;//0.923f
bool VideoSwitch = false;

int random=0;//random mode for test

int stencil_mask[12];
int list, drawpixels, btex;
unsigned char sbuf[12][1080][1920];
int VideoFlag = 1;
int FrameId = 0;

bool ReserveLR = true;

//teapotテクスチャマッピング
static GLubyte Teapotimage[TEAPOT_HEIGHT][TEAPOT_WIDTH][4];
GLuint teapot;

//teapot回転
int preX = 0, preY = 0, dx, dy, IntegralX = 0, IntegralY = 0;
int drugMode = 0;
double sqxy = 0;
GLfloat rMat[16], rMat2[16], rMat3[16];

//Teapotにはるテクスチャ(TGAファイル)の読み込みor生成
void TeapotInitTexture(void){

	FILE *fp;
	errno_t error;
	int x, z;

	// texture file open 
	if ((error = fopen_s(&fp, "test.tga", "rb")) != 0){//fopenだとビルド時に怒られたのでfopen_s使用
		fprintf(stderr, "texture file cannot open\n");
		return;
	}
	fseek(fp, 18, SEEK_SET);//TGAファイルの最初18byteはRGBAではなく画像の大きさとかそういうデータだから飛ばす。
	for (x = 0; x<TEAPOT_HEIGHT; x++){
		for (z = 0; z<TEAPOT_WIDTH; z++){
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

/*bool GLEW_INIT()
{
	GLenum err;
	err = glewInit();
	if (err != GLEW_OK){
		std::cerr << glewGetErrorString(err) << "\n";
		return false;
	}
	return true;
}*/

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
		/*
		glRotatef(30, 1, 0, 0);
		glRotatef(30, 0, 1, 0);
		glRotatef(30, 0, 0, 1);
		*/

		glRotatef(-theta, 0, 1, 0);
		glBindTexture(GL_TEXTURE_2D, teapot);
		glEnable(GL_TEXTURE_2D);

		glBegin(GL_POLYGON);
		glTexCoord2d(0.0, 1.0);	 glVertex3f(-320.0, 180.0, -300.0); //glNormal3f(0, 0, 1);
		glTexCoord2d(0.0, 0.0);	glVertex3f(-320.0, -180.0, -300.0); //glNormal3f(0, 0, 1);
		glTexCoord2d(1.0, 0.0);	glVertex3f(320.0, -180.0, -300.0); //glNormal3f(0, 0, 1);
		glTexCoord2d(1.0, 1.0);	glVertex3f(320.0, 180.0, -300.0); //glNormal3f(0, 0, 1);
		glEnd();

		glDisable(GL_TEXTURE_2D);

		glRotatef(theta, 0, 1, 0);
		glBindTexture(GL_TEXTURE_2D, teapot);
		glEnable(GL_TEXTURE_2D);

		glutSolidTeapot(100);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
	}
	glEndList();
}

void DrawTexture()
{
	btex = glGenLists(1);
	glNewList(btex, GL_COMPILE_AND_EXECUTE);
	{
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 1.0);	glVertex2d(-1.0, 1.0);
		glTexCoord2d(0.0, 0.0);	glVertex2d(-1.0, -1.0);
		glTexCoord2d(1.0, 0.0);	glVertex2d(1.0, -1.0);
		glTexCoord2d(1.0, 1.0);	glVertex2d(1.0, 1.0);
		glEnd();
	}
	glEndList();
}
	
void cali_rendering(void){

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_POLYGON);
	glVertex3f(-300.0, 300.0, 10.0); glNormal3f(0, 0, 1);
	glVertex3f(-300.0, -300.0, 10.0); glNormal3f(0, 0, 1);
	glVertex3f(300.0, -300.0, 10.0); glNormal3f(0, 0, 1);
	glVertex3f(300.0, 300.0, 10.0); glNormal3f(0, 0, 1);
	glEnd();

}

void initStencilMask()
{
	int R_W = 1920;
	int R_H = 1080;
	int i, j, k;
	int CONST_1 = 0 , CONST_2 = 0 , CONST_3 =2;//要調整 -> 調整済
	for (k = 0; k < 12; k++)
		for (i = 0; i < R_W; i++)
	for (j = 0; j < R_H; j++){
		if (k % 3 == 0){
			if ((R_W - 1 - i) % 4 == (((j + 16 + CONST_1) - 2 * ((R_W - 1 - i) % 8 )) % 16 / 4 + k / 3) % 4){
				sbuf[k][j][i] = 0x1;
			}
			else if ((R_W - 1 - i) % 4 == (((j + 16 + CONST_1) - 2 * ((R_W - 1 - i) % 8)) % 16 / 4 + k / 3 + 1) % 4){
				sbuf[k][j][i] = 0x1;
			}
			else sbuf[k][j][i] = 0x0;
		}
		else if (k % 3 == 1){
			if ((R_W - 1 - i) % 4 == (((j + 16 + CONST_2) - 2 * ((R_W - 1 - i) % 8)) % 16 / 4 + k / 3) % 4){
				sbuf[k][j][i] = 0x1;
			}
			else if ((R_W - 1 - i) % 4 == (((j + 16 + CONST_2) - 2 * ((R_W - 1 - i) % 8)) % 16 / 4 + k / 3 + 1) % 4){
				sbuf[k][j][i] = 0x1;
			}
			else sbuf[k][j][i] = 0x0;
		}
		else{
			if ((R_W - 1 - i) % 4 == (((j + 16 + CONST_3) - 2 * ((R_W - 1 - i) % 8)) % 16 / 4 + k / 3) % 4){
				sbuf[k][j][i] = 0x1;
			}
			else if ((R_W - 1 - i) % 4 == (((j + 16 + CONST_3) - 2 * ((R_W - 1 - i) % 8)) % 16 / 4 + k / 3 + 1) % 4){
				sbuf[k][j][i] = 0x1;
			}
			else sbuf[k][j][i] = 0x0;
		}
	}


	//----- 簾を描く -----
	for (k = 0; k < 12; k++){
		stencil_mask[k] = glGenLists(1);
		glNewList(stencil_mask[k], GL_COMPILE_AND_EXECUTE);
		{
			glEnable(GL_STENCIL_TEST);
			glColorMask(0, 0, 0, 0);
			glDepthMask(0);
			glStencilMask(1);
			glStencilFunc(GL_ALWAYS, 0x1, 0x1);
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

			glViewport(1920, 0, R_W, R_H);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRasterPos2i(-1, -1);
			glDrawPixels(R_W, R_H, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, sbuf[k]);

			glColorMask(1, 1, 1, 1);
			glDepthMask(1);
			glDisable(GL_STENCIL_TEST);
		}
		glEndList();
		glEndList();
	}
}

void pattern(unsigned char pic[4][1080][1920][3]){
	int AA = 0, BB = 0;
	int i, j, k, p, nk, rk, c0, c1, c2, mshift, jj;
	int sft[2][8][3] = { 0, 0, 3, 2, 1, 1, 3, 3, 2, 1, 0, 0, 2, 2, 1, 0, 3, 3, 1, 1, 0, 3, 2, 2,
		1, 0, 0, 2, 2, 1, 0, 3, 3, 1, 1, 0, 3, 2, 2, 0, 0, 3, 2, 1, 1, 3, 3, 2 };
	c0 = anmode * 0; c1 = anmode * 1; c2 = anmode * 2;

	/*
	for (i = 0; i<4; i++)
		for (j = 0; j < 1080; j++)
			for (k = 0; k < 1920; k++)
				for (p = 0; p < 3; p++){
				viewadP[i][j + AA][k + BB][p] = pic[(k % 4 + j / 2 + 6 + i) % 4][j][IM_W - k][p];
			}

	for (i = 0; i < 4; i++){//make textures	
			glBindTexture(GL_TEXTURE_2D, TexP[i]);
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 1024, 1024, GL_RGB, GL_UNSIGNED_BYTE, viewadP[i]);
	}*/

	for (i = 0; i < 4; i++){
		if (i == 0){ mshift = 3; }
		else if (i == 1){ mshift = 0; }
		else if (i == 2){ mshift = 5; }
		else if (i == 3){ mshift = 2; }
		for (j = 0; j < IM_H; j++){
			jj = j / 2;
			for (k = 0; k < IM_W; k++){
				nk = (k + i + mshift) % 8;
				if (jj % 2 == 0){
					viewadS[i][j + AA][k + BB][0] = pic[(jj / 2 + sft[0][nk][0]) % 4][j][IM_W - k][0];
					viewadS[i][j + AA][k + BB][1] = pic[(jj / 2 + sft[0][nk][1]) % 4][j][IM_W - k][1];
					viewadS[i][j + AA][k + BB][2] = pic[(jj / 2 + sft[0][nk][2]) % 4][j][IM_W - k][2];
				}
				else if (jj % 2 == 1){
					viewadS[i][j + AA][k + BB][0] = pic[(jj / 2 + sft[1][nk][0]) % 4][j][IM_W - k][0];
					viewadS[i][j + AA][k + BB][1] = pic[(jj / 2 + sft[1][nk][1]) % 4][j][IM_W - k][1];
					viewadS[i][j + AA][k + BB][2] = pic[(jj / 2 + sft[1][nk][2]) % 4][j][IM_W - k][2];
				}
			}
		}
	}

	for (i = 0; i < 4; i++){//make textures	
		texL[i].ReloadSub(0, 0, 1920, 1080, GL_RGB, (BYTE *)viewadS[i]);
	}
}
/*
void pattern(){

	int i, j, k, p, pp, jj;

	for (i = 0; i<4; i++)//timedivision
		for (j = 0; j < IM_H; j++){
		jj = j / 2;
		for (k = 0; k < IM_W; k++)
			for (p = 0; p < 3; p++){
			if (anmode == 0){
				pp = 0;
			}
			else if (anmode == 1){
				pp = p;
			}
			else{
				if (p == 0) pp = 0;
				else if (p == 1) pp = 2;
				else pp = 3;
			}
			if (jj % 3 == 0)//j=0,1,6,7,...
			{
				viewadS[i][j][k][p] = view[(k % 4 + j / 4 + 3 + i + pp) % 4][j][IM_W - k][p];
			}
			else

				if (jj % 3 == 1)//j=2,3,8,9...
				{
				viewadS[i][j][k][p] = view[(k % 4 + j / 4 + 3 + i + (1 - (p + 1) / 2) + pp) % 4][j][IM_W - k][p];
				}
				else viewadS[i][j][k][p] = view[(k % 4 + j / 4 + 3 + i + (1 - p / 2) + pp) % 4][j][IM_W - k][p];

			}
		}

	for (i = 0; i<4; i++){//make textures	
		texL[i].ReloadSub(0, 0, 1920, 1080, GL_RGB, (BYTE *)viewadS[i]);
	}

}
*/
void Receive(TCPClient& client, const std::function<void(boost::system::error_code, std::size_t)>& callback)
{
	auto buffer = std::make_shared<boost::asio::streambuf>(sizeof(float)* 6);
	client.RecieveAsync(buffer, [&, buffer](boost::system::error_code e, size_t l)
	{
		const float* datas = boost::asio::buffer_cast<const float*>(buffer->data());
//		for (int i = 0; i < 2; ++i)
//			std::cout << boost::format("(%f, %f, %f)") % datas[3 * i + 0] % datas[3 * i + 1] % datas[3 * i + 2] << std::endl;
		printf("%f,%f,%f,%f,%f,%f\n", datas[0], datas[1], datas[2], datas[3], datas[4], datas[5]);
		if (cali_flag==1){
			caliX=(datas[3*0+0]+datas[3*1+0])/2+PosX;
			caliY=(datas[3*0+1]+datas[3*1+1])/2+PosY;
			caliZ=(datas[3*0+2]+datas[3*1+2])/2+PosZ;
			ht=1;
			printf("head-tracking: ON\n");
			cali_flag=0;
		}else{
			tmpX=(datas[3*0+0]+datas[3*1+0])/2+PosX;
			tmpY=(datas[3*0+1]+datas[3*1+1])/2+PosY;
			tmpZ=(datas[3*0+2]+datas[3*1+2])/2+PosZ;
			eyeposx[0] = 1000 * (datas[3 * 1 + 0] + PosX);
			eyeposx[1] = 1000 * (datas[3 * 0 + 0] + PosX);
			eyeposy[0] = 1000 * (datas[3 * 1 + 1] + PosY);
			eyeposy[1] = 1000 * (datas[3 * 0 + 1] + PosY);
			eyeposz[0] = 1000 * (datas[3 * 1 + 2] + PosZ);
			eyeposz[1] = 1000 * (datas[3 * 0 + 2] + PosZ);
		}

		if (ht == 1){
			//haba = haba_first + (int)((tmpZ - caliZ) * 1000 / 6.9);
			haba = (int)(haba_first * tmpZ / caliZ);
			if (abs(phaba - haba) < 4){
				haba = phaba;
			}
			phaba = haba;
			delta = ((tmpX - caliX) + (tmpY - caliY) / 3) * 1000 / DotSubPixel;//face moves by sub-pixel units
			if (delta<0.0)move = (int)(delta - 0.5);
			else move = (int)(delta + 0.5);
			MiddleLine = MiddleDefault + move;
			if (abs(pMiddleLine - MiddleLine) < 15){
				MiddleLine = pMiddleLine;
			}
			pMiddleLine = MiddleLine;
		}
		buffer->consume(sizeof(float)* 6);
		Receive(client, callback);
	});
}

int ppm_read(char *filename, unsigned char *pimage){
	FILE *fp;
	if ((fp = fopen(filename, "rb")) == NULL){
		printf("FAIL\n", filename);
		exit(-1);
	}
	fseek(fp, 15, SEEK_SET);//skip head
	fread(pimage, sizeof(char), IM_W*IM_H * 3, fp);
	fclose(fp);
	return 0;
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

int drawImage(void){

	glBegin(GL_QUADS);
	glTexCoord2d(1.0, 0.0);	glVertex2d(-1.0,  1.0);
	glTexCoord2d(1.0, 1.0);	glVertex2d(-1.0, -1.0);
	glTexCoord2d(0.0, 1.0);	glVertex2d( 1.0, -1.0);
	glTexCoord2d(0.0, 0.0);	glVertex2d( 1.0,  1.0); 
	glEnd();
	return 0;
}


int ParallaxBarrier2(int frame){
	
	int width=0,color=0;

	//make barrier
	for(int H=0;H<6;H++)
		for(int W=0;W<2290;W++)
			for (int RGB=0;RGB<3;RGB++){
				barrierF[H][W][RGB]=0;
			}	

	for (int H = 0; H < 6; H++){
		int halfshift=0;
		for(int W=0;W<6840;W++){
			if (H % 2 == 0){
				halfshift = 0;
			}
			else{
				halfshift = - haba / 2;
			}
			if ((((W + H / 2) % 24 - (W - MiddleLine +	halfshift + haba * 100) / haba) / 2 + SFT - frame) % 4 == 0){
				color = W % 3;
				width = W / 3;
				if (anmode < 2){
					barrierF[H][width + anmode*color][color] = 255;
				}
				else{
					barrierF[H][width + 2 * color - color / 2][color] = 255;
				}
			}				
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glBindTexture(GL_TEXTURE_2D, Stripe);
//	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 1160, 3, GL_RGB, GL_UNSIGNED_BYTE, barrierF);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2290, 6, 0, GL_RGB, GL_UNSIGNED_BYTE, barrierF);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
//	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	return 0;
}

void init(void){

	anmode = 0;

	glGenTextures(8, &Tex[0]);
	glGenTextures(8, &TexS[0]);
	glGenTextures(8, &TexP[0]);
	glGenTextures(8, &Stripe);

	sprintf(filePath, "./images/test4/001r.ppm");//black image
	ppm_read(filePath, &view[0][0][0][0]);

	BlackView.ReloadSub(0, 0, 1920, 1080, GL_RGB, (BYTE *)view);
	//glBindTexture(GL_TEXTURE_2D, Tex[0]);
	//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 1024, 1024, GL_RGB, GL_UNSIGNED_BYTE, view);

	TeapotInit();//プラグラム実行中に m 1 の順で押すと表示されるteapotへのテクスチャマッピングの準備

	ht = 0;
	printf("head-tracking: OFF\n");
	printf("mode : SS\n");
	printf("haba = %d\n", haba);
}

void RGBCG(int RGB,int k)
{
	GLfloat nad[] = { 1.0, 1.0, 1.0, 1.0 };

	glCallList(stencil_mask[(3 * (3 - k) + RGB+SHIFT) % 12]);
	if (RGB == 0)	glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
	if (RGB == 1)	glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);
	if (RGB == 11)	glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_STENCIL_TEST);
	if (VideoFlag == 1)
	{
		if (eyeright == 1){
			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x1, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glOrtho(0.0, 1920, 0.0, 1080, 0.0, 1.0);
			glViewport(1920, 0, 1920, 1080);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glEnable(GL_TEXTURE_2D);
			if (LiverMode) VideoMode->DrawRightImageLiver(1920, 1080);
			else VideoMode->DrawRightImage(1920, 1080, videoscale);
		}
		if (eyeleft == 1){
			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x0, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glOrtho(0.0, 1920, 0.0, 1080, 0.0, 1.0);
			glViewport(1920, 0, 1920, 1080);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glEnable(GL_TEXTURE_2D);
			if (LiverMode) VideoMode->DrawLeftImageLiver(1920, 1080);
			else VideoMode->DrawLeftImage(1920, 1080, videoscale);
		}
	}
	else
	{
		if (eyeright == 1){

			glClear(GL_DEPTH_BUFFER_BIT);
			glStencilFunc(GL_EQUAL, 0x1, 0x1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glViewport(1920, 0, 1920, 1080);
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
			glViewport(1920, 0, 1920, 1080);
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

int SPEED = 16;
void DTimer(int totalMilliSeconds)
{
	if (VideoSwitch) VideoMode->Update(0);
	glutTimerFunc(SPEED, DTimer, 0);
	//timer.start();
	//int i=VideoMode->Update(totalMilliSeconds);
	//timer.stop();
	//int ttt = (int)timer.getElapsedTimeInMilliSec();
	//printf("[%d]", ttt);
	//if (i<ttt) glutTimerFunc(SPEED, DTimer, 0);
	//else glutTimerFunc(i - ttt + SPEED, DTimer, 0);
	//glutTimerFunc(SPEED, DTimer, 0);
}
int frame = 0;
bool GetPic = false;
int prt = 0;
void disp(void){
	int i,e;
	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//	glClear(GL_COLOR_BUFFER_BIT);
	if (flag == 1){

		//barrier
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);

		ParallaxBarrier2(kk);
		//		DrawTexture();
		for (int i = 0; i < 180; i++){
			glViewport(0 - i, 6 * i, 2290, 6);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRasterPos2i(-1, -1);
			glBindTexture(GL_TEXTURE_2D, Stripe);
			glBegin(GL_QUADS);
			glTexCoord2d(0.0, 1.0);	glVertex2d(-1.0, 1.0);
			glTexCoord2d(0.0, 0.0);	glVertex2d(-1.0, -1.0);
			glTexCoord2d(1.0, 0.0);	glVertex2d(1.0, -1.0);
			glTexCoord2d(1.0, 1.0);	glVertex2d(1.0, 1.0);
			glEnd();
			//glCallList(btex);
		}
		glEnable(GL_DEPTH_TEST);
		//Hiding the unnecessary area
		if (mrk == 1)
		{
			glBlendFunc(GL_ONE, GL_ONE);
			glBindTexture(GL_TEXTURE_2D, BlackView.GetID());
			{
				glEnable(GL_TEXTURE_2D);
				glViewport(WIN_W / 2, 0, 400, 1080);
				glBegin(GL_QUADS);
				glTexCoord2d(1.0, 0.0);	glVertex2d(-1.0, 1.0);
				glTexCoord2d(1.0, 1.0);	glVertex2d(-1.0, -1.0);
				glTexCoord2d(0.0, 1.0);	glVertex2d(1.0, -1.0);
				glTexCoord2d(0.0, 0.0);	glVertex2d(1.0, 1.0);
				glEnd();
			}
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}
		glDisable(GL_TEXTURE_2D);
		//image
		if (mrk == 1){
				RGBCG(0, kk);
				RGBCG(1, kk);
				RGBCG(11, kk);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		}
		else{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texL[kk].GetID());
			{
				glEnable(GL_TEXTURE_2D);
				glViewport(1920, 0, 1920, 1080);
				glBegin(GL_QUADS);
				glTexCoord2d(1.0, 0.0);	glVertex2d(-1.0, 1.0);
				glTexCoord2d(1.0, 1.0);	glVertex2d(-1.0, -1.0);
				glTexCoord2d(0.0, 1.0);	glVertex2d(1.0, -1.0);
				glTexCoord2d(0.0, 0.0);	glVertex2d(1.0, 1.0);
				glEnd();
			}
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}

		if (running){
			kk++;
			if (kk==4)kk=0;
//			if (kk == 0)kk = 2;
//			else if (kk == 2)kk = 1;
//			else if (kk == 1)kk = 3;
//			else if (kk == 3)kk = 0;
		}
	}


	else{
		sprintf(filePath, "./images/IDW/%03dl.ppm", img);
		ppm_reader(filePath, &view[0][0][0][0]);
		ppm_reader(filePath, &view[1][0][0][0]);
		sprintf(filePath, "./images/IDW/%03dr.ppm", img);
		//ppm_read(filePath, &view[1][0][0][0]);
		ppm_reader(filePath, &view[2][0][0][0]);
		ppm_reader(filePath, &view[3][0][0][0]);
		pattern(view);

		//img=3-img;
		//			img++;
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
		kk=0;
		glutDisplayFunc(disp);
		break;
	case 'w':
		kk=1;
		glutDisplayFunc(disp);
		break;
	case 'e':
		kk=2;
		glutDisplayFunc(disp);
		break;
	case 'r':
		kk=3;
		glutDisplayFunc(disp);
		break;
	case 't':
		if (running) running=0;
		else running=1;
		glutDisplayFunc(disp);
		break;
	case 'h':
		if (ht==0){
		Sleep(100);
		cali_flag=1;
		}
		else{
		Sleep(100);
		ht=0;
		haba=haba_first;
		move=0;
		printf("head-tracking: OFF\n");
		}
		MiddleLine = MiddleDefault;
		glutDisplayFunc(disp);
		break;
	case 'a':
		haba=haba+1;
		printf("haba = %d\n",haba);
		glutDisplayFunc(disp);
		break;
	case 's':
		VideoMode->SetSpeed(200);
		break;
	case'z':
		if (random==0){
			mode=0;
			printf("mode : PP\n");
		}else if (random==1){
			mode=0;
			printf("mode : PP\n");
		}else if (random==2){
			mode=2;
			printf("mode : SS\n");
		}else{
			mode=1;
			printf("mode : SP\n");
		}
		glutDisplayFunc(disp);
		break;
	case'x':
		if (random==0){
			mode=1;
			printf("mode : SP\n");
		}else if (random==1){
			mode=2;
			printf("mode : SS\n");
		}else if (random==2){
			mode=0;
			printf("mode : PP\n");
		}else{
			mode=2;
			printf("mode : SS\n");
		}
		glutDisplayFunc(disp);
		break;
	case'c':
		if (random==0){
			mode=2;
			printf("mode : SS\n");
		}else if (random==1){
			mode=1;
			printf("mode : SP\n");
		}else if (random==2){
			mode=1;
			printf("mode : SP\n");
		}else{
			mode=0;
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
		if (mrk==1)
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
		img=8;
		flag=0;
		glutDisplayFunc(disp);
		break;
	case '8':
		img=9;
		flag=0;
		glutDisplayFunc(disp);
		break;
	case '9':
		img=10;
		flag=0;
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
	case '+':
		videoscale += 0.001;
		printf("%f\n",videoscale);
		glutDisplayFunc(disp);
		break;
	case '-':
		videoscale -= 0.001;
		printf("%f\n", videoscale);
		glutDisplayFunc(disp);
		break;
	case 'S':
		VideoMode->Mode3D = !VideoMode->Mode3D;
		glutDisplayFunc(disp);
		break;
	case 'p':
		VideoMode->printflag=false;
		glutDisplayFunc(disp);
		break;
	case 'R':
		ReserveLR = !ReserveLR;
		if (ReserveLR) printf("RL\n");
		else printf("LR\n");
		glutDisplayFunc(disp);
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
int main(int argc, char ** argv){

	TCPClient client("127.0.0.1", 30000);
	glutInit(&argc , argv);
	glutInitWindowSize(WIN_W/(2-DEMO),WIN_H);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL/*| GLUT_STEREO*/);
	glutCreateWindow("Test");
/*	if (GLEW_INIT() == false){
		return -1;
	}*/

	VideoMode = std::unique_ptr<vmlab::DrawVideo>(new vmlab::DrawVideo);
	VideoSwitch = false;
	glutTimerFunc(0, DTimer, 0);

	init();
	initStencilMask();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	Receive(client, [](boost::system::error_code e, size_t){ std::cout << e.message() << std::endl; });
	glutFullScreen();
	List();
	glutDisplayFunc(disp);
	glutKeyboardFunc(KeyEvent);
	glutKeyboardUpFunc(KeyUp);
	glutIdleFunc(disp);
	glutMainLoop();
	client.Close();
	return 0;
}