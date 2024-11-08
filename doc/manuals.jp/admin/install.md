# Orion のインストール

* [イントロダクション](#introduction)
* [要件](#requirements)
* [以前のバージョンからのアップグレード](#upgrading-from-a-previous-version)

---
> :warning: **orion を本番環境で実行する場合は、[高可用性](extra/ha.md) および[パフォーマンス・チューニング](perf_tuning.md)に関する情報もお読みください。**
---

<a name="introduction"></a>
## イントロダクション

推奨される手順は、公式の [Docker Hub の Orion docker コンテナ](https://hub.docker.com/repository/docker/telefonicaiot/fiware-orion) を使用してインストールすることです。詳細については、[このドキュメント](https://github.com/telefonicaid/fiware-orion/blob/master/docker/README.jp.md)を参照してください。

ただし、Docker 化されたインフラストラクチャがない場合は、ソースからビルドをインストールすることをお勧めします。
[このドキュメント](build_source.md)を確認してください。

<a name="requirements"></a>
## 要件

Docker hub で公式の Orion docker コンテナを使用してインストールする場合は、次のものが必要です:

* [Docker](https://docs.docker.com/engine/install/)
* [Docker compose](https://docs.docker.com/compose/install/) (厳密には必要ありませんが、強くお勧めします)

必要なソースから ビルドした Orion をインストールする場合：

* オペレーティングシステム: Debian。リファレンス・オペレーティングシステムは Debian 12.7 ですが、それ以降の
  Debian 12 バージョンでも動作するはずです
* データベース: MongoDB は、Orion Context Broker がインストールされるのと同じホストで実行するか、ネットワーク経由で
  アクセスできる別のホストで実行する必要があります。推奨される MongoDB バージョンは 6.0 です (Orion は古いバージョンで
  動作する可能性がありますが、まったくお勧めしません!)

システムリソース (CPU, RAMなど) については、[これらの推奨事項](diagnosis.md#resource-availability) を参照してください。

<a name="upgrading-from-a-previous-version"></a>
## 以前のバージョンからのアップグレード

ソフトウェアの観点からは、Orion のアップグレードは、古いコンテナまたは Context Broker を新しいものに置き換えるのと
同じくらい簡単です。ただし、既存のデータも移行する場合は、このセクションに注意する必要があります。

アップグレードパスが 0.14.1, 0.19.0, 0.21.0, 1.3.0, 1.5.0 または 2.2.0 を超えている場合にのみ注意が必要です。それ以外の場合は、このセクションをスキップできます。DB が重要ではない場合 (例えばデバッグ/テスト環境)、アップグレードする前に DB をフラッシュすることもできます

* [pre-0.14.1 のバージョンから 0.14.1 以降にアップグレード](upgrading_crossing_0-14-1.md)
* [pre-0.19.0 のバージョンから 0.19.0 以降にアップグレード](upgrading_crossing_0-19-0.md)
* [pre-0.21.0 のバージョンから 0.21.0 以降にアップグレード](upgrading_crossing_0-21-0.md)
* [pre-1.3.0 のバージョンから 1.3.0 以降にアップグレード](upgrading_crossing_1-3-0.md)
* [pre-1.5.0 のバージョンから 1.5.0 以降にアップグレード](upgrading_crossing_1-5-0.md)
* [pre-2.2.0 のバージョンから 2.2.0 以降にアップグレード](upgrading_crossing_2-2-0.md)

アップグレードが複数のセグメントをカバーしている場合 (たとえば、0.13.0 を使用していて、0.19.0 にアップグレードする場合は、"0.14.1 以前のバージョンから 0.14.1 以降にアップグレードする" と "0.19.0 より前のバージョンから 0.19.0 以降にアップグレードする" が適用されます)。セグメントを順番に実行する必要があります (共通部分は1回だけ行われます。例えば、CB の停止、パッケージの削除、パッケージのインストール、CB の開始)。疑問がある場合は、[StackOverflow を使用 して質問してください](http://stackoverflow.com/questions/ask)。質問に "fiware-orion" タグを含めることを忘れないでください。
