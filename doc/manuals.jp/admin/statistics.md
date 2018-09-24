# 統計情報

**警告** : マニュアルのこのセクションはまだ進行中ですので、現在の Orion の実装は完全には整っていない可能性があります。さらに、統計情報 API はベータ版の瞬間であることを考慮して、さまざまなカウンタまたは JSON 構造の名前の変更が将来行われる可能性があります。

Orion Context broker は、`GET /statistics` と `GET /cache/statistics` を介して、主にテストとデバッグのための統計情報のセットを提供します。これらのオペレーションは、JSON エンコーディングのみをサポートします。

現時点では、統計情報の精度は高負荷条件下で100％保証されません。アトミック・カウンタはすべての場所で使用されるわけではないため、同時に実行されている要求の競合状態によって (理論的には) 一部のデータが失われる可能性があります。しかし、低負荷条件下では、統計情報は正確でなければなりません。これについては [未解決の問題](https://github.com/telefonicaid/fiware-orion/issues/1504)があります。

## GET /statistics

統計情報 JSON は、4つの条件ブロックと2つの無条件フィールドで構成されています :

```
{
  "counters" : { ... },
  "semWait" : { ... },
  "timing" : { ... },
  "notifQueue": { ... },
  "uptime_in_secs" : 65697,
  "measuring_interval_in_secs" : 65697
}
```

条件付きブロックは、コマンドラインで `-statXXX` フラグを使用して有効にします。将来的には、[管理 API](management_api.md)を使用して有効/無効を切り替える可能性があります。第2に、情報の一部を測定することは、パフォーマンス・ペナルティ (通常、時間を測定するためには常に追加のシステムコールが必要) を伴う可能性があるため、明示的なアクティブ化が望ましいです。

* "counters" (`-statCounters` で有効です)
* "semWait" (`-statSemWait` で有効です)
* "timing" (`-statTiming` で有効です)
* "notifQueue" (`-statNotifQueue` で有効です)

無条件フィールドは次のとおりです :

* `uptime_in_secs` : Orion の稼働時間は数秒です
* `measuring_interval_in_secs` : 統計情報を秒単位で測定します。統計情報がリセットされるたびに0に設定されます。Orion の起動後に統計情報がリセットされていない場合、このフィールドは `uptime_in_secs` に一致します

### カウンタ・ブロック

カウンタ・ブロックは、特定のリクエスト型が受信された、または通知が送信された、時間のカウンタに関する情報を提供します。たとえば、次のようになります :

```
{
  ...
  "counters" : {
    "jsonRequests" : 75916,
    "queries" : 3698,
    "updates" : 2416,
    "subscriptions" : 138,
    "registrationRequest": 11,
    "registrations": 41,
    "unsubscriptions" : 6,
    "notificationsReceived" : 216936,
    "notificationsSent" : 579542,
    "individualContextEntity" : 360,
    "allContextEntitiesRequests" : 3,
    "versionRequests" : 1109,
    "statisticsRequests" : 13,
    "invalidRequests" : 2
  },
  ...
}
```

特定のリクエスト型が受信されなかった場合、対応するカウンタは表示されません。

### SemWait ブロック

SemWait ブロックは、メインの内部セマフォの累積待ち時間を提供します。ボトルネックを検出すると、たとえば、DB が遅すぎる場合や DB プールが小さすぎる場合など、`dbConnectionPool‘ 時間が異常に長くなる場合です。詳しくは、[パフォーマンス・チューニング](perf_tuning.md)のセクションを参照してください。

```
{
  ...
  "semWait" : {
    "request" : 0.000000000,
    "dbConnectionPool" : 2.917002794,
    "transaction" : 0.567478849,
    "subCache" : 0.784979145,
    "metrics": 0.000000000,
    "connectionContext" : 0.000000000,
    "timeStat" : 0.124000605
  },
  ...
}
```

### タイミング・ブロック

タイミング情報を提要します。つまり、CB が異なる内部モジュールで実行を通過する時間です。

```
{
  ...
  "timing": {
    "accumulated": {
      "jsonV1Parse": 7.860908311,
      "mongoBackend": 416.796091597,
      "mongoReadWait": 4656.924425628,
      "mongoWriteWait": 259.347915990,
      "mongoCommandWait": 0.514811318,
      "render": 108.162782114,
      "total": 6476.593504743
     },
    "last": {
      "mongoBackend": 0.014752309,
      "mongoReadWait": 0.012018445,
      "mongoWriteWait": 0.000574611,
      "render": 0.000019136,
      "total": 0.015148915
     }
  }
  ...
}
```

このブロックには、2つの主要なセクションがあります :

* `last` : 処理された最後のリクエストに対応する時間です。最後のリクエストが特定のモジュールを使用していない場合 (GET リクエストがパースを使用しないなど)、そのカウンタは0であり、表示されません
* `accumulated` : broker 起動後のすべてのリクエストに対応する累積時間です

特定のカウンタは次のとおりです :

* `total` : HTTP ライブラリがリクエスト/レスポンス・ディスパッチ (擬似エンド・ツー・エンド時間) にかかる時間を除く、リクエスト全体の処理時間です
* `jsonV1Parse` : NGSIv1 JSON パース・モジュールで渡された時間です (疑似セルフタイム)
* `jsonV2Parse` : NGSIv2 JSON パース・モジュールで渡された時間です (疑似セルフタイム)
* `mongoBackend` : mongoBackend モジュールで渡された時間です (疑似セルフタイム)
* `render` : レンダリングモジュールに渡された時間です (擬似セルフタイム)
* `mongo*Wait``Read`, `Write` または `Cmd` オペレーションのために MongoDB を待っている時間です。与えられた要求が MongoDB への複数の read/write/cmd の呼び出しを含む場合、`last` 下の `mongo*Wait` に示された時間は、それらすべてのための蓄積を含むことに注意してください。mongoReadWait の場合、結果カーソルを取得するために使用された時間のみが考慮されますが、カーソル結果を処理する時間 (mongoBackend カウンタに属する時間) は考慮されません

時間は、特定のスレッド・リクエストがモジュールの使用を開始し、使用を終了するまでの時間から測定されます。したがって、何らかの理由でスレッドが停止した場合 (カーネルがそのスケジューリングポリシーに基づいて別のスレッドに優先順位を与えることを決定した場合)、スレッドがスリープしていた時間が再び実行を待っている時間が測定に含まれているため、正確ではありません。このため、擬似 selt/end-to-end 時間と言っています。しかし、低負荷条件下では、この状況は重大な影響を及ぼさないと予想されます

### NotifQueue ブロック

スレッド・プール通知モードで使用される通知キューに関連する情報を提供します。したがって、`-notificationMode` がスレッド・プールに設定されている場合のみ表示されます。

```
{
  ...
  "notifQueue" : {
    "avgTimeInQueue": 0.000077437,
    "in" : 579619,
    "out" : 579619,
    "reject" : 0,
    "sentOk" : 579543,  // Probably will be generalized for all notification modes at the end
    "sentError" : 76,   // Probably will be generalized for all notification modes at the end
    "timeInQueue" : 44.884263230,
    "size" : 0
  }
  ...
}
```

特定のカウンタは次のとおりです :

* `avgTimeInQueue` : 各通知がキュー内で待機する平均時間です。`timeInQueue` を `out` で割った値に等しくなります
* `in` : キューに入る通知数です
* `out` : キューから出る通知数です
* `reject` : キューがいっぱいになったために拒否される通知数です。つまり、エンキューされていない通知も含まれます
* `sentOk` : 正常に送信された通知数です
* `sentError` : 失敗した通知の試行回数です
* `timeInQueue` : キュー内で待機している通知の蓄積時間です
* `size` : キューの現在のサイズです


## GET /cache/statistics

コンテキスト・サブスクリプション・キャッシュ・オペレーション (refresh, insert, remove, update) のカウンタと、キャッシュされたアイテムの現在数を提供します。

```
{
  "ids": "SubscriptionId1, SubscriptionId2, ... SubscriptionIdN",
  "refreshs" : 1,
  "inserts" : 1433,
  "removes" : 6,
  "updates" : 0,
  "items" : 1427  
}
```

"ids" フィールドは本当に長くなる可能性があることに注意してください。レスポンス時間が長すぎるのを避けるため、broker は 'ids' フィールドのサイズの制限を設定します。長さがその制限よりも長い場合、サブスクリプション識別子の完全なリストを提示するのではなく、"too many subscriptions" というテキストが代わりに表示されます。

## 統計情報のリセット

統計情報カウンタをリセットするには、統計 URL に対して DELETE オペレーションを呼び出します。なお、システム状態から来るフィールドはリセットされません、例えばサブキャッシュ項目または通知キューサイズです) :

* DELETE /statistics
* DELETE /cache/statistics

統計情報をリセットすると、次のレスポンスが返されます :

```
{
  "message" : "All statistics counter reset"
}
```
