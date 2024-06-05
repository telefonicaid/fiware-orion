# <a name="top"></a> データ・モデル

* [イントロダクション](#introduction)
* [エンティティ・コレクション](#entities-collection)
* [レジストレーション・コレクション](#registrations-collection)
* [csubs コレクション](#csubs-collection)

<a name="introduction"></a>
## イントロダクション

通常、Orion Contex Broker が透過的に使用するため、MongoDB に直接アクセスする必要はありません。しかし、いくつかの操作 (例えば、バックアップ、障害回復など) では、データベースの構造を知ることは有益です。このセクションでは、この情報を提供します。

一部のアクションは元に戻すことができなので、データベースに直接アクセスする必要がある場合、操作には注意が必要です。最初は [バックアップ](database_admin.md#backing-up-and-restoring-database)をとることをお勧めします。

Orion Context Broker は、データベース内で次のサブセクションで説明する4つのコレクションを使用します。

[トップ](#top)

<a name="entities-collection"></a>
## エンティティ・コレクション

*エンティティ* のコレクションは NGSI エンティティに関する情報を格納します。コレクション内の各ドキュメントはエンティティに対応します。

フィールド :

-   **\_id** は ID 自体と型を含む EntityID を格納します。このために \_id を使用すると、EntityID が一意であることが保証されます。このフィールドの JSON ドキュメントは次のとおりです :
    -   **id** : エンティティの NGSI ID
    -   **type** : エンティティの NGSI 型
    -   **servicePath** : [サービスパス機能](../orion-api.md#service-path)に関連します
-   **attrs** は、そのエンティティに対して作成されたさまざまな属性のキーマップです。キーは、属性名で生成されます ("." は MongoDB document keys では有効な文字ではないので、"=" に変更します)。マップの各要素には、次の情報があります :
    -   **type**: 属性型
    -   **value** : 属性値 (少なくとも1つの更新を受け取った属性の場合) です。バージョン 0.10.1 までは、この値は常に文字列ですが、0.11.0 ではこの値は構造化値を表す JSON オブジェクトまたは JSON ベクトルでもあります ([Orion API 仕様の属性表現](../orion-api.md#json-attribute-representation)のセクションを参照ください)
    -   **md** (オプション) : カスタム・メタデータです。これはメタデータオブジェクトのキーマップです。キーはメタデータ名で生成されます ("." は MongoDB document keys では有効な文字ではないので、"=" に変更します)。例えば、"m.x" というメタデータは "m = x" キーを使用します。各キーのオブジェクト値には、メタデータの **type** と **value** の2つのフィールドがあります
    -   **mdNames** : 文字列の配列です。その要素は、属性のメタデータの名前です。ここでは "." から "=" への置換は行われません
    -   **creDate**: (追加の結果として) 属性の作成に対応するタイムスタンプ (浮動小数点数、ミリ秒の秒を意味します)
    -   **modDate**: 最後の属性の更新に対応するタイムスタンプ (浮動小数点数、ミリ秒の秒を意味します)。
        作成後に属性が変更されていない場合は、creDate と一致します
-   **attrNames** : 文字列の配列です。その要素は、エンティティの属性の名前 (IDs なし) です。この場合、 "." から "=" への置換は行われません
-   **creDate**: (追加の結果として) エンティティ作成日に対応するタイムスタンプ (浮動小数点数、ミリ秒の秒を意味します)
-   **modDate**: 最後のエンティティの更新に対応するタイムスタンプ (浮動小数点数、ミリ秒の秒を意味します)。
   これは通常、少なくとも1つの属性に対応する modDate と同じであることに注意してください
   (いつも同じではありません : 最後の更新が DELETE 操作の場合は同じではありません)。
   エンティティが作成後に変更されていない場合は、creDate と一致します
-   **location** (オプション) : エンティティの地理的位置です、次のフィールドで構成されます :
    -   **attrName** : attrs 配列内の地理的位置を識別する属性名です
    -   **coords** : エンティティの位置を表す GeoJSON です。詳細は以下を参照してください
-   **lastCorrelator** : エンティティ上の最後の更新要求における root correlator の値です。自己通知ループ保護ロジックによって使用されます。*root correlator* とは、サフィックスのない更新要求の `Fiware-Correlator` リクエスト・ヘッダの値を意味します。例えば、`Fiware-Correlator: f320136c-0192-11eb-a893-000c29df7908; cbnotif=32` の root correlator は `f320136c-0192-11eb-a893-000c29df7908` です
-   **expDate** (オプション): エンティティの有効期限のタイムスタンプ (Date オブジェクトとして)。詳細については、[一時的なエンティティの機能](../orion-api.md#transient-entities)を参照してください

`location.coordsin` については、いくつかの形式を使用することができます :

* ポイントを表します:

```
{
  "type": "Point",
  "coordinates": [ -3.691944, 40.418889 ]
}
```

* ラインを表します:

```
{
  "type": "LineString",
  "coordinates": [ [ 10, 0], [0, 10] ]
}
```

* ポリゴンを表します:

```
{
  "type": "Polygon",
  "coordinates": [ [ [ 10, 0], [0, 10], [0, 0], [10, 0] ] ]
}
```

* 最後に、`location.coords` は、[GeoJSON](http://www.macwright.org/2015/03/23/geojson-second-bite.html) 形式で場所を表現する任意の JSON オブジェクトを保持できます。任意の GeoJSON は geo:json 属性型で使用でき、有効なオブジェクトを導入するのはユーザの責任です。上記3つのケースは、実際には "fixed" ケースの GeoJSON 表現です。

座標ペアでは経度-緯度 (longitude-latitude) の順序が使用されますが、これは[地理的クエリ (Geographical Queries)](../orion-api.md#geographica-queries)で使用される順序とは逆であることに注意してください。これは、[MongoDB の地理位置情報の内部実装](http://docs.mongodb.org/manual/tutorial/query-a-2dsphere-index/) (GeoJSON に基づく) が経度-緯度 (longitude-latitude) の順序を使用するためです。 ただし、ユーザに近い他のシステム (GoogleMaps など) は緯度経度形式 (latitude-longitude format) を使用するため、Geographical Queries API には後者を使用しました。

サンプルドキュメント :

```
 {
   "_id":
       "id": "E1",
       "type": "T1",
       "servicePath": "/"
   },
   "attrs": {
       "A1": {
           "type": "TA1",
           "value": "282",
           "creDate" : 1389376081.8471954,
           "modDate" : 1389376120.2154321,
           "md" : {
              "customMD1": {
                 "type" : "string",
                 "value" : "AKAKA"
              },
              "customMD2": {
                 "type" : "integer",
                 "value" : "23232"
              }
           },
           "mdNames": [ "customMD1", "customMD2" ]
       },
       "A2()ID101": {
           "type": "TA2",
           "value": "176",
           "creDate" : 1389376244.6651231,
           "modDate" : 1389376244.6651231
       },
       "position": {
           "type": "location",
           "value": "40.418889, -3.691944",
           "creDate" : 1389376244.6651231,
           "modDate" : 1389376244.6651231
       }
   },
   "attrNames": [ "A1", "A2", "position" ],
   "creDate": 1389376081.8471954,
   "modDate": 1389376244.6651231,
   "location": {
       "attrName": "position",
       "coords": {
           "type": "Point",
           "coordinates": [ -3.691944, 40.418889 ]
       }
   },
   "lastCorrelator" : "aa01d6c6-4f7e-11e7-8059-000c29173617"
 }
```
[トップ](#top)

<a name="registrations-collection"></a>
## レジストレーション・コレクション

*レジストレーション*・コレクションは、レジストレーションに関する情報を格納します。コレクション内の各ドキュメントはレジストレーションに対応しています。

フィールド :

-   **\_id** : レジストレーション ID (レジストレーションを更新するためにユーザに提供される値) です。このために \_id を使用すると、レジストレーション ID は一意であり、レジストレーション ID によるクエリは非常に高速になります (\_id に自動のデフォルト・インデックスがあるため)
-   **format**: 転送されたリクエストを送信するために使用するフォーマット
    NGSIv1 フォーマットの場合、`format` の値として **JSON**を使用してください
    NGSIv2 では、今日現在、**normalized** フォーマットのみがサポートされています
-   **servicePath** : [サービスパス機能](../orion-api.md#service-path)に関連します
-   **status** (オプション) : `active` (アクティブなサブスクリプションの場合) または `inactive` (非アクティブなサブスクリプションの場合)。デフォルト・ステータス (すなわち、ドキュメントがこのフィールドを省略した場合) は "active" です
-   **statusLastChange**: ステータスが最後に更新された時刻 (10進数、秒と秒の端数を意味します)。
    これは主にサブスクリプション・キャッシュ更新ロジックによって使用されます (したがって、DB のステータスは、新しい場合にのみキャッシュから更新されます)
-   **description** (オプションフィールド) : サブスクリプションを説明するフリーテキスト文字列。最大長は1024です
-   **timeout** このフィールドは、サブスクリプションが http 通知のレスポンスを待機する最大時間を設定します。0から1800000までの数値です。0に設定するか省略した場合、デフォルトのタイムアウトが使用されます
-   **expiration** : これはレジストレーションが失効するタイムスタンプです (整数、秒を意味します)
-   **fwdMode**: プロバイダがサポートするフォワーディング・モード : `all`, `query`, `update` または `none`.
    省略した場合 (2.6.0 より前の Orion バージョン)、`all` が想定されます。
-   **contextRegistration** : 要素に以下の情報が含まれる配列です :
    -   **entities** : エンティティのリストを含む配列です (必須)。各エンティティの JSON には、**id**, **type** および **isPattern** が含まれています
    -   **attrs** : 属性のリストを含む配列です (オプション)。各属性の JSON には、**name** および **type** が含まれています
    -   **providingApplication** : このレジストレーションのための提供アプリケーションの URL です (必須)

サンプルドキュメント :

```
 {
   "_id": ObjectId("5149f60cf0075f2fabca43da"),
   "format": "JSON",
   "fwdMode": "all",
   "expiration": 1360232760,
   "contextRegistration": [
       {
           "entities": [
               {
                   "id": "E1",
                   "type": "T1",
                   "isPattern": "false"
               },
               {
                   "id": "E2",
                   "type": "T2",
                   "isPattern": "false"
               }
           ],
           "attrs": [
               {
                   "name": "A1",
                   "type": "TA1"
               },
               {
                   "name": "A2",
                   "type": "TA2"
               }
           ],
           "providingApplication": "http://foo.bar/notif"
      },
      "status": "active"
  ]
 }
```
[トップ](#top)

<a name="csubs-collection"></a>
## csubs コレクション

*csubs* コレクションは コンテキストのサブスクリプションに関する情報を格納します。コレクション内の各ドキュメントはサブスクリプションに対応しています。

フィールド :

-   **\_id** : サブスクリプション ID (サブスクリプションを更新およびキャンセルするためにユーザに提供される値) です。このために \_id を使用すると、サブスクリプション ID は一意であり、サブスクリプション ID によるクエリは非常に高速です (\_id に自動のデフォルト・インデックスがあるため)
-   **servicePath** : [サービスパス機能](../orion-api.md#service-path)に関連します。これは、サブスクリプションによってカプセル化されたクエリに関連付けられたサービスパスです。デフォルトは `/#` です
-   **expiration** : サブスクリプションの有効期限が切れるタイムスタンプです (整数、秒を意味します)。永続的なサブスクリプションの場合、不当に高い値が使用されます。ソースコードの PERMANENT_SUBS_DATETIME を参照してください
-   **lastNotification** : 最後の通知が送信された時刻です (整数、秒を意味します)。これは、通知が送信されるたびに更新され、スロットリング違反を回避します
-   **throttling** : 通知の最小間隔です。0または -1 は、スロットリングがないことを意味します
-   **reference** : 通知の URL です。HTTPまたはMQTTのいずれかを指定します
-   **topic**: MQTTトピック (MQTT 通知のみ)
-   **qos**: MQTT QoS 値 (MQTT 通知のみ)
-   **retain**: MQTT retain 値 (MQTT 通知のみ)
-   **entities** : エンティティの配列 (必須) です。各エンティティの JSON には、**id**, **type**, **isPattern** および **isTypePattern** が含まれています。従来の理由から、**isPattern** は `"true"` または `"false"` (テキスト) で、**isTypePattern** は `true` または `false` (ブール値) であることに注意してください
-   **attrs** : 属性名の配列 (文字列) (オプション) です
-   **blacklist** : `attrs` をホワイトリスト (もし `blacklist` が `false` また存在しない場合) またはブラックリスト (もし `blackslist` が `true` の場合) として解釈する必要があるかどうかを指定するブール値フィールドです
-   **onlyChanged**: 変更された属性のみを通知に含める必要があるか (onlyChanged が true
    の場合)、または含まないか (onlyChanged が false の場合) を指定するブール値フィールド
-   **metadata** : メタデータ名 (文字列) の配列 (オプション) です
-   **conditions** : 通知をトリガーする属性のリストです。
-   **expression** : 更新が来たときに通知を送信するかどうかを評価するために使用される式です。 次のフィールドで構成されています : q, mq, georel, geometry and/or coords (オプション)
-   **count** : サブスクリプションに関連付けられて送信された通知の数です
-   **format** : 通知を送信するために使用する形式。可能な値は、**normalized**, **keyValues**, **simplifiedNormalized**, **simplifiedKeyValues**, **values** です
-   **status** : `active` (アクティブなサブスクリプションの場合) または `inactive` (非アクティブなサブスクリプションの場合)、
    または `oneshot` ([oneshot サブスクリプション](../orion-api.md#oneshot-subscriptions) の場合) のいずれか。Orion API
    は追加の状態 (`expired`など) を考慮しますが、DB にヒットすることはありません (Orion によって管理されます)
-   **description** (オプションフィールド) : サブスクリプションを説明するフリーテキスト文字列。最大長は1024です
-   **custom** : このサブスクリプションがカスタマイズされた通知 (Orion API の機能) を使用するかどうかを指定するブール値フィールドです。このフィールドが存在し、その値が "true" であれば、カスタマイズされた通知が使用されていて、`headers`, `qs`, `method` および `payload` フィールドは考慮されています
-   **headers** : 通知カスタマイズ機能の HTTP ヘッダーキーマップを格納するためのオプションのフィールドです
-   **qs** : 通知カスタマイズ機能のためのクエリパラメータのキーマップを格納するオプションのフィールドです
-   **method** : 通知カスタマイズ機能の HTTP メソッドを格納するためのオプションのフィールドです
-   **payload** : 通知カスタマイズ機能のペイロードを格納するオプションのフィールドです
    その値が `null` の場合、ペイロードを通知に含める必要がないことを意味します。その値が `""` の場合、
    またはフィールドが省略されている場合は、NGSIv2 正規化形式が使用されます
-   **json** : Orion API の通知カスタマイズ機能用に生成された JSON ベースのペイロードに JSON オブジェクトまたは配列を格納するためのオプションのフィールド。この機能の詳細は、[こちら](../orion-api.md#json-payloads)です
-   **ngsi**: Orion API の通知カスタマイズ機能用の NGSI パッチ・オブジェクトを格納するためのオプション・フィールド。
    この機能の詳細は[こちら](../orion-api.md#ngsi-payload-patching)です。このフィールドの値は、値が
    [エンティティ・ コレクション](#entities-collection) の `attrs` の簡略化されたバージョンである `attrs`
    キーを持つオブジェクトです
-   **lastFailure** : 最後の通知の失敗が発生した時刻です (整数、秒を意味します)。サブスクリプションが失敗したことがない場合は存在しません
-   **lastFailureReason**: 最後の失敗の原因を説明するテキストです。
    サブスクリプションが一度も失敗したことがない場合は存在しません。
-   **lastSuccess** : 最後に成功した通知が発生した時刻です (整数、秒を意味します)。サブスクリプションが通知をうまく引き起こさなかった場合は存在しません
-   **lastSuccessCode**: 最後に正常な通知が送信されたときに受信エンドポイントから返された
    HTTP コード (200, 400, 404, 500 など)。 サブスクリプションが成功した通知を一度も引き起こした
    ことがない場合は存在しません
-   **maxFailsLimit**: 接続試行の最大制限を指定するために使用されるオプションのフィールド。これにより、失敗した通知の数に達すると、サブスクリプションは自動的に非アクティブ状態に移行します
-   **failsCounter**: サブスクリプションに関連付けられた連続して失敗した通知の数。これは、通知の試行が失敗するたびに1つずつ増加します。通知の試行が成功すると、0にリセットされます
-   **altTypes**: サブスクリプションに関連付けられた変更タイプ (alteration types) のリストを含む配列。フィールドが含まれていない場合は、デフォルトが想定されます ([このドキュメント](../orion-api.md#subscriptions-based-in-alteration-type)を確認してください)
-   **covered**: すべての `attrs` を通知に含める必要があるか (値がtrueの場合)、トリガー・エンティティに存在するものだけを含める必要があるか
    (値がfalseの場合、またはフィールドが省略されている場合) を指定するブール・フィールド
    詳細については、Orion API 仕様の[対象サブスクリプション・セクション](../orion-api.md#covered-subscriptions)
    を参照してください。
-   **notifyOnMetadataChange**: `true` の場合、メタデータはサブスクリプションのトリガーに関する属性の値の一部と見なされます。`false` の場合、メタデータはサブスクリプションのトリガーに関する属性の値の一部と見なされません。 デフォルトの動作 (省略された場合) は `true` の動作です。

サンプルドキュメント :

```
{
        "_id" : ObjectId("5697d4d123acbf5e794ab031"),
        "expiration" : NumberLong(1459864800),
        "reference" : "http://localhost:1234",
        "servicePath" : "/",
        "entities" : [
                {
                        "id" : ".*",
                        "type" : "Room",
                        "isPattern" : "true",
                        "isTypePattern": false
                }
        ],
        "attrs" : [
                "humidity",
                "temperature"
        ],
        "conditions" : [ "temperature" ],
        "expression" : {
                "q" : "temperature>40",
                "mq" : "temperature.accuracy<1",
                "geometry" : "",
                "coords" : "",
                "georel" : ""
        },
        "format" : "JSON",
        "description": "this is an example subscription",
        "status" : "active",
        "statusLastChange" : 1637226173.6940024
}
```
[トップ](#top)
