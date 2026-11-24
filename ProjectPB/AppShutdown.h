#pragma once
// 終了処理。ESC・ウィンドウの×・起動失敗のどれで終わっても同じ後片付けをする。
// 2回目以降の呼び出しは何もしない。

void ShutdownApp();
