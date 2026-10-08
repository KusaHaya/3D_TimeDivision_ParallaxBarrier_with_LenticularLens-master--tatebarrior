> 現在は外部EyeTracker-RSのOSC受信へ切り替えています。最新の手順は[OSC-Tracking.md](OSC-Tracking.md)を参照してください。以下は以前の内蔵追跡の記録です。

# D435を描画プログラム内で直接使用する

`realsense` ブランチの通常構成です。ProjectPB.exeを起動すると、同じプロセス内で
RealSense D435の取得・顔と両眼の検出・3D計測を開始します。
Python、外部サーバー、TCP接続は不要です。

## 初回の準備

Visual Studioの「C++によるデスクトップ開発」が必要です。
この変更部分はVisual Studio 2022 / MSVC v143でコンパイル確認しています。
古いv141の設定で開かれる場合は、プロジェクトのプロパティでプラットフォーム
ツールセットをインストール済みの新しいものへ変更してください。
Windows SDKも手元にインストールされているバージョンを選択します。

### 方法A: 以前のRealSenseHeadTrackingで導入したライブラリを利用する

既に動作したサンプルの `vcpkg_installed\x64-windows` を再利用できます。
リポジトリの `ProjectPB\RealSenseTracking.props` の `TrackingDepsDir` の値を、
そのフォルダーの絶対パスへ変更してください（`include` と `lib` と `bin` のあるフォルダーです）。
例:

```xml
<TrackingDepsDir Condition="'$(TrackingDepsDir)'==''">C:\dev\RealSenseHeadTracking\out\build\windows-release\vcpkg_installed\x64-windows</TrackingDepsDir>
```

`opencv_objdetect4.lib` と `realsense2.lib` が `lib` にあることを確認してください。
この方法ならライブラリを再インストールする必要はありません。
ローカルのパス変更は他のPCに共有しないでください。

### 方法B: このリポジトリに依存ライブラリを導入する

リポジトリのルート（`vcpkg.json` があるフォルダー）でPowerShellから実行します。

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\vcpkg\vcpkg.exe" install --triplet x64-windows
```

vcpkgのパスは実際のインストール先に合わせてください。
標準設定では `vcpkg_installed\x64-windows` を参照します。
初回はネット接続とライブラリのビルド時間が必要です。RealSense SDKとOpenCVを
取得します。両眼検出のobjdetectには `calib3d` が必要なため有効にしてあります。

## ビルドと起動

1. `realsense` ブランチの最新状態を取得し、既存のVisual Studioソリューションを開く。
2. **Release / x64** を選択してビルドする。
3. RealSense Viewer、以前のHeadTracking.exe、Pythonサーバーを終了する。
4. D435をUSB 3ポートに接続して、**ProjectPBを実行するだけ**で計測が開始される。
5. 一人でカメラに正面を向き、`h` で現在位置を基準にしてヘッドトラッキングを開始する。

必要なRealSense/OpenCVのDLLと `models` フォルダーはビルド時にexeの横へコピーします。
顔検出モデルは初回ビルド時にOpenCV公式配布元から取得し、SHA256で検証します。
初回のみインターネット接続が必要で、2回目以降は取得済みファイルを使います。
取得時にMSB3923などの通信エラーが出る場合は、以下の2ファイルをブラウザーで保存し、
リポジトリの `NativeTracking/models` に置いて再ビルドしてください。

- https://raw.githubusercontent.com/opencv/opencv/4.14.0/data/haarcascades/haarcascade_frontalface_default.xml
- https://raw.githubusercontent.com/opencv/opencv/4.14.0/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml

作成環境ではMSBuild自身のHTTPS取得が失敗したため、公式ファイルを別途取得したうえで
MSBuildのSHA256検証と配置処理を確認しています。
exeを移動する際はこれらも一緒に移動します。
Boost、FFmpeg、OpenGL、ArduinoのCOMポートなど、従来の描画装置の設定は引き続き必要です。
`main.cpp` は従来どおりArduinoをCOM3で開くため、装置の接続状態を確認してください。

## 動作

- カメラは640×480、30fpsでColorとDepthを取得します。
- ワーカースレッドで深度をColorに位置合わせし、正面顔の両眼を検出します。
- 両眼付近の9×9画素の有効深度の中央値から、3D座標を求めます。
- 描画側は最新値のみを読み取ります。カメラ待ちや顔検出で描画を待たせません。
- カメラ未接続・切断の場合は2秒間隔で開始を再試行します。
- 顔、両眼、深度が無効なら追跡を解除します。0.5秒以上古い値も利用しません。
- 再検出後は `h` で再度基準位置を設定してください。
- 終了時には取得スレッドを停止し、カメラを解放します。

顔検出のプレビューウィンドウはありません。状態はコンソールに表示します。
頭の向きや骨格の推定ではなく、両眼の位置によるヘッドトラッキングです。
顔が複数ある場合や眼間距離が35～90mmの範囲外の場合は無効にします。
横顔、遮蔽、眼鏡の反射では計測できない場合があります。

座標は既存の描画側に合わせ、X左・Y上・Z前、単位メートルです。
`main.cpp` の `PosX/PosY/PosZ` と `SIN/COS` はカメラ設置位置と角度に合わせて調整してください。
基準合わせの `h` だけでは設置角度の違いは補正できません。

## 検証状況

ネイティブC++のテスト実行ファイルをRealSense/OpenCVライブラリとリンクし、
座標の符号・単位、深度欠損・外れ値、眼間距離の検証、古い値の無効化、
追跡再開時の平滑化リセット、モデルXMLの読み込みを確認しました。
変更した描画側main.cppも単体コンパイル済みです。
描画アプリ全体のリンクとD435・ディスプレイ・Arduinoを組み合わせた実機動作は未検証です。
作業用コピーでの全体ビルドは既存の `nupengl.core.redist` NuGetパッケージがないため
停止しました。同じエラーが出る場合はVisual StudioでソリューションのNuGetパッケージを復元してください。

`test_native_tracking.cpp` は `RealSenseTracker.cpp` と一緒にコンパイルし、
RealSense/OpenCVのcore・imgproc・objdetectにリンクして実行できます。
引数に `models` フォルダーを渡すとモデル読み込みも確認します。

## モデルの出典

取得するXMLはOpenCV 4.14.0配布物の `haarcascade_frontalface_default.xml` と
`haarcascade_eye_tree_eyeglasses.xml` です。ライセンス・著作権表記は各XML冒頭に保持されます。
元データ: https://github.com/opencv/opencv/tree/4.14.0/data/haarcascades

## 連続性を使う追跡への更新

StartEyeCheck.cmd を起動します。前回の有効な眼位置に近い候補を優先します。顔が一時的に見つからない場合は前の顔領域で眼を探します。
眼候補は前の位置から最大 max(35px, 顔幅の40%) に制限します。
3Dの片眼ごとの変化が 0.04m + 0.6m/s × 経過秒（最大0.2秒）を超える場合は除外します。実機での改善率は未確認です。
MEASURED は新しい測定、HELD は過去の有効値、NO_DATA は初期測定前です。
緑の丸は測定した眼、紫の丸は保持する以前の眼位置です。
保持に時間制限は設けず、検出欠落・カメラ再接続中も最後の座標を使います。保持値は現在の測定ではありません。
大きく移動して追従しない場合は R でテストの履歴をリセットします。本体はアプリ再起動でリセットします。
本体ソースにも反映済みです。本体 exe は Release/x64 のリビルドが必要です。
