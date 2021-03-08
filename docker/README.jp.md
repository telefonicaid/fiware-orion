# Docker で Orion Context Broker を使用する方法

Docker を使用して Orion Context Broker を非常に簡単に実行できます。これを行うにはいくつかの方法があります。複雑さの順に次のとおりです :

- _"Have everything automatically done for me"_ : セクション1 を参照してください。推奨する**最速の方法です**
- _"Only run a Context Broker, I'll take care of where I put my database"_ : セクション2を参照してください。**Orion を１つのコンテナで実行します**
- _"Let me see how this docker thing works from the inside"_ または _"I want to customize my Orion Docker file"_ : セクション3を参照してください。**Docker イメージのビルドです**

これらは同じことをする別の方法ですが、3つすべてを行う必要はありません。

また、Raspberry Pi で Orion を実行することもできます。これを行う方法については、この[ドキュメント](./raspberry_pi.jp.md)を参照してください。

Docker が動作するマシンが必要です。これを行う方法の [ドキュメント](https://docs.docker.com/installation/)を参照してください。

----
## 1. 最速の方法

Docker Compose を使用すると、Orion Context Broker コンテナを MongoDB コンテナに数分でリンクすることができます。この方法では、[Docker Compose](https://docs.docker.com/compose/install/) をインストールする必要があります。

Orion Context Broker を試してみたいし、データベースについて気にしたくない場合や、データの消失に気にしない場合は、この方法を検討してください。

次の手順を実行します :

1. 作業するシステム上のディレクトリを作成します。たとえば `~/fiware` です
2. ディレクトリ内に `docker-compose.yml` という名前の新しいファイルを次の内容で作成します :
	
		 mongo:
		  image: mongo:4.4
		  command: --nojournal
		 orion:
		  image: fiware/orion
		  links:
		    - mongo
		  ports:
		    - "1026:1026"
		  command: -dbhost mongo

3. コマンドラインを使用して作成したディレクトリで、`sudo docker-compose up` を実行します

> `--nojournal` に関しては、それはプロダクション利用では推奨されていませんが、Orion コンテナが高速で、DB が見つからず準備ができていない場合に、mongo コンテナの起動を高速化し、いくつかの競合状態の問題を回避します。

数秒後に、Context broker を実行し、ポート1026でリッスンする必要があります。

以下を実行し、動作することを確認します。

	 curl localhost:1026/version

この方法で行ったことは、[Docker Hub](https://hub.docker.com/) というイメージの公開リポジトリから [Orion Context Broker](https://hub.docker.com/r/fiware/orion/) と [MongoDB](https://hub.docker.com/_/mongo/) 用のイメージをダウンロードすることです。次に、両方のイメージに基づいて2つのコンテナを作成しました。

このシナリオを停止したい場合は、docker-compose が実行されているターミナルで Control+C を押す必要があります。このメソッドを使用して Orion で使用されていたデータはすべて失われます。

----
## 2. 1つのコンテナで実行

このメソッドは、Orion Context Broker を実行しているコンテナを起動しますが、MongoDB インスタンスを提供するのはあなた次第です。この MongoDB インスタンスは、ローカルホスト、ネットワーク上の他のホスト、別のコンテナ、またはネットワークにアクセスできる場所で実行されている必要があります。

> ヒント : これらの方法を試しているか、複数回実行していて、コンテナがすでに存在しているというエラーが表示された場合は、`docker rm orion1` で削除できます。コンテナを停止しなければならない場合は、まず `docker stop orion1` を実行して、止めてください。

これらのコマンドを使用すると、Orion のタグと特定のバージョンにアクセスできます。たとえば、特定のバージョンが必要な場合は、次のコマンドで `fiware/orion` の代わりに `fiware/orion:0.22` を使用できます。バージョンを指定しない場合は、デフォルトで ` 最新 ` のものから取得します。

### 2A. MongoDB はローカルホスト上にある場合

これを実行するには、このコマンドを実行します。

	 sudo docker run -d --name orion1 -p 1026:1026 fiware/orion

すべてが動作することを確認します。

	 curl localhost:1026/version

### 2B. MongoDB が別の Docker コンテナで動作している場合
他のコンテナで MongoDB を実行したい場合は、次のように起動することができます

	 sudo docker run --name mongodb -d mongo:3.4

そして、このコマンドで Orion を実行します

	 sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 fiware/orion -dbhost mongodb

すべてが動作することを確認します。

	 curl localhost:1026/version

このメソッドは、セクション1で説明したものと機能的に同等ですが、docker-compose ファイルではなく手動でステップを実行します。 MongoDB コンテナを無効にするとすぐにデータが失われます。

### 2C. MongoDB が異なるホスト上で動作している場合

別の MongoDB インスタンスに接続する場合は、前のコマンドの**代わりに**、次のコマンドを実行します

	 sudo docker run -d --name orion1 -p 1026:1026 fiware/orion -dbhost <MongoDB Host>

すべてが動作することを確認します。

	 curl localhost:1026/version
----
## 3. Docker イメージを作成

イメージを構築することで、Orion Context Broker コンテナ内で何が起こっているのかをより詳細に制御できます。Docker イメージの作成に慣れている場合、またはイメージの作成方法を変更する必要がある場合にのみ、この方法を使用してください。ほとんどの場合、イメージを構築する必要はないでしょう。すでに構築されているコンテナのみを展開してください。セクション1と2で説明しています。

ステップ :

1. [Orion のソースコード](https://github.com/telefonicaid/fiware-orion/)を Github からダウンロードします (`git clone https://github.com/telefonicaid/fiware-orion/`)
2. `cd fiware-orion/docker`
3. Dockerfile を好みに合わせて変更します
4. Orion を実行 ...
	 * docker-compose で自動化されたシナリオを使用し、新しいイメージを構築する : `sudo docker-compose up`。必要に応じて、提供されている `docker-compose.yml` ファイルを変更することもできます
	 * 手動で MongoDB を別のコンテナで実行します :
                 1. `sudo docker run --name mongodb -d mongo:3.4`
                 2. `sudo docker build -t orion .`
                 3. `sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 orion -dbhost mongodb`.
	 * 手動で MongoDB ホストを見つける場所を指定します :
		 1. `sudo docker build -t orion .`
		 2. `sudo docker run -d --name orion1 -p 1026:1026 orion -dbhost <MongoDB Host>`.

すべてが動作することを確認します

	 curl localhost:1026/version

`docker build` コマンドのパラメータ `-t orion` は、イメージに名前を付けます。この名前は何でもかまいませんし、`-t org/fiware-orion` のような組織も含めています。この名前は後でイメージに基づいてコンテナを実行するために使用されます。

`docker build` のパラメータ `--build-arg`i はビルド時の変数を設定できます。

| ARG             | 説明                                                           | 例                              |
| --------------- | -------------------------------------------------------------- | ------------------------------- |
| IMAGE_TAG       | ベース・イメージのタグを指定します                             | --build-arg IMAGE_TAG=centos7   |
| GIT_NAME        | GitHub リポジトリのユーザ名を指定します                        | --build-arg GIT_NAME=fiware-ges |
| GIT_REV_ORION   | ビルドする Orion バージョンを指定します                        | --build-arg GIT_REV_ORION=2.3.0 |
| CLEAN_DEV_TOOLS | 開発ツールをクリアするかどうかを指定します。0 の場合は残ります | --build-arg CLEAN_DEV_TOOLS=0   |

イメージとビルド・プロセスの詳細については、[Docker のドキュメント](https://docs.docker.com/userguide/dockerimages/)を参照してください。

## 4. その他の情報

Docker コンテナと Orion Context Broker を操作する際に留意すべき事項です。

### 4.1 データの永続性
Docker コンテナ化された Orion Context Broker で行うすべての作業は、非永続的なものです。 MongoDB コンテナを無効にすると、*すべてのデータが失われます*。これは、この README に記載されているいずれの方法でも発生します。

これを防ぐには、MongoDB Docker のドキュメントの "どこにデータを格納するか (*Where to Store Data*)" のセクションの [このリンク](https://registry.hub.docker.com/_/mongo/)を参照してください。その中に MongoDB のデータを永続化する方法とアイデアがあります。

#### 適切な Mongo-DB データベース・インデックスのセットアップ

サブスクリプション、レジストレーション および エンティティの詳細は、データベースから取得されます。`fiware-service`
ヘッダを指定しない場合、データベースのデフォルト名は `orion` です。適切なインデックスを作成することにより、データベース・
アクセスを最適化できます。

例 :

```console
docker exec  db-mongo mongo --eval '
    conn = new Mongo();db.createCollection("orion");
    db = conn.getDB("orion");
    db.createCollection("entities");
    db.entities.createIndex({"_id.servicePath": 1, "_id.id": 1, "_id.type": 1}, {unique: true});
    db.entities.createIndex({"_id.type": 1});
    db.entities.createIndex({"_id.id": 1});' > /dev/null
```

`fiware-service` ヘッダを使用している場合、データベースの名前は異なります。代替データベースが使用されている場合、上記の `conn.getDB()`
ステートメントを変更します。ユースケースによっては、追加のデータベース・インデックスが必要になる場合があります。
[パフォーマンス・チューニング](https://fiware-orion.readthedocs.io/en/master/admin/perf_tuning/index.html#database-indexes) および
[データベース管理](https://fiware-orion.readthedocs.io/en/master/admin/database_admin/index.html)は、Orion ドキュメントにあります。

### 4.2 `sudo` を使用

sudo を使用したくない場合は、[以下の手順](http://askubuntu.com/questions/477551/how-can-i-use-docker-without-sudo)に従ってください。

### 4.3 異なるポートでリッスン

`-p 1026:1026` では、最初の値は localhost でリッスンするポートを表します。マシン上で2番目のコンテキスト broker を実行する場合は、この値を別の値に変更する必要があります。たとえば、`-p 1027:1026` です。

### 4.4 Orion の追加パラメータ

コンテナイメージの名前 (ビルドしている場合は `orion`、リポジトリからプルしている場合は `fiware/orion`) の後ろは、Orion Context Broker のパラメータとして解釈されます。ここでは、MongoDB ホストがどこにあるかを broker に伝えています。これは、他の MongoDB コンテナの名前で表されます。他のコマンドライン・オプションの [ドキュメント](https://github.com/telefonicaid/fiware-orion)を見てください。

Orion は [マルチテナントモード](https://fiware-orion.readthedocs.io/en/master/user/multitenancy/index.html)で動作します。
