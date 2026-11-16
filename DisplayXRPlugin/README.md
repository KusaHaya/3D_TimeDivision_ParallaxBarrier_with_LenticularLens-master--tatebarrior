# ProjectPB DisplayXR接続：第1段階

Windows x64 / D3D11用の**固定位相診断プラグイン**です。公式ABIでDLLをロードし、左右atlasから既存バリア式のP0〜P3を生成し、外部EyeTracker-RSのOSCを受信できます。今回はUSBポートを開きません。Arduinoとの144Hz連続同期は次段階です。

## 今回実装した範囲

- `xrtPluginNegotiate`、明示選択したパネルのprobe、左右2viewデバイス、D3D11 factory。
- 既存GLSLの負値の剰余、符号対称の量子化、RGBサブピクセル、左下原点のYをHLSLへ移植。
- 0〜3の位相を設定で固定。描画回数では位相を進めません。
- 左右眼の名前を保つOSC受信、同一プロセス内で共有するUDP所有者、時刻履歴、通信の新鮮さの管理。
- 生カメラ座標から表示中央原点・X右/Y上/Z観察者方向・mへの変換。
- 校正確認後に、同じ受信眼位置をOpenXR側の眼位置問い合わせと光学補正へ使用。
- 不正な設定、画面サイズ、atlas形式、原点ずれを拒否。画像出力条件に合わない場合は黒出力とDEGRADED状態。

公式runtimeは`ac3ae19215bbcfc6aa091224d109a5b113c1c8dd`へ固定しています。CMakeはこのチェックアウトの公開ヘッダーだけを読み、runtimeをビルド・改変しません。今回のDLLはruntimeの補助ライブラリの関数を使用しないため、`DisplayXRClient.lib`は不要です。同じ公開デバイス構造体を初期化する実装を持ちます。公式starter kitそのもののコピーではありません。

## ビルドと自動検証

Visual Studio 2022のC++開発環境とWindows SDK、CMake、Gitを使用します。公式runtimeを別フォルダーへ取得して上記SHAへ固定し、リポジトリのルートから実行します。

```powershell
./DisplayXRPlugin/scripts/Build.ps1 -RuntimeSource ../displayxr-runtime
```

成果物は`DisplayXRPlugin/build/Release/DisplayXR-ProjectPB.dll`です。

純粋計算テストだけなら`PROJECTPB_BUILD_PLUGIN=OFF`でruntimeやD3D11を使わず構成できます。WARPの画像テストは画面・USB・カメラを使用しません。OSCテストはloopback UDPの39147/39148番を使うため、ローカル通信を許可した環境で実行してください。

## 実機へ接続するときの順序

この手順は自動実行していません。今回の環境ではDisplayXRの既定インストール先とHKLMの登録を確認できず、OpenXRアプリからの実機動作は未検証です。

