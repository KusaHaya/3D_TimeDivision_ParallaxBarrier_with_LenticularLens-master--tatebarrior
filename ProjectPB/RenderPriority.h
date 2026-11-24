#pragma once
// 描画スレッド（GLUTのスレッド）の優先度を上げ、Windowsに一瞬止められて
// swapが遅れる（[DROP]）のを減らす。MMCSS の "Games" タスクとして登録する。
// 比較用に、環境変数 PROJECTPB_RENDER_PRIORITY=normal で元の優先度のまま動かせる。

void RaiseRenderThreadPriority();
void RestoreRenderThreadPriority();
