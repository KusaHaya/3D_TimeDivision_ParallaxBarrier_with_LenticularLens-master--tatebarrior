# 表示アプリ（ProjectPB）のソース構成

以前は `main.cpp` に全機能が入っていたものを、機能ごとに分けた。処理内容・キー操作・タイミングは変更していない。

| 機能 | ファイル | 主な中身 |
| --- | --- | --- |
| 起動 | `main.cpp` | 環境変数の読み込み、Arduino接続、GLUT初期化 |
| 終了 | `AppShutdown.h/.cpp` | ESC・ウィンドウの×・起動失敗のどれでも、動画停止→バックライトを全点灯に戻す→トラッキング停止→テクスチャ解放 |
| 固定値・共有状態 | `AppConfig.h`、`AppState.h/.cpp` | 画面サイズ、`haba`・`MiddleLine`・`kk` などのグローバル変数、カメラ設置位置 `PosX/PosY/PosZ` |
| Arduino通信 | `ArduinoSerial.h/.cpp` | コマンド番号、シリアルを開く処理、1バイト送信 |
| 時分割・フレーム停止 | `TimeDivision.h/.cpp` | 位相の送信、`t` / `q/w/e/r` の停止処理、同期のリセット |
| コマ落ち検出 | `FrameDropMonitor.h/.cpp` | リフレッシュレートの取得、swap間隔からコマ落ちを数えて `[DROP]`・`[PERF]` に表示 |
| ヘッドトラッキング | `HeadTracking.h/.cpp` | OSCの目の位置から `haba` と `MiddleLine` を補正（`h` で基準位置を設定） |
| 静止画 | `StillImage.h/.cpp` | 数字キーで使う画像を起動時にまとめて読み込み、切り替え時はテクスチャを選ぶだけ。読めない画像は警告して今の画像のまま |
| シェーダー | `ShaderLoader.h/.cpp`、`InterleaveRenderer.h/.cpp`、`interleave.frag` | シェーダーのコンパイル（失敗したら起動を止める）、静止画・動画共通の合成描画 |
| 1フレームの流れ | `DisplayLoop.h/.cpp` | `init()`、`disp()`（描画→swap→Arduino送信→`kk` を進める）、性能ログ |
| キー入力 | `KeyInput.h/.cpp` | すべてのキー操作 |
| 動画 | `DrawVideo.*`、`Media.*`、`Texture.*`、`Timer.*` | FFmpegでの動画デコード（変更なし） |
| 目の位置の受信 | `ExternalEyeTracker.*`、`OscEyePacket.h` | EyeTracker-RSからのOSC受信（変更なし） |

Visual Studio のソリューションエクスプローラーでも、同じ単位のフォルダーに分けて表示される。
