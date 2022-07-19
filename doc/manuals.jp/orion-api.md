# FIWARE-NGSI v2 (release 2.1) 仕様

<!-- TOC -->

- [はじめに](#preface)
- [仕様](#specification)
    - [イントロダクション](#introduction)
    - [用語](#terminology)
        - [コンテキスト・データのモデリングと交換 (Context data modelling and exchange)](#context-data-modelling-and-exchange)
            - [コンテキストのエンティティ (Context Entities)](#context-entities)
            - [コンテキストの属性 (Context Attributes)](#context-attributes)
            - [コンテキストのメタデータ (Context Metadata)](#context-metadata)
    - [MIME タイプ (MIME Types)](#mime-types)
    - [JSON エンティティ表現 (JSON Entity Representation)](#json-entity-representation)
    - [JSON 属性表現 (JSON Attribute Representation)](#json-attribute-representation)
    - [簡略化されたエンティティ表現 (Simplified Entity Representation)](#simplified-entity-representation)
    - [部分表現 (Partial Representations)](#partial-representations)
    - [特殊な属性型 (Special Attribute Types)](#special-attribute-types)
    - [組み込み属性 (Builtin Attributes)](#builtin-attributes)
    - [特殊なメタデータ型 (Special Metadata Types)](#special-metadata-types)
    - [組み込みメタデータ (Builtin Metadata)](#builtin-metadata)
    - [フィールド構文の制限事項 (Field syntax restrictions)](#field-syntax-restrictions)
    - [属性名の制限 (Attribute names restrictions)](#attribute-names-restrictions)
    - [メタデータ名の制限 (Metadata names restrictions)](#metadata-names-restrictions)
    - [結果の順序付け (Ordering Results)](#ordering-results)
    - [エラー・レスポンス (Error Responses)](#error-responses)
    - [エンティティの地理空間プロパティ (Geospatial properties of entities)](#geospatial-properties-of-entities)
        - [シンプル・ロケーション・フォーマット (Simple Location Format)](#simple-location-format)
        - [GeoJSON](#geojson)
    - [シンプル・クエリ言語 (Simple Query Language)](#simple-query-language)
    - [地理的クエリ (Geographical Queries)](#geographical-queries)
        - [クエリの解決 (Query Resolution)](#query-resolution)
    - [属性とメタデータのフィルタリング (Filtering out attributes and metadata)](#filtering-out-attributes-and-metadata)
    - [通知メッセージ (Notification Messages)](#notification-messages)
    - [カスタム通知 (Custom Notifications)](#custom-notifications)
- [API ルート (API Routes)](#api-routes)
    - [API エントリ・ポイント (API Entry Point)](#api-entry-point)
        - [API リソースを取得 [GET /v2]](#retrieve-api-resources-get-v2)
    - [エンティティの操作 (Entities Operations)](#entities-operations)
        - [エンティティのリスト (Entities List)](#entities-list)
            - [エンティティをリスト [GET /v2/entities]](#list-entities-get-v2entities)
            - [エンティティを作成  [POST /v2/entities]](#create-entity-post-v2entities)
        - [id によるエンティティの操作 (Entity by ID)](#entity-by-id)
            - [エンティティを取得 [GET /v2/entities/{entityId}]](#retrieve-entity-get-v2entitiesentityid)
            - [エンティティ属性を取得 [GET /v2/entities/{entityId}/attrs]](#retrieve-entity-attributes-get-v2entitiesentityidattrs)
            - [エンティティ属性の更新または追加 [POST /v2/entities/{entityId}/attrs]](#update-or-append-entity-attributes-post-v2entitiesentityidattrs)
            - [既存のエンティティ属性の更新 [PATCH /v2/entities/{entityId}/attrs]](#update-existing-entity-attributes-patch-v2entitiesentityidattrs)
            - [すべてのエンティティ属性を置換 [PUT /v2/entities/{entityId}/attrs]](#replace-all-entity-attributes-put-v2entitiesentityidattrs)
            - [エンティティを削除する [DELETE /v2/entities/{entityId}]](#remove-entity-delete-v2entitiesentityid)
        - [属性 (Attributes)](#attributes)
            - [属性データを取得 [GET /v2/entities/{entityId}/attrs/{attrName}]](#get-attribute-data-get-v2entitiesentityidattrsattrname)
            - [属性データを更新 [PUT /v2/entities/{entityId}/attrs/{attrName}]](#update-attribute-data-put-v2entitiesentityidattrsattrname)
            - [単一の属性を削除 [DELETE /v2/entities/{entityId}/attrs/{attrName}]](#remove-a-single-attribute-delete-v2entitiesentityidattrsattrname)
        - [属性値 (Attribute Value)](#attribute-value)
            - [属性値を取得 [GET /v2/entities/{entityId}/attrs/{attrName}/value]](#get-attribute-value-get-v2entitiesentityidattrsattrnamevalue)
            - [属性値を更新 [PUT /v2/entities/{entityId}/attrs/{attrName}/value]](#update-attribute-value-put-v2entitiesentityidattrsattrnamevalue)
        - [エンティティ型 (Types)](#types)
            - [全エンティティ型のリスト [GET /v2/types]](#list-entity-types-get-v2types)
            - [特定の型のエンティティ情報を取得 [GET /v2/types/{entityType}]](#retrieve-entity-information-for-a-given-type-get-v2types)
    - [サブスクリプションの操作 (Subscriptions Operations)](#subscriptions-operations)
        - [サブスクリプション・ペイロード・データモデル](#subscription-payload-datamodel)
            - [`subscription`](#subscription)
            - [`subscription.subject`](#subscriptionsubject)
            - [`subscription.subject.condition`](#subscriptionsubjectcondition)
            - [`subscription.notification`](#subscriptionnotification)
            - [`subscription.notification.http`](#subscriptionnotificationhttp)
            - [`subscription.notification.httpCustom`](#subscriptionnotificationhttpcustom)
        - [サブスクリプションのリスト](#subscription-list)
            - [サブスクリプションをリスト [GET /v2/subscriptions]](#list-subscriptions-get-v2subscriptions)
            - [サブスクリプションを作成 [POST /v2/subscriptions]](#create-subscription-post-v2subscriptions)
        - [id によるサブスクリプションの操作](#subscription-by-id)
            - [サブスクリプションを取得 [GET /v2/subscriptions/{subscriptionId}]](#retrieve-subscription-get-v2subscriptionssubscriptionid)
            - [サブスクリプションを更新 [PATCH /v2/subscriptions/{subscriptionId}]](#update-subscription-patch-v2subscriptionssubscriptionid)
            - [サブスクリプションを削除 [DELETE /v2/subscriptions/{subscriptionId}]](#delete-subscription-delete-v2subscriptionssubscriptionid)
    - [レジストレーションの操作 (Registration Operations)](#registration-operations)
        - [レジストレーション・ペイロード・データモデル](#registration-payload-datamodel)
            - [`registration`](#registration)
            - [`registration.provider`](#registrationprovider)
            - [`registration.dataProvided`](#registrationdataprovided)
            - [`registration.forwardingInformation`](#registrationforwardinginformation)
        - [レジストレーションのリスト](#registration-list)
            - [レジストレーションをリスト [GET /v2/registrations]](#list-registrations-get-v2registrations)
            - [レジストレーションを作成 [POST /v2/registrations]](#create-registration-post-v2registrations)
        - [id によるレジストレーションの操作](#registration-by-id)
            - [レジストレーションを取得 [GET /v2/registrations/{registrationId}]](#retrieve-registration-get-v2registrationsregistrationid)
            - [レジストレーションを更新 [PATCH /v2/registrations/{registrationId}]](#update-registration-patch-v2registrationsregistrationid)
            - [レジストレーションを削除 [DELETE /v2/registrations/{registrationId}]](#delete-registration-delete-v2registrationsregistrationid)
    - [バッチ操作 (Batch Operations)](#batch-operations)
        - [更新操作 (Update operation)](#update-operation)
            - [更新 [POST /v2/op/update]](#update-post-v2opupdate)
        - [クエリ操作 (Query operation)](#query-operation)
            - [クエリ [POST /v2/op/query]](#query-post-v2opquery)
        - [通知操作 (Notify operation)](#notify-operation)
            - [通知 [POST /v2/op/notify]](#notify-post-v2opnotify)

<!-- /TOC -->

<a name="preface"></a>

# はじめに

これは、[NGSIv2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable) のリリース 2.1 です。2018年9月15日に
リリースされた元の NGSIv2 と完全な下位互換性があります。

<a name="specification"></a>

# 仕様

<a name="introduction"></a>

## イントロダクション

FIWARE NGSI (Next Generation Service Interface) API は、

-   *コンテキスト・エンティティ*の概念を使用した単純な情報モデルに基づく、コンテキスト情報の **データ・モデル**
-   クエリ、サブスクリプション、および更新オペレーションによって情報を交換する**コンテキスト・データ・インターフェイス**
-   コンテキスト情報を取得する方法に関する情報を交換するための **コンテキスト・アベイラビリティ・インタフェース**
    (2つのインタフェースを分離するかどうかは、現在検討中です)

<a name="terminology"></a>

## 用語

<a name="context-data-modelling-and-exchange"></a>

### コンテキスト・データのモデリングと交換 (Context data modelling and exchange)

NGSI データモデルの主な要素は、下図のように、コンテキストのエンティティ、属性およびメタデータです。

![NGSI data model](https://raw.githubusercontent.com/telefonicaid/fiware-orion/master/doc/apiary/v2/Ngsi-data-model.png)

<a name="context-entities"></a>

#### コンテキストのエンティティ (Context Entities)

コンテキストのエンティティ、または単にエンティティは、FIWARE NGSI 情報モデルの中心です。エンティティはモノ、すなわち、
任意の物理的または論理的オブジェクトです。たとえば、センサ、人、部屋、発券システムの問題などです。各エンティティには
**entity id** があります。

さらに、FIWARE NGSI の型システム (type system) により、エンティティは、**エンティティ型 (entity type)** を持つことが
できます。エンティティ型はセマンティック型です。エンティティによって表されるモノの種類を記述することを意図しています。
たとえば、id *sensor-365* のコンテキストのエンティティは、*temperatureSensor* 型を持つことができます。

各エンティティは、その id と型の組み合わせによって一意に識別されます。

<a name="context-attributes"></a>

#### コンテキストの属性 (Context Attributes)

コンテキストの属性は、コンテキストのエンティティのプロパティです。たとえば、現在の車の速度は、エンティティ *car-104*
の属性 *current_speed* のようにモデル化できます。

NGSI データモデルでは、属性は、*属性名 (attribute name)*, *属性型 (attribute type)*, *属性値 (attribute value)* および
*メタデータ (metadata)* を持っています。

-   属性名は、その属性値がエンティティのどのような種類のプロパティを表すかを記述します。例: *current_speed*
-   属性型は、属性値の NGSI 値型 (NGSI value type) を表します。FIWARE NGSI には属性値用の独自の型システムがあるため、
    NGSI 値型は JSON 型 (JSON types) と同じではありません
-   属性値には次のものが含まれます:
    -   実際のデータ
    -   オプション **metadata** は、精度、プロバイダ、タイムスタンプなどの属性値のプロパティを記述します

<a name="context-metadata"></a>

#### コンテキストのメタデータ (Context Metadata)

コンテキストのメタデータは、いくつかの場所で FIWARE NGSI で使用され、そのうちの1つは、上述のように属性値のオプション部分
です。属性と同様に、各メタデータには次のものがあります:

-   **メタデータ名 (metadata name)** には、メタデータの発生場所におけるメタデータの役割を記述します。たとえば、
    メタデータ名 *accuracy* は、そのメタデータ値が与えられた属性値がどの程度正確であるかを記述していることを示します
-   **メタデータ型 (metadata type)** には、メタデータ値の NGSI 値型を記述します
-   **メタデータ値 (metadata value)** は、実際のメタデータを含んでいます

NGSI では、メタデータにネストされたメタデータが含まれることは予期されていないことに注意してください。

<a name="mime-types"></a>

## MIME タイプ (MIME Types)

この仕様の API レスポンス・ペイロードは `application/json` と (属性値型オペレーションのために) `text/plain` MIME タイプに
基づいています。HTTP リクエストを発行するクライアントは、それ以外の受け入れ型で `406 Not Acceptable`
エラーが発生します。

<a name="json-entity-representation"></a>

## JSON エンティティ表現 (JSON Entity Representation)

エンティティは、次の構文を持つ JSON オブジェクトで表されます:

-   エンティティ id は、オブジェクトの `id` プロパティによって指定され、その値はエンティティ id を含む文字列です
-   エンティティ型は、オブジェクトの `type` プロパティによって指定され、その値はエンティティの型名を含む文字列です
-   エンティティ属性は、追加のプロパティによって指定されます。名前は属性の `name` であり、その表現は下の
    [JSON 属性表現](#json-attribute-representation)のセクションで説明します。`id` および `type`
    は属性名として使用できません

この構文の例を以下に示します:

```
{
  "id": "entityID",
  "type": "entityType",
  "attr_1": <val_1>,
  "attr_2": <val_2>,
  ...
  "attr_N": <val_N>
}
```

エンティティの正規化された表現には、常に `id`、`type`、および属性を表すプロパティが含まれます。しかし、簡略化
(simplified) または、部分表現 (partial representations) (以下の[部分表現](#partial-representations)のセクションを参照)
は、一部を残してしまう可能性があります。各オペレーションの仕様には、どの表現が入力として期待されるか、どの表現が
出力として提供 (レンダリング) されるかに関する詳細が含まれます。

<a name="json-attribute-representation"></a>

## JSON 属性表現 (JSON Attribute Representation)

属性は、次の構文を持つ JSON オブジェクトで表されます:

-   属性値は、`value` プロパティによって指定され、その値は任意の JSON データ型になります
-   属性 NGSI 型は、`type` プロパティによって指定され、その値は、NGSI 型を含む文字列です
-   属性メタデータは、`metadata` プロパティによって指定されます。その値は、定義されたメタデータ要素ごとのプロパティを含む
    別の JSON オブジェクトです (プロパティの名前はメタデータ要素の `name` です)。各メタデータ要素は、次のプロパティを
    保持する JSON オブジェクトで表されます:
    -   `value`: その値には、JSON データ型に対応するメタデータ値が含まれています
    -   `type`: その値には、メタデータの NGSI 型の文字列表現が含まれます

この構文の例を以下に示します:

```
{
  "value": <...>,
  "type": <...>,
  "metadata": <...>
}
```

<a name="simplified-entity-representation"></a>

## 簡略化されたエンティティ表現 (Simplified Entity Representation)

実装によってサポートされなければならない 2つの表現モードがあります。これらの表現モードは、エンティティの簡略化された表現
を生成することを可能にします。

-   *keyValues* モード。このモードでは、型とメタデータに関する情報を除外して、エンティティの属性を値のみで表します。
    以下の例を参照してください

```
{
  "id": "R12345",
  "type": "Room",
  "temperature": 22
}
```

-   *values* モード。このモードでは、エンティティを属性値の配列として表します。id と型に関する情報は除外されています。
    以下の例を参照してください。配列内の属性の順序は、`attrs` URI パラメータによって指定されます。(たとえば、
    `attrs=branch,colour,engine`)。`attrs` が使用されない場合、順序は任意です

```
[ 'Ford', 'black', 78.3 ]
```

-   *unique* モード。このモードは、値が繰り返されない点を除いて、*values* モードと同じです

<a name="partial-representations"></a>

## 部分表現 (Partial Representations)

一部のオペレーションでは、エンティティの部分表現を使用します:

-   `id` と` type` は、不変のプロパティであるため、更新オペレーションでは使用できません
-   エンティティ `type` が許されるリクエストでは、それを省略することができます。エンティティ作成オペレーションで省略
    された場合、デフォルトの文字列値 `Thing` が型に使用されます
-   場合によっては、エンティティのすべての属性が表示されるわけではありません。たとえば、エンティティ属性のサブセットを
    選択するクエリ
-   属性/メタデータ `value` は、属性/メタデータが `null` 値を持つことを意味するリクエストでは省略することができます。
    レスポンスでは、値は常に存在します
-   属性/メタデータ `type` はリクエストで省略することができます。属性/メタデータの作成または更新オペレーションで省略
    された場合、その値に応じて、型に対してデフォルトが使用されます:
    -   値が文字列の場合、`Text` 型が使用されます
    -   値が数値の場合、`Number` 型が使用されます
    -   値がブーリンの場合は、`Boolean` が使用されます
    -   値がオブジェクトまたは配列の場合、`StructuredValue` が使用されます
    -   値が null の場合、`None` が使用されます
-   属性 `metadata` はリクエストでは省略することができます。つまり、属性に関連付けられたメタデータ要素がありません。
    レスポンスでは、属性にメタデータがない場合、このプロパティは `{}` に設定されます

<a name="special-attribute-types"></a>

## 特殊な属性型 (Special Attribute Types)

一般に、ユーザ定義の属性型は有益です。それらは不透明な方法で NGSIv2 サーバによって処理されます。それにもかかわらず、
以下に説明する型は、特別な意味を伝えるために使用されます:

-   `DateTime`: 日付を ISO8601 形式で識別します。これらの属性は、より大きい、未満、以上、以下 および範囲のクエリ演算子で
    使用できます。たとえば、参照されたエンティティ属性のみが表示されます

```
{
  "timestamp": {
    "value": "2017-06-17T07:21:24.238Z",
    "type: "DateTime"
  }
}
```

-   `geo:point`, `geo:line`, `geo:box`, `geo:polygon`, `geo:json`。これらはエンティティの場所に関連する特別な
    セマンティクスを持っています。[エンティティの地理空間プロパティ](#geospatial-properties-of-entities)を
    参照してください

<a name="builtin-attributes"></a>

## 組み込み属性 (Builtin Attributes)

NGSIv2 クライアントによって直接変更できないエンティティのプロパティがありますが、追加情報を提供するために NGSIv2
サーバによってレンダリングすることができます。表現の観点から見ると、それらは名前、値、型とともに通常の属性と同じです。

組み込み属性はデフォルトでレンダリングされません。特定の属性をレンダリングするには、URLs (または、POST /v2/op/query
オペレーションのペイロード・フィールド) または、サブスクリプション (`notification` 内の `attrs` サブフィールド) の
`attrs` パラメータにその名前を追加してください。

組み込み属性のリストは次のとおりです:

-   `dateCreated` (型: `DateTime`): エンティティ作成日。ISO 8601 文字列です
-   `dateModified` (型: `DateTime`): エンティティ変更日。ISO 8601 文字列です
-   `dateExpires` (型: `DateTime`): エンティティの有効期限。ISO 8601 文字列です。サーバがエンティティの有効期限を制御
    する方法は、実装の固有側面です

通常の属性と同様に、`q` フィルタと `orderBy` で使うことができます。ただし、リソース URLs では使用できません。

<a name="special-metadata-types"></a>

## 特殊なメタデータ型 (Special Metadata Types)

一般的に言えば、ユーザ定義のメタデータ型は参考になります。それらは、不透明な方法で NGSIv2 サーバによって処理されます。
それでも、以下に説明する型は、特別な意味を伝えるために使用されます:

-   `DateTime`:  ISO8601 形式で日付を識別します。このメタデータは、クエリ演算子の より大きい (greater-than), より小さい
    (less-than), 以上 (greater-or-equal), 以下 (less-or-equal) および 範囲 (range) で使用できます。たとえば (参照される
    属性メタデータのみが表示されます):

```
"metadata": {
      "dateCreated": {
        "value": "2019-09-23T03:12:47.213Z",
        "type": "DateTime"
      }
}
```

<a name="builtin-metadata"></a>

## 組み込みメタデータ (Builtin Metadata)

いくつかの属性プロパティは、NGSIv2 クライアントによって直接、変更可能ではありませんが、NGSIv2 サーバによってレンダリング
されて追加情報を提供することができます。表現の観点から見ると、それらは名前、値、型ともに通常のメタデータと似ています。

組み込みメタデータは、デフォルトではレンダリングされません。特定のメタデータをレンダリングするには、その名前を
`metadata` URL パラメータ (または、POST /v2/op/query オペレーションのペイロード・フィールド) または、サブスクリプション
(`notification` の `metadata` サブフィールド) に追加してください。

組み込みメタデータのリストは次のとおりです:

-   `dateCreated` (型: `DateTime`): 属性作成日。ISO 8601 文字列です
-   `dateModified` (型: `DateTime`): 属性変更日。ISO 8601 文字列です
-   `previousValue` (型: any): 通知でのみ。このメタデータの値は、関連する属性の通知をトリガーするリクエストに対する
    以前の値です。このメタデータの型は、関連付けられた属性の以前の型でなければなりません。`previousValue` の型/値が、
    関連する属性と同じ型/値である場合、その属性は実際に値を変更していません
-   `actionType` (型: `Text`): 通知のみ。添付されている属性が、通知をトリガーしたリクエストに含まれていた場合に
    含まれます。その値は、リクエスト・オペレーションのタイプによって異なります。更新の場合は `update`、作成の場合は
    `append`、削除の場合は `delete` です。その型は常に `Text` です

通常のメタデータと同様、`mq` フィルタでも使用できます。ただし、リソース URLs では使用できません。

<a name="field-syntax-restrictions"></a>

## フィールド構文の制限事項 (Field syntax restrictions)

NGSIv2 API の識別子として使用されるフィールドは、許可される構文に関する特別な規則に従います。これらの規則は:

-   エンティティ id (Entity id)
-   エンティティ型 (Entity type)
-   属性名 (Attribute name)
-   属性型 (Attribute type)
-   メタデータ名 (Metadata name)
-   メタデータ型 (Metadata type)

ルールは次のとおりです:

-   使用できる文字は、制御文字, 空白, `&`, `?`, `/`, `#` の文字を除き、プレーンな ASCII セットの文字です
-   最大フィールド長は 256文字です
-   最小フィールド長は 1文字です

上記の規則に加えて、NGSIv2 サーバ実装が与えられれば、それらのフィールドまたは他のフィールドに構文上の制約を追加して、
たとえばクロス・スクリプト・インジェクション攻撃を回避することができます。

クライアントがシンタックスの観点から無効なフィールドを使用しようとすると、クライアントは原因を説明する、"Bad Request"
エラー・レスポンスを得ます。

<a name="attribute-names-restrictions"></a>

## 属性名の制限 (Attribute names restrictions)

次の文字列を属性名として使用しないでください:

-   `id` は、エンティティ id を表すために使用されるフィールドと競合するためです
-   `type` は、エンティティ型を表すために使用されるフィールドと競合するためです
-   `geo:distance` は、中心点に近接するために `orderBy` で使用される文字列と競合するためです
-   組み込み属性名 ([組み込み属性](#builtin-attributes)の特定のセクションを参照)
-   `*` は、"すべてのカスタム/ユーザ属性" ([属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)を
    参照) という特別な意味を持っています

<a name="metadata-names-restrictions"></a>

## メタデータ名の制限 (Metadata names restrictions)

次の文字列をメタデータ名として使用しないでください:

-   組み込みメタデータ名 ("組み込みメタデータ" の特定のセクションを参照)
-   `*` は、"すべてのカスタム/ユーザ・メタデータ"
    ([属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)を参照) という特別な意味を持っています

<a name="ordering-results"></a>

## 結果の順序付け (Ordering Results)

エンティティのリストを検索するオペレーションは、`orderBy` URI パラメータが、結果を順序付けする際の基準として使用される
属性またはプロパティを指定することを可能にする。`orderBy` の値は次のようになります:

-   キーワード `geo:distance` は、"near" (`georel=near`) の空間関係 (spatial relationship) が使用されているときに
    リファレンス・ジオメトリまでの距離によって結果を並べます
-   カンマで区切られた属性のリストです。組み込み属性、エンティティ id の `id`、エンティティ型の `type` などがあります。
    たとえば、`temperature,!humidity`。結果は最初のフィールドで並べられます。続いて、結果は2番目のフィールドなどの順序で
    並べられます。フィールド名の前の "!" は、順序が逆になっていることを示します

<a name="error-responses"></a>

## エラー・レスポンス (Error Responses)

エラー・ペイロードが存在する場合は、次のフィールドを含む JSON オブジェクトです:

-   `error` (必須, 文字列): エラーのテキスト記述
-   `description` (オプション, 文字列): エラーに関する追加情報

すべての NGSIv2 サーバの実装では、この節で説明する以下の HTTP ステータス・コードと `error` テキストを使用する必要が
あります。しかしながら、`description` フィールドのために使用される特定のテキストは実装の固有側面です。

NGSIv2 の `error` レポートは次のとおりです:

-   着信 JSON ペイロードがパースできない場合、`ParseError` (`400`) が返されます
-   URL パラメータまたはペイロードのいずれかでリクエスト自体によってのみ発生するエラー (つまり、NGSIv2 サーバの
    ステータスに依存しないエラー) は、`BadRequest` (`400`) となります
    -   例外: 受信した JSON ペイロード・エラー。これには別の `error` メッセージがあります (前の箇条書きを参照)
-   空間インデックスの制限を超過しようとすると、`NoResourceAvailable` (`413`) になります。詳細は、"エンティティの
    地理空間プロパティ"を参照してください
-   リクエストに起因する曖昧さは、いくつかのリソースを参照する可能性があります。その id だけを提供するエンティティを更新
    しようとすると、その id を持つ複数のエンティティが存在すると、`TooManyResults` (`409`) になります
-   リクエストによって識別されるリソースが見つからない場合、`NotFound` (`404`) が返されます
-   リクエストと状態の組み合わせに起因するものの、排他的ではないリクエスト (たとえば、既存の属性に対して
    `options=append` を指定した POST) は、`Unprocessable` (`422`) になります
    -   例外: 前の箇条書きで説明した 404, 409 または 413 のエラーにつながるリクエストと状態の条件
-   HTTP 層のエラーは次のように使用されます:
    -   HTTP 405 Method Not Allowed は、`MethodNotAlowed` (`405`) に対応しています
    -   HTTP 411 Length Required は `ContentLengthRequired` (`411`) に対応します
    -   HTTP 413 Request Entity Too Large は、`RequestEntityTooLarge` (`413`) に対応します
    -   HTTP 415 Unsupported Media Type は `UnsupportedMediaType` (`415`) に対応します

<a name="geospatial-properties-of-entities"></a>

## エンティティの地理空間プロパティ (Geospatial properties of entities)

コンテキストのエンティティの地理空間プロパティは、通常のコンテキスト属性を用いて表すことができます。地理空間的
プロパティの提供は、地理的クエリの解決を可能にします。

準拠した実装では、2つの異なる構文をサポートする必要があります:

-   *Simple Location Format*。これは、開発者とユーザが既存のエンティティに素早く簡単に追加できる、非常に軽量な形式です
-   *GeoJSON*。[GeoJSON](https://tools.ietf.org/html/draft-butler-geojson-06) は、JSON (JavaScript Object Notation) に
    基づく地理空間データ交換フォーマットです。GeoJSON は、より高度な柔軟性を提供し、ポイント高度またはより複雑な
    地理空間形状、たとえば、
    [マルチ・ジオメトリ](http://www.macwright.org/2015/03/23/geojson-second-bite.html#multi-geometries)
    の表現を可能にします

クライアント・アプリケーションは、適切な NGSI 属性型を提供することによって、どのエンティティ属性がジオスペース属性を
伝えるかを定義します。通常、これは `location` という名前のエンティティ属性ですが、エンティティが複数の地理空間属性を含む
ユースケースを妨げるものはありません。たとえば、異なる粒度レベルで指定された場所、または異なる精度でさまざまな
ロケーション・メソッドによって提供された場所 (location) です。それでも、空間プロパティ (spatial properties) には、
バックエンド・データベースによって課せられたリソースの制約下にある特別なインデックスが必要であることに注意してください。
したがって、実装では、空間インデックスの制限を超えるとエラーが発生する可能性があります。これらの状況に推奨される HTTP
ステータス・コードは、``413``, *Request entity too large* で、レスポンス・ペイロードで報告されたエラーは、
``NoResourcesAvailable`` でなければなりません。

<a name="simple-location-format"></a>

### シンプル・ロケーション・フォーマット (Simple Location Format)

シンプル・ロケーション・フォーマットは、基本的なジオメトリ (*point*, *line*, *box*, *polygon*) をサポートし、
地理的位置をエンコードする際の典型的な使用例をカバーしています。[GeoRSS Simple](http://www.georss.org/simple.html)
に触発されています。

シンプル・ロケーション・フォーマットは、地球表面上の複雑な位置を表すことを意図していないことに注目してください。
たとえば、高度座標を取得する必要のあるアプリケーションでは、GeoJSON をそのエンティティの地理空間プロパティの表現形式
として使用する必要があります。

シンプル・ロケーション・フォーマットでエンコードされたロケーションを表すコンテキスト属性は、次の構文に準拠している必要が
あります:

-   属性型は、(`geo:point`, `geo:line`, `geo:box`, `geo:polygon`) のいずれかの値でなければなりません
-   属性値は座標のリストでなければなりません。既定では、座標は、
    [WGS84 Lat Long](https://en.wikipedia.org/wiki/World_Geodetic_System#WGS84),
    [EPSG::4326](http://www.opengis.net/def/crs/EPSG/0/4326) 座標リファレンス・システム (CRS) を使用して定義され、
    緯度と経度の単位は小数です。このような座標リストは、`type` 属性で指定されたジオメトリをエンコードすることを可能に
    し、以下で定義される特定の規則に従ってエンコードされます:
    -   `geo:point` 型: 属性値には有効な緯度経度のペアをカンマで区切った文字列を含める必要があります
    -   `geo:line` 型: 属性値に有効な緯度経度ペアの文字列配列を含める必要があります。少なくとも2つのペアが必要です
    -   `geo:polygon` 型: 属性値に有効な緯度経度ペアの文字列配列を含める必要があります。少なくとも4つのペアが存在
        しなければならず、最後のペアは最初のものと同一であるため、ポリゴンには最低 3つの実際のポイントがあります。
        ポリゴンを構成する線分が定義された領域の外縁に残るように、座標ペアを適切に順序付けする必要があります。たとえば、
        次のパス ```[0,0], [0,2], [2,0], [2, 2]``` は無効なポリゴン定義の例です。入力データによって前者の条件が
        満たされていない場合、実装でエラーが発生するはずです
    -   `geo:box` 型: バウンディング・ボックスは矩形領域であり、地図の範囲や関心のある大まかな領域を定義するためによく
        使用されます。ボックスは、緯度経度ペアの2つの長さの文字列配列によって表現されます。最初のペアは下のコーナー、
        2番目のペアは上のコーナーです

注: [この文献](https://github.com/geojson/geojson-spec/wiki/Proposal---Circles-and-Ellipses-Geoms#discussion-notes)で、
実装のさまざまな欠点を説明しているように、サークル・ジオメトリはサポートされていません。

以下の例は、参照される構文を示しています:

```
{
  "location": {
    "value": "41.3763726, 2.186447514",
    "type": "geo:point"
  }
}
```

```
{
  "location": {
    "value": [
      "40.63913831188419, -8.653321266174316",
      "40.63881265804603, -8.653149604797363"
    ],
    "type": "geo:box"
  }
}
```

<a name="geojson"></a>

### GeoJSON

GeoJSON を使用してエンコードされた位置を表すコンテキスト属性は、次の構文に準拠している必要があります:

-   属性の NGSI 型は `geo:json` でなければなりません
-   属性値は有効な GeoJSON オブジェクトである必要があります。GeoJSON の座標で経度が緯度の前に来ることに注目してください

以下の例は、GeoJSON の使い方を示しています。その他の GeoJSON の例は、
[GeoJSON IETF 仕様書](https://tools.ietf.org/html/draft-butler-geojson-06#page-14) にあります。さらに、
[この GeoJSON チュートリアル](http://www.macwright.org/2015/03/23/geojson-second-bite.html)は、
フォーマットの理解に役立ちます。

```
{
  "location": {
    "value": {
      "type": "Point",
      "coordinates": [2.186447514, 41.3763726]
    },
    "type": "geo:json"
  }
}
```

<a name="simple-query-language"></a>

## シンプル・クエリ言語 (Simple Query Language)

シンプル・クエリ言語は、一連の条件に一致するエンティティを取得するための簡単な構文を提供します。クエリは、';' キャラクタ
で区切られたステートメントのリストで構成されます。各ステートメントは一致条件を表します。クエリは、一致するすべての条件
(AND 論理演算子) に一致するすべてのエンティティを返します。

ステートメントには、2種類あります: *単項ステートメント* と *バイナリ・ステートメント*

バイナリ・ステートメントは、属性パス (たとえば、`temperature` や `brand.name`)、演算子と値
(値の形式は演算子に依存します)、たとえば:

```
temperature==50
temperature<=20
```

属性パスの構文は、`.` 文字で区切られたトークンのリストで構成されます。このトークンのリストは、次の規則に従って JSON
プロパティ名を指定します。

-   最初のトークンは、エンティティの NGSI 属性 (*ターゲット NGSI 属性*) の名前です
-   属性値によるフィルタリング (つまり、式が `q` クエリで使用されている) の場合、残りのトークン (存在する場合) は、JSON
    オブジェクトでなければならない、*ターゲット NGSI 属性*のサブ・プロパティへのパスを表します。そのようなサブプロパティ
    は、*ターゲット・プロパティ*として定義されます
-   メタデータによるフィルタリング (つまり、式が `mq` クエリで使用されている) の場合、2番目のトークンはターゲット NGSI
    属性, *ターゲット・メタデータ*に関連付けられたメタデータ名を表し、残りのトークン (存在する場合) は JSON
    オブジェクトでなければならない *ターゲット・メタデータ値*のサブ・プロパティへのパスを表します。そのようなサブ・
    プロパティは、*ターゲット・プロパティ*として定義されます

*ターゲット・プロパティ値*は、上記のトークンのリストによって指定される、JSON プロパティの値、つまり、*ターゲット・
プロパティ*の値として定義されます。

トークンが 1つだけ提供されている場合 (メタデータによるフィルタリングの場合は2つ)、*ターゲット・プロパティ* は
*ターゲット NGSI 属性* (またはメタデータでフィルタリングする場合の *ターゲット・メタデータ*) と
*ターゲット・プロパティ値* は、*ターゲット NGSI 属性*値 (または、メタデータによるフィルタリングの場合の
*ターゲット・メタデータ*値) になります。この場合、*ターゲット NGSI 属性* (または、メタデータによるフィルタリングの場合の
*ターゲット・メタデータ*) の値は JSON オブジェクトであってはなりません。

トークンの一部に `.` が含まれている場合、セパレータとして一重引用符 (`'`) を使用できます。たとえば、次の属性パス
`'a.b'.w.'x.y'` は3つのトークンで構成されます: 最初のトークンは `ab`、2番目のトークンは `w`、3番目のトークンは `xy`
です。

演算子のリスト、および、使用する値の形式は次のとおりです:

-   **等号**: `==`。この演算子は、次の型の右辺を受け入れます:
    -   単一要素、たとえば `temperature==40` です。エンティティがマッチするためには、*ターゲット・プロパティ*
        (temperature) が含まれていなければならず、*ターゲット・プロパティ値*は、クエリ値 (40) でなければなりません。
        または、*ターゲット・プロパティ値*が配列の場合はその値を含んでいなければなりません
    -   カンマで区切られた値のリストです。たとえば、`color==black,red`。エンティティがマッチするためには、*ターゲット・
        プロパティ* が含まれていなければならず、*ターゲット・プロパティ値*が、リスト内の値のうちの**いずれか**でなければ
        なりません (OR 句) 。または、*ターゲット・プロパティ値*が配列の場合は、リスト内の値の**いずれか**を含んで
        いなければなりません。たとえば、`color` という名前の属性を持つエンティティは、その値が ` black` であるとマッチ
        しますが、`color` という名前の属性を持つエンティティは、その値が `white` であるとはマッチしません
    -   範囲 (range)。最小値と最大値として指定され、`..` で区切られています。たとえば、`temperature==10..20` です。
        エンティティがマッチするためには、*ターゲット・プロパティ* (temperature) が含まれていなければならず、
        *ターゲット・プロパティ値*は、範囲の上限と下限の間 (どちらも含まれています) にある必要があります。範囲は、
        ISO8601 形式の日付、数字または文字列を表す*ターゲット・プロパティ*でのみ使用できます
-   **不等号**: `!=`。この演算子は、次の型の右辺を受け入れます:
    -   単一の要素、たとえば `temperature!=41` です。エンティティが一致するには、*ターゲット・プロパティ* (temperature)
        が含まれていなければならず、*ターゲット・プロパティー値*は、クエリ値 (41) で**あってはなりません**
    -   カンマで区切られた値のリスト、たとえば `color!=black,red`。エンティティがマッチするには、
        *ターゲット・プロパティ*が含まれていなければならず、*ターゲット・プロパティ値*が、リスト内のいずれかの値で
        *あってはなりません* (AND 句)。または、*ターゲット・プロパティ値*が配列の場合、リスト内の値の**いずれか**を
        含んでいてはなりません。例えば。属性 `color` が `black` に設定されたエンティティはマッチせず、属性 `color` が
        `white` に設定されたエンティティはマッチします
    -   範囲 (range)、最小値と最大値として指定され、`..` で区切られています。たとえば `temperature!=10..20`。
        エンティティがマッチするためには、*ターゲット・プロパティ* (temperature) が含まれていなければならず、
        *ターゲット・プロパティ値* は上限と下限の間 (どちらも含まれています) にある**必要はありません**。範囲は、
        ISO8601 形式の日付、数字または文字列の日付を表す要素 *ターゲット・プロパティ*でのみ使用できます
-   **より大きい**: `>`。右側は単一の要素でなければなりません。たとえば `temperature>42` です。エンティティがマッチ
    するためには、*ターゲット・プロパティ* (temperature) が含まれていなければならず、*ターゲット・プロパティ値*が
    クエリ値 (42) より厳密に大きくなければなりません。このオペレーションは、date 型、number 型または string 型の
    *ターゲット・プロパティ*に対してのみ有効です (他の型の *ターゲット・プロパティ*で使用されると、予測できない結果に
    なる可能性があります)
-   **未満**: `<`。右側は単一の要素でなければなりません。たとえば、`temperature<43` です。エンティティがマッチするため
    には、*ターゲット・プロパティ* (temperature) が含まれていなければならず、*ターゲット・プロパティ値*は 値 (43)
    より厳密に小さくなければなりません。このオペレーションは、date 型、number 型または string 型の
    *ターゲット・プロパティ*に対してのみ有効です (他の型の *ターゲット・プロパティ*で使用されると、予測できない結果に
    なる可能性があります)
-   **以上**: `>=`。右側は単一の要素でなければなりません。たとえば、`temperature>=44` です。エンティティがマッチするため
    には、*ターゲット・プロパティ* (temperature)が含まれていなければならず、*ターゲット・プロパティ値*は 値 (44) 以上で
    なければなりません。このオペレーションは、date 型、number 型または string 型の*ターゲット・プロパティ*に対してのみ
    有効です (他の型の *ターゲット・プロパティ*で使用されると、予測できない結果になる可能性があります)
-   **以下**: `<=`。右側は単一の要素でなければなりません。たとえば、`temperature<=45` です。エンティティがマッチするため
    には、*ターゲット・プロパティ* (temperature)が含まれていなければならず、*ターゲットプロパティ値*は、値 (45) 以下で
    なければなりません。このオペレーションは、date 型、number 型または string 型の*ターゲット・プロパティ*に対してのみ
    有効です (他の型の *ターゲット・プロパティ*で使用されると、予測できない結果になる可能性があります)
-   **マッチ・パターン**: `~=`。値は正規表現として表現された、与えられたパターンと一致します。`color~=ow`。
    エンティティがマッチするためには、*targetプロパティ* (color) が含まれていなければならず、*ターゲット・プロパティの
    値*が、右側の文字列と一致する必要があります。この例では `ow` (`brown` と `yellow` はマッチし、`black` と `white`
    はマッチしません) です。このオペレーションは、string 型の *ターゲット・プロパティ*に対してのみ有効です

シンボル `:` は `==` の代わりに使用できます。

等号または不等号の場合、一致する文字列に `,` が含まれている場合は、カンマの特殊な意味を無効にするために一重引用符 (`'`)
を使用できます。たとえば、`color=='light,green','deep,blue'`。最初の例は、正確な値  'light,green' または 'deep,blue' と
color を一致させます。また、`q=title=='20'` は文字列 "20" にマッチしますが、数値 20 ではマッチしません。

単項否定ステートメントは単項演算子 `!` を使用しますが、肯定単項ステートメントは演算子をまったく使用しません。単項
ステートメントは、*ターゲット・プロパティ*の存在をチェックするために使用されます。たとえば、`temperature` は、
'temperature' という属性を持つエンティティにマッチします (値に関係なく)。`!temperature` は、'temperature' という属性を
持たないエンティティと一致します。

<a name="geographical-queries"></a>

## 地理的クエリ (Geographical Queries)

地理的クエリは、以下のパラメータを使用して指定されます。

``georel`` は、一致するエンティティとリファレンス・シェイプ (`geometry`) の間の空間的関係 (述語) を指定することを意図
しています。';' で区切られたトークンリストで構成されています。最初のトークンはリレーションシップ名であり、残りのトークン
(あれば) はリレーションシップに関する詳細情報を提供する修飾語です。次の値が認識されます:

-   `georel=near`。``near`` リレーションシップは、一致するエンティティが、リファレンス・ジオメトリにある閾値距離に配置
    しなければならないことを意味します。これは次の修飾子をサポートしています:
    -   `maxDistance`。一致するエンティティを配置する必要がある最大距離をメートルで表します
    -   `minDistance`。一致するエンティティを配置する必要がある最小距離をメートルで表します
-   `georel=coveredBy`。一致するエンティティは、リファレンス・ジオメトリ内に完全に存在するエンティティであることを
    示します。このタイプのクエリを解決するときは、シェイプの境界線をシェイプの一部とみなす必要があります
-   `georel=intersects`。一致するエンティティはリファレンス・ジオメトリと交差するエンティティであることを示します。
-   `georel=equals`。一致するエンティティとリファレンス・ジオメトリの位置に関連付けられたジオメトリは、まったく同じで
    なければなりません
-   `georel=disjoint`。一致するエンティティは、リファレンス・参照ジオメトリと**交差しない**エンティティであることを
    示します

`geometry` はクエリを解決する際に使われるリファレンス・シェイプを定義することを可能にします。次のジオメトリ (シンプル・
ロケーション・フォーマットを参照) をサポートする必要があります。

-   `geometry=point` は、地球表面上の点を定義します
-   `geometry=line` は、折れ線を定義します
-   `geometry=polygon` はポリゴンを定義します
-   `geometry=box` は、バウンディング・ボックス (bounding box) を定義します

**coords** は、指定されたジオメトリとシンプル・ロケーション・フォーマットで規定されている規則に従って、セミコロンで
区切られた地理座標のペアのリストを含む文字列でなければなりません:

-   `geometry=point`。`coords` は、WGS-84 地理座標のペアを含んでいます
-   `geometry=line`。`coords` は、WGS-84 地理座標のペアのリストを含んでいます
-   `geometry=polygon`。`coords` は、少なくとも 4組の WGS-84 地理座標で構成されています
-   `geometry=box`。`coords` は、2組の WGS-84 地理座標で構成されています

例:

`georel=near;maxDistance:1000&geometry=point&coords=-40.4,-3.5`。マッチング・エンティティは、基準点から 1,000メートル
以内に配置する必要があります。

`georel=near;minDistance:5000&geometry=point&coords=-40.4,-3.5`。マッチング・エンティティは、基準点から (少なくとも)
5,000メートル離れていなければなりません。

`georel=coveredBy&geometry=polygon&coords=25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190`。
マッチング・エンティティは、参照されたポリゴン内にあるものです。

<a name="query-resolution"></a>

### クエリの解決 (Query Resolution)

実装が地理的なクエリを解決できない場合、レスポンスの HTTP ステータス・コードは ```422```, *Unprocessable Entity* で
なければなりません。エラー・ペイロードに存在するエラー名は、``NotSupportedQuery`` でなければなりません。

地理的クエリを解決する際には、シンプル・クエリ言語を介して、API 実装は、マッチング目的で使用される地理的位置を含む
エンティティ属性を決定する責任があります。この目的のために、以下の規則を遵守しなければなりません。

-   エンティティに、GeoJSON または、シンプル・ロケーション・フォーマットとしてエンコードされた場所に対応する属性がない
    場合、そのようなエンティティは地理空間プロパティを宣言せず、地理的なクエリに一致しません
-   エンティティがロケーションに対応する1つの属性のみを公開する場合、そのような属性は地理的クエリを解決する際に使用
    されます
-   エンティティが複数のロケーションを公開している場合、ブーリン値が ``true`` の ``defaultLocation`` という名前の
    メタデータ・プロパティを含む属性は、地理的クエリを解決するためのリファレンス・ロケーションとして扱われます
-   複数の属性が公開されているが、いずれもデフォルトのロケーションとしてラベル付けされていない場合、クエリはあいまいで
    あると宣言され、``409`` コードの HTTP エラー・レスポンスが送られなければなりません
-   *default location* とラベル付けされた複数の属性公開ロケーションがある場合、クエリはあいまいであると宣言され、``409``
    コードの  HTTP エラー・レスポンスが送られなければなりません

<a name="filtering-out-attributes-and-metadata"></a>

## 属性とメタデータのフィルタリング (Filtering out attributes and metadata)

`attrs` URL パラメータ または、POST /v2/op/query のフィールド は、検索オペレーションでレスポンスに含める必要のある属性の
リストを指定するために使用できます。同様に、`metadata` URL パラメータ または POST /v2/op/query のフィールドを使用して、
レスポンスに含める必要のあるメタデータのリストを指定することができます。

デフォルトでは、`attrs` が省略された場合、または `metadata` が省略された場合、組み込み属性 (メタデータ) を除くすべての
属性 (すべてのメタデータ) が含まれます。組み込みの属性 (メタデータ) を含めるためには、それらを明示的に `attrs`
(`metadata`) に含める必要があります。

たとえば、属性 A と B のみを含めるには:

`attrs=A,B`

*only* 組み込み属性 (メタデータ) を含めると、ユーザ定義の属性 (メタデータ) は使用できなくなります。組み込み属性
(メタデータ) *と* ユーザー定義属性 (メタデータ) を同時に組み込む場合、

-   ユーザ定義属性 (メタデータ) を明示的に含める必要があります。例えば、ユーザ定義属性 A と B を組み込み属性
    `dateModified` とともに含めるには、`attrs=dateModified,A,B` を使用します
-   特別な値 `*` は、すべてのユーザ定義属性と組み込み属性 `dateModified` とともに含めるために、たとえば、"すべてのユーザ
    定義属性 (メタデータ)" を意味するエイリアスとして `attrs=dateModified,*` を使用できます

`attrs` と `metadata` フィールドは `notification` のサブ・フィールドして、サブスクリプションでも使用でき、
サブスクリプションに関連する通知にどの属性メタデータを含めるかを指定するのと同じ意味を持ちます。

<a name="notification-messages"></a>

## 通知メッセージ (Notification Messages)

通知には2つのフィールドがあります:

-   `subscriptionId` は通知を発信した関連するサブスクリプションを表します
-   `data` はエンティティと関連するすべての属性を含む通知データそのものを持つ配列です。配列内の各要素は異なる
    エンティティに対応します。デフォルトでは、エンティティは `normalized` モードで表されます。しかし、`attrsFormat`
    修飾子を使用すると、簡略化された表現モードをリクエストすることができます

`attrsFormat` が `normalized` の場合、または `attrsFormat` が省略されている場合、デフォルトのエンティティ表現が使用
されます:

```
{
  "subscriptionId": "12345",
  "data": [
    {
      "id": "Room1",
      "type": "Room",
      "temperature": {
        "value": 23,
        "type": "Number",
        "metadata": {}
      },
      "humidity": {
        "value": 70,
        "type": "percentage",
        "metadata": {}
      }
    },
    {
      "id": "Room2",
      "type": "Room",
      "temperature": {
        "value": 24,
        "type": "Number",
        "metadata": {}
      }
    }
  ]
}
```

`attrsFormat` が `keyValues` の場合、keyValues の部分エンティティ表現モードが使用されます:

```
{
  "subscriptionId": "12345",
  "data": [
    {
      "id": "Room1",
      "type": "Room",
      "temperature": 23,
      "humidity": 70
    },
    {
      "id": "Room2",
      "type": "Room",
      "temperature": 24
    }
  ]
}
```

`attrsFormat` が `values` の場合、values の部分エンティティ表現モードが使用されます:

```
{
  "subscriptionId": "12345",
  "data": [ [23, 70], [24] ]
}
```

通知は、通知の受信者が通知ペイロードを処理する必要なくフォーマットを認識できるように、関連するサブスクリプションの
フォーマットの値を持つ `Ngsiv2-AttrsFormat` HTTP ヘッダを含む必要があります。

<a name="custom-notifications"></a>

## カスタム通知 (Custom Notifications)

NGSIv2 クライアントは、単純なテンプレート・メカニズムを使用して、HTTP 通知メッセージをカスタマイズできます。
サブスクリプションの `notification.httpCustom` プロパティは、以下のフィールドをテンプレート化するよう指定します:

-   `url`
-   `headers` (ヘッダ名と値の両方をテンプレート化できます)
-   `qs` (パラメータ名と値の両方をテンプレート化できます)
-   `payload`

5番目のフィールド `method` では、NGSIv2 クライアントが通知の配信に使用する HTTP メソッドを選択できますが、GET, PUT,
POST, DELETE, PATCH, HEAD, OPTIONS, TRACE, CONNECT などの有効な HTTP 動詞しか使用できないことに注意してください。

テンプレートのマクロ置換は、構文 `${..}` に基づいています。特に:

-   `${id}` は、エンティティの `id` に置き換えられます
-   `${type}` は、エンティティの `type` に置き換えられます
-   他の `${token}` は、名前が `token` の属性の値に置き換えられます。属性が通知に含まれていない場合は空文字列に
    置き換えられます。値が数値、bool または null の場合、その文字列表現が使用されます。値が JSON 配列または
    オブジェクトの場合、JSON 表現は文字列として使用されます

例:

与えられたサブスクリプションの次の `notification.httpCustom` オブジェクトを考えてみましょう。

```
"httpCustom": {
  "url": "http://foo.com/entity/${id}",
  "headers": {
    "Content-Type": "text/plain"
  },
  "method": "PUT",
  "qs": {
    "type": "${type}"
  },
  "payload": "The temperature is ${temperature} degrees"
}
```

次に、このサブスクリプションに関連付けられた通知がトリガーされ、id "DC_S1-D41" および 型 "Room" で、値が 23.4 の
"temperature" という属性を含むエンティティの通知データであると仮定します。テンプレートを適用した結果の通知は次のように
なります:

```
PUT http://foo.com/entity/DC_S1-D41?type=Room
Content-Type: text/plain
Content-Length: 31
The temperature is 23.4 degrees
```

いくつかの考慮事項:

-   NGSIv2 クライアントは、置換後に通知が正しい HTTP メッセージであることを確認する責任があります。たとえば Content-Type
    ヘッダが application/xml の場合、ペイロードは 整形式 XML 文書に対応する必要があります。具体的には、テンプレート
    適用後の結果の URL の形式が誤っている場合、通知は送信されません
-   通知するデータに複数のエンティティが含まれている場合は、エンティティごとに個別の通知 (HTTP メッセージ) が送信
    されます。デフォルトの動作とは異なり、すべてのエンティティが同じ HTTP メッセージで送信されます

通知にカスタム・ペイロードが使用されている場合 (フィールド `payload` は対応するサブスクリプションにあります)、通知の
`Ngsiv2-AttrsFormat` ヘッダに `custom` の値が使用されます。

<a name="api-routes"></a>

# API ルート (API Routes)

<a name="api-entry-point"></a>

## API エントリ・ポイント (API Entry Point)

<a name="retrieve-api-resources-get-v2"></a>

### API リソースを取得 [GET /v2]

このリソースには、属性はありません。代わりに、JSON 本体のリンクの形で初期 API アフォーダンス (initial API affordance)
を提供します。

該当する場合は、"url" リンク値、[Link](https://tools.ietf.org/html/rfc5988) または Location ヘッダに従うことを
お勧めします。独自の URL を構築する代わりに、クライアントと実装の詳細を切り離してください。

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、エラー・ペイロード (オプション) を使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

このリクエストは、次の要素を持つ JSON オブジェクトを返します:

-   entities_url: /v2/entities (required, string) - エンティティ・リソースを指す URL
-   types_url: /v2/types (required, string) - 型リソースを指す URL
-   subscriptions_url: /v2/subscriptions (required, string) - サブスクリプション・リソースを指す URL
-   registrations_url: /v2/registrations (required, string) - レジストレーション・リソースを指す URL

<a name="entities-operations"></a>

## エンティティの操作 (Entities Operations)

<a name="entities-list"></a>

### エンティティのリスト (Entities List)

<a name="list-entities-get-v2entities"></a>

#### エンティティをリスト [GET /v2/entities]

[JSON エンティティ表現](#json-entity-representation) に従って、id、型、パターン・マッチング (id または型)によって
異なる基準に一致するエンティティ・オブジェクトの配列、および/または、クエリまたは地理的クエリ
([シンプル・クエリ言語](#simple-query-language) および [地理的クエリ](#geographical-queries)を参照) に一致する
エンティティ・オブジェクトの配列を取得します。与えられたエンティティは、検索されるすべての基準に一致しなければ
なりません。すなわち、基準が論理的 AND 方法で結合されます。そのパターン・マッチング・クエリ・パラメータは、それらに
対応する正確なマッチング・パラメータと互換性がない (すなわち、相互に排他的である) ことに留意してください。すなわち、
`id` の `idPattern` および `type` の `typePattern` レスポンス・ペイロードは、一致するエンティティごとに1つの
オブジェクトを含む配列です。各エンティティは、[JSON エンティティ表現](#json-entity-representation)で説明した JSON
エンティティ表現形式に従います。

_**リクエスト・クエリ・パラメータ**_

このリクエストは、リクエスト・レスポンスをカスタマイズするために、次の URL パラメータを受け入れます。

<!-- Use this tool to prettify the table: http://markdowntable.com/ -->
| パラメータ     | オプション | タイプ   | 説明                                                                                                                                                                                                                                                                                                         | 例                                |
|----------------|------------|----------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| `id`           | ✓          | string   | 要素のコンマ区切りリスト。id がリスト内の要素の1つと一致するエンティティを取得します。`idPattern` と同時に使用できません                                                                                                                                                                                     | Boe_Idearium                      |
| `type`         | ✓          | string   | 要素のコンマ区切りリスト。型がリスト内の要素の1つと一致するエンティティを取得します。`typePattern` と同時に使用できません                                                                                                                                                                                    | Room                              |
| `idPattern`    | ✓          | string   | 正しくフォーマットされた正規表現。id が正規表現と一致するエンティティを取得します。`id `と互換性がありません                                                                                                                                                                                                 | Bode_.\*                          |
| `typePattern`  | ✓          | string   | 正しくフォーマットされた正規表現。型が正規表現に一致するエンティティを取得します。`type` と互換性がありません                                                                                                                                                                                                | Room_.\*                          |
| `q`            | ✓          | string   | temperature>40 (オプション, 文字列) - `;` で区切られたステートメントのリストで構成されるクエリ式。つまり、q=statement1;statement2;statement3 です。 [シンプル・クエリ言語仕様](#simple-query-language)を参照してください                                                                                     | temperature>40                    |
| `mq`           | ✓          | string   | `;` で区切られたステートメントのリストで構成される属性メタデータのクエリ式。つまり、mq=statement1;statement2;statement3 です。[シンプルクエリ言語仕様](#simple-query-language)を参照してください                                                                                                             | temperature.accuracy<0.9          |
| `georel`       | ✓          | string   | 一致するエンティティと参照形状 (reference shape) の間の空間的関係。[地理的クエリ](#geographical-queries)を参照してください                                                                                                                                                                                   | near                              |
| `geometry`     | ✓          | string   | クエリが制限されている地理的領域。[地理的クエリ](#geographical-queries)を参照してください                                                                                                                                                                                                                    | point                             |
| `limit`        | ✓          | number   | 取得するエンティティの数を制限します                                                                                                                                                                                                                                                                         | 20                                |
| `offset`       | ✓          | number   | エンティティが取得される場所からのオフセットを確立します                                                                                                                                                                                                                                                     | 20                                |
| `coords`       | ✓          | string   | ';' で区切られた座標の緯度経度ペアのリスト。[地理的クエリ](#geographical-queries)を参照してください                                                                                                                                                                                                          | 41.390205,2.154007;48.8566,2.3522 |
| `attrs`        | ✓          | string   | データがレスポンスに含まれる属性名のコンマ区切りのリスト。属性は、このパラメータで指定された順序で取得されます。このパラメータが含まれていない場合、属性は任意の順序で取得されます。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください | seatNumber                        |
| `metadata`     | ✓          | string   | レスポンスに含めるメタデータ名のリスト。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください                                                                                                                                             | accuracy                          |
| `orderBy`      | ✓          | string   | 結果を順序付けするための基準。詳細については、[結果の順序付け](#ordering-results)のセクションを参照してください                                                                                                                                                                                              | temperature,!speed                |
| `options`      | ✓          | string   | クエリのオプションのコンマ区切りリスト。次の表を参照してください                                                                                                                                                                                                                                             | count                             |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                                                      |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `count`     | 使用すると、エンティティの総数が `Fiware-Total-Count` という名前の HTTP ヘッダとしてレスポンスに返されます                                                                                                                |
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                          |
| `values`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                             |
| `unique`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。繰り返しの値は省略されます。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、エラー・ペイロード (オプション) を使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

レスポンス・ペイロードは、一致するエンティティごとに1つのオブジェクトを含む配列です。 各エンティティは、JSON
エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分表現](#partial-representations)
のセクションで説明されています) に従います。

例:

```json
[
  {
    "type": "Room",
    "id": "DC_S1-D41",
    "temperature": {
      "value": 35.6,
      "type": "Number",
      "metadata": {}
    }
  },
  {
    "type": "Room",
    "id": "Boe-Idearium",
    "temperature": {
      "value": 22.5,
      "type": "Number",
      "metadata": {}
    }
  },
  {
    "type": "Car",
    "id": "P-9873-K",
    "speed": {
      "value": 100,
      "type": "number",
      "metadata": {
        "accuracy": {
          "value": 2,
          "type": "Number"
        },
        "timestamp": {
          "value": "2015-06-04T07:20:27.378Z",
          "type": "DateTime"
        }
      }
    }
  }
]
```

<a name="create-entity-post-v2entities"></a>

#### エンティティを作成  [POST /v2/entities]

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                             | 例     |
|- ----------|------------|--------|------------------------------------------------------------------|--------|
| `options`  | ✓          | string | クエリのオプションのコンマ区切りリスト。次の表を参照してください | upsert |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                             |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください |
| `upsert`    | 使用すると、エンティティがすでに存在する場合は更新されます。upsert が使用されておらず、エンティティがすでに存在する場合、`422 Unprocessable Entity` エラーが返されます                           |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、作成されるエンティティを表すオブジェクトです。 オブジェクトは、JSON
エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分表現](#partial-representations)
のセクションで説明されています) に従います。

例:

```json
{
  "type": "Room",
  "id": "Bcn-Welt",
  "temperature": {
    "value": 21.7
  },
  "humidity": {
    "value": 60
  },
  "location": {
    "value": "41.3763726, 2.1864475",
    "type": "geo:point",
    "metadata": {
      "crs": {
        "value": "WGS84"
      }
    }
  }
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、201 Created (upsert オプションが使用されない場合) または、204 no Content (upsert
    オプションが使用される場合) を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

レスポンスには、作成されたエンティティの URL を含む `Location` ヘッダが含まれます。

-   Location: /v2/entities/Bcn-Welt?type=Room

<a name="entity-by-id"></a>

### id によるエンティティの操作 (Entity by ID)

<a name="retrieve-entity-get-v2entitiesentityid"></a>

#### エンティティを取得 [GET /v2/entities/{entityId}]

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ | タイプ | 説明                      | 例      |
|------------|--------|---------------------------|---------|
| `entityId` | string | 取得するエンティティの id | `Room`  |


_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                                                                                                                                                                                                                                                                                                 | 例         |
|------------|------------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します                                                                                                                                                                                                                                                                         | `Room`     |
| `attrs`    | ✓          | string | データをレスポンスに含める必要がある属性名のコンマ区切りのリスト。属性は、このパラメータで指定された順序で取得されます。このパラメータが含まれていない場合、属性は任意の順序で取得され、エンティティのすべての属性がレスポンスに含まれます。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください | seatNumber |
| `metadata` | ✓          | string | レスポンスに含めるメタデータ名のリスト。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください                                                                                                                                                                                                     | accuracy   |
| `options`  | ✓          | string | クエリのオプションのコンマ区切りリスト。次の表を参照してください                                                                                                                                                                                                                                                                                                     | count      |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                                                      |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                          |
| `values`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                             |
| `unique`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。繰り返しの値は省略されます。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、"エラー・レスポンス"
    のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

レスポンスは、id で識別されるエンティティを表すオブジェクトです。オブジェクトは、JSON エンティティ表現形式
([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分的表現](#partial-representations)
のセクションで説明されています) に従います。

例:

```json
{
  "type": "Room",
  "id": "Bcn_Welt",
  "temperature": {
    "value": 21.7,
    "type": "Number"
  },
  "humidity": {
    "value": 60,
    "type": "Number"
  },
  "location": {
    "value": "41.3763726, 2.1864475",
    "type": "geo:point",
    "metadata": {
      "crs": {
        "value": "WGS84",
        "type": "Text"
      }
    }
  }
}
```

<a name="retrieve-entity-attributes-get-v2entitiesentityidattrs"></a>

#### エンティティ属性を取得 [GET /v2/entities/{entityId}/attrs]

このリクエストは、エンティティ全体を取得するのと同様ですが、これは `id` と `type` フィールドを省略しています。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ | タイプ | 説明                      | 例     |
|------------|--------|---------------------------|--------|
| `entityId` | string | 取得するエンティティの id | `Room` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                                                                                                                                                                                                                                                                                                 | 例         |
|------------|------------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します                                                                                                                                                                                                                                                                         | `Room`     |
| `attrs`    | ✓          | string | データをレスポンスに含める必要がある属性名のコンマ区切りのリスト。属性は、このパラメータで指定された順序で取得されます。このパラメータが含まれていない場合、属性は任意の順序で取得され、エンティティのすべての属性がレスポンスに含まれます。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください | seatNumber |
| `metadata` | ✓          | string | レスポンスに含めるメタデータ名のリスト。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)の セクションを参照してください                                                                                                                                                                                                    | accuracy   |
| `options`  | ✓          | string | クエリのオプションのコンマ区切りリスト。次の表を参照してください                                                                                                                                                                                                                                                                                                     | count      |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                                                      |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                          |
| `values`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                             |
| `unique`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。繰り返しの値は省略されます。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

ペイロードは、URL パラメータの id で識別されるエンティティを表すオブジェクトです。オブジェクトは JSON
エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分表現](#partial-representations)
のセクションで説明されています) に従いますが、`id` フィールドと `type` フィールドは省略されます。

例:

```json
{
  "temperature": {
    "value": 21.7,
    "type": "Number"
  },
  "humidity": {
    "value": 60,
    "type": "Number"
  },
  "location": {
    "value": "41.3763726, 2.1864475",
    "type": "geo:point",
    "metadata": {
      "crs": {
        "value": "WGS84",
        "type": "Text"
      }
    }
  }
}
```

<a name="update-or-append-entity-attributes-post-v2entitiesentityidattrs"></a>

#### エンティティ属性の更新または追加 [POST /v2/entities/{entityId}/attrs]

エンティティ属性は、`append` オペレーションのオプションが使用されているかどうかに応じて、ペイロード内の属性で更新
されます。

-   `append` が使用されていない場合、エンティティ属性は更新され (以前に存在する場合)、ペイロードに追加されます
    (存在しない場合)
-   `append` が使用されている場合 (つまり、厳密なアペンド・セマンティクス)、ペイロード内の、エンティティ内に以前に存在
    しなかったすべての属性が追加されます。それに加えて、ペイロード内の属性の一部がすでにエンティティに存在する場合、
    エラーが返されます

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ | タイプ | 説明                      | 例     |
|------------|--------|---------------------------|--------|
| `entityId` | string | 更新するエンティティの id | `Room` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ  | オプション | タイプ   | 説明                                                                                    | 例     |
|------------|----------|--------|----------------------------------------------------------------------------------------------|--------|
| `type`     | ✓        | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します | `Room` |
| `options`  | ✓        | string | クエリのオプションのコンマ区切りリスト。次の表を参照してください                             | append |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                                       |
|-------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)セクションを参照してください |
| `append`    | 追加操作を強制します                                                                                                                                                                                       |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、URL パラメータの id で識別されるエンティティに追加または更新する属性を持つオブジェクトです。オブジェクトは
JSON エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分表現](#partial-representations)
のセクションで説明されています) に従いますが、`id`フィールドと`type`フィールドは省略されます。

例:

```json
{
   "ambientNoise": {
     "value": 31.5
   }
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="update-existing-entity-attributes-patch-v2entitiesentityidattrs"></a>

#### 既存のエンティティ属性の更新 [PATCH /v2/entities/{entityId}/attrs]

エンティティ属性は、ペイロード内の属性で更新されます。それに加えて、ペイロード内の1つ以上の属性がエンティティに存在
しない場合、エラーが返されます。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ | タイプ | 説明                      | 例     |
|------------|--------|---------------------------|--------|
| `entityId` | string | 更新するエンティティの id | `Room` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                                                                                                                                                                                             | 例        |
|------------|------------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します                                                                                                                                                                     | `Room`    |
| `options`  | ✓          | string | このメソッドでは、`keyValues` オプションのみが許可されています。使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください | keyValues |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、URL パラメータの id で識別されるエンティティで更新する属性を表すオブジェクトです。オブジェクトは
JSON エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分表現](#partial-representations)
のセクションで説明されています) に従いますが、`id` フィールドと `type` フィールドは省略されます。

例:

```json
{
  "temperature": {
    "value": 25.5
  },
  "seatNumber": {
    "value": 6
  }
}
```

_**レスポンス**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="replace-all-entity-attributes-put-v2entitiesentityidattrs"></a>

#### すべてのエンティティ属性を置換 [PUT /v2/entities/{entityId}/attrs]

ペイロード内の新しいエンティティ属性がエンティティに追加されます。以前にエンティティに存在していた属性が削除され、
リクエスト内の属性に置き換えられます。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ | タイプ | 説明                      | 例     |
|------------|--------|---------------------------|--------|
| `entityId` | string | 変更するエンティティの id | `Room` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                                                                                                                                                                                             | 例        |
|------------|------------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します                                                                                                                                                                     | `Room`    |
| `options`  | ✓          | string | このメソッドでは、`keyValues` オプションのみが許可されています。使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください | keyValues |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、URL パラメータの id で識別されるエンティティに追加または置換された新しいエンティティ属性を表すオブジェクト
です。オブジェクトは JSON エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)のセクションと、
[簡略化されたエンティティ表現](#simplified-entity-representation)および、[部分表現](#partial-representations)
のセクションで説明されています) に従いますが、`id` フィールドと `type` フィールドは省略されます。

例:

```json
{
  "temperature": {
    "value": 25.5
  },
  "seatNumber": {
    "value": 6
  }
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="remove-entity-delete-v2entitiesentityid"></a>

#### エンティティを削除する [DELETE /v2/entities/{entityId}]

エンティティを削除します。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ | タイプ | 説明                      | 例     |
|------------|--------|---------------------------|--------|
| `entityId` | string | 削除するエンティティの id | `Room` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                         | 例     |
|------------|------------|--------|----------------------------------------------------------------------------------------------|--------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します | `Room` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="attributes"></a>

### 属性 (Attributes)

<a name="get-attribute-data-get-v2entitiesentityidattrsattrname"></a>

#### 属性データを取得 [GET /v2/entities/{entityId}/attrs/{attrName}]

属性の属性データを含む JSON オブジェクトを返します。オブジェクトは、属性の JSON 表現に従います
([JSON 属性表現](#json-attribute-representation)のセクションで説明)。

_**リクエスト URL パラメータ**_

これらのパラメータは URL リクエストの一部です。これらは必須です。

| パラメータ | タイプ | 説明                      | 例            |
|------------|--------|---------------------------|---------------|
| `entityId` | string | 取得するエンティティの id | `Room`        |
| `attrName` | string | 取得する属性の名前        | `temperature` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                                                                                             | 例         |
|------------|------------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します                                                                     | `Room`     |
| `metadata` | ✓          | string | レスポンスに含めるメタデータ名のリスト。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください | `accuracy` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   正常なオペレーションには、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

レスポンスは、id で識別されるエンティティに含まれる URL で指定された属性名で識別される属性を表すオブジェクトです。
オブジェクトは、[JSON 属性表現](#json-attribute-representation) (および[部分表現](#partial-representations)
のセクション) で説明されている構造に従います。

例:

```json
{
  "value": 21.7,
  "type": "Number",
  "metadata": {}
}
```

<a name="update-attribute-data-put-v2entitiesentityidattrsattrname"></a>

#### 属性データを更新 [PUT /v2/entities/{entityId}/attrs/{attrName}]

リクエスト・ペイロードは、新しい属性データを表すオブジェクトです。
以前の属性データは、リクエスト内の属性データに置き換えられます。
このオブジェクトは、JSON 表現に従います ([JSON 属性表現](#json-attribute-representation)のセクションを参照)。

_**リクエスト URL パラメータ**_

これらのパラメータは URL リクエストの一部です。これらは必須です。

| パラメータ | タイプ | 説明                      | 例            |
|------------|--------|---------------------------|---------------|
| `entityId` | string | 更新するエンティティの id | `Room`        |
| `attrName` | string | 更新する属性の名前        | `Temperature` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                         | 例     |
|------------|------------|--------|----------------------------------------------------------------------------------------------|--------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します | `Room` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                | 例                 |
|----------------------|----------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓        | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓        | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

リクエスト・ペイロードは、id で識別されるエンティティに含まれる URL で指定された属性名で識別される属性を表す
オブジェクトです。オブジェクトは、[JSON属性表現](#json-attribute-representation) (および
[部分表現](#partial-representations)のセクション) で説明されている構造に従います。

例:

```json
{
  "value": 25.0,
  "metadata": {
    "unitCode": {
      "value": "CEL"
    }
  }
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="remove-a-single-attribute-delete-v2entitiesentityidattrsattrname"></a>

#### 単一の属性を削除 [DELETE /v2/entities/{entityId}/attrs/{attrName}]

エンティティ属性を削除します。

_**リクエスト URL パラメータ**_

これらのパラメータは URL リクエストの一部です。これらは必須です。

| パラメータ | タイプ | 説明                      | 例            |
|------------|--------|---------------------------|---------------|
| `entityId` | string | 削除するエンティティの id | `Room`        |
| `attrName` | string | 削除する属性の名前        | `Temperature` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                         | 例     |
|------------|------------|--------|----------------------------------------------------------------------------------------------|--------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します | `Room` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**レスポンス・コード**_

-   正常なオペレーションには、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="attribute-value"></a>

### 属性値

<a name="get-attribute-value-get-v2entitiesentityidattrsattrnamevalue"></a>

#### 属性値を取得 [GET /v2/entities/{entityId}/attrs/{attrName}/value]

このオペレーションは属性の値  (Attribute Value) を持つ `value` プロパティを返します。

_**リクエスト URL パラメータ**_

これらのパラメータは URL リクエストの一部です。これらは必須です。

| パラメータ | タイプ | 説明                      | 例         |
|------------|--------|---------------------------|------------|
| `entityId` | string | 取得するエンティティの id | `Room`     |
| `attrName` | string | 取得する属性の名前        | `Location` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                         | 例     |
|------------|------------|--------|----------------------------------------------------------------------------------------------|--------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します | `Room` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

`application/json` または `text/plain` を含む `Content-Type` ヘッダ (レスポンス・ペイロードに応じて)

_**レスポンス・ペイロード**_

レスポンス・ペイロードは、オブジェクト、配列、文字列、数値、ブール値、または属性の値を持つ null にすることができます。

-   属性値が JSON 配列またはオブジェクトの場合:
    -   `Accept` ヘッダを `application/json` または `text/plain` に展開できる場合、レスポンス・タイプが
        application/json の JSON または text/plain (`Accept` ヘッダの最初の方、また、`Accept:*/*` の場合は
        `application/json` のどちらか) として値を返します
    -   それ以外の場合は、HTTP エラー "406 Not Acceptable: accepted MIME types: application/json, text/plain"
        を返します
-   属性値が文字列、数値、null、またはブール値の場合:
    -   `Accept` ヘッダを text/plain に展開できる場合は、値をテキストとして返します。文字列の場合、最初と最後に
        引用符が使用されます
    -   それ以外の場合は、HTTP エラー "406 Not Acceptable: accepted MIME types: text/plain" を返します

例:

```json
{
  "address": "Ronda de la Comunicacion s/n",
  "zipCode": 28050,
  "city": "Madrid",
  "country": "Spain"
}
```

<a name="update-attribute-value-put-v2entitiesentityidattrsattrnamevalue"></a>

#### 属性値を更新 [PUT /v2/entities/{entityId}/attrs/{attrName}/value]

リクエスト・ペイロードは新しい属性値です。

_**リクエスト URL パラメータ**_

これらのパラメータは URL リクエストの一部です。これらは必須です。

| パラメータ | タイプ | 説明                      | 例         |
|------------|--------|---------------------------|------------|
| `entityId` | string | 更新するエンティティの id | `Room`     |
| `attrName` | string | 更新する属性の名前        | `Location` |

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                         | 例     |
|------------|------------|--------|----------------------------------------------------------------------------------------------|--------|
| `type`     | ✓          | string | エンティティ型。同じエンティティ id を持つエンティティが複数ある場合のあいまいさを回避します | `Room` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ                                                                                                           | `text/plain`       |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

リクエストのペイロードは、次のように `Content-Type` HTTP ヘッダで指定されたペイロード MIME タイプに応じて、JSON
オブジェクトまたは配列、あるいはプレーン・テキストにすることができます:

-   リクエスト・ペイロードの MIME タイプが `application/json` の場合、属性の値はペイロードにコード化された JSON
    オブジェクトまたは配列に設定されます (ペイロードが有効な JSON ドキュメントでない場合、エラーが返されます)
-   リクエスト・ペイロードの MIME タイプが `text/plain` の場合、次のアルゴリズムがペイロードに適用されます
    -   ペイロードが引用符 (`"`) で開始および終了する場合、値は文字列と見なされます
        (引用符自体は文字列の一部とは見なされません)
    -   `true` または `false` の場合、値はブール値と見なされます
    -   `null` の場合、値は null と見なされます
    -   これらの最初の3つのテストが 'fail' (失敗) した場合、テキストは数字として解釈されます
    -   有効な数値でない場合、エラーが返され、属性の値は変更されません

例:

```json
{
  "address": "Ronda de la Comunicacion s/n",
  "zipCode": 28050,
  "city": "Madrid",
  "country": "Spain"
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="types"></a>

### エンティティ型 (Types)

<a name="list-entity-types-get-v2types"></a>

#### 全エンティティ型のリスト [GET /v2/types]

`values` オプションが使用されていない場合、このオペレーションは 全てのエンティティ型 (Entity types) を持つ JSON 配列を
返します。各要素は、型に関する情報を持つ JSON オブジェクトです。

-   `type`: エンティティ型名
-   `attrs`: 属性名とその型のすべてのエンティティの集合。属性名をキーとする値を持つ JSON オブジェクトで表現されます。
    その値にはそのような属性の情報が含まれます。特に、属性で使用される型のリストその名前とすべてのエンティティ
-   `count`: その型に属するエンティティの数

`values` オプションが使用されている場合、オペレーションはエンティティ型名のリストを文字列として持つ JSON 配列を
返します。

結果はアルファベット順にエンティティ `type` によって順序付けられます。

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                               | 例      |
|------------|------------|--------|------------------------------------|---------|
| `limit`    | ✓          | number | 取得するタイプの数を制限します     | `10`    |
| `offset`   | ✓          | number | いくつかのレコードをスキップします | `20`    |
| `options`  | ✓          | string | オプション                         | `count` |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション | 説明                                                                           |
|------------|--------------------------------------------------------------------------------|
| `count`    | 使用すると、型 (types) の総数が HTTP ヘッダ `Fiware-Total-Count` に返されます  |
| `values`   | 使用すると、レスポンス・ペイロードはエンティティ型のリストを含む JSON 配列です |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

このリクエストは、見つかったさまざまなエンティティ型ごとに、要素を含むオブジェクトを含む JSON 配列を返します:

-   `type`: エンティティ型の名前。型自体
-   `attrs`: すべての属性と、その特定の型に属する各属性の型を含むオブジェクト
-   `count`: その特定のエンティティ型を持つエンティティの数

例:

```json
[
  {
    "type": "Car",
    "attrs": {
      "speed": {
        "types": [ "Number" ]
      },
      "fuel": {
        "types": [ "gasoline", "diesel" ]
      },
      "temperature": {
        "types": [ "urn:phenomenum:temperature" ]
      }
    },
    "count": 12
  },
  {
    "type": "Room",
    "attrs": {
      "pressure": {
        "types": [ "Number" ]
      },
      "humidity": {
        "types": [ "percentage" ]
      },
      "temperature": {
        "types": [ "urn:phenomenum:temperature" ]
      }
    },
    "count": 7
  }
]
```

<a name="retrieve-entity-information-for-a-given-type-get-v2types"></a>

#### 特定の型のエンティティ情報を取得 [GET /v2/types/{entityType}]

このオペレーションは、型 (Entity type) に関する情報を含む JSON オブジェクトを返します:

-   `attrs`: 属性名とその型のすべてのエンティティの集合。属性名をキーとする値を持つ JSON オブジェクトで表現されます。
    その値にはそのような属性の情報が含まれます。特に、属性で使用される型のリストその名前とすべてのエンティティです
-   `count`: その型に属するエンティティの数

_**リクエスト・クエリ・パラメータ**_

| パラメータ   | オプション | タイプ | 説明           | 例     |
|--------------|------------|--------|----------------|--------|
| `entityType` |            | string | エンティティ型 | `Room` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

このリクエストは、取得したエンティティ型の2つのフィールドを持つ JSON を返します:

-   `attrs`: その特定の型に属するエンティティに存在する属性の各型のオブジェクトを含むオブジェクト
    このオブジェクトには配列 `types` が含まれており、指定された型のすべてのエンティティでその属性に対して見つかった
    さまざまな型がすべて含まれています
-   `count`: その特定のエンティティ型を持つエンティティの数

例:

```json
{
  "attrs": {
    "pressure": {
      "types": [ "Number" ]
    },
    "humidity": {
      "types": [ "percentage" ]
    },
    "temperature": {
      "types": [ "urn:phenomenum:temperature" ]
    }
  },
  "count": 7
}
```

<a name="subscriptions-operations"></a>

## サブスクリプションの操作 (Subscriptions Operations)

<a name="subscription-payload-datamodel"></a>

### サブスクリプション・ペイロード・データモデル

<a name="subscription"></a>

#### `subscription`

サブスクリプションは、次のフィールドを持つ JSON オブジェクトで表されます:

| パラメータ                                  | オプション | タイプ  | 説明                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|---------------------------------------------|------------|---------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `id`                                        |            | string  | サブスクリプションの一意の識別子。作成時に自動的に作成されます                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `description`                               | ✓          | string  | クライアントがサブスクリプションを説明するために使用するフリー・テキスト                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| [`subject`](#subscriptionsubject)           |            | object  | サブスクリプションのサブジェクトを説明するオブジェクト                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| [`notification`](#subscriptionnotification) |            | object  | サブスクリプションがトリガーされたときに送信する通知を説明するオブジェクト                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `expires`                                   | ✓          | ISO8601 | ISO8601 形式のサブスクリプションの有効期限。永続的なサブスクリプションでは、このフィールドを省略する必要があります                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `status`                                    |            | string  | `active` (アクティブなサブスクリプションの場合) または `inactive` (非アクティブなサブスクリプションの場合)。サブスクリプションの作成時にこのフィールドが指定されていない場合、新しいサブスクリプションは `active` ステータスで作成され、後でクライアントが変更できます。期限切れのサブスクリプションの場合、この属性は `expired` に設定されます (クライアントがそれを `active`/`inactive` に更新するかどうかは関係ありません)。また、通知で問題が発生しているサブスクリプションの場合、ステータスは `failed` に設定されます。 通知が再び機能し始めるとすぐに、ステータスは `active` に戻ります |
| `throttling`                                | ✓          | number  | 2つの連続する通知の間に経過する必要がある最小時間 (秒単位)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |

<a name="subscriptionsubject"></a>

#### `subscription.subject`

`subject` には次のサブフィールドが含まれます:

| パラメータ                                   | オプション | タイプ  | 説明                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
|----------------------------------------------|------------|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `entities`                                   | ✓          | string  | オブジェクトのリスト。各オブジェクトは次のサブフィールドで構成されています: <ul><li><code>id</code> または <code>idPattern</code>: id または、影響を受けるエンティティのパターン。両方を同時に使用することはできませんが、一方が存在している必要があります。</li> <li><code>type</code> または <code>typePattern</code>: 型 または、影響を受けるエンティティの型パターン。両方を同時に使用することはできません。省略した場合は、"any entity type" (任意のエンティティ型) を意味します |
| [`condition`](#subscriptionsubjectcondition) | ✓          | object  | 通知をトリガーする条件。省略した場合は、"属性を変更すると条件がトリガーされる" ことを意味します                                                                                                                                                                                                                                                                                                                                                                                        |

<a name="subscriptionsubjectcondition"></a>

#### `subscription.subject.condition`

`condition` には次のサブフィールドが含まれます:

| パラメータ   | オプション | タイプ | 説明                                                                                                                                                                           |
|--------------|------------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `attrs`      | ✓          | array  | 通知をトリガーする属性名の配列                                                                                                                                                 |
| `expression` | ✓          | object | `q`, `mq`, `georel`, `geometry` および `coords` で構成される式 (このフィールドについては、上記の[エンティティをリスト](#list-entities-get-v2entities)の操作を参照してください) |

`condition` フィールドに基づいて、通知トリガー・ルールは次のとおりです:

-   `attrs` と `expression` が使用されている場合、`attrs` リスト内の属性の1つが変更され、同時に `expression` が一致する
    たびに通知が送られます
-   `attrs` が使用され、`expression` が使用されない場合、`attrs` リスト内のいずれかの属性が変化するたびに通知が
    送られます
-   `attrs` が使用されておらず、`expression` が使われている場合、エンティティの属性のいずれかが変更され、同時に
    `expression` が一致すると通知が送られます
-   `attrs` と `expression` のどちらも使わない場合は、エンティティの属性のいずれかが変更されるたびに通知が送られます

<a name="subscriptionnotification"></a>

#### `subscription.notification`

`notification` オブジェクトには、次のサブフィールドが含まれています:

| パラメータ                   | オプション | タイプ  | 説明                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|------------------------------|------------|---------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `attrs` または `exceptAttrs` |            | array   | 両方を同時に使用することはできません。<ul><li><code>attrs</code>: 通知メッセージに含まれる属性のリスト。また、<code>attrsFormat</code> <code>value</code> が使用されたときに属性が通知に表示される順序も定義します ("通知メッセージ" のセクションを参照)。空のリストは、すべての属性が通知に含まれることを意味します。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください</li><li><code>exceptAttrs</code>: 通知メッセージから除外される属性のリスト。つまり、通知メッセージには、このフィールドにリストされているもの</li><li><code>attrs</code> も <code>exceptAttrs</code> も指定されていない場合、すべての属性が通知に含まれます</li></ul> |
| [`http`](#subscriptionnotificationhttp) または [`httpCustom`](#subscriptionnotificationhttpcustom) | ✓                 | object | それらの1つが存在する必要がありますが、同時に両方が存在することはできません。これは、HTTP プロトコルを介して配信される通知のパラメータを伝達するために使用されます                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| `attrsFormat`                | ✓          | string  | エンティティが通知でどのように表されるかを指定します。受け入れられる値は、`normalized` (デフォルト)、`keyValues`、または `values` です。<br>`attrsFormat` がそれらとは異なる値をとると、エラーが発生します。詳細については、"通知メッセージ" のセクションを参照してください                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `metadata`                   | ✓          | string  | 通知メッセージに含まれるメタデータのリスト。詳細については、[属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)のセクションを参照してください                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `timesSent`                  | 検索時のみ | number  | 編集できません。GET 操作にのみ存在します。このサブスクリプションのために送信された通知の数                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| `lastNotification`           | 検索時のみ | ISO8601 | 編集できません。GET 操作にのみ存在します。ISO8601 形式の最終通知タイムスタンプ                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `lastFailure`                | 検索時のみ | ISO8601 | 編集できません。GET 操作にのみ存在します。ISO8601 形式の最後の障害タイムスタンプ。サブスクリプションで通知に問題が発生したことがない場合は存在しません                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `lastSuccess`                | 検索時のみ | ISO8601 | 編集できません。GET 操作にのみ存在します。最後に成功した通知の ISO8601 形式のタイムスタンプ。 サブスクリプションに通知が成功したことがない場合は存在しません                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |

<a name="subscriptionnotificationhttp"></a>

#### `subscription.notification.http`

`http` オブジェクトには、次のサブフィールドが含まれています:

| パラメータ | オプション | タイプ | 説明                                                                                                                                                                           |
|------------|------------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `url`      |            | string | 通知が生成されたときに呼び出されるサービスを参照する URL。NGSIv2 準拠のサーバは、`http` URL スキーマをサポートする必要があります。他のスキーマもサポートされる可能性があります |

<a name="subscriptionnotificationhttpcustom"></a>

#### `subscription.notification.httpCustom`

`httpCustom` オブジェクトには、次のサブフィールドが含まれています:

| パラメータ | オプション | タイプ | 説明                                                                                                                                                                   |
|------------|------------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `url`      |            | string | 上記の `http` と同じです                                                                                                                                               |
| `headers`  | ✓          | object | 通知メッセージに含まれる HTTP ヘッダのキー・マップ                                                                                                                     |
| `qs`       | ✓          | object | 通知メッセージに含まれる URL クエリ・パラメータのキー・マップ                                                                                                          |
| `method`   | ✓          | string | 通知を送信するときに使用するメソッド (デフォルトは POST)。有効な HTTP メソッドのみが許可されます。無効な HTTP メソッドを指定すると、400 Bad Request エラーが返されます |
| `payload`  | ✓          | string | 通知で使用されるペイロード。省略した場合、デフォルトのペイロード ("通知メッセージ" のセクションを参照) が使用されます                                                  |

`httpCustom` を使用する場合は、"カスタム通知 " のセクションで説明されている考慮事項が適用されます。

<a name="subscription-list"></a>

### サブスクリプション・リスト

<a name="list-subscriptions-get-v2subscriptions"></a>

#### サブスクリプションをリスト [GET /v2/subscriptions]

システムに存在するすべてのサブスクリプションのリストを返します:

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                               | 例      |
|------------|------------|--------|------------------------------------|---------|
| `limit`    | ✓          | number | 取得するタイプの数を制限します     | `10`    |
| `offset`   | ✓          | number | いくつかのレコードをスキップします | `20`    |
| `options`  | ✓          | string | オプション                         | `count` |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション | 説明                                                                                 |
|------------|--------------------------------------------------------------------------------------|
| `count`    | 使用すると、サブスクリプションの総数が HTTP ヘッダ `Fiware-Total-Count` に返されます |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

ペイロードは、サブスクリプションごとに1つのオブジェクトを含む配列です。各サブスクリプションは、JSON
サブスクリプション表現形式に従います
([サブスクリプション・ペイロード・データモデル](#subscription-payload-datamodel)
のセクションで説明されています)。

例:

```json
[
  {
    "id": "62aa3d3ac734067e6f0d0871",
    "description": "One subscription to rule them all",
    "subject": {
      "entities": [
        {
          "id": "Bcn_Welt",
          "type": "Room"
        }
      ],
      "condition": {
          "attrs": [ "temperature " ],
          "expression": {
            "q": "temperature>40"
      }
    },
    "notification": {
      "httpCustom": {
        "url": "http://localhost:1234",
        "headers": {
          "X-MyHeader": "foo"
        },
        "qs": {
          "authToken": "bar"
        }
      },
      "attrsFormat": "keyValues",
      "attrs": ["temperature", "humidity"],
      "timesSent": 12,
      "lastNotification": "2015-10-05T16:00:00.00Z",
      "lastFailure": "2015-10-06T16:00:00.00Z"
    },
    "expires": "2025-04-05T14:00:00.00Z",
    "status": "failed",
    "throttling": 5
  }
]
```

<a name="create-subscription-post-v2subscriptions"></a>

#### サブスクリプションを作成 [POST /v2/subscriptions]

新しいサブスクリプションを作成します。
サブスクリプションは、このセクションの冒頭で説明した JSON オブジェクトで表されます。

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、JSON サブスクリプション表現形式 ([サブスクリプション・ペイロード・データモデル](#subscription-payload-datamodel)
セクションで説明されています) に従うサブスクリプションを含む JSON オブジェクトです。

例:

```json
{
  "description": "One subscription to rule them all",
  "subject": {
    "entities": [
      {
        "idPattern": ".*",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [ "temperature" ],
      "expression": {
        "q": "temperature>40"
      }
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1234"
    },
    "attrs": ["temperature", "humidity"]
  },
  "expires": "2025-04-05T14:00:00.00Z",
  "throttling": 5
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、201 Created を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

-   作成が成功したとき (レスポンス・コード 201) に、サブスクリプションの作成に使用されたパスの値
    (つまり、`/v2/subsets/62aa3d3ac734067e6f0d0871`) を含むヘッダ `Location` を返します

<a name="subscription-by-id"></a>

### id によるサブスクリプションの操作

<a name="retrieve-subscription-get-v2subscriptionssubscriptionid"></a>

#### サブスクリプションを取得 [GET /v2/subscriptions/{subscriptionId}]

レスポンスは、このセクションの冒頭で説明した JSON オブジェクトによって表されるサブスクリプションです。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ       | タイプ | 説明                            | 例                         |
|------------------|--------|---------------------------------|----------------------------|
| `subscriptionId` | string | 取得するサブスクリプションの id | `62aa3d3ac734067e6f0d0871` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

ペイロードは、JSON サブスクリプション表現形式 ([サブスクリプション・ペイロード・データモデル](#subscription-payload-datamodel)
のセクションで説明されています) に従うサブスクリプションを含む JSON オブジェクトです。

例:

```json
{
  "id": "62aa3d3ac734067e6f0d0871",
  "description": "One subscription to rule them all",
  "subject": {
    "entities": [
      {
        "idPattern": ".*",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [ "temperature " ],
      "expression": {
        "q": "temperature>40"
      }
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1234"
    },
    "attrs": ["temperature", "humidity"],
    "timesSent": 12,
    "lastNotification": "2015-10-05T16:00:00.00Z"
    "lastSuccess": "2015-10-05T16:00:00.00Z"
  },
  "expires": "2025-04-05T14:00:00.00Z",
  "status": "active",
  "throttling": 5
}
```

<a name="update-subscription-patch-v2subscriptionssubscriptionid"></a>

#### サブスクリプションを更新 [PATCH /v2/subscriptions/{subscriptionId}]

サブスクリプションでは、リクエストに含まれるフィールドのみが更新されます。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ       | タイプ | 説明                            | 例                         |
|------------------|--------|---------------------------------|----------------------------|
| `subscriptionId` | string | 更新するサブスクリプションの id | `62aa3d3ac734067e6f0d0871` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、JSON サブスクリプション表現形式 ([サブスクリプション・ペイロード・データモデル](#subscription-payload-datamodel)
セクションで説明されています) に従ってサブスクリプションの変更されるフィールドを含む JSON オブジェクトです。

例:

```json
{
  "expires": "2025-04-05T14:00:00.00Z"
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="delete-subscription-delete-v2subscriptionssubscriptionid"></a>

#### サブスクリプションを削除 [DELETE /v2/subscriptions/{subscriptionId}]

サブスクリプションをキャンセルします。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ       | タイプ | 説明                            | 例                         |
|------------------|--------|---------------------------------|----------------------------|
| `subscriptionId` | string | 削除するサブスクリプションの id | `62aa3d3ac734067e6f0d0871` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="registration-operations"></a>

## レジストレーションの操作 (Registration Operations)

コンテキストのレジストレーション (Registrations) は、特定の地理的領域に位置するものを含むコンテキスト情報空間の特定の
サブセット (エンティティ、属性) のプロバイダの役割を果たすことができるように、外部コンテキスト情報ソースをバインドする
ことを可能にします。

NGSIv2 サーバ実装は、コンテキスト情報源へのクエリおよび/または更新転送を実装することができます。特に、以下の転送
メカニズム (forwarding mechanisms) の一部を実装することができます (完全なリストではありません):

-   レガシー転送 (NGSIv1 オペレーションに基づく)
-   NGSI コンテキスト・ソースの転送仕様

詳細を知るには、対応する仕様を確認してください。

<a name="registration-payload-datamodel"></a>

### レジストレーション・ペイロード・データモデル

<a name="registration"></a>

#### `registration`

コンテキストのレジストレーションは、次のフィールドを持つ JSON オブジェクトで表されます:

| パラメータ                                                    | オプション | タイプ  | 説明                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|---------------------------------------------------------------|------------|---------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `id`                                                          |            | string  | レジストレーションに割り当てられた一意の識別子。作成時に自動的に生成されます                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `description`                                                 | ✓          | string  | このレジストレーションに与えられた説明                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| [`provider`](#registrationprovider)                           |            | object  | レジストレーションされたコンテキストソースを説明するオブジェクト                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| [`dataProvided`](#registrationdataprovided)                   |            | object  | このソースによって提供されるデータを説明するオブジェクト.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `status`                                                      | ✓          | string  | レジストレーションの現在のステータスを可能な値でキャプチャする列挙フィールド: [`active`, `inactive`, `expired` または `failed`]。レジストレーション作成時にこのフィールドが指定されていない場合、新しい登録は `active` ステータスで作成され、後でクライアントによって変更される可能性があります。期限切れの登録の場合、この属性は `expired` に設定されます (クライアントがそれを `active`/`inactive` に更新するかどうかは関係ありません)。また、転送操作で問題が発生したレジストレーションの場合、ステータスは `failed` に設定されます。転送操作が再開されるとすぐに、ステータスは `active` に戻ります |
| `expires`                                                     | ✓          | ISO8601 | ISO8601 形式の登録有効期限。永続的な登録では、このフィールドを省略する必要があります                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| [`forwardingInformation`](#registrationforwardinginformation) |            | object  | プロバイダに対して行われた転送操作に関連する情報。そのような実装が転送機能をサポートする場合、実装によって自動的に提供されます                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |

<a name="registrationprovider"></a>

#### `registration.provider`

`provider` フィールドには以下のサブフィールドが含まれています:

| パラメータ                 | オプション | タイプ | 説明                                                                                                                                                                                                                                                                                                                                                                             |
|----------------------------|------------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `http`                     |            | object | これは、HTTP プロトコルを介して情報を配信するプロバイダのパラメータを伝達するために使用されます (現在サポートされているプロトコルのみ)。<br>提供するインターフェースを提供するエンドポイントとして機能する URL を含む `url` という名前のサブフィールドが含まれている必要があります。エンドポイントには、プロトコル固有の部分 (たとえば、`/v2/entity`) を*含めない*必要があります |
| `supportedForwardingMode`  |            | string | これは、このコンテキスト・プロバイダによってサポートされる転送モードを伝達するために使用されます。デフォルトでは `all`。許可される値は次のとおりです。<ul><li><code>none</code>: このプロバイダはリクエストの転送をサポートしていません</li><li><code>query</code>: このプロバイダはクエリデータへのリクエストの転送のみをサポートしています</li><li><code>update</code>: このプロバイダはデータを更新するためのリクエスト転送のみをサポートします</ li> <li><code>all</code>: このプロバイダは両方のクエリと更新の転送リクエストをサポートします (デフォルト値) </li></ul>                                                                                       |

<a name="registrationdataprovided"></a>

#### `registration.dataProvided`

`dataProvided` フィールドには、以下のサブフィールドが含まれています:

| パラメータ     | オプション | タイプ | 説明                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|----------------|------------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `entities`     |            | array  | オブジェクトのリスト。それぞれが次のサブフィールドで構成されています。<ul><li><code>id</code> または <code>idPattern</code>: id または影響を受けるエンティティのパターン。両方を同時に使用することはできませんが、どちらか一方が存在する必要があります</li><li><code>type</code> または <code>typePattern</code>: 型または影響を受けるエンティティ型のパターン。両方を同時に使用することはできません。省略した場合は、"任意のエンティティ型" を意味します</li></ul>        |
| `attrs`        |            | array  | 提供される属性のリスト (指定されていない場合は、すべての属性)                                                                                                                                                                                                                                                                                                                                                                                                              |
| `expression`   |            | object | フィルタリング式を使用して、提供されるデータの範囲を表すことができます。現在、地理的スコープのみが次のサブ用語でサポートされています<ul><li><code>georel</code>: この仕様の Geoqueries のセクションで指定されている地理的関係のいずれか </li><li><code>geometry</code>: この仕様の Geoqueries のセクションで指定されているサポートされているジオメトリのいずれか</li><li><code>coords</code>: この仕様の Geoqueries のセクションで指定されている座標の文字列表現</li></ul> |

<a name="registrationforwardinginformation"></a>

#### `registration.forwardingInformation`

`forwardingInformation` フィールドには、以下のサブフィールドが含まれています:

| パラメータ       | オプション | タイプ  | 説明                                                                                                                                                                  |
|------------------|------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `timesSent`      | 検索時のみ | number  | 編集できません。GET 操作にのみ存在します。このレジストレーションのために送信されたリクエスト転送の数                                                                  |
| `lastForwarding` | 検索時のみ | ISO8601 | 編集できません。GET 操作にのみ存在します。ISO8601 形式の最終転送タイムスタンプ                                                                                        |
| `lastFailure`    | 検索時のみ | ISO8601 | 編集できません。GET 操作にのみ存在します。ISO8601 形式の最後の障害タイムスタンプ。レジストレーションで転送に問題が発生したことがない場合は存在しません                |
| `lastSuccess`    | 検索時のみ | ISO8601 | 編集できません。GET 操作にのみ存在します。最後に成功したリクエスト転送の ISO8601 形式のタイムスタンプ。レジストレーションの通知が成功したことがない場合は存在しません |

<a name="registration-list"></a>

### レジストレーション・リスト

<a name="list-registrations-get-v2registrations"></a>

#### レジストレーションをリスト [GET /v2/registrations]

システムに存在するすべてのコンテキスト・プロバイダのレジストレーションをリストします。

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                               | 例      |
|------------|------------|--------|------------------------------------|---------|
| `limit`    | ✓          | number | 取得するタイプの数を制限します     | `10`    |
| `offset`   | ✓          | number | いくつかのレコードをスキップします | `20`    |
| `options`  | ✓          | string | オプション                         | `count` |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション | 説明                                                                                 |
|------------|--------------------------------------------------------------------------------------|
| `count`    | 使用すると、レジストレーションの総数が HTTP ヘッダ `Fiware-Total-Count` に返されます |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例         |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`     |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project` |

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

[レジストレーション・ペイロード・データモデル](#registration-payload-datamodel)
に続く各レジストレーションのオブジェクトによって表されるすべてのレジストレーションを含む JSON 配列

例:

```json
[
  {
    "id": "62aa3d3ac734067e6f0d0871",
    "description": "Example Context Source",
    "dataProvided": {
      "entities": [
        {
          "id": "Bcn_Welt",
          "type": "Room"
        }
      ],
      "attrs": [
        "temperature"
      ]
    },
    "provider": {
      "http": {
        "url": "http://contextsource.example.org"
      },
      "supportedForwardingMode": "all"
    },
    "expires": "2017-10-31T12:00:00",
    "status": "active",
    "forwardingInformation": {
      "timesSent": 12,
      "lastForwarding": "2017-10-06T16:00:00.00Z",
      "lastSuccess": "2017-10-06T16:00:00.00Z",
      "lastFailure": "2017-10-05T16:00:00.00Z"
    }
  }
]
```

<a name="create-registration-post-v2registrations"></a>

#### レジストレーションの作成 [POST /v2/registrations]

新しいコンテキスト・プロバイダのレジストレーションを作成します。これは通常、特定のデータのプロバイダとしてコンテキスト・
ソースをバインドするために使用されます。このセクションの冒頭で説明したように、レジストレーションは JSON オブジェクトで
表されます。

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、JSON レジストレーション表現形式に従うレジストレーションを含む JSON オブジェクトです
([レジストレーション・ペイロード・データモデル](#registration-payload-datamodel) セクションで説明されています)。

例:

```json
{
  "description": "Relative Humidity Context Source",
  "dataProvided": {
    "entities": [
      {
        "id": "room2",
        "type": "Room"
      }
    ],
    "attrs": [
      "relativeHumidity"
    ]
  },
  "provider": {
    "http":{
      "url": "http://localhost:1234"
    }
  }
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、201 Created を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

リクエストは、オペレーションが成功すると (リターン・コード 201)、レジストレーションのパス
(つまり、`/v2/registrations/62aa3d3ac734067e6f0d0871`) を含むヘッダ `Location` を返します。

<a name="registration-by-id"></a>

### id によるレジストレーションの操作

<a name="retrieve-registration-get-v2registrationsregistrationid"></a>

#### レジストレーションを取得 [GET /v2/registrations/{registrationId}]

レスポンスは、このセクションの冒頭で説明した JSON オブジェクトによって表されるレジストレーションです。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ       | タイプ | 説明                            | 例                         |
|------------------|--------|---------------------------------|----------------------------|
| `registrationId` | string | 取得するサブスクリプションの id | `62aa3d3ac734067e6f0d0871` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例        |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|-----------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`    |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`|

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

ペイロードは、JSON レジストレーション表現形式に従うレジストレーションを含む JSON オブジェクトです
([レジストレーション・ペイロード・データモデル](#registration-payload-datamodel) セクションで説明されています)。

例:

```json
{
      "id": "62aa3d3ac734067e6f0d0871",
      "description": "Example Context Source",
      "dataProvided": {
        "entities": [
          {
            "id": "Bcn_Welt",
            "type": "Room"
          }
        ],
        "attrs": [
          "temperature"
        ]
      },
      "provider": {
        "http": {
          "url": "http://contextsource.example.org"
        },
        "supportedForwardingMode": "all"
      },
      "expires": "2017-10-31T12:00:00",
      "status": "failed",
      "forwardingInformation": {
        "timesSent": 12,
        "lastForwarding": "2017-10-06T16:00:00.00Z",
        "lastFailure": "2017-10-06T16:00:00.00Z",
        "lastSuccess": "2017-10-05T18:25:00.00Z",
      }
}
```

<a name="update-registration-patch-v2registrationsregistrationid"></a>

#### レジストレーションを更新 [PATCH /v2/registrations/{registrationId}]

リクエストに含まれるフィールドのみがレジストレーション時に更新されます。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ       | タイプ | 説明                            | 例                         |
|------------------|--------|---------------------------------|----------------------------|
| `registrationId` | string | 更新するサブスクリプションの id | `62aa3d3ac734067e6f0d0871` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、JSON レジストレーション表現形式 ([レジストレーション・ペイロード・データモデル](#registration-payload-datamodel)
セクションで説明されています) に従ってレジストレーションの変更されるフィールドを含む JSON オブジェクトです。

例:

```json
{
    "expires": "2017-10-04T00:00:00"
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="delete-registration-delete-v2registrationsregistrationid"></a>

#### レジストレーションを削除 [DELETE /v2/registrations/{registrationId}]

コンテキスト・プロバイダのレジストレーションを取り消します。

_**リクエスト URL パラメータ**_

このパラメータは URL リクエストの一部です。これは必須です。

| パラメータ       | タイプ | 説明                            | 例                         |
|------------------|--------|---------------------------------|----------------------------|
| `registrationId` | string | 削除するサブスクリプションの id | `62aa3d3ac734067e6f0d0871` |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="batch-operations"></a>

## バッチ操作 (Batch Operations)

<a name="update-operation"></a>

### 更新操作 (Update operation)

<a name="update-post-v2opupdate"></a>

#### 更新 [POST /v2/op/update]

このオペレーションにより、単一のバッチ・オペレーション (Batch Operations) で複数のエンティティを作成、更新、
および/または、削除することができます。

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明       | 例          |
|------------|------------|--------|------------|-------------|
| `options`  | ✓          | string | オプション | `keyValues` |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                                       |
|-------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)セクションを参照してください |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードは、2つのプロパティを持つオブジェクトです:

-   `actionType`, 更新アクションの種類を指定するには、`append`, `appendStrict`, `update`, `delete`, `replace`
    のいずれかを指定します
-   `entities`, エンティティの配列。各エンティティは、JSON エンティティ表現形式 ("JSON エンティティ表現"
    のセクションを参照) を使用して指定します

このオペレーションは、`entities` ベクトル内のエンティティと同じ数の個別オペレーションに分割されているので、`actionType`
がそれぞれのエンティティに対して実行されます。`actionType` に応じて、通常の非バッチオペレーションによるマッピングを行う
ことができます:

-   `append`: `POST /v2/entities` (エンティティがまだ存在しない場合)、または `POST /v2/entities/<id>/attrs`
    (エンティティが既に存在する場合) にマップします
-   `appendStrict`: `POST /v2/entities` (エンティティがまだ存在しない場合) または
    `POST /v2/entities/<id>/attrs?options=append` (エンティティが既に存在する場合) にマップします
-   `update`: `PATCH /v2/entities/<id>/attrs` にマップされます
-   `delete`: エンティティに含まれているすべての属性に対して、`DELETE /v2/entities/<id>/attrs/<attrName>`
    にマッピングし、エンティティに属性が含まれていない場合は、`DELETE /v2/entities/<id>` にマッピングします
-   `replace`: `PUT /v2/entities/<id>/attrs` にマッピングします

例:

```json
{
  "actionType": "append",
  "entities": [
    {
      "type": "Room",
      "id": "Bcn-Welt",
      "temperature": {
        "value": 21.7
        },
      "humidity": {
        "value": 60
      }
    },
    {
      "type": "Room",
      "id": "Mad_Aud",
      "temperature": {
        "value": 22.9
      },
      "humidity": {
        "value": 85
      }
    }
  ]
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、204 No Content を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

<a name="query-operation"></a>

### クエリ操作 (Query operation)

<a name="query-post-v2opquery"></a>

#### クエリ [POST /v2/op/query]

このオペレーションは、リクエスト・ペイロードで提供されるフィルタに基づいて、既存のエンティティ間でクエリを実行します。

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明                                                                                                            | 例                   |
|------------|------------|--------|-----------------------------------------------------------------------------------------------------------------|----------------------|
| `limit`    | ✓          | number | 取得するエンティティの数を制限します                                                                            | `10`                 |
| `offset`   | ✓          | number | いくつかのレコードをスキップします                                                                              | `20`                 |
| `orderBy`  | ✓          | string | 結果を順序付けするための基準。詳細については、[結果の順序付け](#ordering-results)のセクションを参照してください | `temperature,!speed` |
| `options`  | ✓          | string | オプション                                                                                                      | `count`              |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                                                      |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `count`     | 使用すると、エンティティの総数が `Fiware-Total-Count` という名前の HTTP ヘッダとして応答に返されます                                                                                                                      |
| `keyValues` | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                          |
| `values`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください                             |
| `unique`    | 使用すると、レスポンス・ペイロードは簡略化されたエンティティ表現の `values` を使用します。繰り返しの値は省略されます。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

ペイロードには、次の要素 (すべてオプション) が含まれている場合があります:

-   `entities`: 検索する検索対象のリストです。各要素は、次の要素を持つ JSON オブジェクトで表されます:
    -   `id` または `idPattern`: 影響を受けるエンティティの id、またはパターン。両方を同時に使用することはできませんが、
        そのうちの1つが存在する必要があります
    -   `type` または `typePattern`: 検索するエンティティの型またはパターン型です。両方を同時に使用することは
        できません。これを省略すると、"任意のエンティティ型" を意味します
-   `attrs`: 提供される属性のリスト (指定されていない場合はすべての属性) です
-   `expression`: `q`, `mq`, `georel`, `geometry`, `coords`で構成される式です。(上記の
    [エンティティをリスト](#list-entities-get-v2entities)の操作を参照してください)
-   `metadata`: レスポンスに含めるメタデータ名のリスト。詳細については、
    [属性とメタデータのフィルタリング](#filtering-out-attributes-and-metadata)を参照してください

例:

```json
{

```
{
  "entities": [
    {
      "idPattern": ".*",
      "type": "Room"
    },
    {
      "id": "Car",
      "type": "P-9873-K"
    }
  ],
  "attrs": [
    "temperature",
    "humidity"
  ],
  "expression": {
      "q": "temperature>20"
  },
  "metadata": [
    "accuracy",
    "timestamp"
  ]
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください

_**レスポンス・ヘッダ**_

成功したオペレーションでは、`application/json` 値を持つ `Content-Type` ヘッダが返されます。

_**レスポンス・ペイロード**_

レスポンス・ペイロードは、一致するエンティティごとに1つのオブジェクトを含む配列、またはエンティティが見つからない場合は
空の配列 `[]` です。エンティティは、JSON エンティティ表現形式 ([JSON エンティティ表現](#json-entity-representation)の
セクションを参照) に従います。

例:

```json
[
  {
    "type": "Room",
    "id": "DC_S1-D41",
    "temperature": {
      "value": 35.6,
      "type": "Number"
    }
  },
  {
    "type": "Room",
    "id": "Boe-Idearium",
    "temperature": {
      "value": 22.5,
      "type": "Number"
    }
  },
  {
    "type": "Car",
    "id": "P-9873-K",
    "temperature": {
      "value": 40,
      "type": "Number",
      "accuracy": 2,
      "timestamp": {
        "value": "2015-06-04T07:20:27.378Z",
        "type": "DateTime"
      }
    }
  }
]
```

<a name="notify-operation"></a>

### 通知操作 (Notify operation)

<a name="notify-post-v2opnotify"></a>

#### 通知 [POST /v2/op/notify]

このオペレーションは、通知ペイロードを消費し、その通知によって含まれるすべてのエンティティのデータが永続化され、必要に
応じて上書きされるようにすることを目的としています。これは、ある NGSIv2 エンドポイントが別の NGSIv 2エンドポイントに
サブスクライブされている場合に役立ちます (フェデレーション・シナリオ)。リクエスト・ペイロードは、NGSIv2
通知ペイロードでなければなりません。その動作は、動作は `POST /v2/op/update` とまったく同じで、`actionType` は `append`
と同じである必要があります。

_**リクエスト・クエリ・パラメータ**_

| パラメータ | オプション | タイプ | 説明       | 例          |
|------------|------------|--------|------------|-------------|
| `options`  | ✓          | string | オプション | `keyValues` |

この特定のリクエストに対して `options` パラメータが持つことができる値は次のとおりです:

| オプション  | 説明                                                                                                                                                                                              |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | 使用すると、リクエスト・ペイロードは簡略化されたエンティティ表現の `keyValues` を使用します。詳細については、[簡略化されたエンティティ表現](#simplified-entity-representation)を参照してください |

_**リクエスト・ヘッダ**_

| ヘッダ               | オプション | 説明                                                                                                                  | 例                 |
|----------------------|------------|-----------------------------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |            | MIME タイプ。`application/json` である必要があります                                                                  | `application/json` |
| `Fiware-Service`     | ✓          | テナントまたはサービス。詳細については、サブ・セクションの[マルチテナンシ](#multitenancy)を参照してください           | `acme`             |
| `Fiware-ServicePath` | ✓          | サービス・パスまたはサブ・サービス。詳細については、サブセクションの[サービス・パス](#service-path)を参照してください | `/project`         |

_**リクエスト・ペイロード**_

[通知メッセージ](#notification-messages) のセクションで説明されているように、リクエスト・ペイロードは NGSIv2
通知ペイロードである必要があります。

例:

```json
{
  "subscriptionId": "5aeb0ee97d4ef10a12a0262f",
  "data": [{
    "type": "Room",
    "id": "DC_S1-D41",
    "temperature": {
      "value": 35.6,
      "type": "Number"
    }
  },
  {
    "type": "Room",
    "id": "Boe-Idearium",
    "temperature": {
      "value": 22.5,
      "type": "Number"
    }
  }]
}
```

_**レスポンス・コード**_

-   成功したオペレーションでは、200 OK を使用します
-   エラーは、2xx 以外のものと、(オプションで) エラー・ペイロードを使用します。詳細については、
    [エラー・レスポンス](#error-responses)のサブセクションを参照してください
