# RealSense Tracking Server

`ProjectPB/main.cpp` はKinect SDKを直接使用していません。従来のKinect Serverから
TCP `127.0.0.1:30000` で左右眼の座標（`float32` 6個）を受け取っています。
このサーバーはD435のRGB画像から両眼を検出し、位置合わせした深度画像から
3D座標を計算します。`realsense` ブランチの描画側とセットで使用してください。
Kinect ServerとKinect SDKは不要です。以前の単体C++サンプルのexeも使用しません。
追跡はPython、描画は既存のVisual Studioソリューションで動作します。

## 対応構成

- Windows 10/11
- RealSense D435（RGB + 深度 640×480 / 30fps、USB 3接続）
- Python 3.10または3.11（64 bit推奨）
- Intel RealSense SDK 2.0

## セットアップ

1. Intel RealSense SDK 2.0をインストールし、RealSense ViewerでColorとDepthが
   同時に取得できることを確認します。
2. PowerShellでこのディレクトリへ移動し、依存パッケージをインストールします。

   ```powershell
   py -3.11 -m venv .venv
   .\.venv\Scripts\python.exe -m pip install -r requirements.txt
   ```

## 実行

このフォルダーでサーバーを起動します。RealSense Viewerと、以前作成した
HeadTracking.exeは終了しておきます。カメラを同時には使用できません。

```powershell
.\.venv\Scripts\python.exe realsense_tracking_server.py
```

プレビューに顔の枠と両眼の赤点が表示され、左上が `tracking` になったことを
確認してからProjectPBを起動します。ProjectPBで `h` キーを押すと現在位置を
基準にヘッドトラッキングが有効になります。終了はプレビュー上で `Q` または
Escです。セットアップ後は同梱の `start.cmd` をダブルクリックして起動することもできます。

描画側は `realsense` ブランチを取得し、既存のソリューションをRelease/x64で
再ビルドしてください。依存ライブラリ（Boost、OpenGL、FFmpeg等）とArduinoの
設定は従来どおり必要です。新しい受信クラスは既存のBoost.Asioを使うため、
描画側にRealSense SDKやOpenCVのC++ライブラリを追加する必要はありません。

サーバーと描画側はどちらを先に起動しても接続を再試行します。
顔・両眼・深度が取得できなくなると追跡を解除します。再検出後は `h` を押して
基準位置を設定し直してください。追跡データが0.5秒以上来ない場合も解除します。

プレビューが不要な場合:

```powershell
.\.venv\Scripts\python.exe realsense_tracking_server.py --no-preview
```

## 既存プログラムとの互換性

送信データは次の24 byteです。

```text
image_left_x, image_left_y, image_left_z, image_right_x, image_right_y, image_right_z
```

各値はlittle-endian `float32`、単位はメートルです。RealSenseの座標系
（X右、Y下、Z前）を、従来のKinect camera space（X左、Y上、Z前）へ変換して
送信します。ここで左右は鏡像化していないカメラ画像上の左右です。
描画側の従来の左右インデックス対応を維持しています。
追跡無効時には6個のNaNを送ります。旧受信処理には無効値チェックがないため、
必ずこのブランチの新しい `RealSenseTrackingClient.h` と `main.cpp` を使ってください。

`PosX` / `PosY` / `PosZ` と `SIN` / `COS` はカメラからディスプレイへの配置変換です。
カメラ交換後は設置位置・角度を測って再調整してください。`h` の基準合わせだけでは
設置角度の違いは補正できません。Y/Z回転は眼座標と頭の中央座標で同じ式に統一しました。
既存バリア式の分母がゼロに近い領域（変換後Z=0.3245mの±0.02m）は追跡を解除します。

## 調整とトラブルシュート

- 顔が検出されない: 顔を正面に向け、顔全体が100 px以上になる距離と照明にします。
- Depthが取得できない: RealSense Viewerを終了します。同時にカメラを占有できません。
- ProjectPBが接続しない: ポート30000を別アプリが使用して
  いないことを確認します。
- 動きが遅い/揺れる: `--smoothing 0.5` で追従を速く、`--smoothing 0.2` で
  揺れを小さくできます。
- 左右移動が逆になる: カメラ映像を別ソフトで左右反転していないか確認します。
- 既存のカメラ設置角・位置と異なる: `ProjectPB/main.cpp` の `SIN` / `COS` と
  `PosX` / `PosY` / `PosZ` を実機配置に合わせて再調整してください。

この実装はOpenCVのカスケード分類器で目の中心を推定します。眼球中心を高精度に
測る装置ではないため、最終的なバリア位置は実機でキャリブレーションしてください。
一人が正面を向く用途を想定し、複数顔、深度不足、眼間距離が35～90mmの範囲外なら
無効にします。横顔や眼鏡の反射では追跡が途切れる場合があります。
Kinectの骨格追跡と同等の精度や姿勢追跡を保証するものではありません。

## 検証

カメラ不要のPythonテスト:

```powershell
.\.venv\Scripts\python.exe -m unittest -v test_tracking
```

受信側テストはDeveloper Command Promptで次を実行できます（Boostのパスは変更してください）。

```bat
cl /EHsc /std:c++14 /DBOOST_ALL_NO_LIB /DBOOST_ERROR_CODE_HEADER_ONLY /I C:\boost_1_73_0 test_client.cpp ws2_32.lib mswsock.lib
test_client.exe
```

座標の符号・単位、深度欠損、無効パケット、TCP分割受信、タイムアウト、再接続をテストしています。
MSVCによる変更済みmain.cppの単体コンパイルも確認済みです。
描画アプリ全体のリンクとD435・ディスプレイ・Arduinoを組み合わせた実機検証は未実施です。
