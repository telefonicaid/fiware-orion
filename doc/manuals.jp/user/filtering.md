# <a name="top"></a> 結果のフィルタリング (Filtering results)

* [イントロダクション](#introduction)
* [シンプルなクエリ言語](#simple-query-language)
* [地理的なクエリ](#geographical-queries)
    
<a name="introduction"></a>
## イントロダクション

NGSIv2 は、シンプルなクエリ言語と地理的クエリを実装しています。これらは、同期クエリ (例、`GET /v2/entities`) とサブスクリプション・通知 (`subject.condition.expression` フィールド内) の両方で使用できます。

[トップ](#top)

<a name="simple-query-language"></a>
### シンプルなクエリ言語

シンプルなクエリ言語では、エンティティ属性が一致しなければならない条件を定義できます。たとえば、属性 "temperature" は 40 より大きくなければなりません。これは、属性メタデータに対する条件を定義するためにも使用できます。属性 "metadata" のメタデータ "accuracy" は 0.9 より大きくなければなりません。

[API ウォークスルーのこのセクション](walkthrough_apiv2#query-entity)に例があります。完全な構文定義は、[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Simple Query Language" セクションにあります。

[トップ](#top)

<a name="geographical-queries"></a>
## 地理的なクエリ

地理的クエリは、例えばマドリードの中心から 15km 近くに位置するすべてのエンティティなど、地理的位置によってフィルタリングすることを可能にする。もちろん、適切に配置されたエンティティは必須です。

両方のトピック (エンティティの位置と地理的クエリ) は [NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Geospacial properties of entities" と "Geographical Queries" のセクションで詳細に取り扱っています。

[トップ](#top)
