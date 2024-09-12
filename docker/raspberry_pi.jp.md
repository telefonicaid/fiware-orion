# Raspberry Pi で Orion Context Broker を使用する方法

Raspberry Pi で Docker を使用すると、Orion Context Broker を非常に簡単に実行できます。

[Raspberry Pi](https://www.raspberrypi.org/) は、低価格のクレジット・カード・サイズのコンピュータです。
ARM ベースのデバイスであり、ARM アーキテクチャ用にコンパイルされたバイナリが必要です。 Orion の Docker
イメージをビルドして実行するには、ARM architecture 用の 64 ビット Linux と Docker を Raspberry Pi に
インストールします。

## 前提条件

### ハードウェア

ターゲット・ハードウェアは、64 ビット ARM アーキテクチャ (aarch64) をサポートする Raspberry Pi 3 および 4 です。

### Raspberry Pi OS

Raspberry Pi OS Bookworm 12 を使用することをお勧めします。[こちら](https://www.raspberrypi.com/software/)から
OS イメージを取得し、インストール手順を確認することができます。

### Docker

Raspberry Pi OS に Docker と Docker compose plugin をインストールします。Docker
のインストールの詳細は[こちら](https://docs.docker.com/engine/install/raspberry-pi-os/)です。

## Orion のビルド方法

Orion の Docker イメージをビルドするには、Orion リポジトリをクローンし、`docker build` コマンドを実行します。

```
git clone https://github.com/telefonicaid/fiware-orion.git
cd fiware-orion/docker
docker build -t orion .
```

## Orion の実行方法

次のような内容の `docker-compose.yml` ファイルを作成します :

```
version: "3"

services:
  orion:
    image: orion:latest
    ports:
      - "1026:1026"
    depends_on:
      - mongo
    command: -dbURI mongodb://mongo

  mongo:
    image: mongo:6.0
    command: --nojournal
```

Orion を実行するには、`docker compose up -d` を実行します。`curl localhost:1026/version` コマンドを実行して、
Orion が起動したかどうかを確認します。
