#pragma once
// 外部EyeTracker-RS（OSC）の目の位置から、バリア幅と中心線を補正する。

void StartHeadTracking();
void StopHeadTracking();
// 毎フレーム呼ぶ。h で基準位置を取り、追跡中は haba と MiddleLine を更新する。
void UpdateTracking();
