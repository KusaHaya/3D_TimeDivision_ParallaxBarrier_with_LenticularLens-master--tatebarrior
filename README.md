> Visual Studio 2026 で C++ 表示アプリをビルドする場合は [現在のビルド手順](NativeTracking/VisualStudio2026.md) を参照してください。現在の追跡入力は外部 EyeTracker-RS の OSC です。[接続手順](NativeTracking/OSC-Tracking.md)

# このリポジトリについて
レンチキュラレンズを使った時分割パララックスバリアシステムのプログラム 

この `realsense` ブランチでは、C++の描画プログラム内でRealSense D435を直接起動します。
PythonサーバーやKinect SDKは不要です。導入手順は [NativeTracking/README.md](NativeTracking/README.md) を参照してください。
計測は別スレッドで動作し、顔を検出したら `h` で基準位置を設定します。

## 目次
1. [各ブランチの説明](#各ブランチの説明)
2. [環境](#環境)
3. [環境構築](#環境構築)
4. [実行方法](#実行方法)
5. [主なキー入力の説明](#主なキー入力の説明)
## 各ブランチの説明
| ブランチ名 | 説明 |
| --- | --- |
| master | VS2017環境でバックライトがLEDのときのプログラム |
| dev | 開発環境
## 環境
1. Visual Studio 2026（v145 ビルドツール）  
手順は [NativeTracking/VisualStudio2026.md](NativeTracking/VisualStudio2026.md)、ソース構成は [ProjectPB/SOURCE_LAYOUT.md](ProjectPB/SOURCE_LAYOUT.md) を参照  
boost : version 1_73_0  
ターゲットプラットフォーム: x64
## 環境構築
1. Intel RealSense SDK 2.0のインストール  
   RealSense ViewerでColorとDepthを同時に取得できることを確認する。  
   C++組み込みトラッキングの導入方法は[NativeTracking/README.md](NativeTracking/README.md)を参照。
2. boostの設定
    - boost1_58_0  
    bullに中のビルド済みboost1_58_0のフォルダをローカルにコピーしてシステム環境変数に**BOOST_DIR**として追加
    - boost1_73_0  
    bullにあるboost1_73_0のzipファイルをローカルに展開してシステム環境変数に**BOOST_DIR_173**として追加
3. Visual Studio
    - 2013  
    bullにあるインストーラとライセンスからインストール
    - 2017  
    [ダウンロードページ](https://visualstudio.microsoft.com/ja/vs/older-downloads/)からVisual Studio Community 2017をインストール  
    ログインには利用可能なMicrosoftアカウントを使用する。
4. このリポジトリをクローン
5. 不足しているファイルをコピー  
※.gitignoreによって*.dllはgitの管理下にないのでbullからコピーする必要がある。  
    - bullにあるディレクトリを`(SolutionDir)\Library\`にコピー
    - bullのffmpegの設定フォルダの中のdllファイルを全部`(SolutionDir)/x64/Release`の中にコピー  
    ※`(SolutionDir)/x64/Release`は一度ビルドに失敗すると生成されているはず  
    このとき`Release\x64`（VS 2013なら`Release\WIN32`）で動作させる必要がある
6. 必要な動画ファイルをコピー（ほかのマシンの中を探せば動画ファイルがあるはず）  
※読み込む動画ファイルは*Media.cpp:68*あたりの動画ファイルへのパスから変更できる
## 実行方法
1. D435を接続し、RealSense ViewerとPythonサーバーを終了する
2. プログラムを実行（内部でD435を開始）。顔を正面に向けてhキーで基準位置を設定する
3. 基板のスイッチを切り替えてバックライトを時分割モードに切り替え（この手順はいつでもいい）
## 主なキー入力の説明
起動時と `?`（または `/`）キーで、コンソールにキー一覧と今の状態が表示されます。

| 入力キー | 動作 |
| --- | --- |
| ESC | 終了（バックライトは全点灯に戻る。ウィンドウの×でも同じ） |
| 0〜7 | 静止画を切り替え（起動時に読み込み済みなので一瞬で切り替わる） |
| 8 | 左＝赤・右＝青のタイミング確認用画像 |
| m / shift + m | 動画表示 / 静止画表示（どちらも時分割を kk=0 から再開） |
| t | 表示中の位相で停止、もう一度押すと再開 |
| q, w, e, r | 停止中に位相0, 1, 2, 3を表示 |
| k | 表示の位相を1つ進める（バックライトとの同期がずれたときに使用） |
| [ / ] | 幅パラメータ haba を +1 / -1（`a` も +1） |
| o / shift + o | 位相補正 SHIFT を +1 / -1 |
| f / shift + f | 傾き slantY を -0.02 / +0.02 |
| u / shift + u | 補正の境界値を -0.05 / +0.05 |
| shift + c | 中心線の表示ON/OFF（測定時はOFF） |
| v / shift + v | 左目の描画ON / OFF |
| b / shift + b | 右目の描画ON / OFF |
| h | ヘッドトラッキング：基準位置を設定して開始 / 停止 |
| j / shift + j | 横方向の補正倍率を -0.1 / +0.1 |
| shift + k | 現在の観測者のz座標と幅パラメータを出力（パラメータ調整用） |
| s（押している間） | 動画の早送り |
| shift + p | 調整値を `ProjectPB_settings.ini` に保存（次回起動時に自動で読み込む） |
| ? または / | キー一覧と今の状態を表示 |

調整値（haba、MiddleLine、SHIFT、slantY、境界値、横方向の倍率、カメラ位置 PosX/Y/Z、設置角）は
`shift + p` で作業フォルダの `ProjectPB_settings.ini` に保存されます。ファイルを消すと初期値に戻ります。
