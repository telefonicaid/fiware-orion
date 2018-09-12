# 更新アクション・タイプ (Update action types)

`POST /v1/updateContext` (NGSIv1) と `POST /v2/op/update` はどちらも `actionType` フィールドを使用します。値は次のものです :

* [`append`](#append) (NGSIv2) または `APPEND` (NGSIv1)
* [`appendStrict`](#appendstrict) (NGSIv2) または `APPEND_STRICT` (NGSIv1)
* [`update`](#update) (NGSIv2) または `UPDATE` (NGSIv1)
* [`delete`](#delete) (NGSIv2) または `DELETE` (NGSIv1)
* [`replace`](#replace) (NGSIv2) または `REPLACE` (NGSIv1)

actionType の値については、次のサブセクションで説明します。NGSIv2 の場合、RESTful なオペレーションと同等のものが記述されています (詳細は [NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)を参照してください)。NGSIv1 の場合には、同様のオペレーションがコンビニエンス・オペレーションとしてあります ([最後の例](#example-about-creation-and-removal-of-attributes-in-ngsiv1)はそれらを例示しています)。

## `append`

このアクション・タイプは、エンティティの作成、既存エンティティの属性の作成、および既存エンティティの既存属性の更新に使用されます。後者の場合は、`update` と同じです。

NGSIv2 では、`POST /v2/entities` (エンティティがまだ存在しない場合) または `POST /v2/entities/<id>/attrs` (エンティティがすでに存在する場合) にマップされます。

## `appendStrict`

このアクション・タイプは、既存エンティティのエンティティまたは属性の作成に使用されます。既存属性を更新するためにこの属性を使用しようとすると (`append` が許可するように)、エラーが発生します。

NGSIv2 では、`POST /v2/entities` (エンティティがまだ存在しない場合) または `POST /v2/entities/<id>/attrs?options=append` (エンティティがすでに存在する場合) にマップされます。

## `update`

このアクション・タイプは、既存属性の変更に使用されます。新しいエンティティまたは属性 (`append` または `appendStrict` が許可するように) を作成するためにそれを使用しようとすると、エラーが発生します。

NGSIv2 では、それは `PATCH /v2/entities/<id>/attrs` にマップされます。

## `delete`

このアクション・タイプは、既存のエンティティ (エンティティ自体を削除せずに) の属性の削除やエンティティの削除に使用されます。

NGSIv2 では、それは、エンティティに含まれるすべての属性の `DELETE/v2/entities/<id>/attrs/<attrName>` にマッピングされるか、エンティティに属性がない場合は `DELETE /v2/entities/<id>` を削除します。

## `replace`

このアクション・タイプは、既存エンティティの属性の置換に使用されます。つまり、既存のすべての属性が削除され、リクエストに含まれる属性が追加されます。

NGSIv2 では、それは `PUT /v2/entities/<id>/attrs` にマップされます。

## NGSIv1 での属性の作成と削除の例

[新しいエンティティを作成](walkthrough_apiv1.md#entity-creation)するために、APPEND アクション・タイプで updateContext を使用する方法を見てきました。さらに、APPEND を使用して、エンティティ作成後に新しい属性を追加することができます。これを例を使って説明しましょう。

最初に、'A' という名前の属性を持つ単純なエンティティ 'E1' を作成します :
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
```

さて、新しい属性を追加するために (それを 'B' と名づけましょう)、'E1' と一致する entityId で updateContext APPEND を使用します :

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "TA",
                    "value": "1"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```
A と B の両方の属性が存在することをそのエンティティへのクエリで確認できます :

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool)<<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "B",
                "type": "TB",
                "value": "2"
            },
            {
                "name": "A",
                "type": "TA",
                "value": "1"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```

APPEND は、既存のコンテキスト要素の UPDATE として解釈されます。ただし、APPEND ではなく APPEND_STRICT を updateAction として使用できます。APPEND_STRICT を使用すると、既存の属性は更新されませんが、エラーが報告されます。APPEND_STRICT リクエストにいくつかの属性 (A や B など) が含まれていて、存在するものと存在しないもの (A が存在し、B が存在しないものなど) がある場合、存在しない属性が追加されます、B が追加されます)、既存のものについてエラーが報告されます (この場合、すでに存在する A についてのエラーが報告されます)。

DELETE アクション・タイプを使用して、同様の方法で属性を削除することもできます。たとえば、属性 'A' を削除するには、次のようにします (空の contextValue 要素に注意してください) :

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "TA",
                    "value": ""
                }
            ]
        }
    ],
    "updateAction": "DELETE"
}
EOF
```

今、エンティティのクエリは属性 B を示します :

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "B",
                "type": "TB",
                "value": "2"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```


POST および DELETE メソッドを使用したコンビニエンス・オペレーションを使用して、属性を追加および削除することもできます。以下を試してください :

新しい属性 'C' と 'D' を追加します :

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "attributes": [
        {
            "name": "C",
            "type": "TC",
            "value": "3"
        },
        {
            "name": "D",
            "type": "TD",
            "value": "4"
        }
    ]
}
EOF
```

属性 'B' を削除 :

```
curl localhost:1026/v1/contextEntities/E1/attribute/B -s -S \
    --header 'Content-Type: application/json'  -X DELETE  \
    --header 'Accept: application/json'  | python -mjson.tool
```

エンティティのクエリ。'C' と 'D' は表示されますが、'B' は表示されません :


```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "C",
                "type": "TC",
                "value": "3"
            },
            {
                "name": "D",
                "type": "TD",
                "value": "4"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```

特定のエンティティから個々の属性を削除することとは別に、すべての属性とそれに対応するメタデータを含むエンティティ全体を削除することもできます。そのためには、次の例のように、actionType として DELETE を使用し、属性のリストを除いた updateContext オペレーションが使用されます :

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1"
        }
    ],
    "updateAction": "DELETE"
}
EOF
```

次の同等のコンビニエンス・オペレーションを使用することもできます :
```
curl localhost:1026/v1/contextEntities/E1 -s -S \
    --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -X DELETE
```

