# JNetHackビルド手順

NetHackにJNetHackのパッチを当てた状態、あるいはgitレポジトリからcloneした状態から開始します。カレントディレクトリはベースディレクトリ(`/src`, `/dat` などがあるディレクトリ)とします。

この状態では文字コードはShift_JISになっていますが、OSによってビルドするための文字コードが異なります。以下の手順では文字コード変換処理も含んでいます。

## Windows

VisualStudio 2015に対応しています。「開発者コマンド プロンプト for VS2015」から実行します。

```
sys/winnt/setup.bat
cd src
nmake install
```

## Linux

NetHackのビルド環境に加えて`nkf`が必要です。

```
sh japanese/set_lnx.sh
make install
```

## MacOS

NetHackのビルド環境に加えて `GCC@5以上`, `nkf` が必要です。XCodeに含まれている`gcc`ではビルドできませんので、homebrewでインストールしてください。

設定ファイルはGCC@5を前提として書かれています。6以上でもビルドできますが、`sys/unix/hints` と `japanese/set_mac.sh` の修正が必要です。

```
sh japanese/set_mac.sh
make install
```
