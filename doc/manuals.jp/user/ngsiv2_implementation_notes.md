# <a name="top"></a>NGSIv2 実装ノート (NGSIv2 Implementation Notes)

* [禁止されている文字](#forbidden-characters)
* [通知のカスタムペイロードデコード](#custom-payload-decoding-on-notifications)
* [カスタム通知を無効にするオプション](#option-to-disable-custom-notifications)
* [カスタム通知の変更不可能なヘッダ](#non-modifiable-headers-in-custom-notifications)
* [エンティティ・ロケーションの属性に制限](#limit-to-attributes-for-entity-location)
* [通知の従来の属性フォーマット](#legacy-attribute-format-in-notifications)
* [日時サポート](#datetime-support)
* [`dateModified` と `dateCreated` 属性](#datemodified-and-datecreated-attributes)
* [`dateModified` と `dateCreated` メタデータ](#datemodified-and-datecreated-metadata)
* [サブスクリプション・ペイロードの検証](#subscription-payload-validations)
* [`actionType` メタデータ](#actiontype-metadata)
* [`noAttrDetail` オプション](#noattrdetail-option)
* [通知スロットリング](#notification-throttling)
* [異なる属性型間の順序付け](#ordering-between-different-attribute-value-types)
* [初期通知](#initial_notifications)
* [レジストレーション](#registrations)
* [`POST /v2/op/notify` でサポートされない `keyValues`](#keyvalues-not-supported-in-post-v2opnotify)
* [廃止予定の機能](#deprecated-features)

このドキュメントでは、Orion Context Broker が [NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)で行った具体的な実装について考慮する必要があるいくつかの考慮事項について説明します。

<a name="forbidden-characters"></a>
## 禁止されている文字

NGSIv2 仕様の "フィールド構文の制限" セクションから :

> 上記のルールに加えて、NGSIv2 サーバの実装では、クロス・スクリプト注入攻撃を避けるために、それらのフィールドまたは他のフィールドに構文上の制限を追加することができます。

Orion に適用される追加の制限事項は、マニュアルの [禁止されている文字](forbidden_characters.md)のセクションに記載されているものです。

[トップ](#top)

<a name="custom-payload-decoding-on-notifications"></a>
## 通知でのカスタムペイロードデコード

禁止された文字の制限のために、Orion は発信カスタム通知に追加のデコード・ステップを適用します。これについては、このマニュアルの [このセクション](forbidden_characters.md#custom-payload-special-treatment)で詳しく説明します。

[トップ](#top)

<a name="option-to-disable-custom-notifications"></a>
## カスタム通知を無効にするオプション

Orion は、`-disableCustomNotifications` [CLI パラメータ](../admin/cli.md)を使用してカスタム通知を無効にするように設定できます。

この場合 :

* `httpCustom` が `http` として解釈されます。すなわち、`url` を除くすべてのサブフィールドは無視されます。
* マクロ置換 `${...}` は実行されません。

[トップ](#top)

<a name="non-modifiable-headers-in-custom-notifications"></a>
## カスタム・通知の変更不可能なヘッダ

カスタム・通知で次のヘッダを上書きすることはできません :

* `Fiware-Correlator`
* `Ngsiv2-AttrsFormat`

そのような試み (例えば `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}`) は無視されます。

[トップ](#top)

<a name="limit-to-attributes-for-entity-location"></a>
## エンティティ・ロケーションの属性に制限

NGSIv2 仕様の "エンティティの地理空間プロパティ" のセクションから :

> クライアントアプリケーションは、(適切な NGSI アトリビュート型を提供することによって) ジオスペース・プロパティを伝えるエンティティ属性を定義する責任があります。通常これは `location` という名前のついたエンティティ属性ですが、エンティティに複数の地理空間属性が含まれているユース・ケースはありません。たとえば、異なる粒度レベルで指定された場所、または異なる精度で異なる場所の方法によって提供された場所です。それにもかかわらず、空間特性には、バックエンド・データベースによって課せられたリソースの制約下にある特別なインデックスが必要であることは注目に値します。したがって、実装では、空間インデックスの制限を超えるとエラーが発生する可能性があります。これらの状況で推奨される HTTP ステータス・コードは `413` です。リクエスト・エンティティが大きすぎます。また、レスポンス・ペイロードで報告されたエラーは、`NoResourcesAvailable` である必要があります。

Orion の場合、その制限は1つの属性です。

[トップ](#top)

<a name="legacy-attribute-format-in-notifications"></a>
## 通知の従来の属性フォーマット

NGSIv2 仕様の `attrsFormat` で述べている値とは別に、Orion は、NGSIv1 形式の通知を送信するために、`legacy` 値もサポートしています。このようにして、ユーザは NGSIv1 レガシー通知レシーバを使用した NGSIv2 サブスクリプション (フィルタリングなど) の拡張の恩恵を受けることができます。

NGSIv1 は非推奨であることに注意してください。したがって、`legacy` 通知形式をもう使用しないことを推奨します。

[トップ](#top)

<a name="datetime-support"></a>
## 日時のサポート

NGSIv2 仕様の "Special Attribute Types" セクションから :

> DateTime : 日付を ISO8601 形式で識別します。これらの属性は、より大きい、より小さい、より大きい、等しい、より小さい、等しいおよび範囲のクエリ演算子で使用できます。

次の考慮事項は、属性の作成/更新時間または `q` と `mq` フィルタで使用される時に考慮しなければならない :

* Datetimes は、date, time, timezone の指定子で構成され、次のいずれかのパターンで表されます :
    * `<date>`
    * `<date>T<time>`
    * `<date>T<time><timezone>`
    * フォーマット `<date><timezone>` は許可されていないことに注意してください。ISO8601 によると : *"タイムゾーン指定子が必要な場合、それは結合された日付と時刻に従います"*
* `<date>` ついては、それがパターンに従わなければならない : `YYYY-MM-DD`
    * `YYYY`: year (4桁)
    * `MM`: month (2桁)
    * `DD`: day (2桁)
* これについて `<time>` は、[ISO8601 仕様](https://en.wikipedia.org/wiki/ISO_8601#Times)に記述されているパターンのいずれかに従わなければならない :
    * `hh:mm:ss.sss` または `hhmmss.sss`。現時点では、Orion は内部的には `.00` として保存されますが、マイクロ秒 (またはより小さい解像度) を含む時間を処理することができます。ただし、これは将来変更される可能性があります ([関連する問題](https://github.com/telefonicaid/fiware-orion/issues/2670)を参照)
    * `hh:mm:ss` または `hhmmss` です
    * ``hh:mm` または `hhmm`。この場合、秒は `00` に設定されます
    * `hh`。この場合、分と秒は `00` に設定されます
    * もし `<time>` 省略された場合、時、分、秒が `00` に設定されます
* `<timezones>` については、[ISO8601 仕様](https://en.wikipedia.org/wiki/ISO_8601#Time_zone_designators)に記述されているパターンのいずれかに従わなければならない :
    * `Z`
    * `±hh:mm`
    * `±hhmm`
    * `±hh`
* ISO8601 は、*" 時間表現で UTC 関係情報が与えられていない場合、その時間は現地時間であると想定される "* と規定している。ただし、クライアントとサーバが異なるゾーンにある場合、これはあいまいです。したがって、この曖昧さを解決するために、時間帯指定子が省略されている場合、Orion は常にタイムゾーン `Z` を想定します

Orion は常にフォーマット `YYYY-MM-DDThh:mm:ss.ssZ` を使用して日時属性/メタデータを提供します。クライアント/レシーバが任意のタイムゾーンで実行されている可能性があるため、UTC/Zulu タイムゾーンを使用していることに注意してください。これは将来変更される可能性があります ([関連する問題](https://github.com/telefonicaid/fiware-orion/issues/2663)を参照)。

属性とメタデータの型としての文字列 "ISO8601" もサポートされています。この効果は、"DateTime" を使用した場合と同じです。

[トップ](#top)

<a name="datemodified-and-datecreated-attributes"></a>
## `dateModified` と `dateCreated` 属性

"属性名の制限" のセクションでは、組み込み属性名をユーザ属性名として使用することはできません。したがって、組み込み属性の `dateCreated` および `dateModified` は、ユーザ属性名として使用できません。

ただし、従来の理由により、Orion は 400 Bad Request エラーでこれらの属性を使用しようとする試みを拒否しません。*dateCreate/dateModified を属性名として使用しないことを強く推奨しますが *、最終的にそうする必要がある場合は、次の点を考慮してください :

作成/更新時に dateModified/dateCreated 属性を使用できます。例 :

```
POST /v2/entities

{
  "id": "FutureEntity",
  "type": "T",
  ...
  "dateModified": {
      "type": "DateTime".
      "value": "2050-01-01T00:00:00Z"
  }
}
```

GET API オペレーションを使用してそのような属性を取得すると、作成または更新された値が返されます。`?attrs=dateModified` を使用する必要はありません。この場合、たとえば :

```
GET /v2/entities/FutureEntity/attrs
```

```
{
  ...
  "dateModified": {
      "type": "DateTime".
      "value": "2050-01-01T00:00:00Z"
  }
}
```

しかし、通知に含まれる場合、その値はユーザ (\*) によって提供される値ではなく、Orion によって内部的に格納されたエンティティの変更/作成に対応する実際の値です。たとえば、そのエンティティの最終更新が 2018年5月28日 11:16:05 に送信されたと仮定します :

```
POST /notify

{
    "data": [
        {
            "dateModified": {
                "metadata": {},
                "type": "DateTime",
                "value": "2018-05-28T11:16:05Z"
            },
            "id": "FutureEntity",
            "type": "T"
        }
    ],
    "subscriptionId": "5b082dc2c17960f8773dd74d"
}
```

(\*) 例外 : 初期通知は、ユーザが指定した値を使用します。 しかし、これはおそらく、[この issue](https://github.com/telefonicaid/fiware-orion/issues/3182) の副作用です。

さらに、フィルタは Orion によって内部的に保存された実際の変更/作成日を使用します。たとえば、次のフィルタは結果を取得しません。ユーザが 2050-01-01T00:00:00Z を dateModified として指定した場合でも、フィルタ条件と一致します :

```
GET /v2/entities?q=dateModified>2030-01-01T00:00:00Z
```

この動作は、[`cases/0876_entity_dates directory`](https://github.com/telefonicaid/fiware-orion/tree/master/test/functionalTest/cases/0876_entity_dates) に含まれている機能テスト `entity_dates_overriden_by_user.test` と `entity_dates_overriden_by_user_subs.test` のケースによって評価されます。詳細な説明が必要な場合は、見てください。

[トップ](#top)

<a name="datemodified-and-datecreated-metadata"></a>
## `dateModified` と `dateCreated` メタデータ

"メタデータ名の制限" のセクションでは、組み込みメタデータ名をユーザのメタデータ名として使用することはできません。したがって、組み込みメタデータの `dateCreated` と `dateModified` は、ユーザのメタデータ名として使用できません。

しかし、従来の理由により、Orion は、400 Bad Request エラーでこれらのメタデータを使用しようとする試みを拒否しません。**メタデータ名として、dateCreate/dateModified を使用しないことを強く推奨します**。しかし、最終的にそうする必要がある場合は、次の点を考慮してください :

作成/更新時に、dateModified/dateCreated メタデータを使用できます。たとえば :

```
POST /v2/entities

{
  "id": "FutureEntity",
  "type": "T",
  ...
  "futureAttr": {
      "value": 42,
      "type": "Number",
      "dateModified": {
          "type": "DateTime".
           "value": "2050-01-01T00:00:00Z"
      }
  }
}
```

ただし、無視されます。Orion は、内部的に属性の作成/変更を行い、GET レスポンス、通知、フィルタでそれらを常に使用します。

この動作は、[`cases/0876_attribute_dates directory`](https://github.com/telefonicaid/fiware-orion/tree/master/test/functionalTest/cases/0876_attribute_dates) に含まれている機能テスト `attrs_dates_overriden_by_user.test` と `attrs_dates_overriden_by_user_subs.test` のケースによって評価されます。

詳細な説明が必要な場合は、見てください。

[トップ](#top)

<a name="subscription-payload-validations"></a>
## サブスクリプション・ペイロードの検証

Orion が NGSIv2 サブスクリプション・ペイロードで実装する特定の検証は、次のとおりです :

* **description**: オプション (最大長1024)
* **subject**: 必須
    * **entities**: 必須
        * **id** or **idPattern**: そのうちの1つは必須です (ただし、同時に両方は許可されません)。id は ID の NGSIv2 制限に従わなければなりません。idPattern は空ではなく、有効な正規表現でなければなりません
        * **type** or **typePattern**: 任意です (ただし、両方同時に許可されません)。type は ID の NGSIv2 制限に従う必要があります。型は空であってはいけません。typePattern は有効な正規表現で、空ではない必要があります
    * **condition**: オプション (但し、存在する場合は内容を持たなければなりません。つまり `{}` は許可されていません)
        * **attrs**: オプション (ただし、存在する場合はリストでなければなりません。空リストも許されます)
        * **expression**: オプション (ただし、存在する場合は内容を持たなければなりません。つまり {} は許可されていません)
            * **q**: オプション (ただし、存在する場合は空でなければなりません。つまり `""` は許可されていません)
            * **mq**: オプション (ただし、存在する場合は空でなければなりません。つまり `""` は許可されていません)
            * **georel**: オプション (ただし、存在する場合は空でなければなりません。つまり `""` は許可されていません)
            * **geometry**: オプション (ただし、存在する場合は空でなければなりません。つまり `""` は許可されていません)
            * **coords**: オプション (ただし、存在する場合は空でなければなりません。つまり `""` 許可されていません)
* **notification**:
    * **http**: `httpCustom` が省略された場合は存在しなければならず、そうでなければ禁止されています
        * **url**: 必須 (有効な URL である必要があります)
    * **httpCustom**: `http` が省略されている場合は存在し、そうでない場合は禁止されていなければなりません
        * **url**: 必須 (空でなければなりません)
        * **headers**: オプション (ただし、存在する場合はコンテンツが必要です。つまり `{}` は許可されていません)
        * **qs**: オプション (ただし、存在する場合はコンテンツが必要です。つまり `{}` は許可されていません)
        * **method**: オプション (存在する場合は有効な HTTP メソッドでなければなりません)
        * **payload**: オプション (空の文字列を使用できます)
    * **attrs**: オプション (ただし、存在する場合はリストでなければなりません。空リストも許可されます)
    * **metadata**: オプション (ただし、存在する場合はリストでなければなりません。空リストも許可されます)
    * **exceptAttrs**: オプションです (ただし、`attrs` も使用されている場合は存在できません。存在する場合は空でないリストでなければなりません)
    * **attrsFormat**: オプション (存在する場合は有効な `attrs` 形式のキーワードでなければなりません)
* **throttling**: オプション (整数でなければなりません)
* **expires**: オプション (日付または空の文字列 "" でなければなりません)
* **status**: オプション (有効なステータス・キーワードである必要があります)

[トップ](#top)

<a name="actiontype-metadata"></a>
## `actionType` メタデータ

NGSIv2 仕様のセクション "組み込みメタデータ" から `actionType` メタデータ関連 :

> その値はリクエストオペレーションの型によって異なります : 更新のための `update`, 作成のための `append`, 削除のための `delete`。その型は常に Text です。

現在の Orion の実装では、"update (更新)" と "append (追加)" がサポートされています。[この問題](https://github.com/telefonicaid/fiware-orion/issues/1494)が完了すると、"delete (削除)" のケースがサポートされます。

[トップ](#top)

<a name="noattrdetail-option"></a>
## `noAttrDetail` オプション

URI param `options` の値 `noAttrDetail` は、NGSIv2 型のブラウジング・クエリ (`GET /v2/types` および `GET /v2/types/<type>`) が、属性型の詳細を提供しないようにするために使用されます。使用すると、各属性名に関連付けられた `types` リストが `[]` に設定されます。

このオプションを使用すると、Orion はこれらのクエリをはるかに迅速に解決します。特に、それぞれが異なる型の多数の属性の場合は、これは、ユース・ケースに属性型の詳細が必要ない場合に非常に便利です。場合によっては、`noAttrDetails` オプションで30秒から 0.5 秒の節約が検出されました。

[トップ](#top)

<a name="notification-throttling"></a>
## 通知スロットリング

サブスクリプション・スロットリングに関する NGSIv2 仕様から :

> throttling : 2つの連続した通知の間に経過する必要のある秒単位の最小限の時間。オプションです。

Orion がこれを実装する方法は、保護期間のスロットル中に通知を破棄することです。したがって、あまりにも近づいてしまえば、通知が失われる可能性があります。ユース・ケースがこのように通知を失うことをサポートしていない場合は、スロットリングを使用しないでください。

さらに、Orion はローカルでスロットリングを実装します。multi-CB 構成では、最終通知の尺度が各 Orion ノードに対してローカルであることを考慮してください。各ノードは DB と定期的に同期してより新しい値を取得しますが ([ここ](../admin/perf_tuning.md#subscription-cache)ではこれ以上)、特定のノードに古い値があることがありますので、スロットリングは100％正確ではありません。

[トップ](#top)

<a name="ordering-between-different-attribute-value-types"></a>
## 異なる属性値型間の順序付け

NGISv2 仕様 "Ordering Results" セクションから :

> エンティティのリストを取得するオペレーションでは、順序付けの結果を得る際に条件として使用される属性またはプロパティを `orderBy` URI パラメータで指定できます

これは、各型が他の型に関してどのように順序づけられるかの実装アスペクトです。Orion の場合、基本的な実装(MongoDB)で使用されているものと同じ基準を使用します。詳細については、[次のリンク](https://docs.mongodb.com/manual/reference/method/cursor.sort/#ascending-descending-sort)を参照してください。

最低から最高まで :

1. Null
2. Number
3. String
4. Object
5. Array
6. Boolean

[Top](#top)

<a name="initial_notifications"></a>
## 初期通知

NGSIv2 仕様では、サブスクリプションの対象となるエンティティの更新に基づいて、特定のサブスクリプションに対応する通知をトリガするルールを "サブスクリプション" セクションで説明しています。そのような定期的な通知以外にも、サブスクリプションの作成/更新時に初期通知を送信することがあります。[初期通知](initial_notification.md)については、ドキュメントの詳細を確認してください。

[トップ](#top)

<a name="registrations"></a>
## レジストレーション

Orion は、次の点を除いて、NGSIv2 仕様に記載されているレジストレーション管理を実装しています。

* `PATCH /v2/registration/<id>` は実装されていません。したがって、レジストレーションを直接更新することはできません。つまり、レジストレーションを削除して再作成する必要があります。[この issue](https://github.com/telefonicaid/fiware-orion/issues/3007)についてはこちらをご覧ください
* `idPattern` および `typePattern` は実装されていません。
* 唯一の有効な `supportedForwardingMode` は `all` です。他の値を使用しようとすると、501 Not Implemented エラー応答で終了します。[この issue](https://github.com/telefonicaid/fiware-orion/issues/3106) についてはこちらをご覧ください
* `dataProvided` 内での `expression` フィールドはサポートされていません。フィールドは単に無視されます。これについては [この issue](https://github.com/telefonicaid/fiware-orion/issues/3107) を見てください。
* `status` での `inactive` 値はサポートされていません。つまり、フィールドは正しく格納され/取得されますが、値が `inactive` の場合でもレジストレーションは常にアクティブです。これについては、[この issue](https://github.com/telefonicaid/fiware-orion/issues/3108) を見てください

NGSIv2 仕様によると :

> NGSIv2 サーバ実装は、コンテキスト情報ソースへのクエリまたは更新転送を実装することができます

Orion がこのような転送を実装する方法は次のとおりです。

Orion は、NGSIv2 仕様に含まれていない追加フィールド `legacyForwarding` を `provider` に実装しています。`legacyForwarding` の値が `true` の場合、そのレジストレーションに関連する転送リクエストに、NGSIv1 ベースのクエリ/更新が使用されます。NGSIv1 は推奨されていませんが、当面は、NGSIv2 ベースの転送が定義されていないため、([この issue](https://github.com/telefonicaid/fiware-orion/issues/3068) を参照)、唯一有効なオプションは常に `"legacyForwarding": true` を使用することです。そうでなければ、結果は、501 Not Implemented エラーのレスポンスになります。

[Top](#top)

<a name="keyvalues-not-supported-in-post-v2opnotify"></a>
## `POST /v2/op/notify` でサポートされない `keyValues`

現在の Orion の実装は、`POST /v2/op/notify` オペレーションの `keyValues` オプションをサポートしていません。それを使用しようとすると、400 Bad Request エラーが発生します。

[Top](#top)


<a name="deprecated-features"></a>
## 廃止予定の機能

安定版の NGSIv2 仕様の変更を最小限に抑えようとしていますが、最後にいくつかの変更が必要でした。したがって、現在の NGSIv2 stable 仕様には現れない機能が変更されていますが、Orion は後方互換性を維持するために([非推奨の機能](../deprecated.md)として)サポートしています。

特に、`options` のパラメータ内の `dateCreated` 及び `dateModified` の使用はまだサポートされています (安定した RC-2016.05 で導入され、RC-2016.10 で除去されました)。例えば `options=dateModified` です。ただし、代わりに `attrs` の使用することを強くお勧めします (すなわち `attrs=dateModified,*`)。

[トップ](#top)
