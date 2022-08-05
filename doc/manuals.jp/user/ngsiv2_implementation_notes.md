# <a name="top"></a>NGSIv2 実装ノート (NGSIv2 Implementation Notes)

* [サブスクリプション・ペイロードの検証](#subscription-payload-validations)
* [`actionType` メタデータ](#actiontype-metadata)
* [あいまいなサブスクリプション・ステータス `failed` は使用されない](#ambiguous-subscription-status-failed-not-used)
* [レジストレーション](#registrations)
* [`POST /v2/op/notify` でサポートされない `keyValues`](#keyvalues-not-supported-in-post-v2opnotify)
* [廃止予定の機能](#deprecated-features)

このドキュメントでは、Orion Context Broker が [NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)で行った具体的な実装について考慮する必要があるいくつかの考慮事項について説明します。

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

<a name="ambiguous-subscription-status-failed-not-used"></a>
## あいまいなサブスクリプション・ステータス `failed` は使用されない

NGSIv2 仕様では、サブスクリプションの `status` フィールドの `failed` 値について説明しています:

> `status`: [...] また、通知で問題が発生しているサブスクリプションの場合、ステータスは `failed` に
> 設定されます。通知が再び機能し始めるとすぐに、ステータスは `active` に戻ります。

ステータス `failed` は、あいまいなため、Orion3.4.0 で削除されました。

* `failed` は、最後に送信された通知が失敗したアクティブなサブスクリプション (つまり、エンティティの
  更新時に通知をトリガーするサブスクリプション) を指す場合があります
* `failed` は、過去にアクティブであり、アクティブなときに送信された最後の通知が失敗した非アクティブな
  サブスクリプション (つまり、エンティティの更新時に通知をトリガーしないサブスクリプション)
  を指す場合があります

つまり、ステータス `failed` を確認しても、サブスクリプションが現在アクティブであるか非アクティブで
あるかを知ることはできません。

したがって、`failed` は Orion Context Broker によって使用されず、サブスクリプションのステータスは、
サブスクリプションが `active` (`failed` バリアント [`oneshot`](#oneshot-subscriptions) を含む) か
`inactive` (バリアント `expired` を含む）かを常に明確に指定します。サブスクリプションが最後の通知で
失敗したかどうかを知るために、`failsCounter` の値をチェックできます (つまり、`failedCounter` が
0より大きいことをチェックします)。

[トップ](#top)

<a name="registrations"></a>
## レジストレーション

Orion は、次の点を除いて、NGSIv2 仕様に記載されているレジストレーション管理を実装しています。

* `PATCH /v2/registration/<id>` は実装されていません。したがって、レジストレーションを直接更新することはできません。つまり、レジストレーションを削除して再作成する必要があります。[この issue](https://github.com/telefonicaid/fiware-orion/issues/3007)についてはこちらをご覧ください
* `idPattern` はサポートされています
* `typePattern` は実装されていません
* `dataProvided` 内での `expression` フィールドはサポートされていません。フィールドは単に無視されます。これについては [この issue](https://github.com/telefonicaid/fiware-orion/issues/3107) を見てください。
* `status` での `inactive` 値はサポートされていません。つまり、フィールドは正しく格納され/取得されますが、値が `inactive` の場合でもレジストレーションは常にアクティブです。これについては、[この issue](https://github.com/telefonicaid/fiware-orion/issues/3108) を見てください

NGSIv2 仕様によると :

> NGSIv2 サーバ実装は、コンテキスト情報ソースへのクエリまたは更新転送を実装することができます

Orion がこのような転送を実装する方法は次のとおりです :

* `POST /v2/op/query` クエリ転送のため
* `POST /v2/op/update` 更新転送のため

コンテキスト情報ソースへの転送に関するより多くの情報は、この[ドキュメント](context_providers.md)にあります。

Orion は NGSIv2 仕様に含まれていない追加フィールド (`provider` 内の) `legacyForwarding`を実装しています。`legacyForwarding` の値が `true` の場合、NGSIv1 ベースのクエリ/更新はそのレジストレーションに関連したリクエストを転送するために使用されます。NGSIv1 は廃止予定ですが、一部のコンテキスト・プロバイダはまだ NGSIv2 に移行されていない可能性があるため、このモードは便利です。

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
