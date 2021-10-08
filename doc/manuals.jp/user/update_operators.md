# 属性値の更新演算子

## イントロダクション

Orion でコンテキストを変更する通常の方法は、特定の値で属性を更新することです。ただし、場合によっては、
既存の属性値で評価される操作を提供する方がよい場合があります。 例を挙げて見てみましょう。

アクセス制御を通過する人の数を測定する属性 `count` を持つエンティティ `AccessControl1` を考えてみましょう。
コンテキスト・アウェア・アプリケーションは、コンテキストの更新を担当します (たとえば、センサ測定に基づく)。
誰かがアクセスを通過するたびに、アプリケーションは次のことを行う必要があります:

* 属性の値を読み取ります。例: `GET /v2/entities/attrs/count`。値が43であると考えてみましょう
* 新しい値を計算します (43 + 1は44です)
* 属性を新しい値で更新します。例: `PUT /v2/entities/attrs/count { "value": 44, "type": "Number" }`

要するに、アプリケーションは読み取り、計算、更新を行う必要があるため、複雑さが増します。

複数のコンテキスト・アウェア・アプリケーションが同じコンテキストに同時にアクセスしている場合、問題はさらに
悪化します。たとえば、単一のアクセス・カウントの代わりに、属性 `count` を持つ `Zone1` があり、どのアクセスに
関係なく、ゾーンに入るすべての人の集計があります (ゾーンに多くのアクセスがあると仮定します)。

アプリケーションAがアクセスの1つを管理し、アプリケーションBが他のアクセスを管理しています。どちらの
アプリケーションも、誰かがそれぞれのアクセスを通過すると、`Zone1` の `count` 属性を更新します。
ほとんどの場合問題はありません:

* 誰かがAによって管理されているアクセスを通過します
* アプリケーションAは、read-calculate-cycle を実行し、`Zone1` の `count` を更新します。更新前の値が43
  だったとしましょう。現在、`count` 値は44です
* しばらくすると、誰かがBによって管理されているアクセスを通過します
* アプリケーションBは、read-calculate-cycle を実行し、`Zone1` の `count` を更新します。したがって、
  `count`値は45になります

ただし、両方のアクセス交差イベントが非常に近い時間に発生すると、問題が発生する可能性があります:

* 誰かがアクセスAを通過すると同時に、誰かがアクセスBを通過します
* アプリケーションAはカウント43を読み取ります
* アプリケーションBは、Aが計算および更新する前にカウントを読み取るため、同じ値を取得します43
* アプリケーション43に1足して、44で更新します
* アプリケーションBも同じことを行い、44で更新します
* したがって、カウントは間違ってしまいます。45である必要がありますが、44です!

この種の問題を*競合状態*と呼びます (アプリケーションAとBでイベントが発生する速度が異なるため)。これは、
Orion 更新演算子機能、特にインクリメント演算子を使用して解決できます。

したがって、両方のアプリケーションは、特定の値 (44や45など) を直接更新する代わりに、次の方法で
*"increment by 1"* (1だけ増加) の更新を送信します:

```
POST /v2/entities/Zone1/attrs/count
{
  "value": { "$inc": 1 },
  "type": "Number"
}
```

同じケースでは次のようになります:

* ある時点で、ゾーン1のカウントは43であり、誰かがアクセスAを通過すると同時に、誰かがアクセスBを
  通過します
* アプリケーションAは `count` を `{"$ inc"：1}` で更新します
* アプリケーションBは `count` を `{"$ inc"：1}` で更新します
* Orion は、操作がアトミックに実行されることを保証します。アプリケーションAによって行われた
  インクリメントが、アプリケーションBによって行われたインクリメントの前に来るか、またはその逆で
  あるかは関係ありません。結果は同じです：45

したがって、更新演算子は両方の問題を解決します:

* 複雑さ。これで、アプリケーションは読み取り-計算-更新サイクルを実行する必要がなくなります。
 演算子を使用して、更新するだけで十分です
