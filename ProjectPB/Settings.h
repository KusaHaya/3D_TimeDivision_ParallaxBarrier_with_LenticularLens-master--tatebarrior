#pragma once
// 調整値の保存と読み込み（作業フォルダの ProjectPB_settings.ini）。
// 起動時に LoadSettings() で読み込み、P キーで SaveSettings() する。
// ファイルがなければ初期値のまま動く。終了時に自動では保存しない。

extern const char* const SettingsFileName;

// 読み込めた項目だけを反映する。ファイルがないときは false。
bool LoadSettings();
// 今の調整値を書き出す。ヘッドトラッキング中は追跡の基準値を保存する。
bool SaveSettings();
// 今の調整値をコンソールに表示する。
void PrintSettings(const char* label);
