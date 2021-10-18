# <a name="top"></a> ログ

* [ログ・ファイル](#log-file)
* [ログ・フォーマット](#log-format)
* [INFO レベルの詳細](#info-level-in-detail)
* [アラーム](#alarms)
* [サマリ・トレース](#summary-traces)
* [ログ・ローテーション](#log-rotation)
* [通知トランザクションのログの例](#log-examples-for-notification-transactions)
* [ログに関連するコマンドライン・オプション](#command-line-options-related-with-logs)

<a name="log-file"></a>
## ログ・ファイル

デフォルトのログ・ファイルは `/tmp/contextBroker.log` です。ログ・ファイルが保存されているディレクトリ (`/tmp` デフォルト) は、`-logDir` コマンドライン・オプションを使用して変更できます。

Orion コンテキスト broker を起動するときに、前のログ・ファイルが存在する場合 :

-   **`-logAppend`** が使用されている場合、ログは既存のファイルに追加されます。
-   **`-logAppend`** が使用されていない場合は、既存のファイルの名前が変更され、その名前に ".old" というテキストが追加されます。

この `-logLevel` オプションでは、ログに出力されるエラーメッセージを選択できます :

- NONE : 全くログがありません
- FATAL : FATAL ERROR メッセージのみがログされます
- ERROR : ERROR メッセージのみがログされます
- WARN (デフォルト) : WARN メッセージと ERROR メッセージがログされます
- INFO : INFO, WARN および ERROR メッセージがログされます
- DEBUG : DEBUG, INFO, WARN および ERROR メッセージがログされます

Orion がフォアグラウンドで実行されると (つまり、`-fg` [CLI 引数](cli.md)を使用して)、標準出力にも同じログトレースが出力されます。

Orion によって公開される [admin API](management_api.md) を使用して、実行時にログレベルを変更 (および取得) することができます。

[トップ](#top)

<a name="log-format"></a>
## ログ・フォーマット

ログ・フォーマットは、[Splunk](http://www.splunk.com/) や [Fluentd](http://www.fluentd.org/) などのツールで処理されるように設計されています 。

ログ・ファイルの各行は、パイプ文字 (`|`) で区切られた複数のキー値フィールドで構成されています。例 :

    time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[986]:main | msg=start command line <contextBroker -fg -logLevel INFO>
    time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[873]:logEnvVars | msg=env var ORION_PORT (-port): 1026
    time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1054]:main | msg=Orion Context Broker is running
    time=2020-10-26T09:45:17.301Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=MongoGlobal.cpp[247]:mongoInit | msg=Connected to mongo at localhost/orion (poolsize: 10)
    time=2020-10-26T09:45:17.304Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1180]:main | msg=Startup completed
    ...
    time=2020-10-26T10:27:02.619Z | lvl=INFO | corr=c99e4592-1775-11eb-ad30-000c29df7908 | trans=1603707992-318-00000000001 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[79]:logInfoRequestWithoutPayload | msg=Request received: GET /v2/entities?type=Device, response code: 200
    ...
    time=2020-10-26T10:32:41.724Z | lvl=INFO | corr=93bdc5b4-1776-11eb-954d-000c29df7908 | trans=1603708355-537-00000000002 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities, request payload (34 bytes): {  "id": "Room1",  "type": "Room"}, response code: 201
    ...
    time=2020-10-26T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[968]:sigHandler | msg=Signal Handler (caught signal 2)
    time=2020-10-26T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[974]:sigHandler | msg=Orion context broker exiting due to receiving a signal

各行のさまざまなフィールドは次のとおりです :

-   **time** : ログラインが [ISO8601](https://es.wikipedia.org/wiki/ISO_8601) フォーマットで生成された瞬間に対応するタイムスタンプです。Orion はタイムスタンプを UTC 形式で出力します。
-   **lvl (レベル)** : 6つのレベルがあります :
    -   FATAL : このレベルは、アプリケーションを終了させる重大なエラーイベントを示します。プロセスはもはや機能しません。
    -   ERROR : このレベルはエラーイベントを示します。解決しなければならない重大な問題があります
    -   WARN : このレベルは潜在的に有害な状況を示します。解決すべき軽微な問題があります
    -   INFO : このレベルは、Orion の進捗をハイライトする情報メッセージを示します。このレベルの詳細については、[このセクション](#info-level-in-detail)を参照してください
    -   DEBUG : このレベルは、アプリケーションをデバッグするのに最も役立つ細かい情報イベントを示します。トレース・レベルが使用されているときにのみ表示されます (`-t` コマンドライン・オプションで設定されます)
    -   SUMMARY : これはログ・サマリ・トレースで使用される特別なレベルで、`-logSummaryCLI` オプションで有効になっています。詳細については、[サマリ・トレースのセクション](#summary-traces)を見てください
-   **corr (correlator id).** : "N/A" (ログメッセージ "out of transaction"、たとえば Orion Context Broker の起動に対応するログ行)、または UUID 形式の文字列です。例 : "550e8400-e29b-41d4-a716-446655440000"。この UUID 文字列には、サフィックスが含まれる場合があります。たとえば、`550e8400-e29b-41d4-a716-446655440000; cbnotif=2` または `550e8400-e29b-41d4-a716-446655440000; cbfwd=1` (これについての詳細は[INFO レベルに関するセクション](#info-level-in-detail)にあります)。この 'correlator id' は着信要求から転送されるか、または着信要求に HTTP ヘッダ "Fiware-Correlator" が含まれていない場合、corr は Orion Context broker によって生成され、ログ・ファイルで使用されます (転送メッセージ、通知およびレスポンスにに HTTP ヘッダーとして送信されるのと同様に)。correlator id は、特定の1つの要求に対して 'message chain' に関係するすべてのアプリケーションの共通識別子です
-   **trans (transaction id)** : "N/A" (Orion Context Broker の起動時に対応するログメッセージ "out of transaction") またはフォーマットの文字列 "1405598120-337-00000000001" になります。トランザクション id 生成ロジックは、すべてのトランザクション ID が一意であることを保証します。別 VM で実行されている Orion インスタンス (同じソースから別々のログを集約する場合に便利です) についても、それらが正確に同じミリ秒で開始されている場合は例外です。トランザクション ID は correlator ID とは無関係であることに注意してください。トランザクション ID はローカルな性質を持ち、correlator ID は意味のあるエンド・ツー・エンドであり、Context Broker 自体とは別のソフトウェアコンポーネントを含んでいます。Orion には2種類のトランザクションがあります :
    -   Orion によって公開された REST API を呼び出す外部クライアントによって開始されたものです。これらのトランザクションの最初のメッセージは、*url* に操作を呼び出すクライアントの IP とポートが含まれ、パスが Orion で呼び出された実際の操作である "Starting transaction **from** *url*" パターンを使用します。これらのトランザクションの最後のメッセージは "Transaction ended" です
    -   Orion が通知を送信するときに開始するものです。これらのトランザクションの最初のメッセージは、"Starting transaction **to** *url*" というパターンを使用します。*url* は、サブスクリプションの参照要素で使用される URL です。つまり、通知を送信するコールバックの URL です。両方の取引型の最後のメッセージは "Transaction ended" です
-   **from** : トランザクションに関連付けられた HTTP 要求のソース IP です。リクエストに `X-Forwarded-For` ヘッダ (前者を上書きする) または `X-Real-IP` (上書き `X-Forwarded-For` と source IP を上書き) が含まれている場合を除きます
-   **srv** : トランザクションに関連付けられているサービスです。リクエストに
    サービスが含まれていない場合 (つまり、`fiware-service`
    ヘッダが欠落している場合)、`<none>` が使用されます
-   **subsrv** :トランザクションに関連付けられているサブサービスです。
    リクエストにサブ・サービスが含まれていなかった場合 (つまり、
    `fiware-servicepath` ヘッダが欠落していた場合)、`<none>` が使用されます
-   **comp (component)** : 現在のバージョンでは常にこのフィールドに "Orion" が使用されます
-   **op** : ログメッセージを生成したソースコード内の関数です。この情報は、開発者だけに役立ちます。
-   **msg (message)** : 実際のログメッセージです。メッセージのテキストには、ファイルの名前とトレースを生成する行番号が含まれます (この情報は、主に Orion 開発者にとって役に立ちます)

[トップ](#top)  

<a name="info-level-in-detail"></a>

## INFO レベルの詳細

**注：** INFO レベルは Orion バージョン 2.5.0 で再設計され、シンプルにするために一部の
トレースが削除されています。これらの古い INFO トレースを表示する場合は、DEBUG モードで
トレース・レベル 240 を使用します (例： `-logLevel DEBUG -t 240`)

起動時に、INFO レベルには、Orion がどのように開始されたか (CLI と環境変数の両方)
に関するトレースと、MongoDB データベースへの接続に関する情報が表示されます:

```
time=2021-03-09T16:14:36.055Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1000]:main | msg=start command line <contextBroker -fg -logLevel INFO>
time=2021-03-09T16:14:36.055Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1068]:main | msg=Orion Context Broker is running
time=2021-03-09T16:14:36.083Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=mongoConnectionPool.cpp[488]:mongoConnectionPoolInit | msg=Connected to mongodb://localhost/?connectTimeoutMS=10000 (dbName: orion, poolsize: 10)
time=2021-03-09T16:14:36.086Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1194]:main | msg=Startup completed
```

実行時に、INFO レベルには、クライアント・リクエスト、ノーティフィケーション、およびフォワードされた
リクエストに関する関連情報が表示されます。 特に:

* *ペイロードなし*のクライアント・リクエストごとに、リクエストの処理が終了すると、Orion はこのような
  トレースを表示します:

```
time=2020-10-26T10:27:02.619Z | lvl=INFO | corr=c99e4592-1775-11eb-ad30-000c29df7908 | trans=1603707992-318-00000000001 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[79]:logInfoRequestWithoutPayload | msg=Request received: GET /v2/entities?type=Device, response code: 200
```

* *ペイロード付き*のクライアント・リクエストごとに、リクエストの処理が終了すると、Orion はこのような
  トレースを表示します:

```
time=2020-10-26T10:32:41.724Z | lvl=INFO | corr=93bdc5b4-1776-11eb-954d-000c29df7908 | trans=1603708355-537-00000000002 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities, request payload (34 bytes): {  "id": "Room1",  "type": "Room"}, response code: 201
```

* 送信されたノーティフィケーションごとに、Orion はこのようなトレースを表示します。Orion は、クライアント・
  リクエストに関連付けられた "root correlator" に `cbnotif=` サフィックスを追加することに注意してください
  (この例では、クライアント・リクエストは `corr=87f708a8-1776-11eb-b327-000c29df7908` を使用します)。
  このサフィックスの値は自動インクリメント・カウンター (最初のノーティフィケーションでは1から始まります)
  であるため、同じ更新によってトリガーされるすべてのノーティフィケーションは、correlator に対して厳密に
  異なる値を持ちます

```
time=2020-10-26T10:32:22.145Z | lvl=INFO | corr=87f708a8-1776-11eb-b327-000c29df7908; cbnotif=1 | trans=1603707992-318-00000000003 | from=0.0.0.0 | srv=s1| subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f914177334436ea590f6edb): POST localhost:1028/accumulate, response code: 200
```

* MQTT 通知の場合、上記のトレースの `msg` フィールドはわずかに異なります:

```
time=2020-10-26T10:32:22.145Z | lvl=INFO | corr=87f708a8-1776-11eb-b327-000c29df7908; cbnotif=1 | trans=1603707992-318-00000000003 | from=0.0.0.0 | srv=s1| subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=MQTT Notif delivered (subId: 60ffea6c1bca454f9a64c96c): broker: localhost:1883, topic: sub2
```

* [コンテキスト・プロバイダ](../user/context_providers.md) (クエリまたは更新) にフォワードされたリクエスト
  ごとに、Orion は次のようなトレースを表示します。Orion は、クライアント・リクエストに関連付けられた
  "root correlator" に `cbfwd=` サフィックスを追加することに注意してください (この例では、クライアント・
  リクエストは `corr=eabce3e2-149f-11eb-a2e8-000c29df7908` を使用します)。このサフィックスの値は
  自動インクリメント・カウンター (最初の転送されたリクエストの場合は1から始まります）であるため、同じ更新に
  よってトリガーされたすべてのフォワードされたリクエストは、correlator に対して厳密に異なる値を持ちます

```
time=2020-10-22T19:51:03.565Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=1 | trans=1603396258-520-00000000007 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da0697f): POST http://localhost:9801/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A1"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A1":{"type":"Text","value":"A1 in CP1","metadata":{}}}], response code: 200
```

いくつかの追加の考慮事項:

* `-logInfoPayloadMaxSize` 設定は、上記のトレースのペイロードが持つ可能性のある最大サイズを指定するために
  使用されます。ペイロードがこの制限を超えると、最初の `-logInfoPayloadMaxSize` バイトのみが出力されます
  (`(...)` の形式の省略記号がトレースに表示されます)。デフォルト値:5キロバイト
* ノーティフィケーションおよびフォワーディング・トレースのレスポンス・コードは、番号
  (ノーティフィケーションまたはフォワードされたリクエストの HTTP レスポンス・コードに対応)
  または接続の問題が発生した場合の文字列のいずれかになります。 例えば:

```
time=2020-10-26T10:32:22.145Z | lvl=INFO | corr=87f708a8-1776-11eb-b327-000c29df7908; cbnotif=1 | trans=1603707992-318-00000000003 | from=0.0.0.0 | srv=s1| subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f914177334436ea590f6edb): POST localhost:1028/accumulate, response code: Couldn't connect to server
```

* クライアント・リクエストがコンテキスト・プロバイダへのフォワーディングをトリガーすると、最初に転送された
  リクエストを開始する前に、 `Starting forwarding for <client request URL>` トレースが出力されます。
  したがって、完全なフォワーディング・ブロック (たとえば、エンティティの個々の属性について5つの
  コンテキスト・プロバイダにクエリを実行するクライアント・クエリ) は次のようになります。それらはすべて、
  フォワードされたリクエストに関連付けられたログ・トレースで、同じ root correlator と `cbfwd=1` から
  `cbfwd=5` のサフィックスを使用することに注意してください

```
time=2020-10-22T19:51:03.556Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908 | trans=1603396258-520-00000000006 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[146]:logInfoFwdStart | msg=Starting forwarding for GET /v2/entities/E1?type=T1
time=2020-10-22T19:51:03.565Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=1 | trans=1603396258-520-00000000007 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da0697f): POST http://localhost:9801/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A1"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A1":{"type":"Text","value":"A1 in CP1","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.573Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=2 | trans=1603396258-520-00000000008 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06980): POST http://localhost:9802/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A2"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A2":{"type":"Text","value":"A2 in CP2","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.584Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=3 | trans=1603396258-520-00000000009 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06981): POST http://localhost:9803/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A3"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A3":{"type":"Text","value":"A3 in CP3","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.593Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=4 | trans=1603396258-520-00000000010 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06982): POST http://localhost:9804/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A4"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A4":{"type":"Text","value":"A4 in CP4","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.601Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=5 | trans=1603396258-520-00000000011 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06983): POST http://localhost:9805/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A5"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A5":{"type":"Text","value":"A5 in CP5","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.602Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908 | trans=1603396258-520-00000000006 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[79]:logInfoRequestWithoutPayload | msg=Request received: GET /v2/entities/E1?type=T1, response code: 200
```

[トップ](#top)  

<a name="alarms"></a>
## アラーム

アラーム条件 :

| アラーム ID   | 重大度   |   検出戦略                                                                                              | 停止条件                                                                                                                                                                                                                            | 説明                                                                                                   | アクション
|:---------- |:----------:|:----------------------------------------------------------------------------------------------------------------- |:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |:------------------------------------------------------------------------------------------------------------- |:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 1          | CRITICAL   | FATAL トレースを検出しました                                                                                   | N/A                                                                                                                                                                                                                                       | Orion Context Broker の起動時に問題が発生しました。FATAL 'msg' フィールドには、特定の問題が詳しく説明されています。 | Orion Context Broker の起動を妨げる問題を解決します。たとえば、リスニングポートが原因で問題が発生した場合、解決方法は Orion リスニングポートを変更するか、すでにポートを使用しているプロセスを終了します
| 2          | CRITICAL   | 'msg' フィールドには、次の ERROR テキストが表示されます : "Runtime Error(`<detail>`)"                                 | N/A                                                                                                                                                                                                                                       | ランタイム・エラー。`<detail>` テキストは、詳細情報を含んでいます                                        | Orion Context Broker を再起動します。それが持続する場合 (例えば、新しいランタイムエラーが次の1時間以内に現れる場合)、問題を開発チームにエスカレーションしてください
| 3          | CRITICAL   | 'msg' フィールドには、次の ERROR テキストが、表示されます : "Raising alarm DatabaseError:`<detail>`"                    | 'msg' フィールドには、次の ERROR テキストが表示されます : "Releasing alarm DatabaseError"。Orion は、DB が再び正常であることを検出すると、このトレースを出力します                                                                                       | データベースエラー。 `<detail>` テキストは詳細な情報を含んでいます                                        | Orion が MongoDB データベースにアクセスできません、かつ/または MongoDB データベースが正常に動作していません。データベース接続とデータベースの状態を確認してください。データベースの修復や Orion への接続が完了したら、問題は解消されます (Orion サービスの再起動は必要ありません)。 Orion Context Broker サービスで特定のアクションを実行する必要はありません
| 4          | WARNING    | 次の WARN テキストが 'msg' フィールドに表示されます : "Raising alarm BadInput `<ip>`: `<detail>`".                   | 次の WARN テキストが 'msg' フィールドに表示されます : "Releasing alarm BadInput `<ip>`"。どこがアラームを引き起こしたのか。Orion は、そのクライアントから正しい要求を受け取ったときにこのトレースを出力します                 | 不正な入力。`<detail>` テキストに詳細情報が含まれています                                             | クライアントが API 仕様に準拠していないリクエストを Orion に送信しました。不正な URL、不正なペイロード、リクエストの構文/意味エラーなどが含まれます。IP に応じて、プラットフォームクライアントまたは外部のサードパーティのクライアントに対応できます。 いずれにしても、問題を把握し修正するためにクライアントの所有者に報告する必要があります。Orion Context Broker サービスで特定のアクションを実行する必要はありません
| 5          | WARNING    | 次の WARN テキストが 'msg' フィールドに表示されます : "Raising alarm NotificationError  `<url>`:  `<detail>`"        | 次の WARN テキストが 'msg' フィールドに表示されます : "Releasing alarm NotificationError "。どこがアラームを引き起こしたのか。Orion は、この URL に通知を送信すると、このトレースを出力します        | 通知の失敗。 `<detail>` テキストに詳細情報が含まれています                                   | Orion は通知をレシーバに送信しようとしていますが、問題が発生しています。これは、ネットワーク接続性またはレシーバの問題、例えばレシーバがダウンしています。第2のケースでは、通知のレシーバの所有者が報告されるべきです。Orion Context Broker サービスで特定のアクションを実行する必要はありません

デフォルトでは、Orion はアラームの起源 (すなわち上げる) と終わり(すなわち解放) をトレースするだけです :


```
time=... | lvl=ERROR | ... Raising alarm DatabaseError: collection: orion.entities - query(): { ... } - exception: ....
time=... | lvl=ERROR | ... Releasing alarm DatabaseError
...
time=... | lvl=WARN  | ... Raising alarm BadInput 10.0.0.1: JSON Parse Error: <unspecified file>(1): expected object or array
time=... | lvl=WARN  | ... Releasing alarm BadInput 10.0.0.1
...

time=... | lvl=WARN  | ... Raising alarm NotificationError localhost:1028/accumulate: (curl_easy_perform failed: Couldn't connect to server)
time=... | lvl=WARN  | ... Releasing alarm NotificationError localhost:1028/accumulate
```

これは、アラームメッセージを発生させてから解除するまでにアラームをトリガした条件。たとえば、10.0.0.1 クライアントからの新しい無効な要求)が再び発生した場合、再度トレースされないことを意味します。ただし、この動作は `-relogAlarms CLI` パラメータを使用して変更できます。`-relogAlarms` を使用する と、トリガー条件が発生するたびにログトレースが出力されます。たとえば :

```
time=... | lvl=WARN | ... Raising alarm BadInput 10.0.0.1: JSON parse error
time=... | lvl=WARN | ... Repeated BadInput 10.0.0.1: JSON parse error
time=... | lvl=WARN | ... Repeated BadInput 10.0.0.1: JSON parse error
time=... | lvl=WARN | ... Repeated BadInput 10.0.0.1: service '/v2/entitiesxx' not found
time=... | lvl=WARN | ... Releasing alarm BadInput 0.0.0.0
```

"Raising" と "Releasing" メッセージの間のログトレースは、
メッセージテキストに "Repeated" を使用します。メッセージの詳細部分はすべてのトレースで必ずしも同じではないので、アラームを再度ログ (re-logging) することは問題をデバッグするときに追加情報を取得する手段になる可能性があることに注意してください。上記の例では、JSON ペイロードの問題を修正した後、Orion API 操作の URL に新しい問題が発生したクライアントに対応する可能性があります。


[トップ](#top)

<a name="summary-traces"></a>
## サマリ・トレース

`-logSummary` [CLI パラメータ](cli.md)を使用してログ・サマリ・トレースを有効にすることができます。この値は、サマリ・レポートの期間 (秒単位) です。例えば、`-logSummary 5` は、サマリ・トレースは5秒ごとに出力されます (`-logLevel' がどのログレベルに設定されていても)。

次のように、4つのトレースが毎回出力されます (わかりやすくするために、いくつかのフィールドを除いて、行を省略しています) :

```
time=... | lvl=SUMMARY | ... Transactions: 2345 (new: 45)
time=... | lvl=SUMMARY | ... DB status: ok, raised: (total: 0, new: 0), released: (total: 0, new: 0)
time=... | lvl=SUMMARY | ... Notification failure active alarms: 0, raised: (total: 0, new: 0), released: (total: 0, new: 0)
time=... | lvl=SUMMARY | ... Bad input active alarms: 5, raised: (total: 12, new: 1), released: (total: 7, new: 2)
```

* 最初の行 (トランザクション) には、最新のサマリ・レポート期間における現在のトランザクション数と新しいトランザクション数が表示されます
* 2行目は [DB アラーム](#alarms)に関するものです。現在の DB ステータス("OK" または "erroneous")、発生した DB アラームの数 (Orion が開始してからの合計と最後のサマリ・レポート期間の両方)、および解放された DB アラームの数 (Orion が開始してからの合計と最後のサマリ・レポート期間の両方)
* 3行目は [通知失敗のアラーム](#alarms)です
これは、アクティブ・通知失敗アラームの現在の数、通知された通知失敗アラームの数 (Orion が開始してからの合計と最後のサマリ・レポート期間の両方)、およびリリースされた通知失敗アラームの数です (Orion が開始してからの合計と最後のサマリ・レポート期間の両方)
* 4行目は、[不正入力アラーム](#alarms)です。現在の不正な入力アラーム数、不正な入力アラームの発生数 (Orion が開始してからの合計と最後の要約レポート期間の両方)、およびリリースされた不良入力アラームの数です (Orion が開始してからの合計と最後の要約レポート期間の両方)

[トップ](#top)

<a name="log-rotation"></a>
## ログ・ローテーション

Logrotate は、contextBroker とともに RPM としてインストールされます。システムは、ログ・ファイルのサイズが 100MB を超える場合 (デフォルトでは30分毎にチェックされています)、1日1回以上回転するように設定されています :

-   毎日のローテーションの場合 : `/etc/logrotate.d/logrotate-contextBroker-daily` : 毎日のログローテーションを有効にする
-   サイズベースのローテーションの場合 :
    -   `/etc/sysconfig/logrotate-contextBroker-size` : 前のローテーションに加えて、このファイルは、ログ・ファイルが特定のしきい値 (デフォルトでは100 MB) を超えた場合にログのローテーションを保証します
    -   `/etc/cron.d/cron-logrotate-contextBroker-size` : 通常の周期 (デフォルトは30分です) で /etc/sysconfig/logrotate-contextBroker-size の実行を保証します

予想される負荷に応じて、デフォルト設定を調整する必要があります。この意味で、INFO ログレベルでは、すべてのトランザクションが約 1-2 KB (Orion 0.14.1 で測定) を消費する可能性があります。たとえば、予想される負荷が約200 TPS の場合、ログ・ファイルは毎秒 200-400 KB 大きくなります。

[トップ](#top)

<a name="log-examples-for-notification-transactions"></a>
## 通知トランザクションのログの例

このセクションでは、通知トランザクションに対応するログの例をいくつか示します。
これは Orion 2.5.0 リリースで生成されたものであり、将来大きな変更は予定されていませんが、
正確なログトレースは新しいバージョンでは多少異なる可能性があります。

このテストでは、Context Broker は次のように開始されました :

```
contextBroker -fg -httpTimeout 10000 -logLevel INFO -notificationMode threadpool:100:10 -multiservice -subCacheIval 180
```

送信成功 (レスポンス・コード 200) :

```
time=2020-10-26T14:48:37.192Z | lvl=INFO | corr=54393a44-179a-11eb-bb87-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000006 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e174b14e7532482ac794): POST localhost:1028/accumulate, response code: 200
```

400 での通知エンドポイントのレスポンス (WARN トレースがプリントされます) :

```
time=2020-10-26T14:49:34.619Z | lvl=WARN | corr=7689f6ba-179a-11eb-ac4c-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000009 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=httpRequestSend.cpp[583]:httpRequestSendWithCurl | msg=Notification response NOT OK, http code: 400
time=2020-10-26T14:49:34.619Z | lvl=INFO | corr=7689f6ba-179a-11eb-ac4c-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000009 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e1fdb14e7532482ac795): POST localhost:1028/giveme400, response code: 400
```

404 での通知エンドポイントのレスポンス (WARN トレースがプリントされます) :

```
time=2020-10-26T14:51:40.764Z | lvl=WARN | corr=c1b8e9c0-179a-11eb-9edc-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000012 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=httpRequestSend.cpp[583]:httpRequestSendWithCurl | msg=Notification response NOT OK, http code: 404
time=2020-10-26T14:51:40.764Z | lvl=INFO | corr=c1b8e9c0-179a-11eb-9edc-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000012 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e27cb14e7532482ac796): POST localhost:1028/giveme404, response code: 404
```

500 での通知エンドポイントのレスポンス (WARN トレースがプリントされます) :

```
time=2020-10-26T14:53:04.246Z | lvl=WARN | corr=f37b5024-179a-11eb-9ce6-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000015 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=httpRequestSend.cpp[583]:httpRequestSendWithCurl | msg=Notification response NOT OK, http code: 500
time=2020-10-26T14:53:04.247Z | lvl=INFO | corr=f37b5024-179a-11eb-9ce6-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000015 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e2cfb14e7532482ac797): POST localhost:1028/giveme500, response code: 500
```

10 秒以内にエンドポイントが応答しない、またはその他の何らかの接続エラーが発生しました (アラームは WARN レベルで発生します) :

```
time=2020-10-26T14:54:15.996Z | lvl=WARN | corr=184b8b80-179b-11eb-9c52-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000018 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError localhost:1028/givemeDelay: notification failure for queue worker: Timeout was reached
time=2020-10-26T14:54:15.996Z | lvl=INFO | corr=184b8b80-179b-11eb-9c52-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000018 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e30db14e7532482ac798): POST localhost:1028/givemeDelay, response code: Timeout was reached
```

応答しないポートのエンドポイント。例えば、localhost：9999 (アラームは WARN ログ・レベルで発生します) :

```
time=2020-10-26T15:01:50.659Z | lvl=WARN | corr=2d3e4cfc-179c-11eb-b667-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000030 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError localhost:9999/giveme: notification failure for queue worker: Couldn't connect to server
time=2020-10-26T15:01:50.659Z | lvl=INFO | corr=2d3e4cfc-179c-11eb-b667-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000030 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e4deb14e7532482ac79c): POST localhost:9999/giveme, response code: Couldn't connect to server
```

解決できない名前のエンドポイント。例えば、foo.bar.bar.com (アラームは WARN ログ・レベルで発生します) :

```
time=2020-10-26T15:03:54.258Z | lvl=WARN | corr=769f8d8e-179c-11eb-960f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000033 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError foo.bar.bar.com:9999/giveme: notification failure for queue worker: Couldn't resolve host name
time=2020-10-26T15:03:54.258Z | lvl=INFO | corr=769f8d8e-179c-11eb-960f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000033 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e559b14e7532482ac79d): POST foo.bar.bar.com:9999/giveme, response code: Couldn't resolve host name
```

到達不能な IP のエンドポイント。例えば、12.34.56.87 (アラームは WARN ログ・レベルで発生します) :

```
time=2020-10-26T15:06:14.642Z | lvl=WARN | corr=c4a3192e-179c-11eb-ac8f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000036 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError 12.34.56.78:9999/giveme: notification failure for queue worker: Timeout was reached
time=2020-10-26T15:06:14.642Z | lvl=INFO | corr=c4a3192e-179c-11eb-ac8f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000036 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e5dbb14e7532482ac79e): POST 12.34.56.78:9999/giveme, response code: Timeout was reached
```

[トップ](#top)

## ログに関連するコマンドライン・オプション

このドキュメントですでに説明されているもの (`-logDir`, `-logAppend`, `-logLevel`, `-t`, `-logInfoPayloadMaxSize`, `-relogAlarms` and `-logSummary`) とは別に、次のコマンドライン・オプションはログに関連しています:

* `-logForHumans`
* `-logLineMaxSize`

詳細については、[コマンドライン・オプションのドキュメント](cli.md)を参照してください。

[トップ](#top)
