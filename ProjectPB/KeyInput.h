#pragma once
// キーボード操作。キーの一覧は README の「主なキー入力の説明」を参照。

// GLUTへキーボードのコールバックを登録する。
void RegisterKeyHandlers();
// キー一覧と今の状態を表示する（? キー、起動時）。
void PrintKeyHelp();
void PrintStatus();
// ウィンドウのタイトルに今の状態を表示する（キー操作のたびに更新）。
void UpdateStatusTitle();
