# 属性メタデータ (Attribute Metadata)

Orion が特別な注意を払うメタデータ要素 (たとえば、`dateCreated` など) とは別に、ユーザはエンティティ属性に独自のメタデータを添付できます。これらのメタデータ要素は、Orion によって透過的に処理されます : Orion は更新時にそれらをデータベースに保存し、クエリおよび通知時にそれらを取得します。

Orion によって解釈される特別なメタデータに使用されるいくつかの予約された名前を除いて、カスタム・メタデータに任意の名前を使用できます :

-   [ID](#metadata-id-for-attributes) (廃止されましたが、それでも禁止されたキーワードとして "ブロック" されています)
-   "location" は現在 [非推奨](../deprecated.md)ですが、引き続きサポートされています
-   NGSIv2 仕様の "組み込みメタデータ" のセクションで定義されているもの

その管理は NGSIv1 と NGSIv2 では若干異なりますので、別のセクションで説明します。

## カスタム属性メタデータ (NGSIv2 を使用)

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

NGSIv1 とは異なり、NGSIv2 は更新される属性値にかかわらずメタデータを更新するオペレーションを定義していません。さらに、属性作成後にメタデータを追加するオペレーションも定義していません。言い換えれば、メタデータ配列全体が、`PUT /v2/entities/{id}/attrs/{attrName}` オペレーションの属性値および型と共に更新されます。

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

## カスタム属性メタデータ (NGSIv1 を使用)

たとえば、属性 "temperature" を持つエンティティ Room1 を作成し、メタデータ "accuracy" を "temperature" に関連付けるには、次のようにします :

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "accuracy",
                            "type": "float",
                            "value": "0.8"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF 
``` 
  
更新される属性値に関係なく、メタデータを更新できます。たとえば、次の updateContext は、"temperature" 自身の値がまだ 26.5 であるにも関わらず、"accuracy" が 0.9 に更新される方法を示します :

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "accuracy",
                            "type": "float",
                            "value": "0.9"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
``` 
      
属性の作成後にメタデータを追加することができます。たとえば、既存の "precision" に加えて、"temperature" に "average" というメタデータを追加する場合は、次のようになります :

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "average",
                            "type": "float",
                            "value": "22.4"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
```
"temperature" には両方の属性が含まれていることを確認できます

``` 
(curl localhost:1026/v1/contextEntities/Room1 -s -S \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "average",
                            "type": "float",
                            "value": "22.4"
                        },
                        {
                            "name": "accuracy",
                            "type": "float",
                            "value": "0.9"
                        }
                    ]
                }
            ]
        }
    ],
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
``` 
      
[サブスクリプション](walkthrough_apiv1.md#context-subscriptions)の観点から見ると、属性値自体が変更されていなくても、指定された属性のメタデータの変更や新しいメタデータ要素の追加は変更と見なされます。メタデータ要素を削除するには、メタデータ要素を削除するには、エンティティ属性を削除 ([DELETE アクション・タイプ](update_action_types.md#delete)を参照してください) し、再作成 ([APPEND アクション・タイプ](update_action_types.md#append)を参照してください) する必要があります。

## 通知中のメタデータ

デフォルトでは、すべてのカスタム (ユーザ) メタデータが通知に含まれます。ただし、このフィールド `metadata` を使用してリストをフィルタリングすることができます。さらに、次の特殊メタデータ (デフォルトでは含まれていません) を含める必要があることを指定するために使用できます。

* previousValue
* actionType

それらの意味の詳細は、NGSIv2 仕様の "組み込みメタデータ" のセクションで見つけることができます。

以下を使用することに注意してください :

```
"metadata": [ "previousValue" ]
```

`previousValue` を含むことになりますが、通知内の属性が持つユーザメタデータは除外されます。`previousValue` とその他の "regular" メタデータを取得する場合は、次のようにします :

```
"metadata": [ "previousValue", "*" ]
```

`"metadata": [ "*" ]` を使用することもできますが、メタデータをまったく含まないのと同じ結果が得られるため、あまり意味がありません。デフォルトの動作はすべてのユーザ・メタデータを含めることです。

NGSIv2 仕様の "Filtering out attributes and metadata" の項を参照してください。
