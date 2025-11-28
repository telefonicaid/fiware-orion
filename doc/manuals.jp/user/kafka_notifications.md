## イントロダクション

Orion は Kafka 経由で通知を送信できます。この場合、通知がトリガーされるたびに、サブスクリプション時に指定された特定の KAFKA
ブローカーまたは KAFKA クラスターに KAFKA メッセージが発行されます。

![](kafka-notifications.png "../../manuals/user/kafka-notifications.png")

運用の観点から見ると、KAFKA サブスクリプションは、
[ドキュメントのこのセクション](walkthrough_apiv2.md#subscriptions) や
[Orion API 仕様](../orion-api.md) で説明されているように、HTTP サブスクリプションに似ています
(たとえば、通知ペイロードは同じで、有効期限やフィルタリング式などを設定できます)。
ただし、`notification` オブジェクトでは `http` ではなく `kafka` を使用します。

```
...
"notification": {
  "kafka": {
    "url": "kafka://broker1:9092,broker2:9092,broker3:9092",
    "topic": "sub1"
  }
}
...
```

`kafka` 内では、次の要素を使用できます。:

* `url` は、使用する KAFKA ブローカーのエンドポイントを指定します。
  URL は `kafka://` で始まり、パスは含みません（つまり、ホストとポートのみが含まれます）
* `topic` は、使用する KAFKA トピックを指定します

KAFKAとHTTPサブスクリプションのもう1つの違いは、前者には以下のフィールドが含まれていないことです:

* `lastSuccessCode`。KAFKA の場合、HTTP レスポンス・コードに相当するものはありません
* `lastFailureReason`。Orion が検出できる唯一の失敗理由は、対応する KAFKA クラスタへの接続失敗です。
   したがって、追加の詳細情報を提供する必要はありません

ただし、`lastSuccess` フィールドと `lastFailure` フィールド (最後の成功/失敗のタイムスタンプを指定する)
は、HTTP サブスクリプションと同じように kafka サブスクリプションでサポートされていることに注意してください。

## カスタム通知

KAFKA サブスクリプションのカスタム通知 ([Orion API 仕様のこのセクション](../orion-api.md#custom-notifications) で説明) は、
次の点を考慮して、HTTP サブスクリプションと同じように機能します:

* `httpCustom` の代わりに `kafkaCustom` が使用されます
* `kafka` で使用されるのと同じフィールドが `kafkaCustom` でも使用できます
* `qs` と `method` は Kafka では同等ではないため使用できません
* `topic`、`payload`、`json`、`ngsi` フィールドではマクロ置換が行われます。`url` は固定値です

## Kafka 通知におけるメッセージの構造

* `Key`: これは、Kafka がメッセージを保存するパーティションを決定するために使用する文字列/バイト値です。Orion では、このキーは通知を生成したサブスクリプション ID に対応します。
* `Headers`: Kafka は、キーと値のペアとしてエンコードされたメッセージ・ヘッダをサポートしています。Orion には以下のデフォルト・ヘッダが含まれています。

- `Fiware-Service`
- `Fiware-ServicePath`

`kafkaCustom` 通知では、これらのヘッダを上書きしたり、サブスクリプションで追加のカスタム・ヘッダを定義したりできます。

`Payload`: (メッセージ・ボディ): これは、Orion が Kafka にパブリッシュするメッセージのメイン・コンテンツです。通知ペイロードまたはカスタム通知によって生成されたコンテンツに対応します。

カスタム通知の詳細については、[Orion API ドキュメントの該当セクション](../orion-api.md#custom-notifications) を参照してください。

## 接続管理

サブスクリプションに関連付けられた KAFKA クラスタのエンドポイントは、サブスクリプション時に `url` フィールドで指定されますが、
そのエンドポイントへの接続は、KAFKA 通知が初めて公開されたときに行われます。

KAFKA への接続が確立されると、通知が発行されている間は接続が開いたままになります。
接続が使用されていない場合（メッセージが送信されていない場合）、Orion は `-kafkaMaxAge` パラメータ（[CLI パラメータ](../admin/cli.md)、デフォルトは 30 日）で定義されたメンテナンス期間後に接続を閉じます。
KAFKA でエラーが発生した場合も接続は閉じられ、Orion が次に通知を発行するときに自動的に再作成されます。
