# <a name="top"></a>NGSIv1 と NGSIv2 の共存に関する考察 (Considerations on NGSIv1 and NGSIv2 coexistence)

NGSIv1 は Orion Context Broker が最初のバージョンから提供している API です。Orion0.23.0 で2015年7月に [NGSIv2](http://telefonicaid.github.io/fiware-orion/api/v2/stable)の開発が開始されました。最終的に NGSIv1 は将来の Orion バージョンでは廃止され、コードから削除されますが (NGSIv2 のみが残る)、これは大きな仕事であり、両方の API バージョンはしばらく共存しています。

このドキュメントでは、このような共存に関して考慮すべき考慮事項について説明します。

* [ネイティブ JSON 型](#native-json-types)
* [フィルタリング](#filtering)
* [`DateTime` 属性型へのサポートの違い](#differences-in-the-support-to-datetime-attribute-type)
* [ID フィールドのチェック](#checking-id-fields)
* [`orderBy` パラメータ](#orderby-parameter)
* [NGSIv2 サブスクリプションでの NGSIv1 通知](#ngsiv1-notification-with-ngsiv2-subscriptions)
* [コンテキスト・プロバイダへの NGSIv2 クエリ更新の転送](#ngsiv2-query-update-forwarding-to-context-providers)
* [NGSIv2 オペレーションを使用して NGSIv1 でレジストレーションを取得](#getting-registrations-created-with-NGSIv1-using-NGSIv2-operations)
* [コンテキスト・アベイラビリティ・サブスクリプション](#context-availability-subscriptions)

<a name="native-json-types"></a>
## ネイティブ JSON 型

NGSIv2 では、値が JSON ネイティブ型 (数値、ブール値、文字列など) を使用する属性 (およびメタデータ) を作成/更新できます。デフォルトで、NGSIv1 は、作成時/更新時に数値とブール値を文字列に変換する JSON パーサーを使用します。したがって、NGSIv1 を使用して `A=2` を設定しようとすると、実際には Orion データベースに `A="2"` が格納されます。ただし、ある程度のネイティブタイプは、[オート・キャスト機能](ngsiv1autocast.md)を使用して、NGSIv1 の格納において可能です。

オートキャストが有効かどうかにかかわらず、NGSIv1 レンダリングは、文字列以外の JSON ネイティブ型を使用して格納された属性値を正しく取得できます。したがって、NGSIv2 を使用して、`A=2` に設定し、NGSIv1 を使用してその属性を取得すると、`A=2` になります。

[トップ](#top)

<a name="filtering"></a>
## フィルタリング

NGSIv2 (`GET /v2/entities?q=<query>`) のために開発されたフィルタリング機能を、NGSIv1 で `POST /v1/queryContext` のペイロードにある Scope 要素を使用して使用することもできます。詳細については、[次のセクション](filtering.md#string-filters)を参照してください。

しかし、フィルタのいくつか (例えば、より大きい/少ない , 範囲など) が数値ためであることを考慮に入れてください。したがって、これらのフィルタ (`POST /v1/queryContext` を使用していますが) が正しく動作するためには、それらが参照する属性が NGSIv2 オペレーションを使用して作成されている必要があります。

さらに、NGSIv2 ジオクエリ・フィルタは NGSIv1 でも使用できます。詳細については、[次のセクション](geolocation.md#eo-located-queries-ngsiv2)を参照してください。

[トップ](#top)

<a name="differences-in-the-support-to-datetime-attribute-type"></a>
## `DateTime` 属性型へのサポートの違い

NGSIv2 は `DateTime` 属性型をサポートして日付を識別します。これらの属性は、より大きい、より小さい、より大きい、等しい、より小さい、等しいおよび範囲のクエリ演算子で使用できます。[NGSIv2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable)の "Special Attribute Types" セクションと、[NGSIv2 implementation notes の "DateTime support" のセクション](ngsiv2_implementation_notes.md#datetime-support)を参照してください。

ただし、`DateTime` 属性型には NGSIv1 API の特別な解釈はありません。つまり、属性値は特別な意味を持たない他の文字列として扱われます。これには2つの意味があります :

* `DateTime` 型の NGSIv1 を使用して作成/更新された属性は、通常の文字列として扱われます
* `DateTime` 型の NGSIv2 を使用して作成された属性 (または NGSIv2 を最後に使用していたもの) は、NGSIv1 を使用して更新されると、日付の性質が "失われ (loose)"、通常の文字列として扱われます
* `DateTime` 型で NGSIv1 を使用して作成された属性 (または NGSIv1 を最後に使用していたもの) は、NGSIv2 を使用して更新されると日付の性質が "取得 (gain)" され、日付フィルタなどで使用されます

[トップ](#top)

<a name="checking-id-fields"></a>
## ID フィールドの確認

NGSIv2 は、[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Field syntax restrictions" セクションで説明されている ID フィールド (エンティティ ID/ 型、属性名/型、メタデータ名/型など) の構文制限を導入しています。下位互換性を維持するために、これらの制限はデフォルトで NGSIv1 API では使用されませんが、`-strictNgsiv1Ids` [CLI パラメータ](../admin/cli.md)を使用して有効にすることができます。

`-strictNgsiv1Ids` が使用されている場合でも、Orion DB には、NGSIv2 ID ルールに準拠していないエンティティ/属性 /medatada を含むことがあります。このようなエンティティ/属性/メタデータが、`-strictNgsiv1Ids` 有効にしないでじょっ攻された時点で Orion によって作成された場合に発生する可能性があります (この機能の実装以前の古いバージョンなど)。そのような場合、NGSIv2 API を使用してそのようなエンティティ/属性/メタデータを取得すると、結果的に結果が不調になることがあります (たとえば、`GET /v2/entities` を使用して、一部のエンティティ IDs に空白を含むエンティティを取得する)

将来、このような状況に対処するために [github に次の問題](https://github.com/telefonicaid/fiware-orion/issues/1733)が作成されました。しかし、一般的には、たとえ NGSIv1 で強制されなくても、常に ID の "Field sysntax restrictions" に従うことは良い習慣です。そうすることで、どのバージョンの API (NGSIv1 または NGSIv2) に関係なく、コンテキスト情報を管理する際に問題を避けることができます。

[トップ](#top)

<a name="orderby-parameter"></a>
## `orderBy` パラメータ

NGSIv2 のために定義された `orderBy` パラメータは、NGSIv1 queryContext オペレーションでも使用することができます ([ページネーション](pagination.md)のドキュメントの詳細を参照してください)。ただし、"geo:distance" の順序は NGSIv2 でのみ使用できます。

[トップ](#top)

<a name="ngsiv1-notification-with-ngsiv2-subscriptions"></a>
## NGSIv2 サブスクリプションでの NGSIv1 通知

NGSIv2 では、サブスクリプションに関連付けられた `attrsFormat` フィールドに応じていくつかの通知モードが可能です。Orion は、NGSIv2 仕様に記載されている値の他に、NGSIv1 形式の通知を送信するための `legacy` 値もサポートしています。このようにして、ユーザは、NGSIv1 のレガシー通知レシーバを使用して、NGSIv2 サブスクリプション (フィルタリングや通知の組み込みメタデータなど) を強化できます。

[トップ](#top)

<a name="ngsiv2-query-update-forwarding-to-context-providers"></a>
## コンテキスト・プロバイダへの NGSIv2 クエリ更新の転送

[NGSIv1 または NGSIv2 オペレーションのいずれかを使用してプロバイダをレジストレーションし](context_providers.md)、NGSIv2 ベースのアップデートおよびクエリをコンテキスト・プロバイダに転送し、NGSIv2 でリプライを取得することができます。 CB to CPr 通信において転送されたメッセージとそのレスポンスは、NGSIv1 を使用して行われますが、将来 [NGSIv2 ベースの転送メカニズムが定義されます](https://github.com/telefonicaid/fiware-orion/issues/3068)。

ただし、次の考慮事項を考慮する必要があります :

* クエリのフィルタリング (例、"GET /v2/entities?q=temperature>40") は、クエリの転送ではサポートされていません。まず、Orion は、CPr に転送された `POST /v1/queryContext` オペレーションにフィルタを含めません。第2に、Orion はクライアントに返信する前に CPr 結果をフィルタリングしません。この制限に対応する問題が作成されました : https://github.com/telefonicaid/fiware-orion/issues/2282
* 転送時には、NGSIv2 の更新/クエリ内のどの型のエンティティも、エンティティ型のないレジストレーションと一致します。しかし、その逆は機能しませんので、型のレジストレーションがあれば、NGSIv2 の更新/クエリで一致を得るために `?type` を使用する必要があります
* 部分的な更新 (例えば、`POST /v2/op/entities` によって、いくつかのエンティティ/属性が更新され、他のエンティティ/属性が CPrs の失敗または欠落のために更新されない) の場合、404 Not Found がクライアントに返されます。この場合の `error` フィールドは `PartialUpdate` であり、`description` フィールドにはどのエンティティ属性が更新に失敗したかに関する情報が含まれています

[トップ](#top)

<a name="getting-registrations-created-with-NGSIv1-using-NGSIv2-operations"></a>
## NGSIv2 オペレーションを使用して NGSIv1 でレジストレーションを取得

一般に、NGSIv1 (特に、`POST /v1/registry/registerContext`) を使用してレジストレーションを作成してから、NGSIv2 (特に、`GET /v2/registrations` または `GET /v2/registrations/<id>`) を使用して取得することは問題ありません。

NGSIv1 は "コンテキスト・レジストレーション" の概念を考慮していることに注意してください。 レジストレーションはいくつかのコンテキスト・レジストレーションから構成され、それぞれはエンティティと属性のセットで構成されています。 NGSIv2 は、中間要素としてのコンテキスト・レジストレーションなしではるかに簡単なアプローチを提案します。すなわち、レジストレーションはエンティティおよび属性のセットに直接関連します。

NGSIv1 を使用して作成された、複数のコンテキスト・レジストレーションを持つレジストレーションを取得する場合、`GET /v2/registrations` または `GET /v2/registrations/<id>` で最初のものだけが考慮されます。 つまり、`GET /v2/registrations` または `GET /v2/registrations/<id>` へのレスポンスの `dataProvided` 要素は、最初のコンテキスト・レジストレーションを使って埋められます (もしあれば、以下は無視されます)。

ほとんどの NGSIv1 レジストレーションは1つのコンテキスト・レジストレーションのみを使用するので、これは問題になるものではありません。機能的な観点から、1つ以上のコンテキストを有するという実用上の利点はありません。たとえば、転送 (forwarding)。しかし、これは [より決定的な解決策](https://github.com/telefonicaid/fiware-orion/issues/3044)として保留中です。

[Top](#top)

<a name="context-availability-subscriptions"></a>
## コンテキスト・アベイラビリティ・サブスクリプション

コンテキスト・アベイラビリティ・サブスクリプションと通知は NGSIv2 には含まれていないことに注意してください。意図的に使用されていない NGSIv1 の機能であり、NGSIv2 に含める努力をする価値はないため、意図的に省略されています。

[Top](#top)
