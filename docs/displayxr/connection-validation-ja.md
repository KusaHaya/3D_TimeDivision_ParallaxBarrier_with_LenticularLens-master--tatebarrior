# DisplayXR接続実装・第1段階の記録

## 実装状態

`displayxr-integration`ブランチに`DisplayXRPlugin/`を追加。元のProjectPB、Arduinoスケッチ、元ブランチのコードは変更していません。

公式runtime基準：`ac3ae19215bbcfc6aa091224d109a5b113c1c8dd`、plugin ABI major 5。Windows x64 / MSVC 19.37.32822 / Windows SDK 10.0.26100 / Releaseでビルドを確認しました。

公開シンボルは`xrtPluginNegotiate`の一つだけです。DLLの依存確認では`DisplayXRClient.dll`、RealSense、OpenCV、FFmpeg、GLUTを静的importしていません。Windows標準DLLとVisual C++ runtimeを使用します。

## 実行した検証

| 検証 | 結果 |
|---|---|
| CMake構成、Windows x64 Releaseビルド | 成功 |
| CPUの旧光学式との比較 | 負の補正値、量子化閾値、4位相、RGB選択が一致 |
| WARP D3D11画像比較 | 1920×1080×RGB×4位相 = 24,883,200要素が一致 |
| atlas座標 | 各眼64×32、実texture160×48の余白付き入力で、眼順・上下・タイルstrideを確認 |
| 描画状態 | 呼び出し元のviewportとprimitive topologyが復元されることを確認 |
| ABI交渉 | major不一致を拒否。短いhost構造体で範囲外読取りをしない経路を確認 |
| 可変サイズ出力 | platform statusでcallerのstruct_size以降のcanaryを破壊しないことを確認 |
| opt-inと設定 | ENABLE既定0、不正位相、未校正TRACKINGを拒否 |
| 眼データ | 眼名、X右/Y上の軸、m単位、過去の問い合わせ、保持時刻、NaN、center無視、左右到着差を確認 |
| UDP loopback | 不正bundleの部分適用防止、排他的bind、左右眼受信に成功 |
| plugin内の追跡接続 | OSC→共有receiver→公式get_eye_positions_at→光学shaderまで通過 |
| 所有権 | 二つのDPが一つのUDP receiverを共有し、一方の破棄後ももう一方がデータを保持 |
| 原点ずれ | panel原点がずれるとDEGRADEDを返す |

CTestは`projectpb_core`と`projectpb_abi_gpu`の2件に成功しました。これらの中に上表の複数項目を含みます。実機の144Hz発光同期を試験した結果ではありません。

最初の制限付き実行環境ではUDPの受信テストだけ失敗しました。通常のWindowsローカル通信環境で再実行すると成功したため、loopbackを許可した環境での結果として記録します。

## サービス側の調査更新

前回設計時にGitHub経由で本文を取得できなかった`comp_d3d11_service.cpp`を、今回の別チェックアウトで確認できました。公式ソースは改変していません。

同ファイルの接続点（上記SHA、1-based行番号）：

| 経路 | 画像変換 | Present |
|---|---:|---:|
| `pipeline_default_policy_render` | 16107 | 16292 |
| `svc_ws_presenters_render` | 20567付近 | 20581 |
| `multi_compositor_render_frame` | 20981 | 21191 |
| `compositor_layer_commit`のstandalone経路 | 27876 | 28008 |
| texture出力の`pipeline_client_texture_weave` | 23443 | この関数はweave出力。実present所有側を別途追跡する必要あり |

`pipeline_present_app_hwnd`（13855以降）にもpresentの共通処理があります。standaloneでは`Present(1, DXGI_PRESENT_DO_NOT_WAIT)`を再試行し、50msの期限でdropする処理があります。`process_atlas`が完了しても、その画像が実パネルへ表示されたことにはなりません。

このため次段階の144Hz連続出力は、アプリframe更新から独立したpresent所有側で管理します。画像変換回数だけで位相を進める設計は採用しません。

## 次の実装と実機検証

1. 本装置側でruntimeの版、指定LCDの名前、144Hzモード、物理寸法、LED HOLDのP0〜P3を確認する。
2. 固定位相で旧GLと新D3D11の見えを比較する。自動テストではGPU画像が一致しても、実LCDのRGB配列・走査・LED対応が正しいとは限らない。
3. 生OSC座標とカメラ外部校正を検証し、光学基準位置を設定して、固定位相のまま追跡補正を比較する。
4. serviceの使用経路を一つに固定し、設計書のepoch/sequence/phase/present_idのtoken契約をruntime別checkoutへ実装する。初期は単一パネル・単一セッションに限定する。
5. LCD同期入力の取得方法とMCU機種を確定し、新protocol、遅着時の消灯、ACK、watchdogを実装する。旧Arduinoへ自動書込みはしない。

未実施：DisplayXR service/OpenXRアプリからの実起動、実RealSense入力、実LCD出力、Arduino接続・書込み、144Hz連続同期、光学クロストーク・遅延測定。
