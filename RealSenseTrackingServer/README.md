# RealSense Tracking Server

`ProjectPB/main.cpp` はKinect SDKを直接使用していません。従来のKinect Serverから
TCP `127.0.0.1:30000` で左右眼の座標（`float32` 6個）を受け取っています。
このサーバーは同じ通信形式でRealSenseの座標を送信するため、ProjectPB側の
バリア計算や描画処理を変更せずにRealSenseへ置き換えられます。

## 対応構成

- Windows 10/11
- Intel RealSense D400シリーズ（D415/D435/D435i/D455など）
- Python 3.10または3.11（64 bit推奨）
- Intel RealSense SDK 2.0

## セットアップ

1. Intel RealSense SDK 2.0をインストールし、RealSense ViewerでColorとDepthが
   同時に取得できることを確認します。
2. PowerShellでこのディレクトリへ移動し、依存パッケージをインストールします。

   ```powershell
   py -3.11 -m venv .venv
   .\.venv\Scripts\Activate.ps1
   python -m pip install -r requirements.txt
   ```

## 実行

ProjectPBより先にサーバーを起動します。

```powershell
python realsense_tracking_server.py
```

プレビューに顔の枠と両眼の赤点が表示され、左上が `tracking` になったことを
確認してからProjectPBを起動します。ProjectPBで `h` キーを押すと現在位置を
基準にヘッドトラッキングが有効になります。終了はプレビュー上で `Q` または
Escです。

プレビューが不要な場合:

```powershell
python realsense_tracking_server.py --no-preview
```

## 既存プログラムとの互換性

送信データは次の24 byteです。

```text
left_x, left_y, left_z, right_x, right_y, right_z
```

各値はlittle-endian `float32`、単位はメートルです。RealSenseの座標系
（X右、Y下、Z前）を、従来のKinect camera space（X左、Y上、Z前）へ変換して
送信します。そのため `ProjectPB/main.cpp` の `PosX` / `PosY` / `PosZ`、
`SIN` / `COS` およびキャリブレーション計算をそのまま使用できます。

## 調整とトラブルシュート

- 顔が検出されない: 顔を正面に向け、顔全体が100 px以上になる距離と照明にします。
- Depthが取得できない: RealSense Viewerを終了します。同時にカメラを占有できません。
- ProjectPBが接続しない: サーバーを先に起動し、ポート30000を別アプリが使用して
  いないことを確認します。
- 動きが遅い/揺れる: `--smoothing 0.5` で追従を速く、`--smoothing 0.2` で
  揺れを小さくできます。
- 左右移動が逆になる: カメラ映像を別ソフトで左右反転していないか確認します。
- 既存のカメラ設置角・位置と異なる: `ProjectPB/main.cpp` の `SIN` / `COS` と
  `PosX` / `PosY` / `PosZ` を実機配置に合わせて再調整してください。

この実装はOpenCVのカスケード分類器で目の中心を推定します。眼球中心を高精度に
測る装置ではないため、最終的なバリア位置は実機でキャリブレーションしてください。
