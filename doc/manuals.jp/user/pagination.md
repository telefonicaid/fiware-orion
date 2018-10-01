# ページネーション (Pagination)

NGSIv2 は、クライアントが大きなリソースセットを取得するのを助けるために、ページ分割メカニズムを実装しています。このメカニズムは、API 内のすべてのリストのオペレーションで働きます。例えば、`GET/v2/entities`, `GET/v2/subscriptions`, `POST/v2/op/query` など。

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

<a name="ordering-results"></a>
## オーダーリングの結果

エンティティ・クエリの場合、`orderBy` URL parameter を使用して、作成時刻とは異なる順序を指定することができます。特に、`orderBy` の値はカンマで区切られた属性のリストであり、結果は最初の属性によって順序付けられます。結びつけて、結果は第2の属性などによって順序付けされます。 属性名の前の "!" は、順序が逆になっていることを意味します。たとえば :

    GET <orion_host>:1026/v2/entities?orderBy=temperature,!humidity

最初に昇順で温度によって、次いで温度結合の場合には降順で湿度によって順序付けられます。

特殊キーワードの `dateCreated` と `dateModified` は、`orderBy` カンマ区切りリスト (`!` 構文を含む) の要素として、エンティティの作成時刻とエンティティの変更時刻をそれぞれ意味するために使用できることに注意してください。
