# <a name="top"></a> データベース管理

* [イントロダクション](#introduction)
* [バックアップ](#backup)
* [リストア](#restore)
* [データベースの承認](#database-authorization)
* [マルチサービス/マルチテナント・データベース分離](#multiservicemultitenant-database-separation)
* [完全なデータベースの削除](#delete-complete-database)
* [インデックスの設定](#setting-indexes)
* [データベース管理スクリプト](#database-management-scripts)
    * [期限切れのドキュメントの削除](#deleting-expired-documents)
    * [最新の更新ドキュメント](#latest-updated-document)
* [データベースによる Orion エラー](#orion-errors-due-to-databa)
	  
<a name="introduction"></a>
## イントロダクション

システム管理者は MongoDB の知識を持っていると仮定します ([MongoDB 教育サイト](https://education.mongodb.com/)には非常に良いコースと無料のコースがあります)。それ以外の場合は、このセクションで説明する手順に非常に注意することをお勧めします。

<a name="backup"></a>
## バックアップ

MongoDB データベースの通常手順を使用します。

mongobackup コマンドを使用して、Orion Context Broker データベースのバックアップを取得します。バックアップを実行する前に broker を停止することを強くお勧めします。

```
mongodump --host <dbhost> --db <db>
```

これにより、 `dump/` ディレクトリにバックアップが作成されます。

[マルチテナント/マルチサービス](#multiservicemultitenant-database-separation)を使用している場合は 、各テナント/サービスのデータベースに手順を適用する必要があります。

[トップ](#top)

<a name="restore"></a>
## リストア

MongoDB データベースの通常手順を使用します。

Orion Context Broker データベースの以前のバックアップをリストアするには、mongorestore コマンドを使用します。バックアップを実行する前に broker を停止し、broker が使用するデータベースを削除 (drop) することを強くお勧めします。

バックアップは `dump/<db>` ディレクトリ . にあると仮定しましょう。それをリストアするには :

```
mongorestore --host <dbhost> --db <db> dump/<db>
```

[マルチテナント/マルチサービス](#multiservicemultitenant-database-separation)を使用している場合は 、各テナント/サービスのデータベースに手順を適用する必要があります。

[トップ](#top)

<a name="database-authorization"></a>
## データベースの認証

MongoDB の認証は `-db, `-dbuser` と `-dbpwd` オプションで設定されます ([コマンドライン・オプションのセクション](cli.md)を参照)。考慮するいくつかの異なるケースがあります :

-   MongoDB インスタンス/クラスタが認証を使用していない場合は、`-dbuser` と `-dbpwd` オプションは使用しないでください
-  `-dbAuthMech` で認証メカニズムを指定できます
-   MongoDB インスタンス/クラスタが認可を使用している場合は、次のようになります :
    -   Orion をシングルサービス/テナントモードで実行している場合 (つまり `-multiservice` でない場合)、1つのデータベース (-db オプションで指定されたもの) のみを使用しているので、認証は、そのデータベースで `-dbuser` と `-dbpwd` を使用して行われます
    -   Orion をマルチサービス/テナントモードで実行している場合 (つまり `-multiservice` の場合)、認証は、`admin` データベースで `-dbuser` と `-dbpwd` を使用して行われます。[このドキュメントの後半](#multiservicemultitenant-database-separation)で説明するように、マルチサービス/テナントモードでは、Orion はいくつかのデータベース (潜在的にオンザフライで作成される可能性があります) を使用します。`admin` データベース上での認証は、それらの全てで許可します

     
[トップ](#top)

<a name="multiservicemultitenant-database-separation"></a>
## マルチサービス/マルチテナント・データベース分離

通常、Orion Context Broker は MongoDB レベル (`-db` コマンドライン・オプションで指定されたもの、通常は "orion")で1つのデータベースのみを使用します。ただし、[マルチテナント/マルチサービス](#multiservicemultitenant-database-separation)を使用する場合は動作が異なり、次のデータベースが使用されます (`<db>` を `-db` コマンドライン・オプションの値にしてください) :

-   既定テナント用のデータベース `<db>` (通常は `orion`)
-   サービス/テナント用 `<tenant>` のデータベース `<db>-<tenant>` (テナントの名前が `tenantA` と付けられ、デフォルト `-db` が使用されている場合、データベースは `orion-tenantA` になります)

テナントデータを含む最初のリクエストが Orion によって処理されることで、サービスごと/テナントのデータベースは "オンザフライ" で作成されます。

最後に、サービス/テナントデータベースごとに、すべてのコレクションおよび管理手順 (バックアップ、リストアなど) が特定のサービス/テナントデータベースに関連付けられます。

[トップ](#top)

<a name="delete-complete-database"></a>
## データベースを完全に削除

この操作は、MongoDB シェルを使用して行われます :

```
mongo <host>/<db>
> db.dropDatabase()
```
[トップ](#top)

<a name="setting-indexes"></a>
## インデックスの設定

パフォーマンス・チューニングのドキュメントの [データベース・インデックスのセクション](perf_tuning.md#database-indexes)を確認してください。

[Top](#top)

<a name="database-management-scripts"></a>
## データベース管理スクリプト

Orion Context Broker には、`/usr/share/contextBroker` ディレクトリ内にインストールされた、データベース内の参照および管理アクティビティに使用できるいくつかのスクリプトが付属しています。

これらのスクリプトを使用するには、pymongo ドライバ (バージョン 2.5 以上) をインストールする必要があります (通常は root として実行するか、sudo コマンドを使用します) :

` pip-python install pymongo`

[トップ](#top)

<a name="deleting-expired-documents"></a>
### 期限切れのドキュメントの削除

NGSI は、レジストレーションとサブスクリプション (コンテキストとコンテキスト・アベイラビリティの両方のサブスクリプション) の有効期限を指定します。期限切れのレジストレーション/サブスクリプションはサブスクリプション更新リクエストを使用して "再アクティブ化" することができて、期間を変更するため、Orion Context Broker は期限切れのドキュメントを削除しません (無視されます)。

ただし、有効期限が切れたレジストレーション/サブスクリプションはデータベース内の領域を消費するため、時々 "パージ" することができます。その作業を手助けするために、garbage- collector.py スクリプトが Orion Context Broker (RPM のインストール後に /usr/share/contextBroker/garbage-collector.py にあります) と一緒に提供されています。


garbage-collector.py は、registrations, csubs および casubs コレクション内の期限切れのドキュメントを探し、次のフィールドでそれらを "マーク (marking)" します :

```
{
  ...,
  "expired": 1,
  ...
}
```

garbage-collector.py プログラムは、解析されるコレクションを引数として取ります。たとえば、csubs と casubs を分析するために実行します :

```
garbage-collector.py csubs casubs
```

garbage-collector.py を実行した後は、mongo コンソールの次のコマンドを使用して、期限切れのドキュメントを簡単に削除できます :

```
mongo <host>/<db>
> db.registrations.remove({expired: 1})
> db.csubs.remove({expired: 1})
> db.casubs.remove({expired: 1})
```
[トップ](#top)

<a name="latest-updated-document"></a>
### 最新の更新ドキュメント

latest-updates.py スクリプトを使用して、データベース内の最新の更新されたエンティティおよび属性のスナップショットを取得することができます。最大4つの引数が必要です :

-   "entities" または "attributes", のいずれかを使用して更新の粒度レベルを設定します
-   使用するデータベース (broker で使用される -db パラメータおよび BROKER\_DATABASE\_NAME と同じ)。mongod インスタンスは、スクリプトが実行されるのと同じマシンで実行する必要があることに注意してください
-   プリントする行の最大数
-   (オプション) エンティティ IDs のフィルタ。データベース・クエリの正規表現として解釈されます

Ej:

    # latest-updates.py entities orion 4
    -- 2013-10-30 18:19:47: Room1 (Room)
    -- 2013-10-30 18:16:27: Room2 (Room)
    -- 2013-10-30 18:14:44: Room3 (Room)
    -- 2013-10-30 16:11:26: Room4 (Room)

[トップ](#top)

<a name="orion-errors-due-to-databa"></a>
## データベースによる Orion エラー

大きなオフセット値を使用してエンティティを取得し、このエラーが発生した場合 :

```
GET /v2/entities?offset=54882

{
    "description": "Sort operation used more than the maximum RAM. You should create an index. Check the Database Administration section in Orion documentation.",
    "error": "InternalServerError"
}
```

DB はリソースの不足によるソート動作の失敗に関連するエラーを発生させている。Orion ログ・ファイルに次のような ERROR トレースが含まれていることを確認できます :

```
Raising alarm DatabaseError: nextSafe(): { $err: "Executor error: OperationFailed Sort operation used more than the maximum 33554432 bytes of RAM. Add an index, or specify a smaller limit.", code: 17144 }
```

これに対する典型的な解決策は、ソートに使用されるフィールドにインデックスを作成することです。特に、(作成日に基づいて) デフォルトのエンティティの順序を使用している場合は、mongo シェルで次のコマンドを使用してインデックスを作成できます :

```
db.entities.createIndex({creDate: 1})
```

[トップ](#top)
