# Docker Swarm クラスタに Orion Context Broker をデプロイする方法

[Docker Swarm](https://docs.docker.com/engine/swarm/) は公式の Docker クラスタ管理ソリューションであり、Docker Engine とネイティブに統合されています。

この[リンク](https://smartsdk.github.io/smartsdk-recipes/data-management/context-broker/ha/readme/)に、Orion Context Broker を Docker Swarm 上で実行する際の詳細な説明と考慮事項があります。

ここでは簡単な概要のみを示します。

Orion HA の展開については、[ここ](../doc/manuals.jp/admin/extra/ha.md) で一般的な説明を参照してください。

##  Docker Swarm テスト・クラスタのインストール

Linux ベースのシステムに Docker Swarm テスト・クラスタをデプロイするには、[miniswarm](https://github.com/aelsabbahy/miniswarm) を使用できます 。

唯一の前提条件は次のとおりです :
* [Docker Machine](https://docs.docker.com/machine/install-machine/)
* [Virtual Box](http://virtualbox.org/)

### miniswarm をインストール

```bash
# As root
$ curl -sSL https://raw.githubusercontent.com/aelsabbahy/miniswarm/master/miniswarm -o /usr/local/bin/miniswarm
$ chmod +rx /usr/local/bin/miniswarm
```

### クラスタを起動

```bash
# 1 manager 2 workers
$ miniswarm start 3
```

上記のコマンドは、1つのマスタ・ノードと 2つのワーカー・ノードを含む、3つのノードの Docker Swarm クラスタを作成します。

### クラスタに接続

```bash
$ eval $(docker-machine env ms-manager0)
```

Docker クライアントは、今、作成したクラスタのマネージャ・ノードに接続します。

実行して確認することができます

```bash
$ docker machine ls

NAME          ACTIVE   DRIVER       STATE     URL                          SWARM   DOCKER        ERRORS
ms-manager0   *        virtualbox   Running   tcp://192.168.99.101:2376            v18.02.0-ce
ms-worker0    -        virtualbox   Running   tcp://192.168.99.102:2376            v18.02.0-ce
ms-worker1    -        virtualbox   Running   tcp://192.168.99.100:2376            v18.02.0-ce
```

`*` のノードは、Docker クライアントが接続するノードです。

## Orion Context Broker を HA にデプロイ

HAに、Orion を導入するには、最初に MongoDB ReplicaSet を配備し、Orion Context Broker の上にデプロイする必要があります。

### MongoDB ReplicaSet クラスタをデプロイ

MongoDB ReplicaSet を Docker Swarm にデプロイする方法の詳細については、[こちら](https://github.com/smartsdk/mongo-rs-controller-swarm)を参照してください。

1. オーバーレイ・ネットワークを作成

    ```bash
    $ docker network create --opt encrypted -d overlay backend
    ncb90nkwpiofoof757te09xmt
    ```

2. docker-compose-mongo.yml ファイルを作成します。または、ヘルス・チェックなどの追加機能を提供する[リポジトリ](https://github.com/smartsdk/mongo-rs-controller-swarm)内のスクリプトを再利用します :

    ```yaml
    version: '3.3'

    services:

      mongo:
        image: mongo:3.6
        entrypoint: [ "/usr/bin/mongod", "--replSet", "rs", "--journal", "--smallfiles", "--bind_ip", "0.0.0.0"]
        volumes:
          - mongodata:/data/db
        networks:
          - backend
        deploy:
          mode: global
          restart_policy:
            condition: on-failure
          update_config:
            # should you update the mongo cluster this configuration ensure
            # that nodes are updated one by one ensuring that the mongo service
            # remains available to other services. the delay ensure that
            # when the new mongo is deployed it has enough time to be connected
            # to the cluster
            parallelism: 1
            delay: 1m30s

      # this service contains the logic to manage the mongo db
      # replicaset consistency
      controller:
        image: martel/mongo-replica-ctrl:latest
        volumes:
          - /var/run/docker.sock:/var/run/docker.sock
        environment:
          - OVERLAY_NETWORK_NAME=backend
          - MONGO_SERVICE_NAME=mongo_mongo
          - REPLICASET_NAME=rs
          - MONGO_PORT=27017
              # - DEBUG=1 #uncomment to debug the script
        entrypoint: python /src/replica_ctrl.py
        networks:
          - backend
        depends_on:
          - "mongo"
        deploy:
          mode: replicated
          replicas: 1
          placement:
            constraints: [node.role==manager]
          restart_policy:
            condition: on-failure

    volumes:
      # External true ensures that the volume is not re-created if already present
      mongodata:
        external: true

    networks:
      backend:
        external:
          name: backend
    ```

3. MongoDB ReplicaSet をデプロイ

    ```bash
    $ docker stack deploy -c docker-compose-mongo.yml mongo
    Creating service mongo_mongo
    Creating service mongo_controller
    ```

4. デプロイメントが正しく機能していることを確認します。サービスのデプロイが完了するまで待つ必要があります :

    ```bash
    $ docker service logs -f mongo_controller
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Waiting mongo service (and tasks) (mongo_mongo) to start
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Mongo service is up and running
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:No previous valid configuration, starting replicaset from scratch
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:replSetInitiate: {'ok': 1.0}
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    ```

### Orion Context Broker クラスタをデプロイ

1. docker-compose-orion.yml ファイルを作成します。または 、ヘルスチェックなどの追加機能を提供するスクリプトを[リポジトリ](https://github.com/smartsdk/smartsdk-recipes/tree/master/recipes/data-management/context-broker/ha)で再利用し、フロントエンドとバックエンドに別々のネットワークを使用します :

    ```yaml
    version: '3'

    services:

      orion:
        image: fiware/orion:latest
        ports:
          - "1026:1026"
        command: -logLevel DEBUG -dbhost mongo_mongo -rplSet rs -dbTimeout 10000
        deploy:
          replicas: 2
        networks:
          - backend

    networks:
      backend:
        driver: overlay
        external: true
    ```

2. Orion Context Broker クラスタをデプロイ

    ```bash
    $ docker stack deploy -c docker-compose-orion.yml orion
    Creating service orion_orion
    ```

3. デプロイメントが正しく機能していることを確認します。サービスのデプロイが完了するまで待つ必要があります。

    ```bash
    $ curl $(docker-machine ip ms-manager0):1026/version -s -S

    {
    "orion" : {
      "version" : "2.4.0-next",
      "uptime" : "0 d, 0 h, 0 m, 27 s",
      "git_hash" : "f2a3d436b2b507c5fd1611492ad7fad386901952",
      "compile_time" : "Thu Oct 29 15:56:28 UTC 2020",
      "compiled_by" : "root",
      "compiled_in" : "4d72f1940cd1",
      "release_date" : "Thu Oct 29 15:56:28 UTC 2020",
      "doc" : "https://fiware-orion.rtfd.io/",
      "libversions": ...
    }
    }
    ```
4. Orion をスケールアップ :

    ```bash
    $ docker service scale orion_orion=3

    orion_orion scaled to 3
    overall progress: 2 out of 3 tasks
    1/3: running   [==================================================>]
    2/3: preparing [=================================>                 ]
    3/3: running   [==================================================>]
    ```
