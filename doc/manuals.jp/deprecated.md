# 非推奨の機能

廃止された機能は、Orion が引き続きサポートしている機能ですが、それ以上の延長や進化はありません。特に :

-   廃止された機能に関連し、他の機能に影響を与えないバグや問題は扱いません。それらは github.com では見つかるとすぐにクローズされます
-   廃止予定の機能に関するドキュメントは、リポジトリのドキュメントから削除されています。ドキュメントは、以前のバージョン (リポジトリのリリース・ブランチまたは FIWARE wiki の pre-0.23.0 のドキュメントのいずれか) に関連付けられたドキュメント・セットで引き続き利用できます
-   廃止された機能は最終的には Orion から削除されます。したがって、廃止された機能に依存しないために、Orion を使用して実装を変更することを強くお勧めします

推奨されなくなった機能のリストと、廃止された機能のバージョンは次のとおりです :

* Orion 2.1.0 の Rush サポート (関連するCLIパラメータ : `-rush`)
* Orion 2.0.0 での NGSIv1 (関連する CLI パラメータ : `-strictNgsiv1Ids`, `-ngsiv1Autocast`)。代わりに NGSIv2 API を使用してください
* `POST /v2/op/query` の `attributes` フィールドは、Orion 1.15.0 にあります。これらの属性を持つエンティティのみを返すためには、クエリに対するレスポンスにどの属性を含めるかを選択する `attrs` と、`expression` 内の `q` の単項属性フィルタ (unary attribute filter) の組み合わせです。それらを代わりに指定していください
* Orion 1.14.0 では `POST /v2/op/update` の `APPEND`, `APPEND_STRICT`, `UPDATE`, `DELETE`,  `REPLACE` の使用は非推奨です。`append`, `appendStrict`, `update`, `delete`, `replace` を代わりに使ってください
* Orion 1.13.0 ではメタデータ ID が推奨されていません (Orion 2.2.0 で削除されました)。一方、この機能は NGSIv2 と互換性がありません。JSON 表現形式の属性名は JSON オブジェクトのキーとして使用されるため、名前を複製することはできません。一方、IDs は、属性名にプレフィックス/サフィックスを使用して簡単に実装することができます。たとえば、`temperature:ground` および `temperature:ceiling` です。 この非推奨の結果、次のオペレーションも非推奨になりました :
    * `GET /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
    * `GET /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
    * `POST /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
    * `PUT /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
    * `PUT /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
    * `DELETE /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
    * `DELETE /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
* Orion 1.5.0 では、NGSIv2 で `dateCreated` および/または `dateModified` 属性を含めるために `optionsURL` パラメータ使用することは推奨されていません。代わりに `attrs`URI パラメータを使用してください
* パス・プレフィックスとして /ngsi10 そして /ngsi9URL は、orion 1.2.0 で廃止されました。代わりに `/v1` と `/v1/registry` を使用してください
* エンティティの場所を指定する `location` メタデータは、Orion 1.1.0 では非推奨です。エンティティの場所を指定する新しい方法は、属性の `geo:point` 型を使用することです。[ユーザ・マニュアルの対応するセクション](user/geolocation.md)を参照してください) 
* Orion 0.26.1 のコマンドライン引数は廃止されました。Orion 1.0.0 で削除されました
    * **--silent** : エラー以外のすべてのログ出力を抑止します。代わりに `-logLevel ERROR` を使用してください
* ONTIMEINTERVAL サブスクリプションは Orion 0.26.0 以降で廃止されました。Orion 1.0.0 では削除されました。ONTIMEINTERVAL サブスクリプションにはいくつかの問題があります。CB に状態 (state) を導入するため、水平スケーリングの設定をより困難にし、ページネーション/フィルタリングの導入を困難にします。実際には、ONTIMEINTERVAL 通知に基づくユース・ケースは、レセプタが queryContext を同じ頻度で実行する等価なユース・ケースに変換できるため、実際には必要ありません。ページ区切りやフィルタリングなどの queryContext の機能を利用してください
* XML は Orion 0.23.0 以降で廃止されました。Orion 1.0.0 では削除されました
* Orion 0.21.0 で以下のコマンドライン引数は廃止さました。0.25.0 で削除されました :
    * **-ngsi9** : broker は NGSI9 のみを実行します。NGSI10 は使用しません
    * **-fwdHost <host>** : broker が "ConfMan モード" で動作している場合、NGIS9 registerContext のホストを転送します
    * **-fwdPort <port>** : broker が "ConfMan モード" で動作している場合の NGIS9 registerContext の転送ポートです
* Configuration Manager のロールは、0.21.0 で非推奨になり、0.25.0 で削除されました
* Associations は、0.21.0 で非推奨になり、0.25.0 で削除されました

## 古い Orion バージョンの使用

常に最新の Orion バージョンを使用することをお勧めしますが、古いバージョンを使用する場合は、次の情報を考慮してください :

* Orion 0.1.1 以降の古い RPMs は [FIWARE yum repository](http://repositories.lab.fiware.org/repo/rpm/6/x86_64) で利用できます
* [Orion github repository](http://github.com/telefonicaid/fiware-orion) には、旧リリースに対応するコード(Orion 0.8.1、オープンソースとして最初に提供されたもの)が用意されています。各リリース番号 (例 : 0.23.0) には、以下が関連付けられています :
	 * tag : 例えば `0.23.0`。ベース・バージョンを指しています
	 * release branch : `release/0.23.0`。このブランチの HEAD は通常、前述のタグと一致します。ただし、一部の修正プログラムがベースバージョンで開発された場合、このブランチにはこのような修正プログラムが含まれています
* 古いバージョンに対応するドキュメント :
	 * 0.23.0 以前では、ドキュメントは FIWARE public wiki ([ユーザ・マニュアル](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide)と[管理マニュアル](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide)) にあります
	 * 0.24.0 以上の場合は、[readthedocs.io](https://fiware-orion.readthedocs.io) でドキュメントを入手できます。左下のパネルを使用して、正しいバージョンに移動します
* Orion 0.24.0 以降に対応する Docker イメージは [Docker Hub](https://hub.docker.com/r/fiware/orion/tags/) にあります

次の表は、現在削除されている機能をサポートする最新の Orion バージョンについての情報を提供します :

| **削除された機能**                                                         | **機能をサポートする Orion ラスト・バージョン** | **バージョンのリリース日**      |
|----------------------------------------------------------------------------|-------------------------------------------------|---------------------------------|
| Rush (関連する CLI パラメータ : `-rush`)                                   | まだ定義されていません                          | まだ定義されていません          |
| NGSIv1 (関連する CLI パラメータ : `-strictNgsiv1Ids`, `-ngsiv1Autocast`)   | まだ定義されていません                          | まだ定義されていません          |
| `POST /v2/entities` オペレーションの `attributes` フィールド               | まだ定義されていません                          | まだ定義されていません          |
| `APPEND`, `UPDATE`, など。`POST /v2/op/update` でのアクション・タイプ      | まだ定義されていません                          | まだ定義されていません          |
| URI パラメータでの `dateCreated` および `dateModified`                     | まだ定義されていません                          | まだ定義されていません          |
| `/ngsi10` および `/ngsi9` URL プレフィックス                                  | まだ定義されていません                          | まだ定義されていません          |
| エンティティのロケーションを指定する `location` メタデータ                 | まだ定義されていません                          | まだ定義されていません          |
| `id` メタデータとそれに関連する NGSIv1 オペレーション                      | 2.2.0                                           | 2018年12月19日                  |
| XML API                                                                    | 0.28.0                                          | 2016年2月29日                   |
| ONTIMEINTERVAL subscription                                                | 0.28.0                                          | 2016年2月29日                   |
| CLI `--silent`                                                             | 0.28.0                                          | 2016年2月29日                   |
| Configuration Manager のロール (`-ngsi`, `-fwdHost` および `-fwdPort` を含む) | 0.24.0                                    | 2015年9月14日            |
| Associations (アソシエーション)                                            | 0.24.0                                          | 2015年9月14日                   |
