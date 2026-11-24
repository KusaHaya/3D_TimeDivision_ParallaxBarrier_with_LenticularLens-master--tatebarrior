#pragma once
// 時分割表示の位相とバックライトの同期、フレーム停止（ホールド）。

// 提示したフレームの位相をArduinoへ送る（RUN中のみ）。
void SendTimeDivisionFrame(int frameIndex);
// 停止中の位相をArduinoへ送り、バックライトを点灯したままにする。
void SendHeldFrame(int frameIndex);
// kk=0 から時分割を再開する。
void RestartTimeDivisionSync(const char* modeName);
// 指定した位相で表示とバックライトを止める（t）。
void HoldFrameAndBacklight(int frameIndex);
// 停止中に表示する位相を選ぶ（q/w/e/r）。
void SelectFrameWhileHeld(int frameIndex);
