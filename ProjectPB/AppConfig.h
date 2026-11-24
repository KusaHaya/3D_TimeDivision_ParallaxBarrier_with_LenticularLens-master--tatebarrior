#pragma once
// 表示アプリ全体で使う固定値（画面サイズ・初期値・カメラ設置角）。

#define IM_W 1920 // image width
#define IM_H 1080 // image height

#define haba_first 330 // default 500

#define REFRESH_RATE 120 // fallback only: the real rate is read from Windows (FrameDropMonitor)

#define SIM_W 1920 // calibration image width
#define SIM_H 1080 // calibration image height

// カメラの設置角の初期値。実行時は cameraTiltSin / cameraTiltCos（設定ファイルで変更可）を使う。
#define SIN 0.139
#define COS 0.99


const int MiddleDefault = 3420;
//const int MiddleDefault = 2340;