* 競合状態。これは、Orion がこれらの操作がコンテキスト属性でアトミックに実行されることを保証するために
  解決されます。多くのアプリケーションは、同じコンテキストを問題なく更新できます

## サポートされている演算子

Orion 更新演算子は、MongoDB によって実装されたもののサブセットに基づいています
([ここ](https://docs.mongodb.com/manual/reference/operator/update/)に説明があります)。
説明は次のとおりです。

### `$inc`

与えられた値だけ増加します。

たとえば、エンティティEの属性Aの既存の値が10の場合、次のリクエストは：

```
POST /v2/entities/E/attrs/A
{
  "value": { "$inc": 2 },
  "type": "Number"
}
```

属性Aの値を12に変更します。

この演算子は、数値 (正または負、整数または10進数) のみを受け入れます:

### `$mul`

与えられた値を乗算します。

たとえば、エンティティEの属性Aの既存の値が10の場合、次のリクエストは：
```
POST /v2/entities/E/attrs/A
{
  "value": { "$mul": 2 },
  "type": "Number"
}
```

属性Aの値を20に変更します。

この演算子は、数値 (正または負、整数または10進数) のみを受け入れます。

### `$min`

現在の値が提供されている値よりも大きい場合、値を更新します。

たとえば、エンティティEの属性Aの既存の値が10の場合、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$min": 2 },
  "type": "Number"
}
```

属性Aの値を2に変更します。ただし、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$min": 20 },
  "type": "Number"
}
```

属性値は変更されません。

数値とは別に、他の値タイプ (文字列など) がサポートされています。

### `$max`

現在の値が提供されている値よりも小さい場合、値を更新します。

たとえば、エンティティEの属性Aの既存の値が10の場合、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$max": 12 },
  "type": "Number"
}
```

属性Aの値を12に変更します。ただし、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$max": 4 },
  "type": "Number"
}
```

属性値は変更されません。

数値とは別に、他の値タイプ (文字列など) がサポートされています。

### `$push`

値が配列である属性で使用するには、配列に項目を追加します。

たとえば、エンティティEの属性Aの既存の値が `[1, 2, 3]` の場合、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$push": 3 },
  "type": "Number"
}
```

属性Aの値を `[1, 2, 3, 3]` に変更します。

### `$addToSet`

push に似ていますが、重複を避けます。

たとえば、エンティティEの属性Aの既存の値が `[1, 2, 3]` の場合、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$addToSet": 4 },
  "type": "Number"
}
```

属性Aの値を `[1, 2, 3, 4]` に変更します。ただし、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$addToSet": 3 },
  "type": "Number"
}
```

属性値は変更されません。

### `$pull`

値が配列である属性で使用するには、パラメータとして渡されたアイテムのすべてのオカレンスを
削除します。

たとえば、エンティティEの属性Aの既存の値が `[1, 2, 3]` の場合、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$pull": 2 },
  "type": "Number"
}
```

属性Aの値を `[1, 3]` に変更します。

### `$pullAll`

値が配列である属性で使用されます。パラメータも配列です。 パラメータとして使用される配列の
メンバのいずれかのオカレンスがすべて削除されます。

たとえば、エンティティEの属性Aの既存の値が `[1, 2, 3]` の場合、次のリクエストは:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$pullAll": [2, 3] },
  "type": "Number"
}
```

属性Aの値を `[1]` に変更します。

## Orion がオペレータをどのように扱うか

Orion は操作自体を実行しませんが、MongoDB に渡します。MongoDB は、データベースに格納されている属性値で
実際に実行されます。したがって、実行セマンティクスは、同等のオペランドについて
[MongoDB ドキュメント](https://docs.mongodb.com/manual/reference/operator/update/)
で説明されているものです。

操作の結果、MongoDB レベルでエラーが発生した場合、エラーはそのままクライアント・レスポンスの
500 Internal Error として処理されます。たとえば、`$inc` 演算子は MongoDB の数値のみをサポートします。
したがって、このリクエストを送信すると:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$inc": "foo" },
  "type": "Number"
}
```