1. 同じSHAに対応するDisplayXR runtimeを準備し、通常のOpenXRサンプルがsim-displayで動くことを確認します。
2. LCDを1920×1080、横向き、144Hz、等倍・SDRにし、Windowsのディスプレイ名を確認します。表示面寸法528×297mmは暫定値です。実測寸法への対応は次の校正作業で行います。
3. 必要なら管理者の64bit PowerShellから`./DisplayXRPlugin/scripts/Register-Dev.ps1`を実行します。開発用DLLのパスだけを登録します。OpenXR ActiveRuntimeや既存runtimeのファイルを変更せず、ENABLEも有効にしません。登録済みの同名開発DLLを置き換える場合は、そのDLLをロードしているアプリ／serviceを先に通常終了してください。
4. `displayxr-cli dp options projectpb`で設定一覧を確認し、`PANEL_DEVICE_NAME`へ対象LCDの正確な名前（例：`\\.\DISPLAY2`）を設定します。`dp list`で対象の選択状態を確認します。
5. 最初は`TRACKING=0`、`PHASE=0`。外部の既存手順でLEDをP0のHOLDに固定し、旧表示アプリを終了してUSBとOSCの所有を解放します。P0画像とP0 LEDの対応を実機で確認してから、`ENABLE=1`で新セッションを開始します。
6. P1、P2、P3も設定変更後にセッションを作り直し、一位相ずつLED HOLDと画像の組を合わせて確認します。設定はfactory生成時に読み、毎フレーム読み直しません。
7. EyeTracker-RSは生RealSense座標（X右、Y下、Z前、m）を送信します。`/eye/left`と`/eye/right`を127.0.0.1:8000へ送信します。旧ProjectPB.exeや別受信アプリを同じポートで起動しないでください。
8. カメラの外部校正を測定して`CAM_TILT_RAD/CAM_X_M/CAM_Y_M/CAM_Z_M`を設定した後、`CALIBRATION_CONFIRMED=1`、`TRACKING=1`を設定してセッションを作り直します。コードの初期値は旧装置からの暫定値です。
9. 手動で光学調整した基準位置の頭部中心を表示座標で測り、`REFERENCE_X_M/REFERENCE_Z_M`へ設定します。基準の`HABA/MIDDLE_LINE/SHIFT`を旧アプリと一致させてから`OPTICAL_TRACKING=1`にします。`HORIZONTAL_GAIN`は光学補正だけに適用し、OpenXRへ返す眼座標を縮小しません。

設定例（対象番号は実環境で確認してください）：

```powershell
displayxr-cli dp set projectpb PANEL_DEVICE_NAME '\\.\DISPLAY2'
displayxr-cli dp set projectpb PHASE 0
displayxr-cli dp set projectpb ENABLE 1
```

設定値は公式hostの`get_setting`だけから読みます。旧`Start.cmd`の環境変数をDisplayXR serviceが引き継ぐ前提にはしていません。終了するときはセッションを終了し、`ENABLE=0`へ戻します。LEDのHOLD解除／照明復帰は既存の装置手順で行います。

## この段階の制約

- **固定位相診断用です。連続4時分割運用の完成版ではありません。** LED状態の確認を返すAPIや、未実装の表示完了通知を実装済みとして返しません。
- 原点0の1920×1080フルターゲットのみ。ウィンドウがある場合は対象パネル全体との一致を確認します。ウィンドウの移動、縮小、局所canvas、透過、HDR、非D3D11は対象外です。NULL HWNDのfactoryはheadlessテストにも使用しますが、実運用のprobeは指定パネルが見つからない限り成功しません。
- SDR encodedのRGBA8/BGRA8 UNORMを受け付けます。sRGB SRV/RTV、linear atlas、texture array、MSAAはこの版では拒否します。
- 現行OSCには撮影時刻・撮影frame ID・検出lock情報がありません。`timestamp_ns`は**受信時刻**で、予測先時刻に書き換えません。`is_tracking`は常にfalseとして未知の物理lockを偽装しません。位置自体は`valid=true`で渡せます。
- 250msより古い受信は内部でstaleとし、最後の位置を保持します。描画backendは通信停滞をDEGRADEDで報告します。送信側が古い座標を繰り返す場合、受信のみではその検出喪失を識別できません。
- OSCの左右の到着差は既存互換の100ms以内。撮影時刻を揃える保証ではありません。不正bundleは部分適用せず破棄します。
- 頭部回転／gaze方向は計測していません。ベースデバイスのposeは非trackedの公称値です。眼位置を使用するDisplayXR側の投影は実runtimeで確認が必要です。
- CPUの`TrackedBarrier`は旧経験式です。0.3245m付近の特異点では最後の光学配置を保持してDEGRADEDを返します。物理レンズの新しい光線モデルへ置き換えたものではありません。

検証結果と次段階は[接続検証記録](../docs/displayxr/connection-validation-ja.md)、全体設計は[技術設計書](../docs/displayxr/technical-design-ja.md)を参照してください。
