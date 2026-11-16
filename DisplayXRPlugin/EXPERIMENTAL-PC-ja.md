# 別の実験用PCでの最初の確認

配布zipを実験用PCの任意の作業フォルダーへ展開してください。今回のDLLは第1段階の固定位相診断版です。

1. 展開したフォルダーでPowerShellを開き、`./DisplayXRPlugin/scripts/Validate-Connection.ps1`を実行します。管理者権限は不要です。
2. 二つのテストがPASSになることを確認します。ソフトウェア描画と39147/39148番のローカルOSC通信を使い、LCDのモード、Arduino、カメラは変更しません。
3. 起動時にVisual C++ runtimeのDLL不足が出た場合は、実験用PCへMicrosoft公式のVisual C++ x64 runtimeを用意してください。Windows標準DLLをこのzipから個別上書きする必要はありません。
4. 次に[接続手順](README.md)に従い、同じ版のDisplayXR runtime、対象LCDのWindows表示名、144Hz設定を確認します。
5. LEDをP0のHOLDにした状態で、PHASE=0の画像を確認します。その後P1〜P3を一つずつ対応させます。プラグインはArduinoへコマンドを送らないため、LEDのHOLD設定は既存の装置手順を使います。
6. カメラ校正を確認してからOSC追跡と光学補正を有効にします。既存表示アプリと新プラグインを同時に8000番へ接続しないでください。

この確認が通っても、144Hzの連続4時分割同期の合格を意味しません。次段階の設計に必要なのは、Arduinoの機種、現在のスケッチ、各位相のLEDの見え、LCD同期信号を取得できるかという実機情報です。

zipにはDLLとテスト実行ファイルに加え、プラグインのソース、ビルド／登録用スクリプト、設計・検証記録を含めています。DisplayXR本体、カメラドライバー、Arduino書込みツールは含めていません。確認結果は`docs/displayxr/connection-validation-ja.md`を参照してください。
