# コマンドラインからの Orion の実行

broker を実行するには、次のコマンドを入力します :

    contextBroker

broker はデフォルトでバックグラウンドで実行されるため、シグナルを使用して broker を停止する必要があります。

たとえば、-port オプションを使用して、Orion Context Broker がリッスンするポートを指定するために、コマンドライン引数を使用できます :

    contextBroker -port 5057

可能なすべてのオプションを知るには、次のセクションをご覧ください。

## コマンドライン・オプション

コマンドライン・オプションは、直接 (コマンドラインから実行する場合)、また /etc/sysconfig/contextBroker ([システムサービスとして](running.md)実行している場合) の異なるフィールドを介して間接的に使用できます。使用可能なオプションのリストを取得するには、次のコマンドを使用します :

    contextBroker -u

オプション (デフォルト値や制限を含む) に関する詳細情報を得るには、以下を使用します :

    contextBroker -U

使用可能なオプションのリストは次のとおりです :

-   **-u** and **-U** : 短い形式または長い形式のそれぞれで使用状況を表示します
-   **--help** : ヘルプを表示します (前と非常に似ています)
-   **--version** : バージョン番号を表示します
-   **-port <port>** : broker がリスンするポートを指定します。デフォルトのポートは1026です
-   **-ipv4** : broker を IPv4 専用モードで実行します。デフォルトでは、broker は IPv4 と IPv6 の両方で動作します。-ipv6 と同時に使用することはできません
-   **-ipv6** : broker を IPv6 専用モードで実行します。デフォルトでは、broker は IPv4 と IPv6 の両方で動作します。-ipv4 と同時に使用することはできません。
-   **-multiservice** : マルチサービス/マルチテナントモードを有効にします。[マルチテナンシーのセクション](../user/multitenancy.md)を参照してください
-   **-db <db>** : 使用する MogoDB データベース、または (`-multiservice` を使用している場合) サービス単位/テナント単位のデータベースのプレフィックス ([マルチテナンシー](../user/multitenancy.md)のセクションを参照してください) です。このフィールドは最大10文字までです
-   **-dbhost <host>** : 使用する MongoDB のホストとポートです。たとえば、`-dbhost localhost:12345` です
-   **-rplSet <replicat_set>** : 指定すれば、Orion CB が MongoDB レプリカセット (スタンドアロン MongoDB インスタンスではなく) に接続されます。使用するレプリカセットの名前は、パラメータの値です。この場合、-dbhost パラメーターは、レプリカ・セットのシードとして使用されるホスト ("," で区切られた) のリストにすることができます
-   **-dbTimeout <interval>** : レプリカセット (-rplSet) を使用する場合にのみ使用され、それ以外の場合は無視されます。レプリカセットへの接続のタイムアウトをミリ秒単位で指定します
-   **-dbuser <user>** : 使用する MongoDB ユーザ。MongoDB が認証を使用しない場合、このオプションは避けなければなりません。[データベース認証セクション](database_admin.md#database-authorization)を参照してください
-   **-dbpwd <pass>** : 使用する MongoDB パスワード。MongoDB が認証を使用しない場合、このオプションは避けなければなりません。[データベース認証セクション](database_admin.md#database-authorization)を参照してください
-   **-dbAuthMech <mechanism>**. `-dbuser` と `-dbpwd` を提供する場合に使用する MongoDB
    認証メカニズム。代替手段はSCRAM-SHA-1 または MONGODB-CR です。デフォルト
    (このフィールドを省略する場合) は SCRAM-SHA-1 です
-   **-dbAuthDb <database>** : `-dbuser` と `-dbpwd` を提供する場合に認証に使用するデータベース
    を指定します。デフォルトは、`-multiservice` を使用しない場合の `-db` または `-multiservice`
    を使用する場合の `"admin"` と同じです
-   **-dbSSL** : MongoDB への接続で SSL を有効にします。MongoDB サーバまたはレプリカ・セットが
    SSL を使用している場合は、このオプションを使用する必要があります (または、逆に、MongoDB
    サーバまたはレプリカ・セットが SSL を使用していない場合は、このオプションを使用する必要は
    ありません)
-   **-dbPoolSize <size>** : データベース・コネクション・プール プールのデフォルトサイズは10接続です
-   **-writeConcern <0|1>** : MongoDB の書き込み操作に対する確認を指定 : 確認 (1) または未確認 (0)。デフォルトは 1です
-   **-https** : セキュアな HTTP モードで作業します (`-cert` および `-key` を参照)
-   **-cert** : https の証明書ファイル。絶対ファイルパスを使用します。 このファイルを生成する方法の例については、[このスクリプト](https://github.com/telefonicaid/fiware-orion/blob/master/test/functionalTest/httpsPrepare.sh)を見てください
-   **-key** : https のプライベート・サーバ・キーファイル。絶対ファイルパスを使用します。このファイルを生成する方法の例については、[このスクリプト](https://github.com/telefonicaid/fiware-orion/blob/master/test/functionalTest/httpsPrepare.sh)を見てください
-   **-logDir <dir\>** : contextBroker のログ・ファイルに使用するディレクトリを指定します
-   **-logAppend** : これを使用すると、空のログ・ファイルではなく、既存の contextBroker ログ・ファイルにログ行が追加されます
-   **-logLevel** : 初期ロギングレベル、サポートされるレベルを選択します :
    - NONE    (致命的なエラーメッセージを含むすべてのログ出力を抑制します),
    - FATAL   (重大なエラーメッセージのみ表示します),
    - ERROR   (エラーメッセージのみ表示します),
    - WARN    (エラーメッセージと警告メッセージを表示します。これがデフォルト設定です),
    - INFO    (エラー、警告、情報メッセージを表示します),
    - DEBUG   (すべてのメッセージを表示します)
    ログレベルは [admin API](management_api.md)を使用して実行時に変更できます
-   **-t <trace level>** : ロギングの初期トレース・レベルを指定します。単一の値 (例えば "-t 70")、範囲 (例えば "-t 20-80")、コンマ区切りのリスト (例えば "-t 70,90")、またはそれらの組み合わせ (例えば "-t 60,80-90")。ロギングにすべてのトレース・レベルを使用する場合は、"-t 0-255" を使用します。トレース・レベルは、[管理用 REST インターフェース](management_api.md)を使用して動的に変更できます。利用可能なトレース・レベルとその値の詳細は、[ここ](https://github.com/telefonicaid/fiware-orion/blob/master/src/lib/logMsg/traceLevels.h)で (C 構造体として) 見つけることができます 
-   **-fg** : broker をフォアグラウンドで実行します (デバッグに便利です)。ログ出力は、標準出力 (ログ・ファイルに加えて、単純化された形式を使用) で出力されます
-   **-localIp <ip>** : broker がリッスンする IP インタフェースを指定します。デフォルトでは、すべてのインタフェースをリッスンします
-   **-pidpath <pid_file>** : broker プロセスの PID を格納するファイルを指定します
-   **-httpTimeout <interval>** : メッセージの転送と通知のタイムアウトをミリ秒単位で指定します。デフォルトのタイムアウト (このパラメータが指定されていない場合) は5秒です
-   **-reqTimeout <interval>** : REST 接続のタイムアウトを秒単位で指定します。デフォルト値はゼロ、つまりタイムアウトなし (永遠に待機) であることに注意してください
-   **-cprForwardLimit** : 単一のクライアント要求に対するコンテキスト・プロバイダへの転送リクエストの最大数 (デフォルトは制限なし)。コンテキスト・プロバイダの転送を完全に無効にするには、0を使用します。
-   **-corsOrigin <domain>** : 許可された発信元を指定して、クロス・ソース・リソースの共有を有効にします (`*` に `__ALL` を使用)。Orion での CORS サポートの詳細については、[ユーザ・マニュアル](../user/cors.md)を参照してください。
-   **-corsMaxAge <time>** : プリフライト要求がキャッシュされる最大時間 (秒) を指定します。設定されていない場合の既定値は86400です。Orion での CORS サポートの詳細については、[ユーザ・マニュアル](../user/cors.md)を参照してください。
-   **-reqMutexPolicy <all|none|write|read>** : 内部 mutex ポリシーを指定します。詳細については、[パフォーマンス・チューニング](perf_tuning.md#mutex-policy-impact-on-performance)のドキュメントを参照してください。
-   **-subCacheIval** : サブスクリプション・キャッシュの更新の呼び出し間隔 (秒単位)。ゼロ値は "リフレッシュしない" を意味します。デフォルト値は60秒で、mono-CB 配置に適しています。([このドキュメント](perf_tuning.md#subscription-cache)のサブスクリプション・キャッシュの詳細を参照してください
-   **-noCache** : コンテキスト・サブスクリプション・キャッシュを無効にするので、サブスクリプション検索は常に DB で行われます。推奨されませんが、デバッグには便利です
-   **-notificationMode** : 通知モードを選択することができます。`transient`, `persistent` または `threadpool:q:n`。デフォルトモードは `transient` です
    * transient モードでは、通知を送信した直後に接続は CB によって閉じられます
    * persistent 接続モードでは、通知が指定された URL パスに初めて送信されたときに、持続的な接続が作成されます (受信者が持続的な接続をサポートしている場合)。同じ URL パスへの通知が行われると、接続が再利用され、HTTP 接続時間が保存されます
    * threadpool モードでは、通知は `q` と `n` のサイズのキューにエンキューされます。スレッドがキューからの通知を取ると、非同期に発信リクエストを行います。このモードを使用する場合は、[スレッド・モデルのセクション](perf_tuning.md#orion)を参照してください
-   **-notifFlowControl guage:stepDelay:maxInterval**. Enables flow control mechanism.
    [ドキュメントのこのセクション](perf_tuning.md#updates-flow-control-mechanism)を参照してください
-   **-simulatedNotification** : 通知は送信されませんが、内部で記録され、 [統計情報](statistics.md)オペレーション (`simulatedNotifications` カウンタ) に表示されます。これは本番用ではありませんが、デバッグでは CB の内部ロジックの観点から通知レートの最大上限を計算すると便利です。
-   **-connectionMemory** : HTTP サーバ・ライブラリが内部的に使用する、接続ごとの接続メモリー・バッファのサイズ (KB 単位) を設定します。デフォルト値は64 KB です
-   **-maxConnections** : 同時接続の最大数です。従来の理由から、デフォルト値は1020であり、下限は1であり、上限はありません。オペレーティング・システムの最大ファイル記述子によって制限されます
-   **-reqPoolSize** : 着信接続のスレッド・プールのサイズです。。デフォルト値は0で、*スレッド・プールがない* ことを意味します
-   **-inReqPayloadMaxSize** 着信リクエストのペイロードの最大許容サイズ (バイト単位)。デフォルト値は1MBです
-   **-outReqMsgMaxSize** *送信メッセージ* のリクエストの最大許容合計サイズ (バイト単位)。デフォルト値は8MBです
-   **-statCounters**, **-statSemWait**, **-statTiming** and **-statNotifQueue** : 統計情報の生成を有効にします。[統計情報のドキュメント](statistics.md)を参照してください
-   **-logSummary** : ログ・サマリ期間を秒単位で記録します。デフォルトは0 です。これは、*ログ・サマリがオフ* であることを意味します。最小値 : 0. 最大値 : 1か月 (3600 * 24 * 31 == 2,678,400 秒)。詳細については、[ログのドキュメント](logs.md#summary-traces)を参照してください
-   **-relogAlarms** : ログ・ファイル内の可能性のある * すべて * のアラーム誘発障害を表示するには、アラームがすでにアクティブであっても、このオプションを使用します。詳細については、[ログのドキュメント](logs.md#alarms)を参照してください
-   **-disableCustomNotifications** : NGSIv2 カスタム・通知を無効にします。特に :
    * `httpCustom` は、`http` として解釈されます。すなわち、`url` を除くすべてのサブフィールドが l 無視されます
    * `${...}` マクロ置換は実行されません
-   **-disableFileLog** : Orion がファイルにロギングするのを避けます (デフォルトの動作はログ・ファイルを使用します)。このオプションは、kubernetes で実行している場合に役に立ちます
-   **-logForHumans** : 人のために標準化されたトレースを作成します。ログ・ファイルのトレースは影響を受けないことに注意してください
-   **-logLineMaxSize** : ログ行の最大長 (超過すると、Orion は `LINE TOO LONG` をログ・トレースとして出力します)。最小許容値:100バイト。デフォルト値:32キロバイト
-   **-logInfoPayloadMaxSize** : リクエストおよび/またはレスポンス・ペイロードを出力する INFO レベルのログ・トレースの場合、これはそれらのペイロードに許可される最大サイズです。ペイロード・サイズがこの設定より大きい場合、最初の `-logInfoPayloadMaxSize` バイトのみが含まれます (そして、`(...)` の形式の省略記号がトレースに表示されます)。デフォルト値：5キロバイト
-   **-disableMetrics** : 'metrics' 機能をオフにします。メトリックの収集は、システムコールやセマフォが関与するため、少しコストがかかります。メトリックオーバーヘッドなしで broker を起動するには、このパラメータを使用します
-   **-insecureNotif** : 既知の CA 証明書で認証できないピアへの HTTPS 通知を許可する。これは、curl コマンドのパラメータ `-k` または `--insecureparameteres` に似ています

## 環境変数を使用した設定

Orion は、環境変数を使用した引数の受け渡しをサポートしています。
以下の表に示すように、各 CLI パラメータには同等の環境変数があります
(`contextBroker -U` も同じ情報を取得するために使用できます)。

2つの事実を考慮する必要があります :

* "フラグ" のように機能する CLI パラメータの環境変数は、
  (つまり、有効か無効かのどちらかですが、実際の値はありません - `-fg` はその1つです)
  大文字と小文字を区別する値の `TRUE` あるいは `true` (パラメータを有効にする) または
  `FALSE` あるいは `false` (パラメータを無効にする) をとることができます
* 競合する場合 (つまり、環境変数と CLI パラメータを同時に使用する場合)、
  CLI パラメータが使用されます

|   環境変数   |   同等の CLI パラメータ    |
| ----------------- | --------- |
|   ORION_LOG_DIR   |   logDir  |
|   ORION_TRACE |   t   |
|   ORION_LOG_LEVEL |   logLevel    |
|   ORION_LOG_APPEND    |   logAppend   |
|   ORION_FOREGROUND    |   fg  |
|   ORION_LOCALIP   |   localIp |
|   ORION_PORT  |   port    |
|   ORION_PID_PATH  |   pidpath |
|   ORION_MONGO_HOST    |   dbhost  |
|   ORION_MONGO_REPLICA_SET |   rplSet  |
|   ORION_MONGO_USER    |   dbuser  |
|   ORION_MONGO_PASSWORD    |   dbpwd   |
|   ORION_MONGO_AUTH_MECH   |   dbAuthMech  |
|   ORION_MONGO_AUTH_SOURCE |   dbAuthDb    |
|   ORION_MONGO_SSL |   dbSSL   |
|   ORION_MONGO_DB  |   db  |
|   ORION_MONGO_TIMEOUT |   dbTimeout   |
|   ORION_MONGO_POOL_SIZE   |   dbPoolSize  |
|   ORION_USEIPV4   |   ipv4    |
|   ORION_USEIPV6   |   ipv6    |
|   ORION_HTTPS |   https   |
|   ORION_HTTPS_KEYFILE |   key |
|   ORION_HTTPS_CERTFILE    |   cert    |
|   ORION_MULTI_SERVICE |   multiservice    |
|   ORION_HTTP_TIMEOUT  |   httpTimeout |
|   ORION_REQ_TIMEOUT   |   reqTimeout  |
|   ORION_MUTEX_POLICY  |   reqMutexPolicy  |
|   ORION_MONGO_WRITE_CONCERN   |   writeConcern    |
|   ORION_CORS_ALLOWED_ORIGIN   |   corsOrigin  |
|   ORION_CORS_MAX_AGE  |   corsMaxAge  |
|   ORION_CPR_FORWARD_LIMIT |   cprForwardLimit |
|   ORION_SUBCACHE_IVAL |   subCacheIval    |
|   ORION_NOCACHE   |   noCache |
|   ORION_CONN_MEMORY   |   connectionMemory    |
|   ORION_MAX_CONN  |   maxConnections  |
|   ORION_TRQ_POOL_SIZE |   reqPoolSize |
|   ORION_IN_REQ_PAYLOAD_MAX_SIZE   |   inReqPayloadMaxSize |
|   ORION_OUT_REQ_MSG_MAX_SIZE  |   outReqMsgMaxSize    |
|   ORION_NOTIF_MODE    |   notificationMode    |
|   ORION_NOTIF_FLOW_CONTROL    |   notifFlowControl    |
|   ORION_DROP_NOTIF    |   simulatedNotification   |
|   ORION_STAT_COUNTERS |   statCounters    |
|   ORION_STAT_SEM_WAIT |   statSemWait |
|   ORION_STAT_TIMING   |   statTiming  |
|   ORION_STAT_NOTIF_QUEUE  |   statNotifQueue  |
|   ORION_LOG_SUMMARY_PERIOD    |   logSummary  |
|   ORION_RELOG_ALARMS  |   relogAlarms |
|   ORION_CHECK_ID_V1   |   strictNgsiv1Ids |
|   ORION_DISABLE_CUSTOM_NOTIF  |   disableCustomNotifications  |
|   ORION_DISABLE_FILE_LOG  |   disableFileLog  |
|   ORION_LOG_FOR_HUMANS    |   logForHumans    |
|   ORION_LOG_LINE_MAX_SIZE |   logLineMaxSize  |
|   ORION_LOG_INFO_PAYLOAD_MAX_SIZE | logInfoPayloadMaxSize |
|   ORION_DISABLE_METRICS   |   disableMetrics  |
|   ORION_INSECURE_NOTIF    |   insecureNotif   |
|   ORION_NGSIV1_AUTOCAST   |   ngsiv1Autocast  |
