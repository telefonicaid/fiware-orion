# ページネーション (NGSIv2 を使用) (Pagination)

NGSIv2 は、NGSIv1 で実装されているものと非常に似ている、ページ分割メカニズムを実装して、クライアントが大きなリソースセットを取得するのを助けます。このメカニズムは、API 内のすべてのリストのオペレーションで働きます。例えば、`GET/v2/entities`, `GET/v2/subscriptions`, `POST/v2/op/query` など。

このメカニズムは、次の3つの URI パラメータに基づいています :

-   **limit** : 要素の最大数を指定します。デフォルトは20、最大値は1000です

-   **offset** : 開始時に指定された数の要素をスキップします。デフォルトは0です

-   **count** (` オプション `) : 有効な場合、レスポンスに合計要素数の `Fiware-Total-Count` ヘッダが追加されます

デフォルトで、結果は作成時間の昇順で返されます。エンティティ・クエリの場合、これは `orderBy` [URL parameter](#ordering-results)で変更できます。

例をあげてみましょう : 特定のクライアントが単一のレスポンスで100以上の結果を処理することはできず、クエリには合計322の結果が含まれています。クライアントは以下を行うことができます。完全性のために URL のみが含まれています。

    GET <orion_host>:1026/v2/entities?limit=100&options=count
    ...
    (`Fiware-Total-Count : 322` ヘッダと共に、最初の100要素が返されます。これにより、クライアントが合計エンティティ数を知り、それに続くクエリの数を知ることができます。)

    GET <orion_host>:1026/v2/entities?offset=100&limit=100
    ...
    (Entities from 101 to 200)

    GET <orion_host>:1026/v2/entities?offset=200&limit=100
    ...
    (Entities from 201 to 300)

    GET <orion_host>:1026/v2/entities?offset=300&limit=100
    ...
    (Entities from 301 to 222)

リクエストが結果の合計数を超えるオフセットを使用する場合、以下に示すように空のリストが返されることに注意してください :

```
GET <orion_host>:1026/v2/entities?offset=1000&limit=100
...
[]
```

# ページネーション (NGSIv1 を使用)

クライアントが多数のレスポンス (たとえば、queryContext リクエストに対する単一の HTTP レスポンスで 1,000,000 の結果に一致するクエリをどのくらいのコストで返すことができるかなど) を問わず、クエリおよびディスカバリ・リクエストを構成するために、[queryContext](walkthrough_apiv1.md#query-context-operation) および [関連するコンビニエンス・オペレーション](walkthrough_apiv1.md#Convenience_Query_Context) そして、[discoverContextAvailability](walkthrough_apiv1.md#discover-context-availability-operation)(および [関連するコンビニエンス・オペレーション](walkthrough_apiv1.md#convenience-discover-context-availability)がページ付けを可能にします。このメカニズムは、次の3つの URI パラメータに基づいています :

-   **limit** : queryContext と discoverContextAvailability のためのエンティティまたはコンテキスト・レジストレーションの最大数を指定します。デフォルトは20、最大許容値は1000

-   **offset** : 開始時に指定された数の要素をスキップします。デフォルトは0

-   **details** ("on" または "off" が指定でき、デフォルトは "off") : "on" を使用した場合の総要素数を含むレスポンスのグローバル errorCode を取得するために使用します。"on" に設定された詳細を使用すると、NGSI 標準が少し壊れていることに注意してください。グローバルな errorCode は、リクエストの一般的なエラーの場合にのみ使用する必要があります。しかし、クライアントは、クエリの合計でいくつの結果が得られているかを事前に知ることは非常に便利だと思います。NGSI を厳格に保ちたい場合は、単に詳細パラメータを無視できます :)

デフォルトで、結果はエンティティ/レジストレーションの作成時間の昇順よって順序付けられて返されます。これは、クライアントがすべての結果を通過している間に新しいエンティティ/レジストレーションが作成された場合、新しい結果が最後に追加される (重複結果を避ける) ことを保証するためです。エンティティ・クエリの場合、これは、[`orderBy` URL parameter](#ordering-results) で変更できます。

例をあげてみましょう : 特定のクライアントは単一のレスポンスで 100以上の結果を処理できず、queryContext には合計322の結果が含まれています。クライアントは以下を行うことができます。完全性のために URL のみが含まれています。

    POST <orion_host>:1026/v1/queryContext?limit=100&details=on
    ...
    (レスポンスの最初の100要素が次の errorCode と共に返されます。これにより、クライアントは、いくつのエンティティが合計されているかを知ることができ、したがって、実行するサブシーケンス・クエリの数を知ることができます)

      "errorCode": {
        "code": "200",
        "reasonPhrase": "OK",
        "details": "Count: 322"
      }

    POST <orion_host>:1026/v1/queryContext?offset=100&limit=100
    ...
    (Entities from 101 to 200)

    POST <orion_host>:1026/v1/queryContext?offset=200&limit=100
    ...
    (Entities from 201 to 300)

    POST <orion_host>:1026/v1/queryContext?offset=300&limit=100
    ...
    (Entities from 301 to 222)

リクエストが "範囲外" オフセットを使用する場合は、以下に示すように 404 NGSI エラーが発生することに注意してください :

```
POST <orion_host>:1026/v1/queryContext?offset=1000&limit=100
...
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context element found",
        "details": "Number of matching entities: 5. Offset is 1000"
    }
}
```

<a name="ordering-results"></a>
## オーダーリングの結果

エンティティ・クエリの場合、`orderBy` URL parameter を使用して、作成時刻とは異なる順序を指定することができます。特に、`orderBy` の値はカンマで区切られた属性のリストであり、結果は最初の属性によって順序付けられます。結びつけて、結果は第2の属性などによって順序付けされます。 属性名の前の "!" は、順序が逆になっていることを意味します。たとえば、NGSIv2 の場合 :

    GET <orion_host>:1026/v2/entities?orderBy=temperature,!humidity

または、NGSIv1 の場合 :

    POST <orion_host>:1026/v1/queryContext?orderBy=temperature,!humidity

最初に昇順で温度によって、次いで温度結合の場合には降順で湿度によって順序付けられます。

特殊キーワードの `dateCreated` と `dateModified` は、`orderBy` カンマ区切りリスト (`!` 構文を含む) の要素として、エンティティの作成時刻とエンティティの変更時刻をそれぞれ意味するために使用できることに注意してください。
