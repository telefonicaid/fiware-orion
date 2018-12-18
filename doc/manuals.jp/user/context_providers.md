# コンテキスト・プロバイダのレジストレーションとリクエスト転送

レジスター・コンテキストのオペレーションは、そのレジストレーションに含まれるエンティティ/属性のコンテキスト情報のソースを識別する URL である、"コンテキスト・プロバイダ" という概念を使用します。

これはフィールド `provider` によって提供されます :

```
...
"provider": {
  "http": {
    "url": "http://mysensors.com/Rooms"
  }
}
...
```

Orion がクエリまたは更新オペレーションを受信して、ターゲットのコンテキスト要素をローカルに、(つまり内部データベースに見つけることができなくても、そのコンテキスト要素にコンテキスト・プロバイダがレジストレーションされている場合、Orion はクエリ/更新要求をコンテキスト・プロバイダに転送します。この場合、Orion は純粋な "NGSI proxy" として機能します (つまり、クエリの結果を内部的にキャッシュしません)。元のリクエストを発行したクライアントの観点から見ると、プロセスはほとんど透過的です。コンテキスト・プロバイダは、クエリ/更新オペレーションをサポートするために (少なくとも部分的に) NGSI API を実装するためのものです。

これを例を使って説明しましょう。

![](../../manuals/user/QueryContextWithContextProvider.png "QueryContextWithContextProvider.png")


* 最初 (メッセージ番号1)、アプリケーションは (おそらくコンテキスト・プロバイダに代わって) Orion に Street4 の温度用のコンテキスト・プロバイダをレジストレーションします。コンテキスト・プロバイダが <http://sensor48.mycity.com/v1> で API を公開していると仮定しましょう
      
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
      "url": "http://sensor48.mycity.com/v1"
    },
    "legacyForwarding": true
  }
}
EOF
```
     
* 次に、クライアントが 温度 (メッセージ番号2) をクエリするとします
     
``` 
curl localhost:1026/v2/entities/Street4/attrs/temperature?type=Street -s -S \
    -H 'Accept: application/json' -d @- | python -mjson.tool
``` 

* Orion はストリート4の温度を知りませんが、<http://sensor48.mycity.com/v1> のコンテキスト・プロバイダはストリート4の温度を知っていることを知っているので (前のステップでのレジストレーションのために)、クエリー (メッセージ番号3) を URL <http://sensor48.mycity.com/v1/queryContext> (レジストレーション時にアプリケーション提供フィールドで使用された URL に "/queryContext" オペレーションを加えて) に転送します。クライアントからの元のリクエストが NGSIv2 を使用したにもかかわらず、クエリが NGSIv1 形式を使用して転送されていることに注意してください。NGSIv1は推奨されていませんが、NGSIv2ベースの転送のサポートがまだ不足しているため、これを行う必要があります。これについては [この issue](https://github.com/telefonicaid/fiware-orion/issues/3068) を参照ください


``` 
{
    "entities": [
        {
            "type": "Street",
            "isPattern": "false",
            "id": "Street4"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
``` 


* <http://sensor48.mycity.com/v1> のコンテキスト・プロバイダは、データ (メッセージ番号4) でレスポンスします

``` 
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "16"
                    }
                ],
                "id": "Street4",
                "isPattern": "false",
                "type": "Street"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
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
-   あるリクエストが複数のコンテキスト・プロバイダ (例えば、3つのコンテキスト要素を含む更新で、それぞれが異なるコンテキスト・プロバイダによって管理されるエンティティ) を含む場合、Orion はリクエストの対応する "ピース (piece)" を各コンテキスト・プロバイダに転送し、クライアントにレスポンスする前にすべての結果を収集します。現在のインプリメンテーション・プロセスは、複数のフォワードを順番に処理します。つまり、フォワードリクエストを次のものに送信する前に、特定の CPr からのレスポンスを待ちます (またはタイムアウト満了します)
-   -`cprForwardLimit` [CLI パラメータ](../admin/cli.md)を使用して、単一のクライアントリクエストに対するコンテキスト・プロバイダへの転送リクエストの最大数を制限することができます。コンテキスト・プロバイダの転送を無効にするには、"0" を使用します
-   転送時には、NGSIv2 の更新/クエリ内のどの型のエンティティも、エンティティ型のないレジストレーションと一致します。しかし、その逆は機能しないので、型を持つレジストレーションがあれば、NGSIv2 の更新/クエリで `?type` を使って一致を得る必要があります。それ以外の場合、この [StackOverflow での投稿](https://stackoverflow.com/questions/48163972/orion-cb-doesnt-update-lazy-attributes-on-iot-agent)のような問題が発生する可能性があります
-   現時点では、Context Broker はフォワードされたクエリの結果に複合属性値 (compound attribute values) を含めることができません。この場合、ブランク (`""`) 値が表示されます。詳細については、
[次の issue](https://github.com/telefonicaid/fiware-orion/issues/3162) を参照してください。ただし、複合値 (compound values) を含む更新にはこの問題はなく、正しく転送されます。
    - 一方、[Dockerhub](https://hub.docker.com/r/fiware/orion/) には、"fiware/orion:fix3162" と書かれた Docker イメージがあり、これには修正が含まれています。 ただし、このイメージは、メモリリークの問題 (未確認) の可能性があるため、プロダクションでの使用はお勧めしません。
-   部分的な更新の場合 (たとえば、いくつかのエンティティ/属性が更新され、他のエンティティ/属性が CPrs の失敗または欠落のために更新されない結果をもたらす `POST /v2/op/entities`)、404 Not Found がクライアントに返されます。この場合の `error` フィールドは `PartialUpdate` であり、`description` フィールドは更新に失敗したエンティティ属性に関する情報を含んでいます