結果はこのエラーになります:

```
500 Internal Server Error

{"error":"InternalServerError","description":"Database Error &#40;collection: orion.entities - update&#40;&#41;: &lt;{ &quot;_id.id&quot; : &quot;E&quot;, &quot;_id.type&quot; : &quot;T&quot;, &quot;_id.servicePath&quot; : &quot;/&quot; },{ &quot;$set&quot; : { &quot;attrs.A.type&quot; : &quot;Number&quot;, &quot;attrs.A.mdNames&quot; : [  ], &quot;attrs.A.creDate&quot; : 1631801113.0986146927, &quot;attrs.A.modDate&quot; : 1631801407.5359125137, &quot;modDate&quot; : 1631801407.5359227657, &quot;lastCorrelator&quot; : &quot;cbe6923c-16f7-11ec-977e-000c29583ca5&quot; }, &quot;$unset&quot; : { &quot;attrs.A.md&quot; : 1, &quot;location&quot; : 1, &quot;expDate&quot; : 1 }, &quot;$inc&quot; : { &quot;attrs.A.value&quot; : &quot;foo&quot; } }&gt; - exception: Cannot increment with non-numeric argument: {attrs.A.value: &quot;foo&quot;}&#41;"}
```

これをデコードして:

```
"error":"InternalServerError","description":"Database Error (collection: orion.entities - update(): <{ "_id.id" : "E", "_id.type" : "T", "_id.servicePath" : "/" },{ "$set" : { "attrs.A.type" : "Number", "attrs.A.mdNames" : [  ], "attrs.A.creDate" : 1631801113.0986146927, "attrs.A.modDate" : 1631801407.5359125137, "modDate" : 1631801407.5359227657, "lastCorrelator" : "cbe6923c-16f7-11ec-977e-000c29583ca5" }, "$unset" : { "attrs.A.md" : 1, "location" : 1, "expDate" : 1 }, "$inc" : { "attrs.A.value" : "foo" } }> - exception: Cannot increment with non-numeric argument: {attrs.A.value: "foo"})"}
```

最後を見ると、MongoDB によって報告されたエラーを見ることができます:

```
Cannot increment with non-numeric argument: {attrs.A.value: "foo"})"}
```

さらに、Orion は、リクエストの属性の値が1つのキー (演算子) のみの JSON オブジェクトであると
想定していることに注意してください。 あなたがこのような奇妙なことをした場合:

```
POST /v2/entities/E/attrs/A
{
  "value": {
    "x": 1
    "$inc": 1,
    "$mul": 10
  },
  "type": "Number"
}
```

（原則としてランダムに）この中の1つが実行されます:

* Aの値が1増加します
* Aはその値に10を掛けます
* Aは、(文字通り) このJSONオブジェクトに更新された値です: `{ "x": 1, "$inc": 1, "$mul": 10 }

したがって、この状況を回避するように注意してください。

## 現在の制限

### エンティティの作成または置換

更新演算子は、エンティティの作成または置換操作では使用できません。たとえば、この方法で
エンティティを作成する場合:

```
POST /v2/entities/E/attrs/A
{
  "id": "E",
  "type": "T",
  "A": {
    "value": { "$inc": 2 },
    "type": "Number"
  }
}
```

作成されたばかりのエンティティの属性Aは、(文字通り) 次の JSON オブジェクトを値として持ちます
: `{ "$inc": 2 }`

ただし、既存のエンティティに新しい属性を追加する場合は機能することに注意してください。
たとえば、属性AとBを持つエンティティEがすでにあり、この方法でCを追加する場合:

```
POST /v2/entities/E
{
  "C": {
    "value": { "$inc": 2 },
    "type": "Number"
  }
}
```

Cは値 `2` で作成されます。
