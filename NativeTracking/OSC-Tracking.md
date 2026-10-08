# EyeTracker-RSとの接続

現在の表示アプリは外部EyeTracker-RSのOSCを受信します。表示側はRealSenseカメラを開かず、OpenCVによる顔・目検出も行いません。

1. DesktopのEyeTracker-RS.exeを起動します。
2. IP addressを`127.0.0.1`、Portを`8000`にします。
3. `/eye/right`と`/eye/left`の送信を有効にし、Startを押します。`/eye/center`も有効のままで構いません。
4. 表示アプリの`x64/Release/Start.cmd`を起動します。
5. コンソールに`receiving both eyes`が出たら、顔を基準位置に置いて`h`を押します。

順序が逆でも受信開始後にhで基準位置を設定できます。同じポートを使う受信アプリは同時に起動しないでください。

## 通信仕様

- OSC over UDP、受信先127.0.0.1:8000。
- `/eye/right`と`/eye/left`、型タグ`,fff`、XYZの順、メートル。
- OSC bundleにも対応。centerは使用しません。
- 左右両目のパケットが100ms以内に揃ったときに更新します。
- 無効な値・形式を破棄します。受信が途切れたら最後の座標を保持します。
- 内蔵追跡で使っていたジャンプ判定と0.1秒の平滑化は、外部データには追加していません。
- RealSenseカメラ座標から表示座標へ、XとYの符号を反転します。実機の左右移動で方向を確認してください。

## 調整

Start.cmdの`PROJECTPB_OSC_PORT`でポートを変更できます。EyeTracker-RSの送信先ポートも同じ番号にしてください。

`PROJECTPB_OSC_X_SIGN`と`PROJECTPB_OSC_Y_SIGN`は現在`-1`です。外部側で軸を反転している場合は該当軸を`1`へ変更できます。

横補正倍率は左右共通で0.7倍です。実行中のj / Shift+jによる調整も使えます。

## 確認済み

実際のEyeTracker-RSからOSCの左右座標を受信し、組み込む受信クラスで両目6成分を取得できました。OSCパーサーの通常メッセージ、bundle、途中で切れたパケット、不正型、NaNのテストを実行しました。表示アプリのビルドとRelease exe更新は成功しています。Arduinoと画面を含めた実機追従の確認はユーザー側で必要です。
