# 属性メタデータ (Attribute Metadata)

Orion が特別な注意を払うメタデータ要素 (たとえば、`dateCreated` など) とは別に、ユーザはエンティティ属性に独自のメタデータを添付できます。これらのメタデータ要素は、Orion によって透過的に処理されます : Orion は更新時にそれらをデータベースに保存し、クエリおよび通知時にそれらを取得します。

Orion によって解釈される特別なメタデータに使用されるいくつかの予約された名前を除いて、カスタム・メタデータに任意の名前を使用できます :

-   [ID](#metadata-id-for-attributes) (廃止されましたが、それでも禁止されたキーワードとして "ブロック" されています)
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
