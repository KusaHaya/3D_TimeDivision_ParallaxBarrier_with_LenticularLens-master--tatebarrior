# Visual Studio 2026 で C++ 表示アプリをビルドする

Unity は不要です。外部 EyeTracker-RS の OSC を受信する ProjectPB.exe を作ります。
この手順は realsense-ray-tracking ブランチの ProjectPB.sln 用です。

## 準備

- Visual Studio 2026 に「C++ によるデスクトップ開発」、v145 ビルドツール、Windows SDK を導入。
- Release / x64 または Debug / x64 を選択。Win32 と ARM は対象外。
- Library/include と Library/lib に従来の x64 依存ライブラリを用意。
  FFmpeg は同梱ヘッダーに対応するバイナリを使用し、別世代に置き換えない。
- Boost のヘッダーを用意する。Boost のコンパイル済み vc141/v145 .lib は不要。
  このアプリで使用する Asio/Format と Boost.System はヘッダーで利用し、自動リンクを無効化する。

## PC ごとの設定

既存の BOOST_DIR_173 または BOOST_DIR が Boost のルートを指していれば利用します。
それ以外は ProjectPB/NativeBuild.local.props を作成します（このファイルはコミットしない）。

```xml
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <BoostRoot>D:\deps\boost_1_73_0</BoostRoot>
    <!-- Library を移動した場合だけ指定 -->
    <NativeDependencyRoot>D:\deps\ProjectPB-Library</NativeDependencyRoot>
  </PropertyGroup>
</Project>
```

BoostRoot の直下に boost/asio.hpp が必要です。
既定の依存先はリポジトリの Library、.lib は Library/lib、DLL は Library/bin。
別の構成別バイナリを使う場合は NativeLibraryDir と NativeRuntimeDir も上書きできます。
x64 の同梱ライブラリを使用するため、旧 nupengl/freeglut NuGet の復元は不要です。

## ビルドと実行

1. ProjectPB.sln を Visual Studio 2026 で開く。
2. Release / x64 を選んでリビルドする。デバッグ時は Debug / x64。
3. DLL は Library/bin と ProjectPB-Transfer から出力先へコピーされる。
   必要な DLL が元フォルダーにない場合は別途配置する。
4. EyeTracker-RS を起動し、OSC /eye/right と /eye/left を 127.0.0.1:8000 へ送信。
5. 出力先の Start.cmd で Arduino の COM ポートを合わせて起動する。
   USB カメラ、Arduino、表示同期の確認は実機で行う。

## 変更と検証の範囲

v145 と Windows SDK の既定選択を維持し、古い VC/SDK パスの上書き、
C:\boost 固定パス、vc141 版 Boost の自動リンク指定、旧 NuGet の必須チェックを除去。
Debug/x64 と Release/x64 に共通の依存設定と DLL 配置を適用。

XML 構造・依存パス・プロジェクト設定は静的検査済み。
Windows/MSBuild と実機を利用できない環境での修正のため、
Windows 上のコンパイル・リンク成功や実機動作は未確認。
