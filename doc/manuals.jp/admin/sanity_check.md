# <a name="top"></a> 健全性チェックの手順

* [エンド・ツー・エンドのテスト](#end-to-end-testing)
* [実行中プロセスのリスト](#list-of-running-processes)
* [ネットワーク・インターフェイスの起動とオープン](#network-interfaces-up-and-open)
* [データベース](#databases)

健全性チェック手順は、システム管理者がインストールのテスト準備が整ったことを確認するための手順です。したがって、単体テスト、統合テスト、ユーザ検証に進む前に、明白なまたは基本的な誤動作が修正されていることを確認するための予備テストセットです。

<a name="end-to-end-testing"></a>
## エンド・ツー・エンドのテスト

-   context broker をデフォルト・ポート (1026) で起動します
-   次のコマンドを実行します

```
curl --header 'Accept: application/json' localhost:1026/version
```

-   バージョン番号が、稼働時間情報およびコンパイル環境情報とともに出力されていることを確認します :

```
{
  "orion" : {
    "version" : "2.6.0-next",
    "uptime" : "0 d, 0 h, 2 m, 30 s",
    "git_hash" : "f2a3d436b2b507c5fd1611492ad7fad386901952",
    "compile_time" : "Thu Oct 29 16:56:16 CEST 2020",
    "compiled_by" : "fermin",
    "compiled_in" : "debvm",
    "release_date" : "Thu Oct 29 16:56:16 CEST 2020",
    "machine" : "x86_64",
    "doc" : "https://fiware-orion.rtfd.io/",
    "libversions": ...
  }
}
```

[トップ](#top)

<a name="list-of-running-processes"></a>
## 実行中プロセスのリスト

"contextBroker" という名前のプロセスが起動され、実行されている必要があります。たとえば以下のとおりです :

```
$ ps ax | grep contextBroker
 8517 ?        Ssl    8:58 /usr/bin/contextBroker -port 1026 -logDir /var/log/contextBroker -pidpath /var/log/contextBroker/contextBroker.pid -dbhost localhost -db orion
```

[トップ](#top)

<a name="network-interfaces-up-and-open"></a>
## ネットワークインターフェイスの起動とオープン

Orion Context Broker は、デフォルト・ポートとして TCP 1026を使用しますが、`-port` コマンドライン・オプションを使用して変更できます。

[トップ](#top)

<a name="databases"></a>
## データベース

Orion Context Broker は MongoDB データベースを使用します。このパラメーターは、
コマンドライン・オプションを使用して提供されます :

* `-dbhost`
* `-db`
* `-dbuser`
* `-dbpwd`
* `-dbAuthMech`
* `-dbAuthDb`
* `-dbSSL`
* `-dbDisableRetryWrites`
* `-dbTimeout`
* `-dbPoolSize`
* `-writeConcern`

`-dbuser`, `-dbpwd`, `-dbAuthMech` および `-dbAuthDb` は、MongoDB が認証を使用
して、つまり `--auth` で実行される場合にのみ使用されることを注意して ください。

mongo コンソールを使用してデータベースが動作していることを確認できます :

```
mongo <dbhost>/<db>
```

mongo コンソールの次のコマンドを使用して、broker が使用するさまざまなコレクションをチェックすることができます。ただし、最初にドキュメントを挿入するときに broker がコレクションを作成するので、broker を初めて実行する、またはデータベースがクリーンアップされている、broker がまだリクエストを受信していない場合、コレクションは存在しません。特定の瞬間に実際のコレクションリストを取得するために `show collections` を使用します。

```
> db.registrations.count()
> db.entities.count()
> db.csubs.count()
```

[トップ](#top)
