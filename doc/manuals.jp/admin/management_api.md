# 管理用 REST インタフェース

## ログレベルとトレース・レベル

Orion Context Broker は、NGSI インターフェースの他に、ログレベルとトレース・レベル (初期値は `-t` と `-logLevel` のコマンドライン・オプションを使用して設定) を変更できる管理用 REST API を公開しています。

ログレベルを変更するには :

```
curl -X PUT <host>:<port>/admin/log?level=<NONE|FATAL|ERROR|WARN|INFO|DEBUG>
```

ログレベルを取得するには :

```
curl <host>:<port>:/admin/log
```

このレスポンスは次のパターンに従います :

```
{
   "level": "INFO"
}
```

トレース・レベルを管理するには :

```
curl --request DELETE <host>:<port>/log/trace
curl --request DELETE <host>:<port>/log/trace/t1
curl --request DELETE <host>:<port>/log/trace/t1-t2
curl --request DELETE <host>:<port>/log/trace/t1-t2,t3-t4
curl --request GET <host>:<port>/log/trace
curl --request PUT <host>:<port>/log/trace/t1
curl --request PUT <host>:<port>/log/trace/t1-t2
curl --request PUT <host>:<port>/log/trace/t1-t2,t3-t4
```

'PUT-requests' は以前のログ設定を上書きします。したがって、トレース・レベルを追加するには、最初に `GET /log/trace` を発行し、その後に PUT リクエストで送信する完全なトレース文字列を組み立てる必要があります。

### 入出力ペイロードに関連するトレース・レベル

次のトレース・レベルは、Orion に トレース中に入出力ペイロードを出力させるために特に有益です :

```
/* Input/Output payloads (180-199) */
  LmtServiceInputPayload = 180,
  LmtServiceOutPayload,
  LmtClientInputPayload,
  LmtClientOutputPayload = 183,  // Very important for harness test notification_different_sizes
  LmtPartialPayload,
  LmtClientOutputPayloadDump,
  LmtCPrForwardRequestPayload,
  LmtCPrForwardResponsePayload,
```

したがって、次のようにしてすべてを有効にすることができます :

```
curl --request PUT <host>:<port>/log/trace/180-199
```

## セマフォー

別の有用な REST API (特に、broker が正しくレスポンスしなくなった場合) は、提供されるセマフォ・リストです :

```
curl <host>:<port>/admin/sem
```

レスポンスは、broker のすべてのセマフォに関する情報のリストです

```
{
    "alarmMgr": {
        "status": "free"
    },
    "connectionContext": {
        "status": "free"
    },
    "connectionEndpoints": {
        "status": "free"
    },
    "dbConnection": {
        "status": "free"
    },
    "dbConnectionPool": {
        "status": "free"
    },
    "logMsg": {
        "status": "free"
    },
    "metrics": {
        "status": "free"
     },
    "request": {
        "status": "free"
    },
    "subCache": {
        "status": "free"
    },
    "timeStat": {
        "status": "free"
    },
    "transaction": {
        "status": "free"
    }
}
```

セマフォの簡単な説明 :  

* **alarmMgr** : Alarm Manager のデータを保護します。
* **connectionContext** : HTTP 通知/転送メッセージを送信するための curl context を保護します。
* **connectionEndpoints** : HTTP 通知/転送メッセージの送信時にカールエンドポイントを保護します。
* **dbConnectionPool** : mongoDB のコネクション・プールを保護します。
* **DBConnection** : mongoDB のコネクション・プールのセットを保護します。
* **LOGMSG** : 2つのメッセージが同時にログ・ファイルに書き込まれないことを確認します。
* **metrics** : Metrics Manager リクエストの内部データを保護し、mongodb への2つの同時要求がないことを確認します。
* **request** : mongodb への2つの同時リクエストがないことを確認します。
* **subCache** : Subscription Cache を保護します。
* **timeStat** : タイミング統計のためにデータを保護する。
* **transaction** : トランザクション ID を保護します (ログ・ファイルの場合)

セマフォのそれぞれについて提供される情報は、*status* です : ` free` または `taken`

[今はセマフォーごとに1つの項目しかありませんが、将来的にはさらに情報を追加することを考えています]
