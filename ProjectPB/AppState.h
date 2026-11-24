#pragma once
// 複数の機能ファイルから共有される実行時の状態。
// 定義は AppState.cpp にある。
#include <GL/glew.h>
#include "AppConfig.h"

// --- バリア調整 ---
extern int SHIFT;               // 静止画と動画の表示位相を合わせる手動補正。
extern int haba;                // phase width with 1/3 pixel (= sub-pixel)
extern int MiddleLine;
extern bool showAlignmentCenter;
extern int barrierThresholdStep;
extern float barrierSlantY;

// --- 時分割 ---
extern int running;             // 0:test mode on, 1:off(time-division running)
extern int kk;                  // 時分割表示の位相（0～3）。
extern int displayedKk;
extern bool frameHoldMode;
extern bool holdCommandPending;

// --- ヘッドトラッキング ---
extern int ht;                  // head-tracking mode 0:OFF, 1:ON
extern int cali_flag;           // calibation mode
extern float eyeposx[2];
extern float eyeposy[2];
extern float eyeposz[2];
extern float delta;
extern float horizontalTrackingGain;
extern float DotPixel;
extern float DotSubPixel;
extern float headTrackShift;
extern float caliX, caliY, caliZ;
extern float tmpX, tmpY, tmpZ;
extern float PosX, PosY, PosZ;
extern double cameraTiltSin, cameraTiltCos; // カメラの設置角（初期値は AppConfig.h の SIN / COS）
extern int trackingBaseHaba;
extern int trackingBaseMiddleLine;

// --- 表示内容 ---
extern int flag;                // an auto parameter acting as a switch signal
extern int img;                 // image number: 1~10
extern int qq;
extern int mrk;                 // 1:動画, 0:静止画
extern int eyeright;            // 右目画像の表示設定。
extern int eyeleft;             // 左目画像の表示設定。
extern bool VideoSwitch;
extern GLuint imageL, imageR;   // 静止画表示用の左右テクスチャ。
