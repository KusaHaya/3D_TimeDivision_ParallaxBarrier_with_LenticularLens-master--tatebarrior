#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "AppState.h"

// 静止画と動画の表示位相を合わせるための手動補正。
// 表示パネルに合わせて調整する。
int SHIFT = 1;

float eyeposx[2];
float eyeposy[2];
float eyeposz[2];
float zfar = 3000;
float zsft = 0;
float alphax = 1.0;
int habat = 0;

float crosstalkFactor = 0.15f; // クロストーク補正係数（15%）。
float headTrackShift = 0.0f; // 頭部追跡による横方向の補正量（サブピクセル単位）。

// 色補正の設定値。
float colorGainR = 1.00f;
float colorGainG = 1.00f;
float colorGainB = 1.00f;
float colorGamma = 1.00f;
bool colorCorrectionEnabled = true;

int running = 1; // 0:test mode on, 1:off(time-division running)
int ht = 0; // head-tracking mode 0:OFF, 1:ON
int mode = 2;// system mode 
// 0:pixel barrier by pixel units
// 1:sub-pixel barrier by pixel units
// 2:sub-pixel barrier by sub-pixel units
char anmode = 0; // anaglyph mode


float delta = 0.0;
float horizontalTrackingGain = 0.7f; // 左右共通の横方向補正倍率。RealSenseの測定値そのものは変更しない。
float DotPixel = 0.27;
float DotSubPixel = 0.09;// DotSubPixel=DotPixel(=0.27)/3
float theta = 30; // ティーポットの回転角。

int flag = 0; // an auto parameter acting as a switch signal
int img = 1; // image number: 1~10
int qq = 1; // an auto parameter acting as a switch signal
int haba = haba_first; // phase width with 1/3 pixel (= sub-pixel)
int kk = 0; // 時分割表示の位相（0～3）。
int displayedKk = 0;
bool frameHoldMode = false;
bool holdCommandPending = false;
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

GLuint imageL, imageR;

int MiddleLine = MiddleDefault;
bool showAlignmentCenter = false; // Diagnostic overlay; OFF during measurements.
// Preserve the manually aligned display as the origin when tracking starts.
int trackingBaseHaba = haba_first;
int trackingBaseMiddleLine = MiddleDefault;
int mrk = 0;
int eyeright = 1; // 右目画像の表示設定。
int eyeleft = 1; // 左目画像の表示設定。
float videoscale = 1.0f;//0.923f
bool VideoSwitch = false;

// キーボード操作用のランダム表示設定。
int random = 0; // random mode for test

int barrierThresholdStep = 16; // 16/20 = 0.8; exact 0.05 steps.
float barrierSlantY = 1.0f;   // 斜めバリアの縦方向係数。

bool ReserveLR = true;

int frame = 0;
bool GetPic = false;
int prt = 0;
