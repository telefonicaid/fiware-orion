# <a name="top"></a> ログ

* [ログ・ファイル](#log-file)
* [ログ・フォーマット](#log-format)
* [アラーム](#alarms)
* [サマリ・トレース](#summary-traces)
* [ログ・ローテーション](#log-rotation)

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

Orion がフォアグラウンドで実行されると (つまり、`-fg` [CLI 引数](cli.md)を使用して)、標準出力にも同じログトレースが (ただし簡略化されて) 出力されます。

Orion によって公開される [admin API](management_api.md) を使用して、実行時にログレベルを変更 (および取得) することができます。

[トップ](#top)

<a name="log-format"></a>
## ログ・フォーマット

ログ・フォーマットは、[Splunk](http://www.splunk.com/) や [Fluentd](http://www.fluentd.org/) などのツールで処理されるように設計されています 。

ログ・ファイルの各行は、パイプ文字 (`|`) で区切られた複数のキー値フィールドで構成されています。例 :

    time=2014-07-18T16:39:06.265Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1217]:main | msg=Orion Context Broker is running
    time=2014-07-18T16:39:06.266Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=MongoGlobal.cpp[122]:mongoConnect | msg=Successful connection to database
    time=2014-07-18T16:39:06.266Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1055]:mongoInit | msg=Connected to mongo at localhost:orion
    time=2014-07-18T16:39:06.452Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1290]:main | msg=Startup completed
    ...
    time=2014-07-18T16:39:22.920Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=pending | srv=pending | subsrv=pending | comp=Orion | op=rest.cpp[615]:connectionTreat | msg=Starting transaction from 10.0.0.1:v1/v1/updateContext
    time=2014-07-18T16:39:22.922Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[1499]:processContextElement | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.922Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[1318]:createEntity | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.923Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[811]:addTriggeredSubscriptions | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.923Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[811]:addTriggeredSubscriptions | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.923Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=rest.cpp[745]:connectionTreat | msg=Transaction ended
    ...
    time=2014-07-18T16:39:35.415Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000002 | from=pending | srv=pending | subsrv=pending | comp=Orion | op=rest.cpp[615]:connectionTreat | msg=Starting transaction from 10.0.0.2:48373/v1/queryContext
    time=2014-07-18T16:39:35.416Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000002 | from=10.0.0.2 | srv=s1 | subsrv=/A | comp=Orion | op=MongoGlobal.cpp[877]:entitiesQuery | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:35.416Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000002 | from=10.0.0.2 | srv=s1 | subsrv=/A | comp=Orion | op=rest.cpp[745]:connectionTreat | msg=Transaction ended
    ...
    time=2014-07-18T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[968]:sigHandler | msg=Signal Handler (caught signal 2)
    time=2014-07-18T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[974]:sigHandler | msg=Orion context broker exiting due to receiving a signal

各行のさまざまなフィールドは次のとおりです :

-   **time** : ログラインが [ISO8601](https://es.wikipedia.org/wiki/ISO_8601) フォーマットで生成された瞬間に対応するタイムスタンプです。Orion はタイムスタンプを UTC 形式で出力します。
-   **lvl (レベル)** : 6つのレベルがあります :
    -   FATAL : このレベルは、アプリケーションを終了させる重大なエラーイベントを示します。プロセスはもはや機能しません。
    -   ERROR : このレベルはエラーイベントを示します。解決しなければならない重大な問題があります
    -   WARN : このレベルは潜在的に有害な状況を示します。解決すべき軽微な問題があります
    -   INFO : このレベルは、Orion の進捗をハイライトする情報メッセージを示します
    -   EBUG : このレベルは、アプリケーションをデバッグするのに最も役立つ細かい情報イベントを示します。トレース・レベルが使用されているときにのみ表示されます (`-t` コマンドライン・オプションで設定されます)
    -   SUMMARY : これはログ・サマリ・トレースで使用される特別なレベルで、`-logSummaryCLI` オプションで有効になっています。詳細については、[サマリ・トレースのセクション](#summary-traces)を見てください
-   **corr (correlator id).** : "N/A" (ログメッセージ "out of transaction"、たとえば Orion Context Broker の起動に対応するログ行)、または UUID 形式の文字列です
例 : "550e8400-e29b-41d4-a716-446655440000"。この 'correlator id' は着信要求から転送されるか、または着信要求に HTTP ヘッダー "Fiware-Correlator" が含まれていない場合、corr は Orion Context broker によって生成され、ログ・ファイルで使用されます (転送メッセージ、通知およびレスポンスにに HTTP ヘッダーとして送信されるのと同様に)。correlator id は、特定の1つの要求に対して 'message chain' に関係するすべてのアプリケーションの共通識別子です
-   **trans (transaction id)** : "N/A" (Orion Context Broker の起動時に対応するログメッセージ "out of transaction") またはフォーマットの文字列 "1405598120-337-00000000001" になります。トランザクション id 生成ロジックは、すべてのトランザクション ID が一意であることを保証します。別 VM で実行されている Orion インスタンス (同じソースから別々のログを集約する場合に便利です) についても、それらが正確に同じミリ秒で開始されている場合は例外です。トランザクション ID は correlator ID とは無関係であることに注意してください。トランザクション ID はローカルな性質を持ち、correlator ID は意味のあるエンド・ツー・エンドであり、Context Broker 自体とは別のソフトウェアコンポーネントを含んでいます。Orion には2種類のトランザクションがあります :
    -   Orion によって公開された REST API を呼び出す外部クライアントによって開始されたものです。これらのトランザクションの最初のメッセージは、*url* に操作を呼び出すクライアントの IP とポートが含まれ、パスが Orion で呼び出された実際の操作である "Starting transaction **from** *url*" パターンを使用します。これらのトランザクションの最後のメッセージは "Transaction ended" です
    -   Orion が通知を送信するときに開始するものです。これらのトランザクションの最初のメッセージは、"Starting transaction **to** *url*" というパターンを使用します。*url* は、サブスクリプションの参照要素で使用される URL です。つまり、通知を送信するコールバックの URL です。両方の取引型の最後のメッセージは "Transaction ended" です
-   **from** : トランザクションに関連付けられた HTTP 要求のソース IP です。リクエストに `X-Forwarded-For` ヘッダ (前者を上書きする) または `X-Real-IP` (上書き `X-Forwarded-For` と source IP を上書き) が含まれている場合を除きます
-   **srv** : トランザクションに関連付けられたサービスです。またはトランザクションが開始されてもサービスがまだ取得されていない場合は "pending" です
-   **subsrv** : トランザクションに関連付けられたサブサービスです。またはトランザクションが開始されたがサブサービスがまだ取得されていない場合は "pending" です
-   **comp (component)** : 現在のバージョンでは常にこのフィールドに "Orion" が使用されます
-   **op** : ログメッセージを生成したソースコード内の関数です。この情報は、開発者だけに役立ちます。
-   **msg (message)** : 実際のログメッセージです。メッセージのテキストには、ファイルの名前とトレースを生成する行番号が含まれます (この情報は、主に Orion 開発者にとって役に立ちます)

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
