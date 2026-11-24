#pragma once
// コマ落ち（V-Syncを1回以上飛ばしたフレーム）の検出。
// 時分割は「1回のswap = 1回のリフレッシュ」を前提にしているため、
// swapの間隔がリフレッシュ周期より長いと、その位相が長く表示されている。
#include <chrono>

using FrameClock = std::chrono::steady_clock;

// ディスプレイのリフレッシュレートを取得して表示する。init() から1回呼ぶ。
void InitFrameDropMonitor();
int DisplayRefreshHz();
// 停止中・画像読み込み・モード切替など、意図した中断の後に呼ぶ。
// 次のフレームを新しい基準にして、中断をコマ落ちとして数えない。
void ResetFrameDropBaseline();
// RUN中に1フレーム提示するたびに、swap直後の時刻を渡す。
void RecordPresentedFrame(FrameClock::time_point presentedAt);
// 前回の呼び出し以降に落ちたフレーム数（性能ログ用）。
unsigned TakeDroppedFrames();
unsigned long long TotalDroppedFrames();
