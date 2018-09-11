# <a name="top"></a> 結果のフィルタリング (Filtering results)

* [イントロダクション](#introduction)
* [NGSIv2 フィルタリング](#ngsiv2-filtering)
    * [シンプルなクエリ言語](#simple-query-language)
    * [地理的なクエリ](#geographical-queries)
* [NGSIv1 フィルタリング](#ngsiv1-filtering)
    * [存在型フィルタ](#existence-type-filter)
    * [存在しない型のフィルタ](#no-existence-type-filter)
    * [エンティティ型フィルタ](#entity-type-filter)
    * [ジオロケーション・フィルタ](#geo-location-filter)
    * [NGSIv2 のジオロケーション・フィルタ](#geo-location-filter-ngsiv2)
    * [文字列のフィルタ](#string-filters)
    
<a name="introduction"></a>
## イントロダクション

NGSIv2 と NGSIv1 は異なるフィルタリング・メカニズムを持っています。たとえば、NGSIv1 では、フィルタリングは `scope` ペイロード要素の使用に大きく依存しています。両方のアプローチ (NGSIv2 と NGSIv1) については、それぞれ別のセクションで説明します。

[トップ](#top)

<a name="ngsiv2-filtering"></a>
## NGSIv2 フィルタリング

NGSIv2 は、シンプルなクエリ言語と地理的クエリを実装しています。これらは、同期クエリ (例、`GET /v2/entities`) とサブスクリプション・通知 (`subject.condition.expression` フィールド内) の両方で使用できます。

<a name="simple-query-language"></a>
### シンプルなクエリ言語

シンプルなクエリ言語では、エンティティ属性が一致しなければならない条件を定義できます。たとえば、属性 "temperature" は40より大きくなければなりません。

[API ウォークスルーのこのセクション](walkthrough_apiv2#query-entity)に例があります。完全な構文定義は、[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Simple Query Language" セクションにあります。

[トップ](#top)

<a name="geographical-queries"></a>
### 地理的なクエリ

地理的クエリは、例えばマドリードの中心から 15km 近くに位置するすべてのエンティティなど、地理的位置によってフィルタリングすることを可能にする。もちろん、適切に配置されたエンティティは必須です。

両方のトピック (エンティティの位置と地理的クエリ) は [NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Geospacial properties of entities" と "Geographical Queries" のセクションで詳細に取り扱っています。

[トップ](#top)

<a name="ngsiv1-filtering"></a>
## NGSIv1 フィルタリング

Orion Context Broker は、NGSI10 クエリ・オペレーションで結果をフィルタリングするために使用できるいくつかのフィルタを実装しています。これらのフィルタは通常、[パターン付き queryContext](walkthrough_apiv1.md#query-context-operation) または [すべてのエンティティを取得するコンビニエンス・オペレーション](walkthrough_apiv1.md#getting-all-entities)で使用されます :

一般的な規則として、標準オペレーションで使用されるフィルタは scope 要素を使用します :

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "myEntityType",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "restriction": {
        "scopes": [
            {
                "type": "FIWARE::Filter::foobar",
                "value": ""
            }
        ]
    }
}
EOF
```

コンビニエンス・オペレーションのフィルタは URL のパラメータとして含まれています :

```
curl localhost:1026/v1/contextEntities?filter=value -s -S  --header 'Content-Type: application/json'  \
    --header 'Accept: application/json' | python -mjson.tool
```
フィルタは累積的です。言い換えると、いくつかのフィルタを指定するために、同じ制限 (標準的なオペレーションの場合) または複数の URL 引数を '&' で区切って複数のスコープを使用できます。結果はそれらの間の "and" です。

[トップ](#top)

<a name="existence-type-filter"></a>
### 存在型フィルタ

この型に対応するスコープは、"FIWARE::Filter::Existence" です。

```
{
    "restriction": {
        "scopes": [
            {
                "type": "FIWARE::Filter::Existence",
                "value": "entity::type"
            }
        ]
    }
}
```
  
このフィルタに対応する URL パラメータは 'exist' です。

    curl localhost:1026/v1/contextEntities?exist=entity::type ...

現在のバージョンでは、存在を確認できる唯一のパラメータは "entity::type" に対応するエンティティ型です。

[トップ](#top)

<a name="no-existence-type-filter"></a>
### 存在しない型のフィルタ

この型に対応するスコープは "FIWARE::Filter::Not::Existence" です。

```
... 
    {
        "restriction": {
            "scopes": [
                {
                    "type": "FIWARE::Filter::Not::Existence",
                    "value": "entity::type"
                }
            ]
        }
    }
...
```
 
このフィルタに対応する URL パラメータは '!exist' です。

    curl localhost:1026/v1/contextEntities?!exist=entity::type ...

現在のバージョンでは、存在しないことをチェックできる唯一のパラメータは、"entity::type" に対応するエンティティ型です。[次のセクション](empty_types.md#using-empty-types)で説明するように、型を持たないクエリが "任意の型 (any type)"に解決されるので、これが "型なしのエンティティ (entity without type)" を選択する唯一の方法であることに注意してください。

[トップ](#top)

<a name="entity-type-filter"></a>
### エンティティ型のフィルタ

通常のエンティティ型を使用できる場合、このフィルタに対応するスコープはありません :

```
...
    {
        "type": "Room",
        "isPattern": "...",
        "id": "..."
    }
...
```
このフィルタに対応する URL パラメータは 'entity::type' です。

    curl localhost:1026/v1/contextEntities?entity::type=Room ...

[トップ](#top)

<a name="geo-location-filter"></a>
### ジオロケーション・フィルタ

この型に対応するスコープは、"FIWARE::Location" です。詳細は [次のセクション](geolocation.md#geo-located-queries)で説明します。

Orion の現在のバージョンでは、同等のコンビニエンス・オペレーション・フィルタはありません。

[トップ](#top)

<a name="geo-location-filter-ngsiv2"></a>
### NGSIv2 のジオロケーション・フィルタ

この型に対応するスコープは "FIWARE::Location::NGSIv2" です。詳細は [次のセクション](geolocation.md#geo-located-queries-ngsiv2)で説明します。

[トップ](#top)

<a name="string-filters"></a>
### 文字列のフィルタ

2つの異なる型の文字列フィルタをサポート :

* 属性をフィルタリング (scope type: "FIWARE::StringQuery") と
* 属性のメタデータをフィルタリング (scope type: "FIWARE::StringQuery::Metadata")

これらのスコープでは、equality, unequality, greater/less than, in-array, range or existence (属性と属性のメタデータの) などのフィルタ条件を表現できます。

NGSIv1 には、これらのフィルタに対応する URL パラメータはありません。NGSIv2 では、`q` および `mq` URI パラメータに対応します。

文字列フィルタおよび `q` や `mq` パラメータの値の詳細な構文の説明について、[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)を参照してください。

これらのスコープは NGSIv1 で使用できますが、属性値を数値に設定するには、NGSIv2 を使用する必要があります (NGSIv1 は常に文字列を値として使用します)。


#### 属性のフィルタリング

この型に対応するスコープは "FIWARE::StringQuery" です。

```
...
    {
        "restriction": {
            "scopes": [
                {
                    "type": "FIWARE::StringQuery",
                    "value": "temperature<24;humidity==75..90;status==running"
                }
            ]
        }
    }
...
```

NGSIv2 では、このスコープを表すために `q` URI パラメータをクエリで使用できます :

    curl 'localhost:1026/v2/entities?q=temperature<24;humidity==75..90;status==running'

#### メタデータのフィルタリング

この型に対応するスコープは "FIWARE::StringQuery::Metadata" です。

```
...
    {
        "restriction": {
            "scopes": [
                {
                    "type": "FIWARE::StringQuery::Metadata",
                    "value": "temperature.accuracy<1;humidity.accuracy==1..2"
                }
            ]
        }
    }
...
```

NGSIv2 では、このスコープを表すために `mq` URI パラメータをクエリで使用できます :

    curl 'localhost:1026/v2/entities?mq=temperature.accuracy<1;humidity.accuracy==1..2'

[トップ](#top)
