# Orion のインストール

* [イントロダクション](#introduction)
* [要件](#requirements)
* [インストール](#installation)
    * [yum の使用(推奨)](#using-yum-recommended)
    * [RPM ファイルの使用](#using-rpm-file)
* [以前のバージョンからのアップグレード](#upgrading-from-a-previous-version)
    * [DB に格納されたデータの移行](#migrating-the-data-stored-in-db)
    * [標準アップグレード](#standard-upgrade)

<a name="introduction"></a>
## イントロダクション

推奨される手順は、CentOS 7.x で RPM パッケージを使用してインストールすることです。ソースからのビルドに興味がある場合は、[このドキュメント](build_source.md)を確認してください。

<a name="requirements"></a>
## 要件

* システムリソース : [これらの推奨事項](diagnosis.md#resource-availability)を参照
* オペレーティング・システム : CentOS/RedHat。リファレンス・オペレーティング・システムは CentOS 7.4.1708 ですが、それ以降の CentOS/RedHat 7.x バージョンでも動作します
* データベース : MongoDB は、Orion Context Broker をインストールするホストと同じホストで実行するか、ネットワーク経由でアクセス可能な別のホストで実行する必要があります。推奨される MongoDB のバージョンは 3.6 です。古いバージョンでは動作しますが、推奨しません
* RPM の依存関係 (これらのパッケージのいくつかは、公式の CentOS/RedHat リポジトリにはありませんが、EPEL にあります。EPEL リポジトリを設定する必要があります。<http://fedoraproject.org/wiki/EPEL> を参照してください) :
    * contextBroker パッケージ (必須) は次のパッケージに依存しています : libstdc++, boost-thread, boost-filesystem, gnutls, libgcrypt, libcurl, openssl, logrotate, libuuid

<a name="installation"></a>
## インストール

利用可能なパッケージは2つあります :

* Nightly, 毎晩マスター・ブランチからビルドされます
* Release, "official" リリースの RPM。通常のリリース期間は 1-2 ヶ月です

タイプ (nightly or release) によって、パッケージのバージョンが異なります :

* Release パッケージ : contextBroker-X.Y.Z-1
* Nightly パッケージ : contextBroker-X.Y.Z-yyyy.mm.dd

Nightly パッケージのバージョンは常にリリース前になりますので、Nightly パッケージはテスト目的でのみ使用することをお勧めします。

この点から、Nightly ビルドを使用する場合は、別のバージョンのシステムがあることに注意してください。

<a name="using-yum-recommended"></a>
### yum の使用 (推奨)

FIWARE yum リポジトリを設定します ([この記事で説明します](http://stackoverflow.com/questions/24331330/how-to-configure-system-to-use-the-fi-ware-yum-repository/24510985#24510985))。そして、root として、以下を実行します :

```
yum install contextBroker
```

上記のコマンドは、yum キャッシュのために失敗することがあります。その場合は、`yum clean all` を実行して再試行してください。

<a name="using-rpm-file"></a>
### RPM ファイルの使用

[FIWARE Files area](https://nexus.lab.fiware.org/service/rest/repository/browse/el/7/x86_64/) から直接パッケージをダウンロードします。両方のタイプのパッケージを提供しています。

次に、rpm コマンドを使用してパッケージをインストールします (root として) :

```
rpm -i contextBroker-X.Y.Z-1.x86_64.rpm
```

<a name="upgrading-from-a-previous-version"></a>
## 以前のバージョンからのアップグレード

アップグレード手順は、インストールされている Orion バージョンからアップグレードするための *アップグレードパス* が、必要なバージョン番号を超えているかどうかによって異なります :

* MongoDB バージョンのアップグレード
* DB に格納されたデータの移行 (DB データモデルの変更による)

<a name="migrating-the-data-stored-in-db"></a>
### DB に格納されたデータの移行

アップグレードパスが 0.14.1, 0.19.0, 0.21.0, 1.3.0, 1.5.0 または 1.14.0 を超えている場合にのみ注意が必要です。それ以外の場合は、このセクションをスキップできます。DB が重要ではない場合 (例えばデバッグ/テスト環境)、アップグレードする前に DB をフラッシュすることもできます

* [pre-0.14.1 のバージョンから 0.14.1 以降にアップグレード](upgrading_crossing_0-14-1.md)
* [pre-0.19.0 のバージョンから 0.19.0 以降にアップグレード](upgrading_crossing_0-19-0.md)
* [pre-0.21.0 のバージョンから 0.21.0 以降にアップグレード](upgrading_crossing_0-21-0.md)
* [pre-1.3.0 のバージョンから 1.3.0 以降にアップグレード](upgrading_crossing_1-3-0.md)
* [pre-1.5.0 のバージョンから 1.5.0 以降にアップグレード](upgrading_crossing_1-5-0.md)
* [pre-2.2.0 のバージョンから 2.2.0 以降にアップグレード](upgrading_crossing_2-2-0.md)

アップグレードが複数のセグメントをカバーしている場合 (たとえば、0.13.0 を使用していて、0.19.0 にアップグレードする場合は、"0.14.1 以前のバージョンから 0.14.1 以降にアップグレードする" と "0.19.0 より前のバージョンから 0.19.0 以降にアップグレードする" が適用されます)。セグメントを順番に実行する必要があります (共通部分は1回だけ行われます。例えば、CB の停止、パッケージの削除、パッケージのインストール、CB の開始)。疑問がある場合は、[StackOverflow を使用 して質問してください](http://stackoverflow.com/questions/ask)。質問に "fiware-orion" タグを含めることを忘れないでください。

<a name="standard-upgrade"></a>
### 標準アップグレード

yum を使用している場合は、以下でアップグレードを実行できます (root として) :

```
yum install contextBroker
```

上記のコマンドは、yum キャッシュのために失敗することがあります。その場合は、`yum clean all` を実行 して再試行してください。

RPM ファイルを使用してアップグレードする場合は、まず [FIWARE yum repository](https://nexus.lab.fiware.org/service/rest/repository/browse/el/7/x86_64/) から新しいパッケージをダウンロードしてください。両方のタイプのパッケージを提供しています。

rpm コマンドを使用してパッケージをアップグレードします (root として) :

```
rpm -U contextBroker-X.Y.Z-1.x86_64.rpm
```
