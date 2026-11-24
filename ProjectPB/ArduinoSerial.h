#pragma once
// Arduino（バックライト制御）とのシリアル通信。
#include <string>

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
#define HOLD_FRAME_0 210
#define HOLD_FRAME_1 211
#define HOLD_FRAME_2 212
#define HOLD_FRAME_3 213

// 115200 baud で開く。失敗したら false とエラーメッセージを返す。
bool OpenArduinoSerial(const char* port, std::string& errorMessage);
bool IsArduinoOpen();
// Arduinoへコマンドを1バイトで送信する。
void SendArduinoCommand(int command);
