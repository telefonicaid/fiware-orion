# フロー・インデックス

このドキュメントでは、開発ドキュメントで使用されているさまざまな図の索引を提供しています。

番号シーケンスにいくつかのギャップがあることに注意してください (たとえば、MB-19 と MB-23 の間)。
これは、Orion Context Broker 機能の変更により一部の図が削除されたためであり、問題ありません。

リクエスト管理の関連図  (RQ management) : 

* [RQ-01: リクエストの受信](sourceCode.md#flow-rq-01)
	* RQ-02 に続きます
* [RQ-02: リクエストの処理](sourceCode.md#flow-rq-02)
	* RQ-01 からの続きです
	* PP-* に続きます
	* MB-* に続きます

パーシングに関連する図 (PP prefix) : 

* [PP-01: NGSIv1 ペイロードのパーシング](jsonParse.md#flow-pp-01)
    * RQ-02, FW-02 または FW-04 からの続きです
* [PP-02: テキスト・ペイロードのパーシング](sourceCode.md#flow-pp-02)
    * RQ-02 からの続きです
* [PP-03: NGSIv2 ペイロードのパーシング](jsonParseV2.md#flow-pp-03)
    * RQ-02 からの続きです

フォワーディングに関連する図 (FW prefix):

* [FW-01: コンテキスト・プロバイダへの更新の転送](cprs.md#flow-fw-01)
	* MB-01 または MB-02 に続きます
	* FW-02 に続きます
* [FW-02: `updateForward()` 関数の詳細](cprs.md#flow-fw-02)
	* FW-01 からの続きです
	* PP-01 に続きます
* [FW-03: クエリをコンテキスト・プロバイダに転送](cprs.md#flow-fw-03)
	* MB-07 に続きます
	* FW-04 に続きます
* [FW-04: `queryForward()` 機能の詳細](cprs.md#flow-fw-04)
	* FW-03 からの続きです
	* PP-01 に続きます

mongoBackend ロジックに関連する図 (MB and MD prefixes):

* [MB-01: mongoUpdate - UPDATE/REPLACE - entity found](mongoBackend.md#flow-mb-01)
    * RQ-02 または FW-01 からの続きです
    * MD-01 に続きます
    * MD-02 に続きます
* [MB-02: mongoUpdate - UPDATE/REPLACE - entity not found](mongoBackend.md#flow-mb-02)
    * RQ-02またはFW-01 からの続きです
    * MD-02 に続きます
* [MB-03: mongoUpdate - APPEND/APPEND_STRICT - existing entity](mongoBackend.md#flow-mb-03)
    * RQ-02 からの続きです
    * MD-01 に続きます
    * MD-02 に続きます
* [MB-04: mongoUpdate - APPEND/APPEND_STRICT - new entity](mongoBackend.md#flow-mb-04)
    * RQ-02 からの続きです
    * MD-01 に続きます
* [MB-05: mongoUpdate - DELETE - not remove entity](mongoBackend.md#flow-mb-05)
    * RQ-02 からの続きです
    * MD-01 に続きます
    * MD-02 に続きます
* [MB-06: mongoUpdate - DELETE - remove entity](mongoBackend.md#flow-mb-06)
    * RQ-02 からの続きです
* [MB-07: mongoQueryContext](mongoBackend.md#flow-mb-07)
    * RQ-02 または FW-03 からの続きです
* [MB-08: mongoEntityTypes](mongoBackend.md#flow-mb-08)
    * RQ-02 からの続きです
* [MB-09: mongoEntityTypesValues](mongoBackend.md#flow-mb-09)
    * RQ-02 からの続きです
* [MB-10: mongoAttributesForEntityType](mongoBackend.md#flow-mb-10)
    * RQ-02 からの続きです
* [MB-11: mongoCreateSubscription](mongoBackend.md#flow-mb-11)
    * RQ-02 からの続きです
* [MB-12: mongoUpdateSubscription](mongoBackend.md#flow-mb-12)
    * RQ-02 からの続きです
* [MB-13: mongoGetSubscription](mongoBackend.md#flow-mb-13)
    * RQ-02 からの続きです
* [MB-14: mongoListSubscriptions](mongoBackend.md#flow-mb-14)
    * RQ-02 からの続きです
* [MB-15: mongoUnsbuscribeContext](mongoBackend.md#flow-mb-15)
    * RQ-02 からの続きです
* [MB-16: mongoSubscribeContext](mongoBackend.md#flow-mb-16)
    * RQ-02 からの続きです
    * MB-11 に続きます
* [MB-17: mongoUpdateContextSubscription](mongoBackend.md#flow-mb-17)
    * RQ-02 からの続きです
    * MB-12 に続きます
* [MB-18: mongoRegisterContext](mongoBackend.md#flow-mb-18)
    * RQ-02 からの続きです
* [MB-19: mongoDiscoverContextAvailability](mongoBackend.md#flow-mb-19)
    * RQ-02 からの続きです
* [MB-23: mongoRegistrationGet](mongoBackend.md#flow-mb-23)
    * RQ-02 からの続きです
* [MB-24: mongoRegistrationsGet](mongoBackend.md#flow-mb-24)
    * RQ-02 からの続きです
* [MB-25: mongoRegistrationCreate](mongoBackend.md#flow-mb-25)
    * RQ-02 からの続きです
* MB-26: mongoRegistrationUpdate (*保留中*)
    * RQ-02 からの続きです
* [MB-27: mongoRegistrationDelete](mongoBackend.md#flow-mb-27)
    * RQ-02 からの続きです
* [MD-01: `processSubscriptions()` function detail](mongoBackend.md#flow-md-01)
    * MB-01, MB-03, MB-04 または MB-05 からの続きです
    * NF-01 または NF-03 に続きます
* [MD-02: `searchContextProviders()` function detail](mongoBackend.md#flow-md-02)
    * MB-01、MB-02、MB-03またはMB-05 からの続きです

通知に関連する図 (NF prefix): 

* [NF-01: スレッド・プールを使用しないエンティティ属性の更新/作成に関する通知](sourceCode.md#flow-nf-01)
  * MD-01 からの続きです
* [NF-03: スレッド・プールによるエンティティ属性の更新/作成に関する通知](sourceCode.md#flow-nf-03)
  * MD-01 からの続きです

サブスクリプション・キャッシュに関連する図 (SC prefix):

* [SC-01: サブスクリプション・キャッシュのリフレッシュ](subscriptionCache.md#flow-sc-01)
* [SC-02: アクティブ-アクティブ構成におけるサブスクリプションの伝播](subscriptionCache.md#flow-sc-02)
