# 時分割パララックスバリア：共有レンズモデル

Unity 6000.6.4f1 / Built-in Render Pipeline。`realsense-ray-tracking`ブランチのUnitySimulationをUnity Hubに追加し、Assets/Scenes/Simulation.unityを開いてPlay。Gameビュー1440×960推奨。Python・RealSense・Arduinoは不要です。

## 全タブ共通の光学条件

LED → 平面 → PMMA内部 → 凸面 → 液晶 → 観察者。液晶までの距離は**凸面頂点から**、LEDまでの距離は**入射平面から**と定義します。

- LED幅3 mm、中心間隔22.5 mm、LED–平面150 mm。
- 凸面頂点–液晶6.5 mm。
- レンズ物理ピッチ0.700 mm、曲率半径0.500 mm、傾き横：縦=1：3（観察者から見て＼）。
- PMMA屈折率1.49、平面–頂点厚さ2 mmは計算上の仮定で調整可能。
- 液晶528×297 mm、1920×1080、RGB横配列。初期視点は液晶前600 mm、眼間64 mm。

レンズ仕様の参考：https://www.ntkj.co.jp/products/lenticular-lens/ 。焦点距離をメーカー保証値として扱っていません。以前の6.10 mmの仮値と0.141 mmの間隔は通常実行のどのモードでも使用しません。

LED/LENS TESTとIMAGE TEST・GEOMETRYは同じ設定を共有します。距離・LED幅・間隔をどのタブで変更しても他のタブに反映されます。設定保存・読み込みも共通。古い設定JSONには新しいレンズ設定の初期値を補い、古い像幅や結像距離を再利用しません。

## LED / LENS TEST

2つの距離を独立して動かし、液晶位置での照射分布と水平断面を確認します。Place display at calculated image planeで結像位置へ移動します。屈折率・厚さも調整可能です。幅は半値幅ではなく幾何光学的な全照射幅です。

位相kではLED中心 `(k+4*j)*22.5 mm` の列だけを点灯。初期j=-4..4の9本、groups each sideを0にすると1本です。位相は全タブで共通。レンズレットは必要な範囲で連続して存在すると仮定します。

LED/レンズの各組の積分を1に規格化した相対分布で、距離変更時の絶対照度ではありません。画像は固定ゲイン、断面グラフは縦軸を自動調整し最大値を表示します。Export all 4 illumination phasesで4位相のPNG・CSV・設定JSONを保存します。

## IMAGE TEST

初期描画方式は元プログラムの **ProjectPB: still shader**。realsenseコミット1246f420のinterleave.fragに基づき、斜め座標S=3*x+channel+slantY*y（左下原点）に対し、q=(S-(MiddleLine-48*haba))/max(haba,1)、value=S-q+2*phase+SHIFTを計算します。正の剰余value mod 8が4以上ならL、未満ならR。初期haba=330、MiddleLine=3420、SHIFT=1、slantY=1。

ProjectPB: SBS integer shaderはqをゼロ方向へ整数化します。入力はどちらも左右別画像。元の色補正は含みません。Calibrate depth hereとDepth correctionでhabaにint((Zref-Z)/2)を加算（mm）。横位相はMiddleLine/SHIFTで手動調整し、カメラ固有の横追跡は移植していません。

**Geometric allocation** は新しい共通のレンズ逆追跡でL/R割当を決定します。このモードのみBlack/Mix/Left priorityの重なり処理を選択できます。hで描画用視点を固定し、観察視点を動かして比較できます。

PNG/JPGを左右別々に選ぶか、内蔵L/R・白黒診断画像を使用します。LCD DRIVEは1枚の物理的な液晶画像です。左右の受光結果は必ずその同じ画像と同じ点灯位相から計算します。

Average all 4 phases OFFで単位相、ONで静止視点の線形光量平均。ゲインは固定（初期4）で暗部の自動補完はありません。Export synchronized phases 0-3で同じ視点の全位相・平均を保存します。観察画像は液晶正面の座標へ整形した比較画像で、透視投影の網膜像ではありません。

## GEOMETRY / 4位相集計

単位相：緑=左のみ、青=右のみ、ピンク=両方、暗色=どちらにも届かない。

Show exclusive coverage：4位相のうち、緑=左だけに単独提示の機会あり、青=右だけ、白=左右それぞれに別々の機会あり、暗色=どちらにも専用機会なし。描画画像の正誤を判定する表示ではありません。

CPU全画素検査とGPU表示は同じ逆追跡式・有限LED配列を使います。全6,220,800 RGB中心を検査し、左右の独立提示機会と全RGBが満たされる画素数を集計。設定変更後は再検査が必要です。全体縮小表示の模様だけで判定しないでください。

3D図の線も、液晶から凸面・平面・LEDへ逆追跡した代表光線です。緑/青は点灯LEDに到達、暗い線は到達しない経路。面間隔の拡大は図だけに適用します。

## 共通計算

D=LED–平面、T=厚さ、n=屈折率、R=曲率半径、t=頂点–液晶として、A=D+T/n、f=R/(n-1)、結像距離d=1/(1/f-1/A)。初期d約1.0273 mm、照射像–液晶約5.4727 mm。液晶が結像面より手前でも計算できます。

液晶の斜め座標p=x+slope*y、目の座標e=xe+slope*ye、目の奥行きZから、theta=(e-p)/Z、凸面座標v=p-t*theta。レンズ水平ピッチL=0.7*sqrt(1+slope^2)とし、開口内位置u=v-L*floor(v/L+0.5)。LED面の到達座標は **v-A*(theta+u/f)**。これが点灯LEDの幅内か判定します。照射像の帯に当たるだけで可視とする旧方式は実行経路から除外しました（旧方式の解析テストのみ残存）。

照射分布の順方向式はレンズ中心a、開口内位置u、LED位置bから x=a+u+t*((a+u-b)/A-u/f)。全幅はtW/A+abs(1+t/A-t/f)*L。逆追跡と同じ近軸屈折を表します。

## 限界と検証

近軸光学モデルであり、曲面上での厳密なSnell屈折、球面収差、全反射、回折、開口遮蔽、レンズの有限外形、LED放射特性、瞳径、サブピクセル開口面積、液晶ガラスの屈折、LCD応答・Arduino同期は含みません。特にこの高開口数レンズや斜入射では実機と差が出ます。相対的な照射分布と眼方向の可視判定は別の量で、絶対光量の一致は保証しません。

Validation/Run-ModelChecks.ps1（.NET 10）またはParallax > Run Model Checksで検査。順逆光線一致、位相、結像、幅、有限LED配列、従来の解析ケースを検査します。C#とD3D HLSL単体コンパイルを確認済み。Unity 6000.6.4でのGPU画面・実機比較は未検証です。

保存先はApplication.persistentDataPath/Results（画面に表示）。Parallax > Build Windows SimulatorでWindowsビルドし、別PCにはBuilds/Windows全体を渡します。
