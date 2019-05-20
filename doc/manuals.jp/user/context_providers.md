# コンテキスト・プロバイダのレジストレーションとリクエスト転送

レジスター・コンテキストのオペレーションは、そのレジストレーションに含まれるエンティティ/属性のコンテキスト情報の_ソース_を識別する URL である、"コンテキスト・プロバイダ" という概念を使用します。

この_ソース_ (url) は、`provider` によって提供されます :

```
...
"provider": {
  "http": {
    "url": "http://mysensors.com/Rooms"
  }
}
...
```

Orion がクエリまたは更新オペレーションを受信して、ターゲットのコンテキスト要素をローカルに、(つまり内部データベースに見つけることができなくても、そのコンテキスト要素にコンテキスト・プロバイダがレジストレーションされている場合、Orion はクエリ/更新要求をコンテキスト・プロバイダに転送します。この場合、Orion は純粋な "NGSI proxy" として機能します (つまり、Orion はクエリの結果を内部的にキャッシュしません)。元のリクエストを発行したクライアントの観点から見ると、プロセスはほとんど透過的です。コンテキスト・プロバイダは、クエリ/更新オペレーションをサポートするために (少なくとも部分的に) NGSI API を実装する必要があります。

これを例を使って説明しましょう。

![](../../manuals/user/QueryContextWithContextProvider.png "QueryContextWithContextProvider.png")


* 最初 (メッセージ番号1)、アプリケーションは (おそらくコンテキスト・プロバイダに代わって) Orion に Street4 の温度用のコンテキスト・プロバイダをレジストレーションします。コンテキスト・プロバイダが <http://sensor48.mycity.com/v2> で API を公開していると仮定しましょう
      
```
curl localhost:1026/v2/registrations -s -S -H 'Content-Type: application/json' -H 'Accept: application/json' -d @-  <<EOF
{
  "dataProvided": {
    "entities": [
      {
        "id": "Street4",
        "type": "Street"
      }
    ],
    "duration": "P1M"
    "attrs": [
      "temperature"
    ]
  },
  "provider": {
    "http": {
      "url": "http://sensor48.mycity.com/v2"
    }
  }
}
EOF
```
     
* 次に、クライアントが 温度 (メッセージ番号2) をクエリするとします
     
``` 
curl localhost:1026/v2/entities/Street4/attrs/temperature?type=Street -s -S \
    -H 'Accept: application/json' -d @- | python -mjson.tool
``` 

* Orion は Street4 の温度を知りませんが、(前のステップでのレジストレーションにより) <http://sensor48.mycity.com/v2> が Street4 の温度を知っていることを知っているので、クエリを URL <http://sensor48.mycity.com/v2/entities> に転送します (すなわち、レジストレーション時に `url` フィールドで使用された URL の PATH に "/entities" を追加します)

NGSIv2 転送が使用されている場合、転送されたクエリは次のようになります。
(クライアントのリクエストのように) なぜ、GET /v2/entities が使われないのか
不思議に思うかもしれません。これは、クエリが異なるタイプの複数の
エンティティを含むとき、このオペレーションに制限があるためです
(`POST /v2/op/query` は完全に表現力があり、その制限はありません)。


```
POST http://sensor48.mycity.com/v2/op/query

{
  "entities": [
    {
      "id": "Street4",
      "type": "Street"
    }
  ],
  "attrs": [
    "temperature"
  ]
}
```

* <http://sensor48.mycity.com/v2> のコンテキスト・プロバイダは、ペイロード (メッセージ番号4) でレスポンスします :

``` 
[
  {
    "id": "Street4",
    "type": "Street",
    "temperature": {
      "value": "16",
      "type": "float"
    }
  }
]
``` 

* Orion はクライアントへのレスポンスを転送します (メッセージ番号5)
 
``` 
{
   "value": 16,
   "type": "Number
}
``` 
  
いくつかの追加のコメント :

-   -`httpTimeout` [CLI パラメータ](../admin/cli.md)は、CPr のタイムアウトを設定するために使用します。CPr に転送されたリクエストがそのタイムアウトを超えている場合、Orion は接続をクローズし、CPr がレスポンスしていないとみなします
-   特定のリクエストが複数のコンテキスト・プロバイダ (例えば、3つのコンテキスト要素を含む更新で、それぞれが異なるコンテキスト・プロバイダによって管理されるエンティティ) が含まれる場合、Orion はリクエストの対応する "ピース (piece)" を各コンテキスト・プロバイダに転送し、クライアントにレスポンスする前にすべての結果を収集します。現在のインプリメンテーションは、複数の転送をを順番に処理します。つまり、Orion は、転送リクエストを送信する前に、前の CPr からのレスポンス、またはタイムアウトの期限切れを待ちます。
-   -`cprForwardLimit` [CLI パラメータ](../admin/cli.md)を使用して、単一のクライアントリクエストに対するコンテキスト・プロバイダへの転送リクエストの最大数を制限することができます。コンテキスト・プロバイダの転送を完全に無効にするには、"0" を使用します
-   転送時には、NGSIv2 の更新/クエリ内のどの型のエンティティも、エンティティ型のないレジストレーションと一致します。しかし、その逆は機能しないので、型を持つレジストレーションがあれば、NGSIv2 の更新/クエリで `?type` を使って一致を得る必要があります。それ以外の場合、この [StackOverflow での投稿](https://stackoverflow.com/questions/48163972/orion-cb-doesnt-update-lazy-attributes-on-iot-agent)のような問題が発生する可能性があります
-   クエリの転送では、クエリ・フィルタリングはサポートされていません (たとえば、`GET /v2/entities?q=temperature>40`)。まず、Orion は CPr に転送された `POST /v2/op/query` オペレーションにフィルタを含めません。第2に、Orion はクライアントに返信する前に CPr の結果をフィルタリングしません。この制限に対応する issue が作成されました : https://github.com/telefonicaid/fiware-orion/issues/2282
-   部分的な更新の場合 (たとえば、いくつかのエンティティ/属性が更新され、他のエンティティ/属性が CPrs の失敗または欠落のために更新されない結果をもたらす `POST /v2/op/update`)、404 Not Found がクライアントに返されます。この場合の `error` フィールドは `PartialUpdate` で、`description` フィールドはどのエンティティ属性が更新されなかったかについての情報を含んでいます
