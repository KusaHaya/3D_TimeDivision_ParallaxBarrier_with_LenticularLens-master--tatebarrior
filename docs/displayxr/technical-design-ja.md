# DisplayXR対応技術設計：144Hz・4時分割パララックスバリア
作成日：2026-11-12（日本時間）／状態：ソース調査に基づく設計。実装・実機検証前。

## 1. 決定事項と調査対象
- 元ブランチ：`realsense-ray-tracking`、調査時HEAD：`a1853d0b33f0f632cae7eb492b5cfccc3d456c85`。
- 新規ブランチ：`displayxr-integration`。上記HEADから作成。元ブランチへの書き込みは行わない。
- 公式runtimeの調査スナップショット：`ac3ae19215bbcfc6aa091224d109a5b113c1c8dd`（mainを調査時に固定）。
- 公式vendor-template：`d796cef1cf9e9f75a30cf0e4c8fcff4e72e448f7`。
- 最初の対応範囲はWindows x64、D3D11、指定LCD一台、1920×1080の等倍出力、固定144Hz、全画面、不透明、単一セッション。VRR・HDR・画面拡大縮小・ウィンドウ移動・複数アプリ混在は初期範囲に含めない。
- **外部Display Processor（DP）プラグインで画像変換と追跡を実装し、4時分割の確実な運用にはpresent所有側との同期契約を追加する。** 公式公開DP ABIだけでLCD/LEDの同期保証が完成するとは扱わない。
- 4時分割は4視点ではない。OpenXR側は左右2視点、出力側で同じ左右ペアから4種類のサブピクセル選択マスクを作る。

前会話の「runtime自体を改造せずプラグインだけで完結」は、画像変換PoCには適用できるが、表示結果とバックライトの位相一致を保証する段階では再検討が必要。

## 2. 既存コードから確認できた事実

| 対象 | 実際の処理・設計への影響 |
|---|---|
| `ProjectPB/ProjectPB.vcxproj` | ビルド対象は`ExternalEyeTracker.cpp`。残存する`RealSenseTracker.cpp`を現在の稼働経路と取り違えない。 |
| `ProjectPB/main.cpp::UpdateTracking` | 外部OSCの両眼座標を受け、平均位置からhaba/MiddleLineを更新。現在の画像処理は視点ごとの3Dシーンを描き直すものではない。 |
| `ExternalEyeTracker.cpp` | UDP 127.0.0.1:8000、`/eye/right`→配列0、`/eye/left`→配列1。m単位、既定でX/Yを反転。左右到着差100ms以内でペア化。欠測時は最後の値を無期限保持し、Pollはvalidが立った後は欠測だけでfalseにならない。 |
| `main.cpp::disp` | UpdateTracking→現在kkで画像生成→glutSwapBuffers→200+displayedKkを1バイト送信→kk=(kk+1)%4。画像準備だけの呼び出しではswapも位相進行もしない。 |
| `main.cpp::init` | `wglSwapIntervalEXT(1)`。swap復帰を基準に送信するが、LCD走査やUSB遅延は測定していない。 |
| `main.cpp` | `REFRESH_RATE 120`が残存。ただし現行dispの進行は固定120HzタイマーではなくVSyncで駆動。この定数だけで実表示120Hzと断定しない。Arduino側は144Hz既定。 |
| `interleave.frag` / `sbs_interleave.frag` | 画素ごとのRGBサブピクセルについて左右を選択。`timeStep=0..3`でマスクを2サブピクセルずつ移動。静止画は左右別texture、動画はSBS。 |
| `Arduino.txt` | 115200bps。200..203で位相パルス、210..213で上下ペア連続点灯。micros基準で非ブロッキング点灯。論理位相→LEDは`{0,3,2,1}`。 |
| Arduino起動・終了 | 起動時とLIGHT_EXIT(110)で8出力全点灯。RESET_SYNC(20)で消灯・前位相消去。ENABLE_TIMEDIVISION(12)で再初期化して時分割へ。通信切断時の自動消灯機構はない。 |

