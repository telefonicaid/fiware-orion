# <a name="top"></a>セマフォ

Orion は、以下のような繊細なデータやリソースを保護するために、多数のセマフォを管理しています。

* [Mongo リクエスト](#mongo-request-semaphore) (Mongo requests)
* [トランザクション ID](#transaction-id-semaphore) (Transaction ID)
* [サブスクリプション・キャッシュ](#subscription-cache-semaphore) (Subscription cache)
* [タイミング統計情報](#timing-statistics-semaphore) (Timing statistics)
* [Mongo コネクション・プール](#mongo-connection-pool-semaphores) (Mongo connection pool)
* [メトリック・マネージャ](#metrics-manager-semaphore) (Metrics Manager)
* [アラーム・マネージャ](#alarm-manager-semaphore) (Alarm Manager)
* [ログ・ファイル](#log-file-semaphore) (Log file)
* [通知キュー](#notification-queue-semaphore) (Notification queue)
* [通知キュー統計情報](#notification-queue-statistics-semaphore) (Notification queue statistics)

これらのセマフォのうち、最初の4つは、`lib/common/sem.[cpp|h]` のヘルパー関数を使用し、他のものはそれぞれの構造体/クラスの一部です。

<a name="mongo-request-semaphore"></a>
## Mongo リクエスト・セマフォ
*Mongo リクエスト・セマフォ* が、`lib/common/sem.cpp` 内に存在し、そのセマフォ変数が `reqSem` です。セマフォを取る/与える機能は、`reqSemTake()` と `reqSemGive()` です。

Mongo リクエスト・セマフォには4つの異なる作業モードが存在するため、この `reqSemTake()` 関数は多少特殊です。

* なし (None)
* **読み取り**操作の場合のみ
* **書き込み**操作の場合のみ
* **読み取りと書き込み**の両方の操作

"None" は、セマフォが使用されていないことを意味します。

このセマフォの動作モードは、[CLI オプション](../admin/cli.md)の `-reqMutexPolicy` を使用して設定されます。デフォルト値は "読み取りと書き込みの両方の操作"です。[Orion 管理マニュアルのこのセクション](../admin/perf_tuning.md#mutex-policy-impact-on-performance)にミューテックス・ポリシーの詳細についての記載があります。

このセマフォは、[**mongoBackend**](sourceCode.md#srclibmongobackend) の最上位レベルの関数、すなわちサービス・ルーチンによって呼び出される外部関数で**のみ**、データベースへのすべてのリクエストに対して使用されます。

[トップ](#top)

<a name="transaction-id-semaphore"></a>
## トランザクション ID セマフォ
トランザクション ID セマフォは、`lib/common/sem.cpp` 内に存在し、そのセマフォ変数は、`transSem` です。セマフォを取る/与える機能は、`transSemTake()` と `transSemGive()` です。

Orion が受け取る各 REST リクエストには、一意の**トランザクション ID** が与えられます。

トランザクションの一意の識別子を保証するために、Brokerの `startTime` (ミリ秒まで) がプレフィックスとして使用されます。これで Broker 間の一意性がほぼ保証されます。また、トランザクションの実行番号が識別子に追加されます。

32 ビット符号付き数が使用されるため、その最大値は 0x7FFFFFFF (2,147,483,647) です。実行番号 (running number) がオーバー・フローすると、Broker の開始時刻にミリ秒が追加されます。オーバー・フロー後に実行番号が再び1から始まるので、 実行番号のオーバー・フロー後の最初のトランザクションと VERY の最初のトランザクションとを区別するために、これを必要とします (両方が実行番号1を持つので)。Broker の開始時刻が XXXXXXXXX.123 であるとします :

* XXXXXXXXX.123.1 -> VERY 最初のトランザクション
* XXXXXXXXX.124.1 -> 番号のオーバー・フローを実行した後の最初のトランザクション

全体が、[**logMsg** library](sourceCode.md#srcliblogmsg) のロギング・ライブラリによって提供されるスレッド変数 `transactionId` に保管されます。

さて、**実行番号 (running number)** はインクリメントされるときに保護される必要があり、このセマフォはその目的のために使用されます。

`lib/common/globals.cpp` ファイル内の `transactionIdSet()` 関数を参照してください。

[トップ](#top)

<a name="subscription-cache-semaphore"></a>
## サブスクリプション・キャッシュ・セマフォ
サブスクリプション・キャッシュ・セマフォが `lib/common/sem.cpp` 内に存在し、そのセマフォ変数は、`cacheSem` です。セマフォを取る/与える関数は、`cacheSemTake()` and `cacheSemGive()` であり、2つの異なるライブラリの関数で使用されます。

* [**lib/mongoBackend**](sourceCode.md#srclibmongobackend) と
* [**lib/cache**](sourceCode.md#srclibcache)

サブスクリプション・キャッシュの実装、*特にリフレッシュ方法*のために、このセマフォは、キャッシュ・ライブラリの低レベル関数では、通常はそうであるように、実行できません。しかし高レベル関数では取る/与えられないので、実装が少し複雑になります。

このセマフォの詳細は、[サブスクリプション・キャッシュの専用ドキュメント](subscriptionCache.md)にすでに存在しています。[`subCacheSync()` 関数のセクション](subscriptionCache.md#subcachesync)で、説明したセマフォの考慮事項に特に注意してください。

[トップ](#top)

<a name="timing-statistics-semaphore"></a>
## タイミング統計情報セマフォ
*タイミング統計情報セマフォ*は `lib/common/sem.cpp` 内に存在し、そのセマフォ変数は、`timeStatSem` です。セマフォを取る/与える機能は、`timeStatSemTake()` と `timeStatSemGive()` です。

タイミング統計情報は実行時にボトルネックを検出するためのツールとして考案されましたが、システムコールを使用して時間を測定するため、Orion のパフォーマンスに影響を与えます。デフォルトではオフに設定されていましたが、[CLI parameter](../admin/cli.md) `-statTiming` でオンにできます。

統計測定値は、`lib/rest/rest.cpp` 内の requestCompleted() 関数によって集められ、`renderTimingStatistics()` 関数の `lib/common/statistics.cpp` 内にまとめられます。これらの関数はどちらも明らかにセマフォを使用します。他の場所では使用されません。

[トップ](#top)

<a name="mongo-connection-pool-semaphores"></a>
## Mongo コネクション・プール・セマフォ
Orion はデータベースへのコネクション用のプールを実装しており、このプールは接続を取得/解放するためにセマフォによる保護が必要です。

実際には、2つのセマフォが使用されます。これらの2つのセマフォを保持する変数は次のとおりです :

* `connectionPoolSem`
* `connectionSem`

それらは、`lib/mongoBackend/mongoConnectionPool.cpp` の `mongoConnectionPoolInit()` で初期化され、同じファイルの2つの関数で取得/与えられます : 

* `mongoPoolConnectionGet()`
* `mongoPoolConnectionRelease()`

セマフォを保持する変数は静的であるため、このファイル `lib/mongoBackend/mongoConnectionPool.cpp` の外部ではアクセスできません。

これは、Mongo コネクション・プールの保護方法です :

* バイナリ・セマフォは、プール自体を保護します (`connectionPoolSem`)
* カウント・セマフォは、フリーのコネクションが確立されるまで呼び出し元を待機させます (`connectionSem`)

コネクション数には限りがあります。最初に行うことは、コネクションがプール内の N 個のコネクションのいずれかになるのを待つことです。これは、"POOL SIZE" で初期化されたカウント・セマフォを待つことによって行われます。つまり、プールサイズが N の場合、セマフォを N 回取ることができます。

コネクションが利用可能になると、`sem_wait(&connectionSem)` が返され、プール自体を保護するセマフォを取得する必要があります。私たちはプールのベクトルを変更しようとしています。一度に1つのスレッドでしか行うことはできません。

コネクションを取得した後、セマフォ `connectionPoolSem` が与えられ、コネクション・プールへのすべての変更が完了しました。しかし、他のセマフォは `connectionSem` を保持しており、接続を終了するまで与えられません。

`mongoPoolConnectionRelease()` 関数はカウント・セマフォ `connectionSem` を解放します。コネクションを使用し終わった後、`mongoPoolConnectionRelease()` 関数を呼び出すことは非常に重要です。

[トップ](#top)

<a name="metrics-manager-semaphore"></a>
## メトリック・マネージャ・セマフォ
メトリック・マネージャはメトリックのリストを保護するためにセマフォを必要とし、このセマフォは、`sem` と呼ばれる `MetricsManager` クラスのプライベート・メンバです。セマフォにアクセスするための2つのプライベート・メソッドも開発されています :

* `MetricsManager::semTake()`
* `MetricsManager::semGive()`

メソッドはプライベートであるため、`MetricsManager` クラスのインスタンス (Orionのシングルトン) によってのみアクセスできます。

セマフォは、メトリックのリストが読み込まれたり更新されたりするたびに使用されます。これは、`MetricsManager` の4つのメソッドと他の方法で行われます :

* `MetricsManager::add()`
* `MetricsManager::release()`
* `MetricsManager::reset()`
* `MetricsManager::toJson()`

[トップ](#top)

<a name="alarm-manager-semaphore"></a>
## アラーム・マネージャ・セマフォ
アラーム・マネージャはメトリック・マネージャと非常によく似ており、セマフォは同じパターンに従います。`AlarmManager` クラスには、`sem` 呼ばれるプライベート・フィールドおよびメソッドがあります :

* `AlarmManager::semTake()`
* `AlarmManager::semGive()`

セマフォは、アラームのリストを保護し、次の方法でアクセスします :

* `notificationErrorLogAlwaysSet()`
* `badInputLogAlwaysSet()`
* `dbErrorLogAlwaysSet()`
* `dbError()`
* `dbErrorReset()`
* `notificationError()`
* `notificationErrorReset()`
* `badInput()`
* `badInputReset()`

**注意** : アラーム・マネージャのセマフォは `AlarmManager` クラスの中でプライベートです、しかし、`semTake()` および `semGive()` メソッドは、**パブリック** です。これは間違いですが、メソッドもプライベートである必要があります。それらは `AlarmManager` のメソッドの中から呼び出されるだけなので、メソッドをプライベートにすることに問題はありません。

[トップ](#top)

<a name="log-file-semaphore"></a>
## ログ・ファイル・セマフォ
Orion はログ・ファイルを保持しており、同時に2つのスレッドからログ・ファイルを保護するためにセマフォが必要です。このセマフォを保持する変数は `sem` と呼ばれ、それは、`lib/logMsg/logMsg.cpp` にあります。これは静的変数なので、このファイルの外からは参照できません。

セマフォは、`lmSemInit()` 関数内で初期化されると、`semTake()` および `semGive()` の2つの静的関数で使用され、それらの順番に次のように使用されます : 

* `lmOut()`
* `lmClear()`


[トップ](#top)

<a name="notification-queue-semaphore"></a>
## 通知キューセマフォ
通知を送信するために、[CLI parameter](../admin/cli.md) `-notificationMode` を使って、スレッド・プールを使用するとき、スレッド・プール内のワーカーに通知を送るためにキューが使用されます。このキューはセマフォによって保護されています。

`boost::mutex` タイプのセマフォは、`mtx` と呼ばれ、それは、`src/lib/common/SyncQOverflow.h` で見つかる、`SyncQOverflow` クラスのプライベート・メンバでです :

```
template <typename Data> class SyncQOverflow
{
private:
  std::queue<Data>           queue;
  mutable boost::mutex       mtx;
  boost::condition_variable  addedElement;
  size_t                     max_size;

public:
  SyncQOverflow(size_t sz): max_size(sz) {}
  bool     try_push(Data element);
  Data     pop();
  size_t   size() const;
};
```

`QueueWorkers` クラスは `SyncQOverflow` タイプのプライベート・メンバーを含み、`QueueNotifier` クラスは ` QueueWorkers` タイプのプライベート・メンバーを含みます。

最後に、`src/app/contextBroker/contextBroker.cpp` の `contextBrokerInit()` は、CLI パラメータ `-notificationMode` が **threadpool** に等しいときにリクエストされたときに、シングルトンとして `QueueNotifier` のインスタンスを生成します。

テンプレートクラスの `SyncQOverflow` の `try_push()` メソッドは、通知キューに項目をプッシュする前にこのセマフォを取得し、同様に `pop()` メソッドは要素をポップする前にセマフォを取得します。

このセマフォは、通知キューへのプッシュとポップを保護します。しかし、キュー内のアイテムを待機させるメカニズムが必要であり、この目的のために `addedElement` と呼ばれる `boost::condition_variable` が使用されます。`addedElement` は` SyncQOverflow` クラスのプライベート・メンバーです。

[トップ](#top)

<a name="notification-queue-statistics-semaphore"></a>
## 通知キュー統計セマフォ
通知キューの統計は、`lib/ngsiNotify/QueueStatistics.cpp` の `mtxTimeInQ`セマフォにより保護されています。このセマフォは `boost::mutex` タイプであり、通知キューのタイミング統計が変更/クエリされるたびに使用されます :

* `QueueStatistics::getTimeInQ()`
* `QueueStatistics::addTimeInQWithSize()`
* `QueueStatistics::getQSize()`
* `QueueStatistics::reset()`

[トップ](#top)
