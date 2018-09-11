# 一時的なエンティティ (Transient Entities)

一時的なエンティティは、通常のエンティティです。つまり、id/type、属性のセットなどありますが、有効期限のタイムスタンプを持ちます。その時点に達すると、エンティティは Orion によって管理されるコンテキストから自動的に削除されます。

したがって、最初の非常に重要なアドバイスです : **一時的なエンティティを使用して、期限切れになるとエンティティが自動的にデータベースから削除され、回復する方法がないため注意が必要です**。エンティティの有効期限が切れた、つまり、削除された場合、一時的なエンティティで設定した情報が関係ないことを確認します。

さらに、**すでに正確な名前 `dateExpires` の属性を使用している場合は、[下位互換性の考慮事項のセクション](#backward-compatibility-considerations)を参照してください**。

## `dateExpires` 属性

エンティティの有効期限のタイムスタンプは、NGSIv2 組み込み属性 `dateExpires` によって定義されます。これは、[NGSIv2 仕様に従って](http://telefonicaid.github.io/fiware-orion/api/v2/stable)、`DateTime` 型の属性です。その値はエンティティが期限切れになる日時です。

他の NGSIv2 組み込み属性と同じように、`dateExpires` はデフォルトでは表示されません。取得するには、GET ベースのクエリで `attrsURI` パラメータ または `POST /v2/op/query` で "`attrs`" フィールドを使用する必要があります。 詳細については、[NGSIv2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable)のセクション "組み込み属性" と "属性とメタデータのフィルタリング" を参照してください。

## 有効な遷移 (Valid transitions)

### `dateExpires` 属性を持つエンティティを作成

`dateExpires` 属性を含む場合、エンティティは一時的な性質で作成されます。例えば :

```
POST /v2/entities
{
  "id": "t1",
  "type": "Ticket",
  ...
  "dateExpires": {
    "value": "2028-07-07T21:35:00Z",
    "type": "DateTime"
  }
}
```

2028年7月7日 21:35 UTC に有効期限が切れるエンティティを作成します。

その他の考慮事項 :

* `dateExpires` は、有効な `DateTime` 値を持っていなければなりません。さもなければ、400 Bad Request が返されます。詳細については NGSIv2 の仕様を確認してください

* `dateExpires` が過去に設定されていると、エンティティが期限切れになっています。ちょっと変わっていますが、機能的には正しいです

### `dateExpires` 属性を持っていないエンティティに追加

`dateExpires`  属性を通常のエンティティ (たとえば "t2") に追加できます。たとえば :

```
POST /v2/entities/t2/attrs
{
  "dateExpires": {
    "value": "2028-10-12T14:23:00Z",
    "type": "DateTime"
  }
}
```

2028年10月12日 14:23 UTC に期限切れになります。

その他の考慮事項 :

* `dateExpires` は、有効な `DateTime` 構文を持っていなければなりません。さもなければ、400 Bad Request が返されます。詳細は NGSIv2 仕様を確認してください

* `dateExpires` が、過去に設定されていると、エンティティは自動的に期限切れになります

### `dateExpires` 属性を既に 持っているエンティティを更新

Context Broker では、属性値を更新するいくつかの方法があります。詳細については NGSIv2 仕様を確認してください。たとえば、属性リソースの URL で PUT を使用する場合 :

```
PUT /v2/entities/t2/attrs/dateExpires
{  
  "value": "2028-12-31T23:59:00Z",
  "type": "DateTime"
}
```

有効期限が 2028年12月31日 23:59 UTC に変更されます。

その他の考慮事項 :

* `dateExpires` は、有効な `DateTime` 構文を持っていなければなりません。さもなければ、400 Bad Request が返されます。詳細は NGSIv2 仕様を確認してください

* `dateExpires` が過去に設定されている場合、エンティティは自動的に有効期限が切れます

### エンティティから `dateExpires` 属性を削除

最後に、一時的なエンティティから `dateExpires` 属性を削除することができます :

```
DELETE /v2/entities/t2/attrs/dateExpires
```

これは、エンティティを通常のエンティティにして、すなわち一時的なエンティティではなくして、有効期限により削除されなくなります。

## 有効期限が切れたエンティティの削除

有効期限は、MongoDB 機能に依存して、[特定のクロック時にドキュメントを期限切れ](https://docs.mongodb.com/manual/tutorial/expire-data/#expire-documents-at-a-specific-clock-time)にします。これは、60秒ごとに起動するバックグラウンド・スレッドに基づいているため、有効期限が過ぎると、一時的なエンティティがデータベースに60秒間、または MongoDB の負荷が高い場合はさらに多少、データベースに残る可能性があります。詳細は、[MongoDB のドキュメント](https://docs.mongodb.com/manual/core/index-ttl/#timing-of-the-delete-operation)
を参照してください。

TTL モニタスレッドのデフォルトのスリープ間隔は MongoDB で変更できますが、そのトピックはこのドキュメントの範囲外です。詳細については、[このリンク](http://hassansin.github.io/working-with-mongodb-ttl-index#ttlmonitor-sleep-interval)をご覧ください。

**一時的なエンティティが削除されると、それを回復することはできません**。

<a name="backward-compatibility-considerations"></a>
## 下位互換性に関する考慮事項

一時的なエンティティは Orion 1.15.0 に導入されました。Orion 1.14.0 まで `dateExpires` は特別な意味を持たない通常の属性として解釈されます。Orion 1.15.0 にアップグレードする前に、アプリケーションで `dateExpires` という名前の付いた属性をすでに使用している場合はどうなりますか？

`dateExpires` を使用している既存のエンティティは、属性が更新されるまで同じ方法で使用し続けます。つまり、`dateExpires` が `DateTime` でない場合、例えば、GET オペレーションなどで、数値、通常の文字列など、同じ値を保持します。`dateExpires` が `DateTime` である場合、その日時は有効期限と解釈されません。つまり、エンティティは日時が経過しても削除されません。

しかし、属性が特別な意味を持たずに前の値を保持する場合でも、`dataExpires` は組み込み属性になりますので、GET ベースのクエリでは `attrs` URI パラメータで、`POST /v2/op/query` では `"attrs"` フィールドで明示的に要求されている場合を除いては表示されません。

`dateExpires` 属性が初めて更新されると、それは前のセクションで説明した振る舞いで、指定されたエンティティの有効期限を意味するようになります。 **エンティティが望ましくない方法で自動的に削除される可能性があるので、その属性の値に基づいてクライアント側の有効期限を実装している場合は、これを考慮してください**。
