# Raspberry Pi で Orion Context Broker を使用する方法

Raspberry Pi で Docker を使用すると、Orion Context Broker を非常に簡単に実行できます。

[Raspberry Pi](https://www.raspberrypi.org/) は、低価格のクレジット・カード・サイズのコンピュータです。
ARM ベースのデバイスであり、ARM アーキテクチャ用にコンパイルされたバイナリが必要です。 Orion の Docker
イメージをビルドして実行するには、ARM architecture 用の 64 ビット Linux と Docker を Raspberry Pi に
インストールします。Raspberry Pi にインストールされたオペレーティング・システムで Orion を直接ビルド
して実行する場合は、その方法に関する
[ドキュメント](../doc/manuals.jp/admin/build_source.md#ubuntu-1804-lts)を参照してください。

## 前提条件

### ハードウェア

ターゲット・ハードウェアは、64 ビット ARM アーキテクチャ (aarch64) をサポートする Raspberry Pi 3 および 4 です。

### Linux OS

現在のところ、Raspberry Pi で64 ビット Linux を使用するオプションは多くありません。Ubuntu 20.04 LTS
を使用することをお勧めします。[こちら](https://ubuntu.com/download/raspberry-pi)から OS イメージを取得し、
インストール手順を見つけることができます。

### Docker

次のコマンドに従って、Ubuntu に Docker をインストールできます :

```
sudo cp -p /etc/apt/sources.list{,.bak}
sudo apt-get update
sudo apt-get install -y \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg-agent \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
   "deb [arch=arm64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get install -y docker-ce
```

Docker のインストールの詳細は[こちら](https://docs.docker.com/install/linux/docker-ce/ubuntu/)です。

### Dokcer compose

aarch64 の Docker Compose のバイナリは提供されていません。ソースコードからビルドする必要があります。
次のようにコマンドを実行して、Docker Compose のバージョン 1.27.4 をインストールできます :

```
git clone -b 1.27.4 https://github.com/docker/compose.git
cd compose/
sudo ./script/build/linux
sudo cp dist/docker-compose-Linux-aarch64 /usr/local/bin/docker-compose
```

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
    command: -dbhost mongo

  mongo:
    image: mongo:4.4
    command: --nojournal
```

Orion を実行するには、`docker-compose up -d` を実行します。`curl localhost:1026/version` コマンドを実行して、
Orion が起動したかどうかを確認します。
