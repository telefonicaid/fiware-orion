# <a name="top"></a> データ・モデル

* [イントロダクション](#introduction)
* [エンティティ・コレクション](#entities-collection)
* [レジストレーション・コレクション](#registrations-collection)
* [csubs コレクション](#csubs-collection)
* [casubs コレクション](#casubs-collection)

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
    -   **servicePath** : [サービスパス機能](../user/service_path.md)に関連します
-   **attrs** は、そのエンティティに対して作成されたさまざまな属性のキーマップです。キーは、属性名で生成されます ("." は MongoDB document keys では有効な文字ではないので、"=" に変更します)。マップの各要素には、次の情報があります :
    -   **type**: 属性型
    -   **value** : 属性値 (少なくとも1つの更新を受け取った属性の場合) です。バージョン 0.10.1 までは、この値は常に文字列ですが、0.11.0 ではこの値は構造化値を表す JSON オブジェクトまたは JSON ベクトルでもあります ([ユーザ・マニュアルの属性の構造化された値](../user/structured_attribute_valued.md)のセクションを参照ください)
    -   **md** (オプション) : カスタム・メタデータです。これはメタデータオブジェクトのキーマップです。キーはメタデータ名で生成されます ("." は MongoDB document keys では有効な文字ではないので、"=" に変更します)。例えば、"m.x" というメタデータは "m = x" キーを使用します。各キーのオブジェクト値には、メタデータの **type** と **value** の2つのフィールドがあります
    -   **mdNames** : 文字列の配列です。その要素は、属性のメタデータの名前です。ここでは "." から "=" への置換は行われません
    -   **creDate** : 属性の作成に対応するタイムスタンプ (整数として) です (追加の結果として)
    -   **modDate** : 最後の属性の更新に対応するタイムスタンプ(整数として) です。作成後に属性が変更されていない場合は、creDate と一致します
-   **attrNames** : 文字列の配列です。その要素は、エンティティの属性の名前 (IDs なし) です。この場合、 "." から "=" への置換は行われません
-   **creDate** : エンティティ作成日 (追加の結果として)に対応するタイムスタンプ(整数として) です
-   **modDate** : 最後のエンティティ更新に対応するタイムスタンプ (整数として) です。少なくとも1つの属性に対応する modDate と同じであることに注意してください (いつも同じではありません : 最後の更新が DELETE オペレーションの場合は同じではありません)。エンティティが作成後に変更されていない場合は、creDate と一致します
-   **location** (オプション) : エンティティの地理的位置です、次のフィールドで構成されます :
    -   **attrName** : attrs 配列内の地理的位置を識別する属性名です
    -   **coords** : エンティティの位置を表す GeoJSON です。詳細は以下を参照してください
-   **lastCorrelator** : エンティティ上の最後の更新要求における `Fiware-Correlator` ヘッダの値です。自己通知ループ保護ロジックによって使用されます
-   **expDate** (オプション): エンティティの有効期限のタイムスタンプ (Date オブジェクトとして)。詳細については、[一時的なエンティティの機能](../user/transient_entities.md)を参照してください

`location.coordsin` については、いくつかの形式を使用することができます :

* ポイントを表します (geo:point で使用されるもの) :

```
{
  "type": "Point",
  "coordinates": [ -3.691944, 40.418889 ]
}
```

* ラインを表します (geo:line で使用されているもの) :

```
{
  "type": "LineString",
  "coordinates": [ [ 10, 0], [0, 10] ]
}
```

* ポリゴンを表します (geo:box と geo:polygon で使用されるもの) :

```
{
  "type": "Polygon",
  "coordinates": [ [ [ 10, 0], [0, 10], [0, 0], [10, 0] ] ]
}
```

* 最後に、`location.coords` は、[GeoJSON](http://www.macwright.org/2015/03/23/geojson-second-bite.html) 形式で場所を表現する任意の JSON オブジェクトを保持できます。任意の GeoJSON は geo:json 属性型で使用でき、有効なオブジェクトを導入するのはユーザの責任です。上記3つのケースは、実際には "fixed" ケースの GeoJSON 表現です。

座標ペアは、[geo-location API](../user/geolocation.md) で使用されている順序とは反対の経度－緯度順 (longitude-latitude order) を使用することに注意してください。これは内部の [MongoDB ジオロケーション実装](http://docs.mongodb.org/manual/tutorial/query-a-2dsphere-index/) (GeoJSON に基づいています) が経度－緯度順を使用するためです。しかし、ユーザに近い他のシステム (Google マップなど) では、緯度－経度形式 (latitude-longitude format) を使用しているため、後者を API に使用しています。

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
           "creDate" : 1389376081,
           "modDate" : 1389376120,
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
           "creDate" : 1389376244,
           "modDate" : 1389376244
       },
       "position": {
           "type": "location",
           "value": "40.418889, -3.691944",
           "creDate" : 1389376244,
           "modDate" : 1389376244
       }
   },
   "attrNames": [ "A1", "A2", "position" ],
   "creDate": 1389376081,
   "modDate": 1389376244,
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
-   **format** : 転送されたリクエストを送信するために使用する形式。現在のところ唯一認められている値は **JSON** (NGSIv1 フォーマットを意味します) ですが、将来変更される可能性があります ([NGSIv2 ベースの転送に関する問題](https://github.com/telefonicaid/fiware-orion/issues/3068))
-   **servicePath** : [サービスパス機能](../user/service_path.md)に関連します
-   **status** (オプション) : `active` (アクティブなサブスクリプションの場合) または `inactive` (非アクティブなサブスクリプションの場合)。デフォルト・ステータス (すなわち、ドキュメントがこのフィールドを省略した場合) は "active" です
-   **description** (オプションフィールド) : サブスクリプションを説明するフリーテキスト文字列。最大長は1024です
-   **expiration** : これはレジストレーションが失効するタイムスタンプです
-   **contextRegistration** : 要素に以下の情報が含まれる配列です :
    -   **entities** : エンティティのリストを含む配列です (必須)。各エンティティの JSON には、**id**, **type** および **isPattern** が含まれています
    -   **attrs** : 属性のリストを含む配列です (オプション)。各属性の JSON には、**name**, **type** および **isDomain** が含まれています
    -   **providingApplication** : このレジストレーションのための提供アプリケーションの URL です (必須)

サンプルドキュメント :

```
 {
   "_id": ObjectId("5149f60cf0075f2fabca43da"),
   "format": "JSON",
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
                   "type": "TA1",
                   "isDomain": "false"
               },
               {
                   "name": "A2",
                  "type": "TA2",
                   "isDomain": "true"
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
-   **servicePath** : [サービスパス機能](../user/service_path.md)に関連します。これは、サブスクリプションによってカプセル化されたクエリに関連付けられたサービスパスです。デフォルトは `/#` です
-   **expiration** : サブスクリプションの有効期限が切れるタイムスタンプです。永続的なサブスクリプションの場合、不当に高い値が使用されます。ソースコードの PERMANENT_SUBS_DATETIME を参照してください
-   **lastNotification** : 最後の通知が送信された時刻です。これは、通知が送信されるたびに更新され、スロットリング違反を回避します
-   **throttling** : 通知の最小間隔です。0または -1 は、スロットリングがないことを意味します
-   **reference** : 通知の URL です
-   **entities** : エンティティの配列 (必須) です。各エンティティの JSON には、**id**, **type**, **isPattern** および **isTypePattern** が含まれています。従来の理由から、**isPattern** は `"true"` または `"false"` (テキスト) で、**isTypePattern** は `true` または `false` (ブール値) であることに注意してください
-   **attrs** : 属性名の配列 (文字列) (オプション) です
-   **blacklist** : `attrs` をホワイトリスト (もし `blacklist` が `false` また存在しない場合) またはブラックリスト (もし `blackslist` が `true` の場合) として解釈する必要があるかどうかを指定するブール値フィールドです
-   **metadata** : メタデータ名 (文字列) の配列 (オプション) です
-   **conditions** : 通知をトリガーする属性のリストです。
-   **expression** : 更新が来たときに通知を送信するかどうかを評価するために使用される式です。 次のフィールドで構成されています : q, mq, georel, geometry and/or coords (オプション)
-   **count** : サブスクリプションに関連付けられて送信された通知の数です
-   **format** : 通知を送信するために使用する形式。可能な値はは **JSON**  (NGSIv1 レガシー形式の JSON 通知を意味する)、**normalized**, **keyValues**, **values** (最後の3つは NGSIv2 形式で使用されます) です
-   **status** : `active` (アクティブなサブスクリプションの場合) または `inactive` (非アクティブなサブスクリプションの場合)
-   **description** (オプションフィールド) : サブスクリプションを説明するフリーテキスト文字列。最大長は1024です
-   **custom** : このサブスクリプションがカスタマイズされた通知 (NGSIv2 API の機能) を使用するかどうかを指定するブール値フィールドです。このフィールドが存在し、その値が "true" であれば、カスタマイズされた通知が使用されていて、`headers`, `qs`, `method` および `payload` フィールドは考慮されています
-   **headers** : NGSIv2 の通知カスタマイズ機能の HTTP ヘッダーキーマップを格納するためのオプションのフィールドです
-   **qs** : NGSIv2 の通知カスタマイズ機能のためのクエリパラメータのキーマップを格納するオプションのフィールドです
-   **method** : NGSIv2 の通知カスタマイズ機能の HTTP メソッドを格納するためのオプションのフィールドです
-   **payload** : NGSIv2 の通知カスタマイズ機能のペイロードを格納するオプションのフィールドです
-   **lastFailure** : 最後の通知の失敗が発生した時刻です。サブスクリプションが失敗したことがない場合は存在しません
-   **lastSuccess** : 最後に成功した通知が発生した時刻です。サブスクリプションが通知をうまく引き起こさなかった場合は存在しません

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
        "status" : "active"
}
```
[トップ](#top)

<a name="casubs-collection"></a>
## casubs コレクション

*casubs* コレクションは、コンテキスト・アベイラビリティのサブスクリプションに関する情報を格納します。コレクション内の各ドキュメントはサブスクリプションに対応しています。

フィールド :

-   **\_id** : サブスクリプション ID (サブスクリプションを更新およびキャンセルするためにユーザに提供される値) です。このために \_id を使用すると、サブスクリプション ID は一意であり、サブスクリプション ID によるクエリは非常に高速です (\_id に自動デフォルト・インデックスがあるため)
-   **expiration** : これは、サブスクリプションが期限切れになるタイムスタンプです
-   **reference** : 通知を送信する URL です
-   **entities** : エンティティの配列 (必須) です。各エンティティの JSON には、**id**, **type**, および **isPattern** が含まれています。**isTypePattern** は `true` または `false` (boolean) であるのに対し、従来の理由から、**isPattern ** は `"true"` または `"false"` (text) であることに注意してください
-   **attrs** : 属性名の配列 (文字列) (オプション) です
-   **lastNotification** : 特定のサブスクリプションに関連付けられて送信された最後の通知に通知するタイムスタンプです
-   **count** : サブスクリプションに関連付けられて送信された通知の数です。
-   **format** : 通知を送信するための形式です。現在、JSON 通知を意味する "JSON" は NGSIv1 形式です。

サンプルドキュメント :

```
 {
   "_id": ObjectId("51756c2220be8dc1b5f415ff"),
   "expiration": 1360236300,
   "reference": "http://notify.me",
   "entities": [
       {
           "id": "E5",
           "type": "T5",
           "isPattern": "false"
       },
       {
           "id": "E6",
           "type": "T6",
           "isPattern": "false"
       }
   ],
   "attrs": [
       "A1",
       "A2"
   ],
   "lastNotification" : 1381132312,
   "count": 42,
   "format": "JSON"
 }
```
[トップ](#top)
