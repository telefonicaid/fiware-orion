# 更新アクション・タイプ (Update action types)

`POST /v2/op/update` は `actionType` フィールドを使用します。値は次のものです :

* [`append`](#append)
* [`appendStrict`](#appendstrict)
* [`update`](#update)
* [`delete`](#delete)
* [`replace`](#replace)

actionType の値については、次のサブセクションで説明します。RESTful な操作と同等のものは、[NGSIv2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/) にも記載されています。

## `append`

このアクション・タイプは、エンティティの作成、既存エンティティの属性の作成、および既存エンティティの既存属性の更新に使用されます。後者の場合は、`update` と同じです。

`POST /v2/entities` (エンティティがまだ存在しない場合) または `POST /v2/entities/<id>/attrs` (エンティティがすでに存在する場合) にマップされます。

## `appendStrict`

このアクション・タイプは、既存エンティティのエンティティまたは属性の作成に使用されます。既存属性を更新するためにこの属性を使用しようとすると (`append` が許可するように)、エラーが発生します。

`POST /v2/entities` (エンティティがまだ存在しない場合) または `POST /v2/entities/<id>/attrs?options=append` (エンティティがすでに存在する場合) にマップされます。

## `update`

このアクション・タイプは、既存属性の変更に使用されます。新しいエンティティまたは属性 (`append` または `appendStrict` が許可するように) を作成するためにそれを使用しようとすると、エラーが発生します。

`PATCH /v2/entities/<id>/attrs` にマップされます。

## `delete`

このアクション・タイプは、既存のエンティティ (エンティティ自体を削除せずに) の属性の削除やエンティティの削除に使用されます。

エンティティに含まれるすべての属性の `DELETE/v2/entities/<id>/attrs/<attrName>` にマッピングされるか、エンティティに属性がない場合は `DELETE /v2/entities/<id>` を削除します。

## `replace`

このアクション・タイプは、既存エンティティの属性の置換に使用されます。つまり、既存のすべての属性が削除され、リクエストに含まれる属性が追加されます。

`PUT /v2/entities/<id>/attrs` にマップされます。