追跡経路：[現在の手順](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/NativeTracking/OSC-Tracking.md)、[受信実装](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/ExternalEyeTracker.cpp#L36)、[ビルド設定](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/ProjectPB.vcxproj)。
描画と送信：[disp](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/main.cpp#L757)、[VSync設定](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/main.cpp#L480)。
LED：[スケッチ](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/Arduino.txt)、[既存の検証記録と未確認事項](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/NativeTracking/BacklightHold.md)。

### 2.1 既存の光学式を維持する
基準条件は`haba=330, MiddleLine=3420, SHIFT=1, slantY=1, correctionThreshold=0.8`。
R/G/Bの各サブピクセルc=0,1,2について：

```text
S = 3*floor(x_GL) + c + slantY*floor(y_GL)
q = (S - (MiddleLine - 48*haba)) / max(haba,1)
Q(q) = sign(q) * (floor(abs(q)) + [fraction(abs(q)) >= threshold])
left = positive_mod(S - Q(q) + 2*phase + SHIFT, 8) >= 4
```

これは現行の経験的補正式であり、新しいレイトレーシング光学モデルへの置換は別の変更として評価する。負値の剰余と量子化境界をGLSLとHLSLで一致させる。

D3DのSV_Positionは左上基準、既存gl_FragCoordは左下基準なので、全画面等倍出力では`y_GL=H-1-floor(SV_Position.y)`、`x_GL=floor(SV_Position.x)`。画像UVの反転と光学マスクYの反転は別々に検証する。RGB/BGR配列も実パネルを確認する。

[静止画の式](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/interleave.frag#L39)、[SBSの式](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/sbs_interleave.frag)。

## 3. DisplayXR公式実装との接続点

| 公式の実在する接続点 | 本装置の実装 |
|---|---|
| `xrtPluginNegotiate` / `xrt_plugin_iface` | `DisplayXR-ProjectPB.dll`、id=`projectpb`。ABI majorとstruct_sizeを尊重して交渉する。 |
| `probe`, `probe_displays`, `get_display_info` | 明示選択したLCDのmonitor identity/EDIDと装置設定を照合。他モニターを1920×1080だけで横取りしない。未接続・未準備は理由付きで辞退。 |
| `create_device` | 2Dモードと左右2視点の3Dモード。3Dはtile_columns=2, tile_rows=1。最初は各眼view_scale=(1,1)で比較し、画質確認後に縮小を検討する。4時分割をview_count=4としない。 |
| `create_dp_d3d11` | D3D11 DP生成。GL/Vulkan/D3D12/Metal factoryは初期段階ではNULL。 |
| `process_atlas` | runtimeの左右atlas SRVを読み、現在bindされているRTVへ1位相分のパターンを描く。RTVやswapchainの所有権は移さない。 |
| `get_predicted_eye_positions`, `get_eye_positions_at(target_ns)` | 共通の時刻付き追跡履歴から両眼を返す。眼順序は0=left、1=right。表示面中央原点・m・X右/Y上/Z観察者方向。 |
| `set_frame_timing`, `set_predicted_scanout`（D3D11） | 測定済み遅延と今回の予測走査までの時間を受け取る。追跡予測に利用できるが、表示完了通知でもLEDトリガでもない。 |
| `request_display_mode`, `get_hardware_3d_state`, `get_backend_state` | LED側が準備完了してから3D状態を返す。USB断・位相不明はDEGRADED等で報告。 |
| `get_scanout_caps` | 等倍が必要なのでTOLERATES_RESAMPLEは立てない。GPU weaveの標準scopeはCANVAS。本装置の全画面限定条件は追加の時間同期要件であり、このscopeを時間同期保証と誤解しない。 |
| `destroy` | USB/UDPの所有者を解放。複数DPや一時的なDP再生成があっても共有装置を二重に開かない。 |

公式根拠：[外部プラグイン導入](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/docs/guides/vendor-plugin-onboarding.md#L13)、[plugin ABI](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/include/xrt/xrt_plugin.h#L1223)、[D3D11 DP ABI](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/include/xrt/xrt_display_processor_d3d11.h#L82)、[眼座標](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/include/xrt/xrt_display_metrics.h#L25)、[scanout caps](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/include/xrt/xrt_display_scanout.h)。

### 3.1 公式テンプレートを使う際の注意
公式のvendor-templateを`DisplayXRPlugin/`配下の独立CMakeプロジェクトの出発点とし、drv_exampleをdrv_projectpbへ置換する。テンプレートのstub画像は本装置のインタリーブ式ではない。
テンプレートのCMake既定pinはv2.12.0で、今回調査したmainの全追加スロットを備える前提にはできない。採用するruntimeとヘッダーを同一SHAへ固定し、構造体サイズと機能マクロでスロットの有無を確認する。本設計の調査SHAをそのまま配布版品質認定とは扱わない。

[テンプレートCMake](https://github.com/DisplayXR/displayxr-vendor-template/blob/d796cef1cf9e9f75a30cf0e4c8fcff4e72e448f7/CMakeLists.txt)、[stub processor](https://github.com/DisplayXR/displayxr-vendor-template/blob/d796cef1cf9e9f75a30cf0e4c8fcff4e72e448f7/src/drv_example/example_processor_d3d11.cpp)、[2視点のモード表](https://github.com/DisplayXR/displayxr-vendor-template/blob/d796cef1cf9e9f75a30cf0e4c8fcff4e72e448f7/src/drv_example/example_device.c)。

### 3.2 atlasと色
view 0（left）、view 1（right）をruntimeが渡すview_width/view_heightとtile gridに従って読む。atlas全体の寸法が常に3840×1080とは仮定しない。タイル境界のサンプリング混入を避ける。
まず不透明・SDR・基準色補正なしでGL出力と照合し、その後`set_atlas_encoding`とhandoff color capabilityに合わせてsRGBの復号／再符号化を一度だけ行う。現行静止画と動画では色補正の処理が同一とは限らないので、最初から同じ補正が有効と仮定しない。

## 4. 追跡設計

### 4.1 最初はOSCを維持する
EyeTracker-RSだけがRealSenseを開く。プラグイン内の共通TrackingHubがOSCを一度だけ受信する。旧ProjectPB.exeと同時に8000番をbindしない。比較実験は逐次実行する。
旧`RealSenseTracker.cpp`は代替入力として保管し、初期統合で再有効化しない。内蔵実装の30fps設定を、外部EyeTracker-RSの実測レートと取り違えない。

新しい内部サンプルは`{left_m,right_m,receive_ns,capture_ns?,sequence,source_state}`。現行OSCは撮影時刻・frame ID・新規測定と保持の区別を運ばないため、受信時刻を撮影時刻と称さない。左右ペアは可能なら同じOSC bundleの値を優先し、独立メッセージの100ms窓による異時刻混合をログする。将来送信側に同一frame ID/撮影時刻/qualityを追加する。

- 受信停止と顔検出喪失は別の状態。送信側が保持値を送り続ける場合、受信だけで検出喪失を確定できない。
- 欠測では最後の値を保持しても`is_tracking=false`にできる状態を追加する。撮影時刻・品質が得られないPoCでは「通信が新鮮」と「物理追跡lock」を区別し、追跡精度・遅延の保証対象を限定する。
- `timestamp_ns`は回答の根拠となったサンプル時刻。target_nsへ書き換えない。公式時刻領域`os_monotonic_get_ns`へ明示的に変換する。
- 初期は受信最新値のzero-order hold。実測遅延と撮影時刻が得られてから短い上限付き速度予測を追加する。旧保持値の再受信から偽の速度を生成しない。

### 4.2 座標変換を一本化
現行OSCはcameraのX右/Y下/Z前をX左/Y上/Z前へ反転している。mainにはさらにSIN/COSとPosX/Y/Z、眼配列入替、mm化がある。これらをDisplayXR出力へそのまま重ねると二重反転・1000倍誤りになる。
新規パスは生camera座標→実測した剛体変換`T_display_from_camera`→DisplayXR表示座標（m）へ一本化。旧光学式用のX左等の変換は別のlegacy adapterに閉じ込める。SIN=0.139/COS=0.99は丸め値なので、校正では正規化した回転を使う。

表示寸法の暫定値はコードの528×297mm。ただし1920pxに対する0.27mm/pixelは528/1920=0.275mmと一致しない。画面実寸・有効発光領域・サブピクセルピッチを測り、OpenXR投影用寸法と光学補正用ピッチの相違を校正データで明示する。

`h`相当の操作は光学基準位置の校正として残し、OpenXR空間を毎回ゼロへ移す操作と混同しない。横補正gain=0.7は光学補正式に適用し、OpenXRへ返す測定眼位置を0.7倍しない。旧式の`abs(z-0.3245)<0.02`回避条件も維持する。

別の公式viewer-tracking provider APIも存在するが、初期実装ではDP所有TrackingHubへ統一する。別provider化は、光学weaverとruntimeが同一の選択済み眼サンプルを使えることを検証してから行う。
[公式の時刻付きtracking provider](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/include/xrt/xrt_tracking_plugin.h#L166)、[現行校正式](https://github.com/KusaHaya/3D_TimeDivision_ParallaxBarrier_with_LenticularLens-master--tatebarrior/blob/a1853d0b33f0f632cae7eb492b5cfccc3d456c85/ProjectPB/main.cpp#L237)。

## 5. 144Hz・4位相のスケジュール

`T=1/144=6.944444...ms`、`4T=27.777778...ms`。4位相一巡は36Hzであり、各眼が完全な4位相組を受け取る頻度を144Hzと表記しない。各LCDフレームには左右混在マスクがあるため、単純な左右交互表示の72Hzとも異なる。

| 論理位相 | shader timeStep | 旧コマンド | 物理LED index | TOP pin | BOTTOM pin |
|---|---:|---:|---:|---:|---:|
| P0 | 0 | 200 | 0 | 4 | 9 |
| P1 | 1 | 201 | 3 | 7 | 12 |
| P2 | 2 | 202 | 2 | 6 | 11 |
| P3 | 3 | 203 | 1 | 5 | 10 |

現行144Hzファームウェアのイベント時刻は「コマンド受信から」の値：

| 区間 | TOP | BOTTOM |
|---|---|---|
| 0〜2.083ms | 消灯 | 前位相（初回なし） |
| 2.083〜3.750ms | 現位相 | 前位相 |
| 3.750〜6.250ms | 現位相 | 現位相 |
| 6.250ms〜次コマンド | 消灯 | 消灯 |

上下を同時に現位相へ切り替える設計に変更しない。前位相BOTTOMの維持はLCD走査を考慮する現行仕様。LED対応表は既存記録でも仮説であり、実機で左右の見えを再確認する。

## 6. 同期方式と公式ABIの限界

### 6.1 `process_atlas`からシリアル送信しない
D3D11 compositorはprocess_atlasを実行した後にpresent所有側へ進む。同関数は別経路でも呼ばれ、画像処理呼出回数と物理表示回数の一致は契約されていない。shader処理内でphase++、200+phase送信、sleepによる6.944ms待機は行わない。
`set_frame_timing`は直前の測定残差、`set_predicted_scanout`は今回の予測時間であり、swap成功・実走査・位相IDの通知ではない。

[実際のprocess_atlas呼出](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/compositor/d3d11/comp_d3d11_compositor.cpp#L3746)、
[present所有側](https://github.com/DisplayXR/displayxr-runtime/blob/ac3ae19215bbcfc6aa091224d109a5b113c1c8dd/src/xrt/compositor/d3d11/comp_d3d11_target.cpp#L414)。
targetではDXGI_PRESENT_DO_NOT_WAITによる再試行・期限切れ等があるため、「weaveできた＝表示された」とはできない。

### 6.2 段階A：現行互換の計測用PoC
新ブランチの旧GLアプリから光学式・シリアル処理を共通化して、現行方式の計測基準を残す。swap後送信はこのベースライン専用とする。LCD光学マーカー、TOP/BOTTOM LED、USB受信時刻を測り、位相ずれの分布を取得する。公式DPではLEDを固定holdにして各位相画像を検証する。これならruntime変更前でも接続の正否を切り分けられる。

### 6.3 段階B：present所有側への最小追加
**以下は提案APIであり、公式に実装済みの関数名ではない。**
runtime側にvendor非依存のtemporal-output capabilityとpresent tokenを追加する。装置固有の光学・COMコマンドはプラグイン側に置く。

```text
TemporalFrameToken:
  epoch, sequence, phase, source_generation
  predicted_scanout_ns, output_period_ns, calibration_generation
BeforeWeave(token)  -> そのtokenを使って1位相だけ描く
PresentQueued(token, status, present_id) -> queue採用／失敗を通知
ScanoutObserved(present_id, refresh_counter, observed_ns)
TimingLost(reason) / Disarm()
```

- present所有側が一つの位相スケジューラを持ち、1物理リフレッシュにつき一つのtokenを発行する。
- phaseはtokenから決める。再描画・ミラー・スクリーンショットによって進めない。
- Present成功はqueue採用を意味し、光学表示を意味しない。DXGI統計とrefresh counterで対応を追跡する。統計は事後情報なので今回のLEDトリガには直接使わず、検証・補正・異常検出に使う。
- 初期はpresent待ち行列を最小化し、統計と実パネル光学時刻の対応を実測する。統計が取得不能・停止・不連続なら同期保証モードを停止する。
- アプリが36Hzでも出力は144Hzで連続再weave/re-presentする必要がある。これはDP関数単体の責務ではなくpresentループの責務。
- アプリの新しい左右atlasはまずP0でのみラッチし、一巡中は同じcontent generationを用いる。追跡値・haba/MiddleLineも最初は一巡で固定し、後に各位相の予測更新と比較する。
- アプリが止まっても所有コピー上の最後の左右画像から4位相を継続生成する。借用SRVを無期限保存しない。GPUのコピー完了と利用終了をfence/queryで管理する。
- drop/repeat/USB遅延で位相が不明なら暗転→epoch更新→P0で再同期。LCDだけ、LEDだけを追いつかせない。
- hardwareがP0の表示を確認できない間は点灯開始しない。初期化・モード変更で点灯したまま位相をリセットしない。

Windows service経路も対応対象になるが、今回のGitHub取得では大きな`comp_d3d11_service.cpp`本文が空で返り、直接確認できなかった。その経路の正確な挿入関数は未確定。in-processのcompositor/targetは本文確認済み。実装開始時にserviceの実present箇所と再weaveループをローカルソースで確認し、同じtoken契約を適用する。service対応を完了扱いにせず、初期PoCの実行経路をログで固定する。

### 6.4 段階C：物理走査に同期するLED方式
推奨は、MCUに実際のLCD走査と対応する外部同期基準を入力し、将来フレーム用のphase/sequenceを先に準備する方式。パネルVSYNCを取得できる場合は絶縁・電気条件を確認して使用し、取得できない場合は画面内光学マーカー＋フォトダイオードを候補とする。D435のフレーム時刻をLCD同期信号の代用にしない。

marker方式では検出位置・LCD応答による遅延を校正する。既存の2.083/3.750/6.250msを、USB受信基準からVSYNC基準へそのままコピーしない。既存実測の「受信→走査」オフセットを含め、上下ごとに再測定する。

USBのみの場合は、PC単調時刻とMCU microsの対応、往復遅延、drift、将来時刻予約による方式を評価できる。ただしOS/USBの遅延上限が保証されず、欠落表示も分からないため、物理信号なしでの厳密な同期保証はしない。まず計測用モードとして扱う。

新規firmwareは旧スケッチと分けて互換性を保つ：
- capability/firmware version、refresh/timing設定読戻し。
- length/version/epoch/sequence/phase/target_tick/CRCを持つパケット。
- `PREPARE`→ACK→物理同期で実行、`DISARM`、`HOLD`、heartbeat。
- MCUはsequence欠落・遅着・epoch不一致・sync喪失で消灯する。PC側にも状態を返す。
- 通常runには通信watchdogを設け、明示HOLDとは別状態にする。
- 115200bpsでは8N1で1byte約86.8µs、24byte約2.083ms。帯域平均が足りても締切余裕は減るため、予定コマンドは前もって送る。baud変更は両端・実機で評価する。
- 既存の起動／終了全点灯はlegacy照明モードとして残せるが、XR run中の同期喪失は全消灯とする。旧LIGHT_EXITを障害時の消灯APIとして使わない。

### 6.5 時間予算
6.944msの中で描画・queue・走査が重なる。TOP/BOTTOMの発光中心は同時ではなく、単一の眼予測時刻では誤差が残る。初期は一巡固定の眼座標で安定性を測り、その後、各位相と上下発光の重心に対する予測を比較する。2視点の一枚から上下で別時刻のシーンを完全再現できるわけではない。
6.250msの全消灯時刻から次周期までは名目約694µsだが、これを許容USB jitterとみなさない。実LCDのsettling時間と光学クロストークから許容位相誤差を決める。

## 7. ファイル単位の変更方針
下表は将来の実装計画。この文書の追加時点でコードは変更しない。

| ファイル／追加先 | 変更方針 |
|---|---|
| `ProjectPB/main.cpp` | UpdateTrackingの校正計算、シリアル、位相管理を共通層へ分離。旧GL経路を比較用として残す。残存120Hz定数は用途を調査し、起動時の実モード照合へ置換する。 |
| `ProjectPB/ExternalEyeTracker.h/.cpp` | 新規EyeSampleの眼名、受信時刻、状態を返すAPIへ拡張。旧Pollをadapterとして残す。有限値検査・左右ペア品質・到着間隔を記録する。 |
| `ProjectPB/OscEyePacket.h` | 既存fff/bundle互換を保ち、将来の時刻・sequenceフィールドを明示versionで追加する。 |
| `ProjectPB/interleave.frag`, `sbs_interleave.frag` | 校正の基準実装として維持。新HLSLと全位相のマスク比較を行い、必要な修正だけ同時反映。 |
| `ProjectPB/Arduino.txt` | 既存互換実験用を維持。新同期protocolへの無言置換をしない。 |
| `ProjectPB/ProjectPB.vcxproj` / `ExternalEyeTracking.props` | 共通層参照を追加。RealSenseの二重起動や新プラグインへのFFmpeg/GLUT依存混入を避ける。 |
| `ProjectPB/Start.cmd` | 旧アプリ用。runtime serviceの設定にこの子プロセス環境変数が伝わる前提を置かない。 |
| `NativeTracking/OSC-Tracking.md`, `BacklightHold.md` | legacy／新protocol、手動hold／run、送信側品質の制約と実機測定値を追記。 |
| `DisplayXRPlugin/CMakeLists.txt`（新規） | 公式templateを基にruntime SHA固定、x64、plugin DLL、共通層・HLSLを構築。 |
| `DisplayXRPlugin/src/drv_projectpb/projectpb_plugin.c`（新規） | negotiation、probe、display info、設定schema、platform state、DP factory。 |
| `.../projectpb_device.c`（新規） | 2D/左右2viewモード、物理寸法、render scale、tracking modeを一貫して宣言。 |
| `.../projectpb_processor_d3d11.cpp`（新規） | atlas読込、校正uniform、HLSL描画、眼問い合わせ、mode状態。COM I/Oはrender threadで待たない。 |
| `DisplayXRPlugin/shaders/projectpb_interleave.hlsl`（新規） | 既存式の厳密移植、Y基準、atlas UV、正の剰余、量子化境界、eye mask。 |
| `Shared/TrackingHub.*`, `DisplayCalibration.*`, `BarrierModel.*`（新規） | OSC所有・時刻履歴、camera→display変換、光学式と校正世代を共通化。 |
| `Shared/BacklightTransport.*`, `TemporalOutput.*`（新規） | serial専用worker、ACK/締切・state machine、epoch/token管理。 |
| `firmware/projectpb_sync/projectpb_sync.ino`（新規） | 物理sync入力、timer compare、予約packet、watchdog、実行ログ。MCU機種・入力pin・timer資源を実機確認して決定。 |
| `DisplayXRPlugin/installer/`（新規） | 独立DLL登録。id=projectpb、表示対象とCOMはhost settingsで設定。runtime所有ディレクトリへ上書きしない。 |
| `tests/displayxr/`（新規） | 光学マスク・座標・token・故障注入・ABI試験。 |
| `docs/displayxr/validation/`（新規） | 機器条件、CSV、波形、クロストーク、合否記録。 |

DisplayXR runtimeの別checkout/forkで必要になる変更：
- `src/xrt/include/xrt/xrt_display_processor_d3d11.h`または新しい版付きtemporal ABI：提案token/capability/通知を末尾追加。既存スロットの並べ替えやsignature変更は避ける。
- `src/xrt/compositor/d3d11/comp_d3d11_compositor.cpp`：選択したtokenをweaveへ渡し、アプリ更新と144Hz出力を分離する。
- `src/xrt/compositor/d3d11/comp_d3d11_target.cpp/.h`：present ID・成否・refresh統計の対応付けと通知。
- `src/xrt/compositor/d3d11_service/comp_d3d11_service.cpp/.h`：本文を取得してpresent所有点を特定後に同じ契約を適用。現時点は挿入関数未確定。
- ABI gate/loader/tests：旧pluginでは従来動作、新機能はcapabilityに応じて有効化。公式upstream採用の可否は別途協議する。

このリポジトリの新ブランチだけでは外部runtimeの変更は入らない。runtime側patchは別成果物としてSHAと差分を管理する。

## 8. 段階的な実装・検証計画

| 段階 | 実施 | 合格条件／次へ進む条件 |
|---|---|---|
| 0 基準保存 | Windows実モード144Hz、パネルID、GPU/driver、Arduino機種・firmware、配線、h校正値、実寸を記録。旧GLでP0..P3 holdと連続runを測定。 | 全位相のLCD画像とLED配線が対応。左右反転、USB遅延、走査位置別応答の基準記録がある。 |
| 1 純粋計算 | GLSL/HLSL/CPUで負座標・量子化境界・SHIFT・haba極値を比較。左右赤青、RGB単色、片眼OFFを使用。 | 1920×1080×RGB×4位相の左右選択が一致。無効値・z特異点を拒否。 |
| 2 plugin接続 | pinned templateを先にbuildし、自作DLLを交渉・probe・登録。LEDはholdで静止比較。 | 2view/2×1のatlasが正しい眼順・Y方向で表示。実LCDとdisplay infoが一致。旧ABI/短structでも破壊しない。 |
| 3 追跡 | OSC録画再生、実機左右/上下/前後移動、片眼遮蔽、送信停止、再開、port競合を試験。 | 眼順・m単位・回転・表示中央が一致。光学校正とOpenXR投影が同一測定から計算される。保持と測定を混同しない。 |
| 4 present同期 | 144Hzスケジューラとtokenを導入。36/72/144Hzのアプリ、静止、負荷、Present失敗・repeat・dropを注入。 | 物理出力単位でP0→P1→P2→P3。アプリ停止でも出力継続。位相不明時は暗転し、誤ったLEDを点灯しない。 |
| 5 MCU同期 | logic analyzer/oscilloscopeでsync入力、TOP/BOTTOM、光学markerを同時計測。USB遅延・欠落・切断・micros wrap・再接続を注入。 | 実際の光学時刻に対する各LED窓が校正許容内。sequence/epoch不整合で消灯・再同期。 |
| 6 光学評価 | 固定左右眼位置、画面上中下・左右、複数奥行き、ゆっくり／速い頭部移動で評価。 | 各位相の左右漏れ、視域、輝度、色、フリッカー、motion-to-photonを旧GL基準と比較。事前に定めた研究上の許容値を満たす。 |
| 7 運用評価 | session作成破棄、2D/3D変更、service再起動、画面/USB切断、競合起動。 | 単一所有・資源解放・状態表示・再接続が再現可能。未対応モードで誤動作せず、測定条件つきの手順書がある。 |

各実機runでは最低限`epoch,sequence,phase,source_generation,eye_sample_time,weave_begin/end,present_id,predicted_scanout,observed_refresh,USB_tx/ACK,MCU_event_ticks`を保存する。2秒平均FPSだけでは同期合格としない。
144Hz連続10分（86,400 refresh）を最初の連続運転試験とし、位相不整合ゼロ、欠落は全件検出・暗転復帰できることを目標とする。これは提案する試験条件で、実施済みの結果ではない。時間誤差のp50/p95/p99/max、左右クロストークは上下位置別に報告する。数値合格閾値は段階0/5の光学計測から決定し、未測定の「1msなら十分」等は採用しない。

## 9. 今回完了したこと・未完了のこと
完了：元ブランチと公式公開コードの読取り、現行OSC経路・光学式・LEDタイミングの確認、新ブランチ作成、接続点／不足する同期契約／変更ファイル／検証計画の設計。
未実施：plugin実装、runtime改造、ビルド・ABI実行試験、LCD/Arduino/RealSense実機測定。
未確定：serviceの実装挿入関数、MCU機種と外部syncの取得方法、EyeTracker-RSの撮影時刻・quality提供可否、実パネル走査・応答、物理ピッチ、LED位相反転仮説の妥当性。
