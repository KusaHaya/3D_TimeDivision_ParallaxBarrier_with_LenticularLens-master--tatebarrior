# 別PCでProjectPBを実行する

## 起動方法

1. ZIPをすべて展開します。日本語・空白・長いパスを避け、例えば `C:\ProjectPB` に置いてください。
2. D435をUSB 3ポートへ接続します。RealSense Viewer、以前の追跡サンプル、Pythonサーバーは終了してください。
3. 従来のArduinoと表示装置を接続します。Arduinoには従来と同じ制御プログラムが書き込まれている必要があります。
4. WindowsのデバイスマネージャーでArduinoのCOM番号を確認します。
5. `Start.cmd` をメモ帳で開き、`PROJECTPB_SERIAL_PORT=COM3` のCOM3を実際の番号に変更して保存します。
6. **Start.cmdをダブルクリックします。** 作業フォルダーを正しく設定して、同梱exeを起動します。
7. 一人でカメラを正面に見て、`h` キーで現在位置を基準にトラッキングを開始します。

Python、Visual Studio、vcpkgは実行時には不要です。Gitクローン済みのソースも実行時には使いません。
今回の一式を使用する場合、前に渡したDLLや画像ZIPを追加で展開する必要はありません。

## 同梱内容

- 移動用に再ビルドしたexe（VS2026 18.11 Insiders / x64 Release）
- 必要な64bit DLL 24個（RealSense、OpenCV、FFmpeg、SDL2、OpenGL補助ライブラリ、C++ランタイム）
- models: 顔検出・両眼検出モデル2個。各XML内のライセンス表記を保持しています。
- passthrough.vert、interleave.frag、sbs_interleave.frag
- images/IDW: 001～008の左右画像16個
- media/avater.mp4: 元PCのCドライブ直下にあった動画
- Start.cmd: 起動場所とCOM番号と動画の場所を指定するファイル
- Prerequisites/vc_redist.x64.exe: Microsoftのx64 C++ランタイムインストーラー（必要な場合のみ）
- file-manifest.json: コピーしたファイルのサイズとSHA256
- source-changes.patch: 今回の持ち運び対応のソース差分

## 元のexeからの変更

動画の既定パスを `C:\avater.mp4` から `media\avater.mp4` に変更しました。
ArduinoのCOM番号を環境変数 `PROJECTPB_SERIAL_PORT` で指定できるようにしました。
動画は環境変数 `PROJECTPB_VIDEO` で変更できます。どちらもStart.cmdに記載しています。
シリアル接続に失敗したときは、原因をコンソールに表示して終了します。

これらの持ち運び対応は同梱exeに反映済みです。GitHubへの追加反映は行っていません。
クローンしたソースで同じ変更を再ビルドする場合は、リポジトリのrealsenseブランチで
同梱パッチが適用可能か確認してから適用します（同等の変更を既に適用している場合は不要です）。
今回のexeをそのまま使う場合はソース操作不要です。

## PCや装置側に必要なもの

- 64bit Windows。移行先のOS・GPU環境での実動作は未確認です。
- GPUに合ったグラフィックスドライバー。シェーダーはGLSL 3.30を使用しています。
- 現在のコードはステレオバッファ（GLUT_STEREO）を要求するため、それに対応した表示環境が必要です。
- 元の表示装置・時分割制御用Arduinoと、そのUSBシリアル接続に必要なドライバー。
- RealSense D435とUSB 3接続。カメラがWindowsから認識されている必要があります。

ドライバー、Arduinoの書き込み内容、ディスプレイ設定はファイルをコピーするだけでは移せません。
画面設定はコード上1920×1080で、フレーム周期・同期・カメラの位置角度は元環境と合わせて確認してください。
Kinect SDKは不要です。

## トラブル時

- COM接続エラー: COM番号と接続を確認し、シリアルモニター等の別アプリを終了してください。
- MSVCP/VCRUNTIME関連エラー: Prerequisitesのvc_redist.x64.exeを実行してください。管理者権限が必要な場合があります。
- シェーダーや画像がない: ZIPを全展開し、Start.cmdから起動してください。
- RealSense開始エラー: USB接続と他アプリのカメラ使用を確認してください。内部で再接続を試行します。
- OpenGLやステレオ表示のエラー: 移行先GPU・ドライバー・表示設定を確認してください。
- 動画が出ない: media/avater.mp4があることとStart.cmdのPROJECTPB_VIDEOを確認してください。

## 検証範囲

再ビルド成功、コピー元とのSHA256一致、Windows標準以外のDLL依存関係の収集を確認しました。
DLL検索先を制限した起動確認で、同梱DLLを使ってexeが読み込まれ、指定した無効COMポートに対して
想定どおりエラー終了することも確認しました（実際の装置へ命令を送らない確認です）。
この確認では描画ウィンドウやカメラを起動していません。
D435・Arduino・ディスプレイを組み合わせた実機動作と、移行先PCでの動作は未確認です。
