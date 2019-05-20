# <a name="top"></a>コンテキスト・プロバイダ

* [更新リクエストの転送](#forwarding-of-update-requests)
* [クエリ・リクエストの転送](#forwarding-of-query-requests)
* [エンティティのシャドーイングに関する警告](#a-caveat-about-shadowing-entities)

Orion Context Broker は、[ユーザ&プログラマ・マニュアル](../user/context_providers.md)で説明されているように、コンテキスト・プロバイダのコンセプトをサポートしています。要するに、更新/クエリの場合、エンティティ/属性が見つからない場合、Orion はそのレジストレーション・リストをチェックし、そのリストにある場合、そのエンティティがあるコンテキスト・プロバイダに登録されていることを意味します。したがって、リクエストはそのコンテキスト・プロバイダに転送されます。コンテキスト・プロバイダのIP, ポート および パスは、レジストレーション・リクエスト `RegisterContextRequest` の一部である `struct ContextRegistration` の `providingApplication` フィールドにあります。

<a name="forwarding-of-update-requests"></a>
## 更新リクエストの転送
NGSIv1 (非推奨) では、リクエスト `POST /v1/updateContext` には、`updateActionType` と呼ばれるフィールドがあります。このフィールドは5つの異なる値を取ることができます :

* UPDATE
* APPEND
* DELETE
* APPEND_STRICT
* REPLACE

> サイドノード : 最初の3つは "標準 NGSIv1" であり、後の2つは NGSIv2 のために追加されたものです。

* `UPDATE` または `REPLACE` を伴うリクエストは、リクエストの転送を引き起こす可能性があります。**ローカルで見つからず、レジストレーションで見つかった場合のみ**
* `APPEND` を使用すると、アクションは常にローカルになります。エンティティ/属性がすでに存在する場合は更新されます。そうでない場合は、ローカルに作成されます
* エンティティ/属性がすでにローカルに存在する場合は `APPEND_STRICT ` が失敗し、そうでない場合はエンティティ/属性がローカルに作成されます
* `DELETE` は、常にローカルです

`UPDATE` または `REPLACE` のような複数のコンテキスト要素と `updateActionType` を伴う更新リクエストは、異なるコンテキスト・プロバイダへのいくつかの転送と、ローカルに見つかるエンティティ/属性のローカル更新に分割できます。 コンテキスト・プロバイダからのすべてのレスポンスが到着するまで、最初のリクエストに対するレスポンスは送信されません。

<a name="flow-fw-01"></a>
![Forward an update to Context Providers](../../manuals/devel/images/Flow-FW-01.png)

_FW-01: コンテキスト・プロバイダへの更新の転送_

`postUpdateContext()` を呼び出すようになるサービス・ルーチンがあることに注意してください。[サービス・ルーチンマッピングのドキュメント](ServiceRoutines.txt)の詳細を参照してください。

* 着信ペイロードのすべての属性は、**Not Found** とマークされます (ステップ1)
* [mongoBackend ライブラリ](sourceCode.md#srclibmongobackend)はリクエストを処理し(図 [MB-01](mongoBackend.md#flow-mb-01) または [MB-02](mongoBackend.md#flow-mb-02) を参照してください)、次の3つのいずれかの方法でリクエスト内のすべての属性をマークします (ステップ2)

    * Not Found (見つからない)
    * Found in Local Context Broker (ローカルの Context Broker で見つかった)
    * Found in Remote Context Provider (リモートの Context Broker で見つかった)

* リモートのコンテキスト・プロバイダで検出される属性は、転送する必要があります。ローカル属性は単純な更新ですが、見つからなかった属性はレスポンスでそのようにマークされます。
* `ContextElementResponse` の新しいベクトルが作成され、転送されるすべての属性で埋められます (ステップ3)。これらのレスポンスは、**mongoBackend** から出力されたレスポンス・ベクトルに追加されます。属性が "見つからない" 場合、`ContextElementResponse` は 404 Not Found で準備されます
* 内部ループ (ステップ4) : `mongoUpdateContext()` は属性の値を記入しません。これは通常のレスポンスの一部ではありませんが、更新リクエストを転送するために属性の値が存在しなければなりません。このループは、転送されるすべての属性の値を入力します
* 内部ループ (ステップ5) : コンテキスト・プロバイダごとに1つの `UpdateContextRequest` オブジェクトを作成し、これらのオブジェクトを転送される属性で埋めます
* 各リクエストは、すべての属性を含む対応するコンテキスト・プロバイダに送信されます (ステップ3)。図 [FW-02](#flow-fw-02) の詳細を参照してください
* コンテキスト・プロバイダからのレスポンスは、転送を誘発するリクエストを発行しているクライアントへの合計レスポンスにマージされます (ステップ7)。フォワードはシリアライズされ、各フォワードはレスポンスを待ってから続行します。

<a name="flow-fw-02"></a>
![`updateForward()` function detail](../../manuals/devel/images/Flow-FW-02.png)

_FW-02: `updateForward()` 機能詳細_

* コンテキスト・プロバイダの文字列をパーシングして、IP, ポート, URI パスなどを抽出します (ステップ1)
* 転送は REST リクエストとして行われるため、REST リクエストをコンテキスト・プロバイダに送信できるようにオブジェクトをテキスト (JSON) にレンダリングする必要があります (ステップ2)
* 転送リクエストは、`httpRequestSend()` の助けで (ステップ3)、 [libcurl](https://curl.haxx.se/libcurl/) (ステップ4) を使用して送信されます。libcurl は、リクエストをコンテキスト・プロバイダに順番に送信します (ステップ5)
* コンテキスト・プロバイダからのテキストのレスポンスがパーシングされ、`UpdateContextResponse` オブジェクトが作成されます (ステップ6)。構文パーシングの詳細は、図 [PP-01](jsonParse.md#flow-pp-01) に示されています

[Top](#top)

<a name="forwarding-of-query-requests"></a>
## クエリ・リクエストの転送
更新と同様に、クエリもコンテキスト・プロバイダに転送されます。ローカルに見つからないクエリリクエストのすべての属性が登録リストで検索され、見つかった場合、リクエストは対応するコンテキスト・プロバイダに転送されます。更新リクエストの転送に関しては、クエリリクエストを N個の転送に分割することができ、転送リクエストに対するすべてのレスポンスを受信して​​最終レスポンスにマージするまで、初期リクエストに対するレスポンスは送信されません。

<a name="flow-fw-03"></a>
![Forward a query to Context Providers](../../manuals/devel/images/Flow-FW-03.png)

_FW-03: コンテキストをコンテキスト・プロバイダに転送_

`postQueryContext()` を呼び出すようになるサービス・ルーチンがたくさんあることに注意してください。[サービス・ルーチンマッピングのドキュメント](ServiceRoutines.txt)の詳細を参照してください。

`postQueryContext()` は `requestV` と呼ばれる `QueryContextRequest` のベクトルを生成します。このベクトルはそれぞれレンダリングされ、コンテキスト・プロバイダに送られます。
`QueryContextRequest` 項目は、[**mongoBackend**](sourceCode.md#srclibmongobackend) 関数 `mongoQueryContext()` の出力に基づいて記入されます。

* `mongoQueryContext()` が呼び出されて、クエリに一致する属性を見つける場所の "map" を取得します (ステップ1)。図 [MB-07](mongoBackend.md#flow-mb-07)を参照してください。一致するローカル属性は `mongoQueryContext()` からのレスポンスに既に埋め込まれていることに注意してください
* `forwardPending()` 関数が呼び出されます (ステップ2)。`mongoQueryContext()` からのレスポンスに転送が含まれる場合、`true` を返します。そうでなければ `false` を返し、そうであれば `postQueryContext()` が呼び出し元に戻ることができます。`forwardsPending()` が図で `true` を返すと仮定しましょう
* `QueryContextRequest` のベクトルをコンテキスト・プロバイダに転送する各アイテムと `mongoQueryContext()` によって返された、各 `ContextElementResponse` の 各 `Attribute` を作成し、その属性を `QueryContextRequest` のベクトルの正しい項目に入れます (ステップ3)。アイテムが見つからない場合は、アイテムを作成してベクトルに追加します。
* 内部ループ : コンテキスト・プロバイダへの実際の転送 : 
    * `QueryContextRequest` のベクトルの各項目に対して、クエリをコンテキスト・プロバイダに順番に送信し、そのレスポンスを待つために (ステップ4)、`queryForward()` を呼び出します。図 [FW-04][FW-04](#flow-fw-04) を参照してください
    * 関数の合計レスポンスにこのレスポンスをマージします (ステップ5)
* コンテキスト・プロバイダからのすべてのレスポンスとローカル・レスポンス・パーツ (ローカルに検出された属性) をマージして、開始クライアントにレスポンスします

<a name="flow-fw-04"></a>
![`queryForward()` function detail](../../manuals/devel/images/Flow-FW-04.png)

_FW-04: `queryForward()` 機能の詳細_

* コンテキスト・プロバイダの文字列をパーシングして、IP, ポート, URI パスなどを抽出します (ステップ1)
* 転送リクエストを作成する必要があります (ステップ2)。NGSIv1 の場合、POST /v1/queryContext を使用して REST リクエスト (プレーン・テキスト) をコンテキスト・プロバイダに送信できるように、バイナリ・オブジェクトの情報をテキストに抽出する必要があります。NGSIv2 の場合、POST /v2/op/query の使用が必要です。
* 転送リクエストは、`httpRequestSend()` の助けで (ステップ3)、 [libcurl](https://curl.haxx.se/libcurl/) (ステップ4) を使用して送信されます。libcurl は、リクエストをコンテキスト・プロバイダに順番に送信します (ステップ5)
* コンテキスト・プロバイダからのテキストのレスポンスがパーシングされ、`UpdateContextResponse` オブジェクトが作成されます (ステップ6)。構文パーシングの詳細は、図 [PP-01](jsonParse.md#flow-pp-01) に示されています

<a name="a-caveat-about-shadowing-entities"></a>
## エンティティのシャドーイングに関する警告
コンテキスト・プロバイダのメカニズムは、標準のレジストレーション・リクエストを使用して実装されており、これは望ましくない状況につながる可能性があります。私たちは、この潜在的な "シャドーイング" 問題に少なくとも気づくことが重要であると感じています。

以下のシナリオを想像してみてください。

* 属性 A1 を有するエンティティ E1 を供給するコンテキスト・プロバイダ CP1 を有しています。CP1 の E1/A1 に関するレジストレーションが Context Broker に送信されます
* クライアントは Context Broker に E1/A1について問い合わせを行い、E1/A1 はローカルには見つかりませんが、レジストレーションではクライアントは期待される結果を得るため、CP1 への転送を引き起こします
* ここで、リクエストが Context Broker に入り、属性 A1 を持つエンティティ E1 を作成 (APPEND) します
* そして、その問題 : クライアントは Context Broker に E1/A1 について問い合わせを行い、属性がローカルで見つかると単に返されだけです。転送は行われていません

コンテキスト・プロバイダ CP1 の E1/A1 は、Context Broker 経由でシャドウされているため、もはや見ることができません。

[Top](#top)
