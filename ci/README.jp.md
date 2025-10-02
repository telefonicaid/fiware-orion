## 概要
このリポジトリでは GitHub Actions が有効になっているため、マージを許可する前に各プルリクエストがチェックされます。
このシステムは、新しい PR がマスターに到達するたびにマスターブランチから構築される `telefonicaiot/fiware-orion:ci` に基づいており、
すべてのビルド依存関係がオンボードにあるクリーンな環境を提供します。この Docker のビルドに使用される Dockerfile は、
`ci/deb` ディレクトリにあります。

テスト対象の PR ブランチが変更されたため、`telefonicaiot/fiware-orion:ci` は再構築されないことに注意してください。したがって、
新しいライブラリまたはベースシステムを必要とする機能を開発している場合は、そのようなライブラリまたはベースシステムを
`ci/deb/build-dep.sh` および/または `Dockerfile` に追加する PR を*最初に*実行する必要があります。その PR がマスターに
マージされ、`telefonicaiot/fiware-orion:ci` が再構築されると (Docker Hub の https://hub.docker.com/r/telefonicaiot/fiware-orion/builds
で進行状況を確認)、新しい機能は、GitHub アクションでテストする準備ができています。

GitHub Actions チェックは段階に分かれており、 "サポートされているテスト" セクションで説明されています。

## サポートされているテスト
CI の現在のバージョンは以下をサポートします:

* ファイル・コンプライアンス・チェック
* ペイロード・チェック
* スタイル・チェック
* 単体テスト
* 機能テスト

ファイル・コンプライアンス、ペイロード、およびスタイル・チェックは、1つの 'compliance' テストにまとめられています。

# すべてのテストをローカルで実行する方法

機能テストは次のように実行できます:

```
docker compose -f ci/deb/docker-compose-ci.yml -f ci/deb/docker-compose-ci.functional.yml --project-directory . up
```

これにより、`docker-compose-ci.yml` を使用して必要なすべてのサービス (MongoDB、MQTT ブローカー、Kafka ブローカーなど) が設定され、`docker-compose-ci.functional.yml` (そのために `telefonicaiot/fiware-orion:ci` を使用) を使用してテストが実行されます。

あるいは、次のコマンドで valgrind テストを実行することもできます:

```
docker compose -f ci/deb/docker-compose-ci.yml -f ci/deb/docker-compose-ci.valgrind.yml --project-directory . up
```

イメージを使った別の方法については、次のセクションをお読みください。

# イメージをローカルで使用する方法

場合によっては、CI イメージをローカルで実行する必要があります (たとえば、GitHub アクション・ジョブで見つかった問題を
デバッグするためです)。その場合、次のチート・シートが役立ちます:

まず、ターミナルで必要なサービスを実行する必要があります（事前にローカル・インスタンスを停止してください。そうしないと競合が発生します）。

```
cd /path/to/fiware-orion
docker compose -f ci/deb/docker-compose-ci.yml --project-directory . up
```

テストを終了するときは、ターミナルで Ctrl+C を押すことでサービスを停止できます。

次に、イメージをダウンロードします（または、[ローカルでビルド](#how-to-build-the-image-locally)）。

```
docker pull telefonicaiot/fiware-orion:ci
```

たとえば、GitHub Actions と同じ方法でイメージを実行するには、次のようにします:

```
cd /path/to/fiware-orion
docker run --network host --rm -e CB_NO_CACHE=ON -e FT_FROM_IX=1001 -v $(pwd):/opt/fiware-orion telefonicaiot/fiware-orion:ci build -miqts functional
```

インタラクティブな bash を使用してイメージを実行するには:

```
cd /path/to/fiware-orion
docker run --network host -ti -v $(pwd):/opt/fiware-orion telefonicaiot/fiware-orion:ci bash
```

bash シェルを起動したら、同様の実行を行うことができます:

```
root@debian11:/opt# CB_NO_CACHE=ON FT_FROM_IX=1001 build -miqts functional
```

または、`testHarness.sh` スクリプトを直接実行することもできます (たとえば、単一のテストを実行するため):

```
root@debian11:/opt# . /opt/ft_env/bin/activate
(ft_env) root@debian11:/opt# cd /opt/fiware-orion/test/functionalTest/
(ft_env) root@debian11:/opt/fiware-orion/test/functionalTest# ./testHarness.sh cases/3541_subscription_max_fails_limit/mqtt_subscription_without_maxfailslimit_and_failscounter.test
```

**注:** 上記の手順により、Orion は root ユーザを使用してコンパイルされます。デバッグ・セッションの終了後に
再帰的な所有者の変更を行うことをお勧めします。このようなものです：

```
sudo chown -R fermin:fermin /path/to/fiware-orion
```

**注2:** `build` スクリプトは `makefile` ファイルに変更を加えます。デバッグ・セッションの後に `git checkout makefile`
を実行して変更を取り消します。

# ローカルでイメージを構築する方法

場合によっては、Dockerhub からプルするのではなく、ローカルでイメージをビルドしたいことがあります。以下のコマンドが役立ちます。

まず、Dockerhub からダウンロードした既存の `telefonicaiot/fiware-orion:ci` イメージを削除することをお勧めします:

```
docker rmi <id of the image>
```

次に、コンテナをビルドします:

```
cd /path/to/fiware-orion/ci/deb
docker build -t telefonicaiot/fiware-orion:ci .
```
