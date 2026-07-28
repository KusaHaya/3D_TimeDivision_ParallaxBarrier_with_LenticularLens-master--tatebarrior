# 3D Time-Division Parallax Barrier with Lenticular Lens

レンチキュラレンズを使った、時分割パララックスバリア方式の立体表示システムです。

## ブランチ

| ブランチ名 | 説明 |
| --- | --- |
| `master` | Visual Studio 2017環境・LEDバックライト向け |
| `dev` | 開発用 |
| `naname` | 斜めバリア向けの実装 |

## 動作環境

- Windows
- Visual Studio 2017
- x64
- Boost 1.73.0
- Kinect SDK

## セットアップ

1. [Kinect SDK](https://www.microsoft.com/en-us/download/details.aspx?id=44561)をインストールします。
2. Boost 1.73.0を用意し、環境変数 `BOOST_DIR_173` に展開先を設定します。
3. Visual Studio Community 2017をインストールします。
4. このリポジトリをクローンします。
5. ライセンス上再配布できないDLLやメディアファイルは、各自で正規の配布元から入手し、必要なディレクトリへ配置してください。
6. `Release | x64` 構成でビルドします。

> 一部の外部ライブラリ、DLL、動画ファイル、ハードウェア、およびKinectトラッキングサーバーは、このリポジトリに含まれていません。

## 実行方法

1. Kinectトラッキングサーバーを起動します。
2. 本プログラムを起動します。
3. 使用するハードウェアを時分割モードに切り替えます。

Kinectトラッキングサーバーの参考実装:

- `visual-media-lab/3D_Kinect_Tracking_Server`

## 主なキー操作

| 入力キー | 動作 |
| --- | --- |
| `h` | ヘッドトラッキングのON/OFF |
| `m` | 動画モードへ移動 |
| `Shift + m` | 静止画モードへ移動 |
| `t` | 時分割モードのON/OFF |
| `q`, `w`, `e`, `r` | フレーム0〜3を描画 |
| `1` | 静止画1 / Teapotへ切り替え |
| `2` | 静止画2 / 動画表示へ切り替え |
| `0`, `3`〜`7` | 静止画を切り替え |
| `Shift + b` / `b` | 右目描画のOFF / ON |
| `Shift + v` / `v` | 左目描画のOFF / ON |
| `Shift + ;`, `-` | 動画モードのスケール変更 |
| `[` / `]` | 幅パラメータの増減 |
| `k` | バックライトとの同期調整 |
| `Shift + k` | 観測者のz座標と幅パラメータを出力 |

## ライセンス

このプロジェクトは [MIT License](LICENSE) の下で公開されています。

外部ライブラリ、SDK、素材には、それぞれのライセンスが適用されます。
