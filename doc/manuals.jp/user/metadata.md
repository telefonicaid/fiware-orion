# 属性メタデータ (Attribute Metadata)

Orion が特別な注意を払うメタデータ要素 (たとえば、`dateCreated` など) とは別に、ユーザはエンティティ属性に独自のメタデータを添付できます。これらのメタデータ要素は、Orion によって透過的に処理されます : Orion は更新時にそれらをデータベースに保存し、クエリおよび通知時にそれらを取得します。

Orion によって解釈される特別なメタデータに使用されるいくつかの予約された名前を除いて、カスタム・メタデータに任意の名前を使用できます :

-   "location" は現在 [非推奨](../deprecated.md)ですが、引き続きサポートされています
-   NGSIv2 仕様の "組み込みメタデータ" のセクションで定義されているもの

## カスタム属性メタデータ

たとえば、属性 "temperature" を持つエンティティ Room1 を作成し、メタデータ "accuracy" を "temperature" に関連付けるには、次のようにします :

```
curl localhost:1026/v2/entities -s -S --header 'Content-Type: application/json' \
    -d @- <<EOF
{
  "id": "Room1",
  "type": "Room",
  "temperature": {
    "value": 26.5,
    "type": "Float",
    "metadata": {
      "accuracy": {
        "value": 0.8,
        "type": "Float"
      }
    }
  }
}
EOF
```

NGSIv2 は更新される属性値にかかわらずメタデータを更新するオペレーションを定義していません。さらに、属性作成後にメタデータを追加するオペレーションも定義していません。言い換えれば、メタデータ配列全体が、`PUT /v2/entities/{id}/attrs/{attrName}` オペレーションの属性値および型と共に更新されます。

温度にメタデータが含まれていることを確認できます :

```
curl localhost:1026/v2/entities/Room1 -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

どのレスポンスが

```
{
    "id": "Room1",
    "temperature": {
        "metadata": {
            "accuracy": {
                "type": "Float",
                "value": 0.8
            }
        },
        "type": "Float",
        "value": 26.5
    },
    "type": "Room"
}
```

現時点では、NGSIv2 では一度導入された個々のメタデータ要素を削除することはできません。ただし、`metadataset` を使用して属性を `{}` に更新するすべてのメタデータを削除できます。

[サブスクリプション](walkthrough_apiv2.md#subscriptions)の観点から見ると、属性値自体は変更されていなくても、特定の属性のメタデータの変更は変更とみなされます。

<a name="updating-metadata"></a>
## メタデータの更新

属性が更新されると、次のルールが適用されます:

* *以前に存在しなかった*属性更新リクエストに含まれるメタデータが属性に追加されます
* *既存の*属性更新リクエストに含まれるメタデータが属性で更新されます
* リクエストに含まれていない既存のメタデータは、属性に変更されません (つまり、
  以前の値を保持します)

たとえば、メタデータ `unit` と `avg` を持つ属性 `temperature` を考えてみましょう。これらの値は、
現時点では次のとおりです:

* `unit`: `"celsius"`
* `avg`: `25.4`

Context Broker はこのようなリクエストを受け取ります:

```
PUT /v2/entities/E/attrs/temperature
{
  "value": 26,
  "type": "Number",
  "metadata": {
    "avg": {
      "value": 25.6,
      "type": "Number"
    },
    "accuracy": {
      "value": 98.7,
      "type": "Number"
    }
  }
}
```

更新を処理した後、属性 `temperature` のメタデータは次のようになります:

* `unit`: `"celsius"` (既存であり、リクエストで触れていない)
* `avg`: `25.6` (既存だが、リクエストで触れた)
* `accuracy`: `98.7` (リクエストによって追加されたメタデータ)

このデフォルトの動作におけるメタデータの "stikyness" の背後にある理論的根拠は、
[Orion リポジトリのこの Issue](https://github.com/telefonicaid/fiware-orion/issues/4033)
でより詳細に説明されています。

### `overrideMetadata` オプション

`overrideMetadata` オプションを使用して、デフォルトの動作をオーバーライドできます。
その場合、リクエストのメタデータは、属性に存在していた以前のメタデータを置き換えます。
たとえば、以前と同じ初期状況ですが、リクエストに `overrideMetadata` オプションを追加します:

```
PUT /v2/entities/E/attrs/temperature?options=overrideMetadata
{
  "value": 26,
  "type": "Number",
  "metadata": {
    "avg": {
      "value": 25.6,
      "type": "Number"
    },
    "accuracy": {
      "value": 98.7,
      "type": "Number"
    }
  }
}
```

更新を処理した後、属性 `temperature` のメタデータは次のようになります:

* `avg`: `25.6` (既存だが、リクエストで触れた)
* `accuracy`: `98.7` (リクエストによって追加されたメタデータ)

`unit` メタデータが削除されていることに注意してください。

`overrideMetadata` オプションは、リクエストの `metadata` フィールドを省略して
(同等に、`"metadata":{}` を使用して) 特定の属性のメタデータをクリーンアップ
するためにも使用できます。例えば:

```
PUT /v2/entities/E/attrs/temperature?options=overrideMetadata
{
  "value": 26,
  "type": "Number"
}
```

`overrideMetadata` オプションは、属性値の更新操作
(例えば、`PUT /v2/entities/E/attrs/temperature/value`) では無視されることに
注意してください。その場合、操作のセマンティクスは、値のみが更新されることを
明示します (`type` および `metadata` 属性フィールドは変更されません)。
