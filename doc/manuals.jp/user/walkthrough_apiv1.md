# <a name="top"></a>FIWARE NGSI APIv1 ウォークスルー

* [イントロダクション](#introduction)
* [始める前に ...](#before-starting)
    * [例](#example-case)
    * [チュートリアル用の broker の起動](#starting-the-broker-for-the-tutorials)
    * [アキュムレータ・サーバの起動](#starting-accumulator-server)
    * [broker にコマンドを発行](#issuing-commands-to-the-broker)
* [NGSI10 を使用したコンテキスト管理](#context-management-using-ngsi10)
    * [NGSI10 標準オペレーション](#ngsi10-standard-operations)
        * [エンティティの作成](#entity-creation)
        * [コンテキストのクエリ・オペレーション](#query-context-operation)
        * [コンテキスト要素の更新](#update-context-elements)
        * [コンテキストのサブスクリプション](#context-subscriptions)
        * [NGSI10 標準オペレーション URLs のサマリ](#summary-of-ngsi10-standard-operations-urls)
    * [NGSI10 のコンビニエンス・オペレーション](#ngsi10-convenience-operations)
        * [コンビニエンスなエンティティの作成](#convenience-entity-creation)
        * [コンビニエンスなコンテキストのクエリ](#convenience-query-context)
        * [すべてのエンティティの取得](#getting-all-entities)
        * [すべての型と型に関する詳細情報のブラウズ](#browsing-all-types-and-detailed-information-on-a-type)
        * [コンビニエンスなコンテキストの更新](#convenience-update-context)
        * [コンテキストのサブスクリプションのコンビニエンス・オペレーション](#convenience-operations-for-context-subscriptions)
        * [NGSI10 のコンビニエンス・オペレーション URLs のサマリ](#summary-of-ngsi10-convenience-operations-urls)
* [NGSI9 を使用したコンテキスト・アベイラビリティ管理](#context-availability-management-using-ngsi9)
    * [NGSI9 標準オペレーション](#ngsi9-standard-operations)
        * [コンテキストの登録オペレーション](#register-context-operation)
        * [コンテキスト・アベイラビリティのディスカバー・オペレーション](#discover-context-availability-operation)
        * [コンテキスト・アベイラビリティのサブスクリプション](#context-availability-subscriptions)
        * [NGSI9 標準オペレーション URLs のサマリ](#summary-of-ngsi9-standard-operations-urls)
    * [NGSI9 コンビニエンス・オペレーション](#ngsi9-convenience-operations)
        * [コンビニエンスなコンテキストの登録](#convenience-register-context)
        * [コンビニエンス・オペレーションを使用した型のみによるエンティティのレジストレーション](#only-type-entity-registrations-using-convenience-operations)
        * [コンビニエンスなコンテキスト・アベイラビリティのディスカバー](#convenience-discover-context-availability)
        * [コンテキスト・アベイラビリティのサブスクリプションのためのコンビニエンス・オペレ
ーション](#convenience-operations-for-context-availability-subscriptions)
        * [NGSI9 コンビニエンス・オペレーション URLs のサマリ](#summary-of-ngsi9-convenience-operations-urls)

<a name="introduction"></a>
## イントロダクション

[このウォークスルーの NGSIv2 version](walkthrough_apiv2.md) もあります。NGSIv2 でまだ開発されていないコンテキスト管理のアベイラビリティ機能 (別名 NGSI9) が必要な場合を除いて、一般的に、NGSIv2 (つまり他のドキュメント) を使用する必要があります。疑わしい場合は、NGSIv2 を使用する必要があります。

このウォークスルーでは、Orion Context Broker に慣れ親しんでプロセスを楽しむのに役立つと期待される実践的アプローチを採用しています。

ウォークスルーは Apiary 形式 ([ここ](http://telefonicaid.github.io/fiware-orion/api/v1/)で) でも、部分的に見つけることができます。

[NGSI10 を使用したコンテキスト管理](#context-management-using-ngsi10)と、[NGSI9 を使用したコンテキスト・アベイラビリティ管理](#context-availability-management-using-ngsi9)の最初の2つのセクションが主なものです。コンテキスト管理 (車の温度などのエンティティに関する情報) とコンテキストのアベイラビリティ管理 (エンティティ自体についてではなく、その情報のプロバイダに関する情報) の両方について、基本的な Context broker の機能について説明します。これを使用するために考慮するいくつかの意見です :

-   コンテキスト管理とコンテキスト・アベイラビリティ管理は、独立した機能です。NGSI インタフェースの異なる部分、NGS10 および NGSI9 にそれぞれ対応します。そのため、broker はある目的か別の目的、またはその両方で使用できます
-   各メインセクションは2つのサブセクションに分割されていることに注意してください。最初のセクションは標準的なオペレーションであり、2つ目はコンビニエンス・オペレーションです。実際には、各サブセクションは、個別のチュートリアル (このセクションからの合計4つのチュートリアル) で、このドキュメントのコマンドをコピーして貼り付けるだけで段階的に実行できます
-   開始する前に、または途中で紛失して最初から開始する必要がある場合、[チュートリアルの broker の起動](#starting-the-broker-for-the-tutorials)で説明したように、Orion Context Broker を再起動します
-   標準的なオペレーションでパートを始めることをお勧めします。そして、そのパートをコンビニエンス・オペレーションで行います。前者で説明されている説明やコンセプトが必要です

開始する前に NGSI モデルが基礎としている理論的概念に精通することをお勧めします。例えば、エンティティ、属性など。これについての FIWARE のドキュメントをご覧ください。例 : [このプレゼンテーション](http://bit.ly/fiware-orion)

[トップ](#top)

<a name="before-starting"></a>
## 始める前に ...

開始する前に、チュートリアルで使用されている事例と、Orion Context Broker を実行してオペレーションする方法を紹介しましょう。

[トップ](#top)

<a name="example-case"></a>
#### 例

Orion Context Broker を使用してコンテキスト情報を管理したいと考えています。部屋は Room1, Room2, Room3, Room4  で、各部屋には温度と大気圧の2つのセンサがあります。Room4 は大気圧センサのみです。速度と GPS センスで位置を測定できるセンサを備えた2台の車 (Car1 と Car2) を持っていると考えてみましょう。

![](../../manuals/user/Orion-example-case.png "Orion-example-case.png")

ほとんどの場合、チュートリアルで Room1 と Room2 を使用します。Room3, Room4, Car1 および Car2 は、[コンテキスト・アベイラビリティ・サブスクリプション](#context-availability-subscriptions)に関するセクションでのみ使用されます。

Orion Context Broker は、センサ情報を提供するコンテキスト・プロデューサ・アプリケーションと、グラフィカル・ユーザ・インタフェースで表示するためにその情報を処理するコンテキスト・コンシューマ・アプリケーションと対話します。チュートリアルでは、両方の種類のアプリケーションの役割を果たします。

[トップ](#top)

<a name="starting-the-broker-for-the-tutorials"></a>
### チュートリアル用の broker の起動

開始する前に、"[インストールおよび管理ガイド](../admin/install.md)" の説明に従って broker をインストールする必要があります。

チュートリアルでは、Orion Context Broker データベースに以前のコンテンツがないことを前提としています。これを行うには、[データベースの削除手順](../admin/database_admin.md#delete-complete-database)に従います。

broker を起動するには、root または sudo コマンドを使用して :

```
/etc/init.d/contextBroker start
```

broker を再起動するには、root または sudo コマンドを使用して :

```
/etc/init.d/contextBroker restart
```

[トップ](#top)

<a name="starting-accumulator-server"></a>
### アキュムレータ・サーバの起動

チュートリアル (サブスクリプションと通知に関連するもの) の一部では、通知を受け取ることができるコンシューマ・アプリケーションの役割を果たすためにいくつかのプロセスが必要です。そのためには、[GitHub](https://github.com/telefonicaid/fiware-orion/blob/master/scripts/accumulator-server.py) で入手可能なアキュムレータ・スクリプトをダウンロードしてください。これは非常に単純な "ダミー" アプリケーションであり、与えられた URL (以下の例では、localhost:1028/accumulate を使用していますが、別のホストやポートを指定することができます) を単にリッスンし、それが実行されるターミナル・ウィンドウで受け取ったものをエコーします。次のコマンドを使用して実行します :

```
# cd /dir/where/accumulator-server/is/downloaded
# chmod a+x accumulator-server.py
# ./accumulator-server.py --port 1028 --url /accumulate --host ::1 -v
```

[トップ](#top)

<a name="issuing-commands-to-the-broker"></a>
### broker にコマンドを発行

broker にリクエストを発行するには、`curl` コマンドライン・ツールを使用します。GNU/Linux システムでもほぼどこにでもあり、簡単にコピーして貼り付けることができるこのドキュメントの例を含めて簡素化するで、`curl` を選択しました。もちろん、それを使用することは必須ではなく、代わりに [RESTClient](http://restclient.net/) などのREST クライアント・ツールを使用することができます。実際には、おそらく、アプリケーションの REST クライアント部分を実装するプログラミング言語ライブラリを使用して Orion Context Broker とやりとりすることになるでしょう。

このドキュメントのすべての curl の例の基本パターンは次のとおりです :

-   POST の場合 :

```
curl localhost:1026/<operation_url> -s -S [headers]' -d @- <<EOF
[payload]
EOF
```

-   PUT の場合 :

```
curl localhost:1026/<operation_url> -s -S [headers] -X PUT -d @- <<EOF
[payload]
EOF
```

-   GET の場合 :

```
curl localhost:1026/<operation_url> -s -S [headers]
```

-   DELETE の場合 :

```
curl localhost:1026/<operation_url> -s -S [headers] -X DELETE
```

\[headers\] に関しては、次のものを含める必要があります :

-   Accept ヘッダを使用して、レスポンスで受信するペイロード形式を指定します。明示的に JSON を指定する必要があります

```
curl ... --header 'Accept: application/json' ...
```

-   リクエスト (POST, PUT, PATCH など) でペイロードを使用する場合のみ、Context-Type ヘッダを使用してフォーマット (JSON) を指定する必要があります

```
curl ... --header 'Content-Type: application/json' ...
```

いくつかの追加意見 :

-   ほとんどの場合、EOF を使用して複数行のブロック(*here-documents*) の先頭と最後をマークするために、マルチライン・シェル・コマンドを使用して curl への入力を提供しています。場合 (GET と DELETE) によっては、`-d @-` をペイロードを使用しないので省略します

-   例では、broker がポート1026 をリッスンしていると仮定しています。異なる設定を使用している場合は、curl コマンドラインでこれを調整してください

-   レスポンスで JSON をきれいに印刷するために、Python を msjon.tool とともに使用することができます。チュートリアルの例ではこのスタイルを使用しています :

```
(curl ... | python -mjson.tool) <<EOF
...
EOF
```

-   次のコマンドを使用して、curl がシステムにインストールされていることを確認します :

```
which curl
```

[トップ](#top)

<a name="context-management-using-ngsi10"></a>
## NGSI10 を使用したコンテキスト管理

<a name="ngsi10-standard-operations"></a>
### NGSI10 標準オペレーション

このセクションでは、Orion Context Broker がサポートするさまざまな標準 NGSI10 オペレーションについて説明し、リクエストとレスポンスの例を示します。NGSI 実装の使用を容易にするために FIWARE プロジェクトによって定義された他のオペレーション・ファミリーの " コンビニエンス (convenience)" と区別するために、OMA NGSI 仕様から直接得られたものとして "標準(standard)" という用語を使用します。

**このチュートリアルを開始する前に、[このドキュメントで前述した](#starting-the-broker-for-the-tutorials)ように broker を再起動することを忘れないでください。**

このセクションの最後には、コンテキスト管理オペレーションで Orion Context Broker を使用して、コンテキスト・プロデューサとコンシューマの両方のアプリケーションを作成するための基本知識があります :

-   updateContext
-   queryContext
-   subscribeContext
-   updateContextSubscription
-   unsubscribeContext

[トップ](#top)

<a name="entity-creation"></a>
#### エンティティの作成

Orion Context Broker は空の状態から開始するので、最初に特定のエンティティの存在を認識させる必要があります。特に、2つの属性 (温度と気圧) を持つ Room1 と Room2 エンティティを "作成 (create)" します。これを行うには、APPEND アクション・タイプの updateContext オペレーションを使用します。他のメイン・アクション・タイプ、UPDATE については [次のセクションで説明](#update-context-elements)します。

まず、Room1 を作成します。エンティティ作成時に、Room1 の温度と気圧がそれぞれ 23ºC  と 720mmHg であるとします。

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "23"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "720"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
} 
EOF
```

pdateContext リクエストペイロードには、contextElement 要素のリストが含まれています。各 contextElement はエンティティに関連付けられており、その識別情報は `id`, `type` および `isPattern` フィールド(この場合は Room1 の識別情報が提供されます)によって提供され、属性のリストを含みます。属性リストの各要素は、エンティティの特定の属性 (名前で識別される) の値を提供します。contextElement 要素のリストとは別に、ペイロードには updateAction 要素も含まれています。APPEND を使用します。これは、新しい情報を追加することを意味します。

Orion Context Broker は型のチェックを行いません。例えば コンテキスト・プロデューサ・アプリケーションが温度の値を更新したときに、この値が `25.5` や `-40.23` のような浮動小数点でフォーマットされていて、`hot` のようなものではないことをチェックしません。

このリクエストを受け取ると、broker は内部データベースにエンティティを作成し、その属性の値を設定し、次のようにレスポンスします :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

ご覧のとおり、リクエストと同じ構造に従います。リクエストがこれらのコンテキスト要素に対して正しく処理されたことを確認するだけです。この場合 contextValue 要素がなぜ空であるのだろうかと疑問に思うかもしれませんが、リクエストでそれらを提供するため、実際にはレスポンスの値は必要ありません。

次に、同様の方法で Room2 を作成します。この場合、温度と気圧をそれぞれ 21 ºC と 711mmHg に設定します。

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool ) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "21"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "711"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```

このリクエストに対するレスポンスは次のとおりです :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

属性値の単純な値 (文字列) の他に、複雑な構造やカスタム・メタデータを使用することもできます。これらはそれぞれ、[このセクション](structured_attribute_valued.md#structured-attribute-values)と[他のセクション](metadata.md#custom-attribute-metadata)で説明されている先進的なトピックです。

[トップ](#top)

<a name="query-context-operation"></a>
#### コンテキストのクエリ・オペレーション

次に、コンシューマ・アプリケーションの役割を果たすために、Orion Context Broker が格納しているコンテキスト情報にアクセスして、グラフィカル・ユーザ・インタフェースで室温を表示するなど、興味深いことをしたいとしましょう。この場合、たとえば Room1 のコンテキスト情報を取得するために、NGSI10 queryContext リクエストが使用されます :

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ]
} 
EOF
```

レスポンスには Room1 に属するすべての属性が含まれており、温度と気圧が updateContext (23oC と 720mmHg) を使用してエンティティ作成時に設定した値を持つことを確認できます。

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "720"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

リクエストに空の属性要素を使用すると、レスポンスにはエンティティのすべての属性が含まれます。次のリクエストに示されているように、取得された属性の実際のリスト (温度など) を含めると :

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ]
} 
EOF
```

このレスポンスは次のとおりです :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

さらに、Orion Context Broker の強力な機能は、エンティティ ID に正規表現を使用できることです。たとえば、正規表現 "Room.*" を使用して、ID が "Room" で始まるエンティティをクエリすることができます。この場合、次のように isPattern を "true" に設定する必要があります :

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        },
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2"
        }
    ],
    "attributes": [
        "temperature"
    ]
} 
EOF
```
```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "true",
            "id": "Room.*"
        }
    ],
    "attributes": [
        "temperature"
    ]
} 
EOF
```
  

両方とも同じレスポンスを生成します :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "21"
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

最後に、以下のように存在しないエンティティまたは属性をクエリしようとするとエラーが発生することに注意してください :

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room5"
        }
    ]
} 
EOF
```
```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "humidity"
    ]
} 
EOF
```

どちらのリクエストも同じエラーレスポンスを生成します :

```
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context elements found"
    }
}
```

追加コメント :

-   また、クエリで地理的な範囲を使用することもできます。これは、[このセクション](geolocation.md#geolocation-capabilities)で説明する先進的なトピックです
-   デフォルトでは20個のエンティティしか返されないことに注意してください。このチュートリアルでは問題ありませんが、実際の使用シナリオではないかもしれません。この動作を変更するには、このマニュアルの [ページネーション](pagination.md#pagination)に関するセクションを参照してください
-   JSON レスポンスの場合、*？attributeFormat = object* URI パラメータを使用して、デフォルト動作のベクトルではなく JSON オブジェクト、つまり key-values マップとして属性を取得できます :

```
(curl 'localhost:1026/v1/queryContext?attributeFormat=object' -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ]
}
EOF
```

[トップ](#top)

<a name="update-context-elements"></a>
#### コンテキスト要素の更新

UPDATE アクション・タイプの updateContext オペレーションを使用して、エンティティ属性の値を更新できます。updateContext を考慮する基本的なルールは、APPEND は新しいコンテキスト要素を作成し、UPDATE は既存のコンテキスト要素を更新します。ただし、エンティティがすでに存在する場合、Orion は APPEND を UPDATE と解釈します。[APPEND_STRICT](update_action_types.md#append_strict) を使用することを避けることができます。

ここで、我々は、コンテキスト・プロデューサ・アプリケーション、すなわちコンテキスト情報のソースの役割を果たします。このアプリケーションでは、Room1 の温度と気圧をそれぞれ 26.5ºC と 763mmHg に設定したいと考えているので、次のリクエストを出します :

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
     --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "763"
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
} 
EOF
```

ご覧のように、リクエストの構造は、アクション・タイプとして UPDATE を使用することを除いて、[エンティティを作成するために APPEND を使用して updateContext](#entity-creation) で使用したものとまったく同じです。

このリクエストを受け取ると、broker は内部データベース内のエンティティ属性の値を更新し、次のようにレスポンスします :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
      
```

ここでも、レスポンスの構造は、[エンティティを作成するために APPEND を使用して updateContext](#entity-creation)に使用したものとまったく同じです。

updateContext オペレーションは、多くのエンティティと属性を更新できるように、非常に柔軟です。これは、リストに含める contextElements の問題に過ぎません。1つの updateContext オペレーション (少なくとも理論上は) で、Orion Context Broker のデータベース全体 (おそらく何千ものエンティティ/属性を含む) を更新することさえできます。

この柔軟性を説明するために、2つの別々の updateContext リクエスト (27.4ºC とその気圧を 755 mmHg に設定) で Room2 を更新する方法を示します。それぞれ1つの属性のみを対象としています。これは、エンティティのすべての属性を updateContext に含める必要はなく、更新するエンティティの属性だけを含める必要があることも示しています。他の属性は現在の値を維持します。

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "27.4"
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
} 
EOF
```
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2",
            "attributes": [
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "755"
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
} 
EOF 
```
  
これらのリクエストに対するレスポンスは、それぞれ次のとおりです :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```
  
```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

これで、[前述のように](#query-context-operation) queryContext オペレーションを使用して、Room1 と Room2 の属性が実際に更新されたことを確認できます。

属性値の単純な値 (文字列) のほかに、複雑な構造を使用することもできます。これは、[このセクション](structured_attribute_valued.md#structured-attribute-values)で説明する先進的なトピックです。

APPEND や UPDATE の他に、エンティティ属性を置き換える REPLACE などの `actionType` フィールドの追加の可能性があります。エンティティに属性 A と B があり、A で updateContext REPLACE リクエストを送信した場合、最後のエンティティは A、すなわち、属性 B は除去されています。 完全なリストの [アクション・タイプに関するセクション](update_action_types.md)を見てください。

[トップ](#top)

<a name="context-subscriptions"></a>
#### コンテキストのサブスクリプション

これまで知っていた NGSI10 のオペレーション (updateContext と queryContext) は、同期コンテキスト・プロデューサ・コンテキストと、コンシューマ・アプリケーションの基本的なビルディング・ブロックです。しかし、Orion Context Broker には、"何か" が発生したときにコンテキスト情報をサブスクライブする機能 ("何か" のさまざまなケースについて説明します) に、アプリケーションが非同期通知を得るという強力な機能があります。このようにして、queryContext リクエスト、つまりポーリングを連続して繰り返す必要はなく、Orion Context Broker は情報が来たときにそれを知ることができます。

開始する前に、[アキュムレータ・サーバを起動](#starting-accumulator-server-for-the-tutorials)して通知をキャプチャします。

サブスクリプションは、一部の属性が変更されたときに通知を受けたい場合に使用されます。次の例を考えてみましょう :

```
(curl localhost:1026/v1/subscribeContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "pressure"
            ]
        }
    ],
    "throttling": "PT5S"
}
EOF
```

ペイロードに含まれるさまざまな要素を詳細に検討してみましょう :

-   エンティティおよび属性は、どのコンテキスト要素が通知メッセージに含まれるかを定義します。この例では、Room1 エンティティの温度属性を通知に含める必要があることを指定しています
-   通知を送信するコールバック URL は、reference 要素で定義されます。以前に起動した accumulator-server.py プログラムの URL を使用しています。subscribeContext リクエストごとに1つの参照のみを含めることができます。ただし、同一のコンテキスト要素(つまり、同じエンティティと属性)にいくつかのサブスクリプションを問題なく置くことができます。デフォルト URL スキーマ(指定しない場合)は "http" です。たとえば、reference として "localhost:1028" を使用すると、実際には "http://localhost:1028" と解釈されます
-   サブスクリプションには、[ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) 標準フォーマットを使用して指定された期間があります。この期間が終了すると、サブスクリプションは単純に無視されます。ただし、broker データベースにはまだ格納されており、[管理マニュアル](../admin/database_admin.md#deleting-expired-documents)に記載されている手順を使用してパージする必要があります。サブスクリプションの期間を延長するには、このドキュメントの後半で説明します。私たちは "1 ヶ月" を意味する "P1M" を使用しています
-   notifyCondition 要素は、サブスクリプションの "トリガ" を定義します。これは、ONCHANGE 型を使用します。condValues ベクトルには、属性名のリストが含まれています。それらは、" トリガリング属性 "、すなわち [エンティティ作成](#entity-creation)または [コンテキスト要素の更新](#update-context-elements)のために作成/変更される属性を定義する通知をトリガーします。ルールは、リストの属性の少なくとも1つが変更された場合 (ある種の "OR" 条件など)、通知が送信されます。ただし、通知には属性ベクトルの属性が含まれていますが、必ずしも condValue に属性を含める必要はありません。たとえば、この場合、Room1 の気圧が変化すると、Room1 の温度値が通知されますが、気圧自体は通知されません。また、気圧を通知したい場合は、属性ベクトルに "pressure(気圧)" を含める必要があります。または、すでに知っている空の属性ベクトルを使用するには、"エンティティのすべての属性" を意味します。今、ここの例は、変更はそれほど有用ではないかもしれません。この例は、サブスクリプションの巨大な柔軟性を示すためにのみ、この方法で選択されています
-   condValue リストを空または省略して、属性の名前に関係なく、エンティティ属性の変更に対して通知トリガーを作成することができます
-   throttling 要素は、通知間の最小到着時間を指定するために使用されます。したがって、上記の例のように5秒にスロットリングを設定すると、その期間に実際に何回変更が行われても、前の通知が5秒前に送信された場合に通知が送信されなくなります。これは、通知レセプターに、属性値を頻繁に更新するコンテキスト・プロデューサに対して自分自身を保護する手段を与えることです。multi-CB 構成では、最後の通知手段が各 CB ノードに対してローカルであることを考慮に入れます。各ノードは DB と定期的に同期してより新しい値を取得しますが (詳細は [こちら](../admin/perf_tuning.md#subscription-cache))、特定のノードが古い値を持つことがありますので、スロットリングは100％正確ではありません

そのリクエストに対応するレスポンスには、サブスクリプション ID (サブスクリプションの更新とキャンセルに使用される24桁の16進数です。このチュートリアルの後半で必要となるため、書き留めてください) と duration と throttling acknowledgement が含まれています :

```
{
    "subscribeResponse": {
        "duration": "P1M",
        "subscriptionId": "51c0ac9ed714fb3b37d7d5a8"
    }
}
```

accumulator-server.py を見てみましょう。次のような notifyContextRequest が表示されます。あなたがどれくらい待っていても、ちょうど1つだけです :

```
POST http://localhost:1028/accumulate
Content-Length: 492
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8",
    "originator": "localhost",
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "26.5"
                    }
                ],
                "type": "Room",
                "isPattern": "false",
                "id": "Room1"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Orion Context Broker は、notifyContextRequest ペイロードを使用して POST HTTP メソッド(サブスクリプションの参照として使用される URL)を使用して NGSI10 subscribeContext に通知します。`subscriptionId` 要素(subscribeContext リクエストへのレスポンスに一致するもの)とオリジネーター要素を除いて、[コンテキストのクエリ・オペレーション](#query-context-operation)で使用されているものと同じ contextResponses ベクトルがあります。

現在、オリジネーターは常に "localhost" です。これ以降のバージョンでこれを使用するより柔軟な方法を検討します。

実際に更新を行わないと、accumulator-server.py がこのメッセージを受け取る理由は不思議に思うかもしれません。これは初期通知によるもので、詳細については [ここ](initial_notification.md)で説明します。

[コンテキストの更新](#pdate-context-elements)から知っていることに基づいて、次の4つの更新を順番に実行して、実行の合間でで5秒以上経過させてください(スロットリングによる通知の損失を避けるため) :

-   Room1 温度を27に更新 : 温度がトリガ属性ではないため、何も起こりません
-   Room1 の気圧を765に更新 : Room1 の現在の値 (27) を通知します
-   Room1 の気圧を765に更新 : broker が賢明であるため、updateContext リクエストの前の値も765であるため、実際の更新は行われず、したがって通知は送信されません
-   Room2 の気圧を740に更新 : サブスクリプションは Room2 ではなく Room1 のため、何も起こりません

次に、スロットリングがどのように強制されているかを確認します。5秒経過することなく Room1 の気圧を速やかに更新すると、2番目の通知が accumulator-server.py に到着しないことがわかります。

サブスクリプションは、NGSI10 updateContextSubcription を使用して更新できます。リクエストには、変更するサブスクリプションと実際の更新ペイロードを識別する `subscriptionId` が含まれています。たとえば、duration コマンドを延長するために `duration` 間隔を変更して duration を変更したい場合。もちろん、コピー貼り付け後の `subscriptionId` の値を、前の手順で subscribeContext レスポンスにあるものと置き換えてください :

```
(curl localhost:1026/v1/updateContextSubscription -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8",
    "duration": "P2M"
}
EOF
```

レスポンスは、subscribeContext リクエストのレスポンスに非常に似ています :

```
{
    "subscribeResponse": {
        "subscriptionId": "51c0ac9ed714fb3b37d7d5a8",
        "duration": "P2M"
    }
}
```

最後に、リクエスト・ペイロードで `subscriptionId` を使用する NGSI10 unsubscribeContext オペレーションを使用してサブスクリプションを取り消すことができます (コピー＆ペースト後の `subscriptionId` 値を、前の手順で subscribeContext レスポンスで取得したものと置き換えてください) :

```
(curl localhost:1026/v1/unsubscribeContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8"
}
EOF
```

レスポンスは、取り消しが成功したことを確認しただけです。

```
{
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    },
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8"
}
```

さらにいくつかの更新を行い、accumulator-server.py を見て、通知フローが停止していることを確認することができます。

[トップ](#top)

<a name="summary-of-ngsi10-standard-operations-urls"></a>
#### NGSI10 標準オペレーション URLs のサマリ

各標準オペレーションには一意の URL があります。それらのすべては POST メソッドを使用します。概要は次のとおりです :

-   <host:port>/v1/updateContext
-   <host:port>/v1/queryContext
-   <host:port>/v1/subscribeContext
-   <host:port>/v1/updateContextSubscription
-   <host:port>/v1/unsubscribeContext

[トップ](#top)

<a name="ngsi10-convenience-operations"></a>
### NGSI10 コンビニエンス・オペレーション

このセクションでは、Orion Context Broker がサポートする FIWARE NGSI REST API NGSI10 の一部として説明されているさまざまなコンビニエンス・オペレーションについて説明し、リクエストとレスポンスの例を示します。コンビニエンス・オペレーションは、OMA NGSI 仕様で定義されている標準オペレーションを補完する NGSI 実装の使用を容易にするために、FIWARE プロジェクトによって定義された一連のオペレーションです。

**このチュートリアルを開始する前に、[このドキュメント](#starting-the-broker-for-the-tutorials)で前述したように broker を再起動することを忘れないでください。**

このセクションの最後では、前のセクションで説明した標準的なオペレーションのコンビニエンスな代替手段として、コンビニエンス・オペレーションを使用する方法を学びました。このチュートリアルを前もってやっておき、アップデートやクエリのコンテキストなどを熟知し、2つのアプローチを比較できるようにすることを強くお勧めします。

[トップ](#top)

<a name="convenience-entity-creation"></a>
#### コンビニエンスなエンティティの作成

Orion Context Broker は空の状態から開始するので、最初に特定のエンティティの存在を認識させる必要があります。したがって、最初に温度と気圧の属性を持つ Room1 エンティティを作成してみましょう (初期値)

```
(curl localhost:1026/v1/contextEntities/Room1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF

{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "23"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "720"
        }
    ]
} 
EOF
```

レスポンスは次のとおりです :

```
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ],
    "id": "Room1",
    "isPattern": "false",
    "type": ""
}
```

今、Room2 と同じことをやってみましょう :

```
(curl localhost:1026/v1/contextEntities/Room2 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "21"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "711"
        }
    ]
}
EOF
```

そのレスポンスは :

```
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ],
    "id": "Room2",
    "isPattern": "false",
    "type": ""
}
```

次の方法で属性、およびその途中に含まれるエンティティを作成することもできます。[このセクション](update_action_types.md#append)で説明するように、後に追加の属性を追加できます :

```
(curl localhost:1026/v1/contextEntities/Room3/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -X POST -d @- | python -mjson.tool) <<EOF
{
    "value" : "21"
} 
EOF
```

[標準的なオペレーションに基づくエンティティの作成](#entity_creation)と比較して、以下の違いがあります :

-   `/v1/contextEntities/{EntityID}` リソースで POST verb を使用して新しいエンティティを作成しています
-   コンビニエンス・オペレーション・リクエストを使用して、一度に複数のエンティティを作成することはできません
-   コンビニエンス・オペレーションにおけるリクエストとレスポンスのペイロードは、標準オペレーションで使用されるものと非常によく似ています。これは、contextAttribute 要素と contextResponse 要素が同じであるためです
-   URl の "/type/Room/id/Room1" で "Room1" を置き換えて、型を定義することができます。一般的には "/type//id/" です

代替として、"POST/v1/contextEntitites" を使用してエンティティを作成することができます。この場合、エンティティ情報 (ID と型) はペイロードに含まれ、URL は以下のようにそのフィールドとは独立しています :

```
(curl localhost:1026/v1/contextEntities -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF
{
    "id": "Room1",
    "type": "Room",
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "23"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "720"
        }
    ]
}
EOF
```

属性値の単純な値 (文字列) の他に、複雑な構造やカスタム・メタデータを使用することもできます。これらはそれぞれ、[このセクション](structured_attribute_valued.md#structured-attribute-values)と[他セクション](metadata.md#custom-attribute-metadata)で説明されている先進的なトピックです。

[トップ](#top)

<a name="convenience-query-context"></a>
#### コンビニエンスなコンテキストのクエリ

最後に、コンテキスト情報をクエリするためのコンビニエンス・オペレーションについて説明します。たとえば、Room1 の属性など、特定のエンティティのすべての属性値をクエリできます :

```
curl localhost:1026/v1/contextEntities/Room1 -s -S 
    --header 'Accept: application/json' | python -mjson.tool
```
そのレスポンスは :
```
{
    "contextElement": {
        "attributes": [
            {
                "name": "temperature",
                "type": "float",
                "value": "23"
            },
            {
                "name": "pressure",
                "type": "integer",
                "value": "720"
            }
        ],
        "id": "Room1",
        "isPattern": "false",
        "type": "Room"
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
```
また、特定のエンティティの単一の属性、たとえば Room2 の temperature をクエリすることもできます :

```
curl localhost:1026/v1/contextEntities/Room2/attributes/temperature -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

そのレスポンスは :

```
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "21"
        }
    ],
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
```

[標準の queryContext オペレーション](#query-context-operation)と比較して、次の点が異なります :

-   コンビニエンス・オペレーションでは、リクエストにペイロードなしで GET メソッドを使用します。標準オペレーションより簡単です
-   エンティティのすべての属性をクエリするためのコンビニエンス・オペレーションのレスポンスで使用される response contextElementResponse 要素は、標準の queryContext のレスポンス内に表示されるものと同じ構造を持ちます。ただし、エンティティの単一の属性のクエリへのレスポンスとして使用されるコンビニエンス・オペレーションのレスポンスの contextAttributeResponse 要素は新しいものです
-   URl の "/type/Room/id/Room1" で "Room1" を置き換えて、型を定義することができます。一般的には "/type//id/" です

以下に示すように、同じ型に属するすべてのエンティティ、すべての属性、または特定のエンティティによってクエリを実行することもできます。まず、標準の updateContext APPEND オペレーションを使用して Car 型の2つのエンティティを作成します。前のセクションで説明したように、コンビニエンス・オペレーションを使用して型を持つエンティティを作成することはできません :

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Car",
            "isPattern": "false",
            "id": "Car1",
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "value": "75"
                },
                {
                    "name": "fuel",
                    "type": "float",
                    "value": "12.5"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
} 
EOF
```
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
     --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Car",
            "isPattern": "false",
            "id": "Car2",
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "value": "90"
                },
                {
                    "name": "fuel",
                    "type": "float",
                    "value": "25.7"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```

すべての属性を取得するようリクエストします :

```
curl localhost:1026/v1/contextEntityTypes/Car -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

レスポンス :

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "75"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "12.5"
                    }
                ],
                "id": "Car1",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "90"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "25.7"
                    }
                ],
                "id": "Car2",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

1つのアトリビュートだけを取得するようリクエストします。例えば、スピード :

```
curl localhost:1026/v1/contextEntityTypes/Car/attributes/speed -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

レスポンス :

``` 
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "75"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "12.5"
                    }
                ],
                "id": "Car1",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "90"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "25.7"
                    }
                ],
                "id": "Car2",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
} 

``` 

追加コメント :

-   また、クエリで地理的な範囲を使用することもできます。これは高度なトピックで、[このセクション](geolocation.md#geolocation-capabilities)で説明します
-   *?attributeFormat=object* URI パラメータを使用すると、ベクターではなく JSON オブジェクト、つまりキー値マップとして属性を取得できます。デフォルト動作です :

``` 
curl localhost:1026/v1/contextEntities/Room1?attributeFormat=object -s -S \
    --header 'Accept: application/json' | python -mjson.tool

```


```
 {
    "contextResponses": [
        {
            "contextElement": {
                "attributes": {
                    "pressure": {
                        "type": "integer",
                        "value": "720"
                    },
                    "temperature": {
                        "type": "float",
                        "value": "23"
                    }
                },
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"'
            }
        }
    ]
}

``` 

[トップ](#top)

<a name="getting-all-entities"></a>
#### すべてのエンティティの取得

以下のコンビニエンス・オペレーションを使用して、すべてのエンティティを取得できます :

```
curl localhost:1026/v1/contextEntities -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool
```
このチュートリアルの場合、Room1 と Room2 の両方を返します :

``` 
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "720"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": ""
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "21"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "711"
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": ""
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
``` 

追加コメント :

-   Orion に格納されているすべてのエンティティを取得することは、エンティティの数が限られている>場合を除いて、本当に良い考えではありません。[フィルタに関するセクション](filtering.md#filters)を見てください
-   デフォルトでは、20個のエンティティしか返されないことに注意してください(このチュートリアルでは問題ありませんが、実際の使用シナリオではそうではないかもしれません)。この動作を変更するには、このマニュアルの [ページネーションに関するセクション](pagination.md#pagination)を参照してください
-   [以前のセクション](#convenience-query-context)で説明したように、*?attributeFormat=object* URI パラメータを使用して 、ベクトル (デフォルト動作) ではなく JSON オブジェクト、つまり key-values マップとして属性を取得できます

[トップ](#top)

<a name="browsing-all-types-and-detailed-information-on-a-type"></a>
#### すべての型と型に関する詳細情報のブラウズ

Orion Context Broker に存在するすべてのエンティティ型のリストを取得するには、次のオペレーションを使用します :

``` 
curl localhost:1026/v1/contextTypes -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool
``` 

レスポンスは次のようになります :

``` 
{
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    },
    "types": [
        {
            "attributes": [
                "speed",
                "fuel",
                "temperature"
            ],
            "name": "Car"
        },
        {
            "attributes": [
                "pressure",
                "hummidity",
                "temperature"
            ],
            "name": "Room"
        }
    ]
}
``` 
ご覧のとおり、各型の属性情報が提供されています。いくつかの重要な意見です :

-   NGSI が与えられた型のすべてのエンティティに同じ属性のセット (つまり、同じ型のエンティティは異なる属性セットを持つことができます) を強制しないとすれば、このオペレーションによって返される型ごとの属性セットは、その型に属する各エンティティの属性セットの和集合です
-   属性情報に興味がない場合は、型のリストのみを取得するために *?collapse=true* パラメータを使用できます

さらに、次のオペレーションを使用して、特定の型の詳細情報を取得することができます。その情報は、すべての属性のリストに当てはまります :

``` 
curl localhost:1026/v1/contextTypes/Room -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool
``` 

レスポンスは次のようになります :

``` 
{
    "attributes": [
        "hummidity",
        "pressure",
        "temperature"
    ],
    "name": "Room",
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
``` 
       
[ページネーション・メカニズム](pagination.md#pagination) は、上記のオペレーションでも機能することに注意してください。

さらに、このコンビニエンス・オペレーションには標準オペレーションの対応がないことに注意してください。

[トップ](#top)

<a name="convenience-update-context"></a>
#### コンビニエンスなコンテキストの更新

Room1 の温度と気圧の値を設定しましょう :

``` 
(curl localhost:1026/v1/contextEntities/Room1/attributes -s -S \
    --header 'Content-Type: application/json'  --header 'Accept: application/json' \
    -X PUT -d @- | python -mjson.tool) << EOF
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "26.5"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "763"
        }
    ]
}
EOF
``` 

レスポンスは次のとおりです :

``` 
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
``` 

今、Room2 と同じことをやってみましょう :

``` 
(curl localhost:1026/v1/contextEntities/Room2/attributes -s -S \
    --header 'Content-Type: application/json'  --header 'Accept: application/json' \
    -X PUT -d @- | python -mjson.tool) << EOF
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "27.4"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "755"
        }
    ]
} 
EOF
``` 
 
そのレスポンスは :

``` 
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
``` 

特定のエンティティの単一の属性を次のように更新できます :

``` 
(curl localhost:1026/v1/contextEntities/Room2/attributes/temperature -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -X PUT -d @- | python -mjson.tool) <<EOF
{
    "value": "26.3"
} 
EOF
``` 

[標準の updateContext オペレーション](#update-context-elements)と比較して、次の点が異なります :

-   コンビニエンス・オペレーション・リクエストを使用して、一度に複数のエンティティを更新することはできません
-   コンビニエンス・オペレーションにおけるリクエストとレスポンスのペイロードは、標準オペレーションで使用されるものと非常によく似ていますが、contextAttributeList と contextResponse 要素は同じです
-   URl の "/type/Room/id/Room1" で "Room1" を置き換えて、型を定義することができます。一般的には "/type//id/" です

属性値の単純な値 (文字列) の他に、複雑な構造やカスタム・メタデータを使用することもできます。これらはそれぞれ、[このセクション](structured_attribute_valued.md#structured-attribute-value)と[他のセクション](metadata.md#custom-attribute-metadata)で説明されている先進的なトピックです。

[トップ](#top)

<a name="convenience-operations-for-context-subscriptions"></a>
#### コンテキスト・サブスクリプションのためのコンビニエンス・オペレーション

コンテキスト・サブスクリプションを管理するには、次のコンビニエンス・オペレーションを使用できます :

-   POST/v1/contextSubscriptions : [標準の susbcribeContext オペレーション](#context-subscriptions)と同じペイロードを使用してサブスクリプションを作成します
-   PUT/v1/contextSubscriptions/{subscriptionID} : [標準の updateContextSubscription オペレーション](#context-subscriptions)と同じペイロードを使用して、{subscriptionID} によって識別されるサブスクリプションを更新します。ペイロードの ID は、URL の ID と一致する必要があります
-   DELETE/v1/contextSubscriptions/{subscriptionID} : {subscriptionID} によって識別されるサブスクリプションをキャンセルします。この場合、ペイロードは使用されません

[トップ](#top)

<a name="summary-of-ngsi10-convenience-operations-urls"></a>
#### NGSI10 のコンビニエンス・オペレーション URLs のサマリ

コンビニエンス・オペレーションでは、URL を使用してリソースを識別し、HTTP verb を使用して通常の REST 規則に従いそのリソースのオペレーションを識別します : GET は情報の取得に使用され、POST は新しい情報の作成に使用され、PUT は情報の更新に使用され、DELETE は情報を削除するために使用されます。

[次のドキュメント](https://docs.google.com/spreadsheet/ccc?key=0Aj_S9VF3rt5DdEhqZHlBaGVURmhZRDY3aDRBdlpHS3c#gid=0)で概要を見つけます。

[トップ](#top)

<a name="context-availability-management-using-ngsi9"></a>
## NGSI9 を使用したコンテキスト・アベイラビリティ管理

<a name="ngsi9-standard-operations"></a>
### NGSI9 標準オペレーション

このセクションでは、Orion Context Broker がサポートするさまざまな標準 NGSI9 オペレーションについて説明し、リクエストとレスポンスの例を示します。NGSI 実装の使用を容易にするために FIWARE プロジェクトによって定義された他のオペレーション・ファミリー (" コンビニエンス (convenience)") と区別するために、OMA NGSI 仕様から直接得られたものとして "標準(standard)" という用語を使用します。

**このチュートリアルを開始する前に、[このドキュメントで前述](#starting-the-broker-for-the-tutorials)したように broker を再起動することを忘れないでください。**

このセクションの最後では、Orion Context Broker を NGSI9 標準オペレーションで使用して、コンテキスト・プロデューサとコンシューマの両方アプリケーションを作成するための基本的な知識が得られます :

-   registerContext
-   discoverContextAvailability
-   subscribeContextAvailability
-   updateContextAvailabilitySubscription
-   unsubscribeContextAvailability

[トップ](#top)

<a name="register-context-operation"></a>
#### コンテキストの登録のオペレーション

まず、Room1 と Room2 を登録する必要があります。これを行うために、次の NGSI9 registerContext オペレーションを使用します :

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room1"
                },
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room2"
                }
            ],
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "isDomain": "false"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M"
}
EOF
``` 

ペイロードには contextRegistration 要素のリストが含まれ、各要素には次の情報が含まれます :

-   登録するエンティティのリスト。このチュートリアルの場合、それらは *Room1* と *Room2* のエンティティです。各エンティティについて、*型* (この場合、型として "room" を使用しています) と *ID* (それぞれ "Room1" および "Room2" です) を指定します。*isPattern* フィールドは、実際に registerContext で使用されていないので、それは常に "false" の値を有します
-   エンティティに登録する属性のリスト。このチュートリアルの場合、温度と気圧の属性です。それぞれについて、*名前*、*型* および、*ドメイン属性* かどうかを定義します
    -   Orion Context Broker は型のチェックを行いません(例えば、コンテキスト・プロデューサ・アプリケーションが温度の値を更新したときに、この値が "hot" のようなものではなく、"25.5" または "-40.23" のような浮動小数点としてフォーマットされているかどうかをチェックしません。)。さらに、ドメイン属性はサポートされていないため、isDomain は常に "false" に設定する必要があります
-   提供するアプリケーションの URL。" アプリケーションの提供 " またはコンテキスト・プロバイダとは、登録されるエンティティおよび属性の実際のコンテキスト情報を表す URL を意味します。この例では、すべてのセンサが http://mysensors.com/Rooms によって提供されていると仮定しています。もちろん、これは偽の URL です。このマニュアルの後に、アプリケーションの提供に関する詳細情報があります

この例では、1つの contextRegistration 要素を使用して両方のルームを登録していますが、2つの contextRegistration を使用することもできます。これは、通常、両方の部屋が異なる提供アプリケーションを有する場合です。例えば、<http://mysensors.com/Rooms1>, <http://mysensors.com/Rooms2>。さらに、各センサーが異なる提供アプリケーションに関連付けられている場合、4つの異なる contextRegistration を使用します(例えば、<http://mysensors.com/Rooms1/temperature>, <http://mysensors.com/Rooms1/pressure>, <http://mysensors.com/Rooms2/temperature>, <http://mysensors.com/Rooms2/pressure>)。

最後に、ペイロードには duration 要素が含まれていることに注意してください。duration 要素は、レジストレーション期間を設定します。その期間が過ぎた後は期限切れとみなすことができます ([期間は延長できます](duration.md#extending-duration))。duration 形式には [ISO 8601標準](https://en.wikipedia.org/wiki/ISO_8601) を使用します。このチュートリアルでは "1 ヶ月" を意味する "P1M" を使用しています。非常に大量の、おそらくこのチュートリアルを完了するのに十分な時間です :)。

次のレスポンスを得ます :

``` 
{
    "duration": "P1M",
    "registrationId": "52a744b011f5816465943d58"
}
``` 

registrationId (その値は、現在の時刻のタイムスタンプを使用して生成されるため、リクエストを実行するときに異なります :) は、レジストレーションへの一意の参照を提供する24桁の16進数です。[このマニュアルの後半](context_providers.md#updating-registrations)で説明するように、登録を更新するために使用されます。

[トップ](#top)

<a name="discover-context-availability-operation"></a>
#### コンテキスト・アベイラビリティのディスカバー・オペレーション

今、broker は Room1 と Room2 に関するレジストレーション情報を持っています。どうすればその情報にアクセスできますか？NGSI9 の discoverContextAvailability オペレーションの使用で、たとえば、次のように Room1 のレジストレーションをディスカバーできます :

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ]
} 
EOF
``` 

これにより、次のレスポンスが生成されます :

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": "float"
                    },
                    {
                        "isDomain": "false",
                        "name": "pressure",
                        "type": "integer"
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": "Room"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

リクエストには空の属性が使用されています。そうすることで、どの属性が登録されていても、ディスカバー (Discover) は Room1 を検索します。より正確にするには、検索する属性の名前を含めることができます :

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

次のレスポンスが生成されます :

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": "float"
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": "Room"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

broker にレジストレーション情報がない場合、broker はレジストレーション情報を返します。したがって、次のリクエスト :

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "humidity"
    ]
}
EOF
``` 

次のレスポンスを生成します :

``` 
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context element registrations found"
    }
}
``` 

Room1 と Room2 の両方の温度をディスカバーするなど、エンティティのリストを検索することもできます :

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        },
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

次のレスポンスが生成されます :

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": "float"
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": "Room"
                    },
                    {
                        "id": "Room2",
                        "isPattern": "false",
                        "type": "Room"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

最後に、Orion Context Broker の強力な機能は、エンティティ ID に正規表現を使用できることです。たとえば、正規表現 "Room.*" を使用して ID が "Room" で始まるエンティティをディスカバーできます。この場合、次のように isPattern を "true" に設定する必要があります :

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "true",
            "id": "Room.*"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

デフォルトでは 20個のレジストレーションのみが返されることに注意してください (このチュートリアルでは問題ありませんが、実際の使用シナリオではないと思われます)。この動作を変更するには、このマニュアルの [ページネーションに関するセクション](pagination.md#pagination)を参照してください。

[トップ](#top)

<a name="context-availability-subscriptions"></a>
#### コンテキスト・アベイラビリティ・サブスクリプション

これまで知った NGSI9 のオペレーション (registerContext と discoverContextAvailability) は、同期コンテキスト・プロデューサとコンテキスト・コンシューマのアプリケーションの基本的なビルディングブロックです。しかし、Orion Context Broker には、"何か" が発生したときにコンテキスト情報のアベイラビリティを確保する機能(アプリケーションが非同期通知を受け取る場合)があります。このようにして、discoverContextAvailability リクエスト (つまりポーリング) を継続的に繰り返す必要はなく、Orion Context Broker は情報が来たときにそれを知ることができます。

accumulator-server.py プログラムがまだ実行中であると仮定します。それ以外の場合は、[前のセクション](#starting-accumulator-server-for-the-tutorials)の説明に従って起動してください。

コンテキスト・アベイラビリティ・サブスクリプションは、コンテキスト情報 (つまり、いくつかのエンティティの属性の値) ではなく、コンテキストソース自体のアベイラビリティについて通知される場合に使用されます。これが意味することを例で明確にします。

たとえば、アプリケーションが最終的なユーザに提供しているグラフィカルユーザインターフェイスに新しいルームアイコンを描画する必要があるため、Orion Context Broker が新しいルームのレジストレーションを認識するたびに、コンテキスト・コンシューマ・アプリケーションが通知を受けたいと考えてみましょう。したがって、型 "Room" の新しいエンティティが ([registerContext オペレーション](#register-context-operation)を使用して) broker に登録されるたびに、broker は通知を送信できる必要があります。

この動作を設定するには、次の NGSI9 subscribeContextAvailability リクエストを使用します :

``` 
(curl localhost:1026/v1/registry/subscribeContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "attributes": [
        "temperature"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M"
}
EOF
``` 



ペイロードには次の要素があります :

-   エンティティおよび属性は、関心のあるコンテキストのアベイラビリティ情報を定義します。これらは、通知に含めるコンテキスト・レジストレーションを選択するために使用されます。この例では、"Room" 型のエンティティ("any" は ".*" パターンで表現され、任意のストリングにマッチする正規表現です)の "temperature" 属性に関するコンテキストのアベイラビリティに関心があると述べています
-   通知を送信するコールバック URL は、*reference* 要素で定義されます。以前に起動した accumulator-server.py プログラムの URL を使用しています。subscribeContextAvailability リクエストにつき1つの参照のみを含めることができます。ただし、同一のコンテキストアベイラビリティ要素(つまり、同じエンティティおよび属性)に何ら問題なく複数のサブスクリプションを含めることができます。デフォルト URL スキーマ(指定しない場合)は "http" です。たとえば、リファレンスとして "localhost:1028" を使用すると、実際には "http://localhost:1028" と解釈されます
-   サブスクリプションには、duration(registerContext リクエストと同じ形式の duration 要素で指定)があります。この期間が終了すると、サブスクリプションは無視されます(ただし、broker データベースにはまだ格納されており、[管理マニュアル](../admin/database_admin.md#deleting-expired-documents)に記載されている手順を使用してパージする必要があります)。サブスクリプションの期間を延長するには、[このドキュメントの後半](duration.md#extending-duration)で説明します。このチュートリアルでは "1 ヶ月" を意味する "P1M" を使用しています

ご覧のとおり、subscriptionContextAvailability の構造は [NGSI10 subscribeContext](#context-subscriptions) の構造に似ていますが、この場合 notifyConditions も throttling も使用しません。

subscribeContextAvailability リクエストへのレスポンスは、サブスクリプション ID (サブスクリプションの更新とキャンセルに使用される24桁の16進数 - このチュートリアルの後の手順で必要となるため、サブスクリプションを書き留めてください) と duration acknowledgement です。ここでも、subscribeContext とよく似ています。

``` 
{
    "duration": "P1M",
    "subscriptionId": "52a745e011f5816465943d59"
}
```             

accumulator-server.py を見ると、次の初期通知が表示されます :

``` 
POST http://localhost:1028/accumulate
Content-Length: 638
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json
                                                                                        
{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Room",
                        "isPattern": "false",
                        "id": "Room1"
                    },
                    {
                        "type": "Room",
                        "isPattern": "false",
                        "id": "Room2"
                    }
                ],
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

Orion Context Broker は、notifyContextAvailabilityRequest ペイロードを使用して、サブスクリプションの参照として使用される URL で、POST HTTP メソッドを使用して NGSI9 subscribeContextAvailability を通知します。subscribeId 要素 (subscribeContextAvailability リクエストへのレスポンスに一致するもの) と originator 要素を除いて、contextResponse ベクトルは [discoverContextAvailability レスポンス](#discover-context-availability-operation)で使用されたものと同じです。

現在、originator は常に "localhost" です。これ以降のバージョンでこれを使用するより柔軟な方法を検討します。

初期通知には、subscribeContextAvailability リクエストで使用されるエンティティ/属性と一致する、現在、登録されているすべてのエンティティが含まれます。つまり、レジストレーションは Room1 と Room2 の温度に対応します。Room1 と Room2 は2つの属性 (温度と気圧) を登録していますが、subscribeContextAvailability の属性ベクトルには温度のみが含まれているため、温度のみが示されています。

このケースでは、最初の notifyContextAvailabilityRequest を送信する必要があるかどうかについて、NGSI 仕様では明確ではありません。一方で、新しいレジストレーションのために通知を受信する前に初期レジストレーションを知っておくと便利かもしれないとのデベロッパーもいます。一方、アプリケーションは discoverContextAvailability を使用して初期状態を取得できます。したがって、この動作はそれ以降のバージョンで変更される可能性があります。あなたの意見は何ですか？:)

温度と気圧で新しい部屋 (Room3) を登録するとどうなるか見てみましょう :

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room3"
                }
            ],
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "isDomain": "false"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M"
}
EOF
``` 
予想どおり、accumulator-server.py は新しいレジストレーションを通知されます。ここでも、Room3 のレジストレーションには温度と気圧が含まれていますが、通知には最初の属性のみが含まれています。


``` 
POST http://localhost:1028/accumulate
Content-Length: 522
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Room",
                        "isPattern": "false",
                        "id": "Room3"
                    }
                ],
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 
サブスクリプションに一致しないコンテキスト・レジストレーションが通知をトリガーしないことを確認することもできます。たとえば、属性の気圧だけで部屋 (Room4) を登録してみましょう。サブスクリプションには属性ベクトルの温度のみが含まれていることに注意してください。

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room4"
                }
            ],
            "attributes": [
                {
                    "name": "pressure",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M"
} 
EOF
``` 

新しい通知が accumulator-server.py に到着していないことを確認できるようになりました。

コンテキスト・サブスクリプションと同様に、NGSI9 updateContextAvailabilitySubscription を使用して、コンテキスト・アベイラビリティ・サブスクリプションを更新できます。リクエストには、変更するサブスクリプションを識別する *subscriptionId* と実際の更新ペイロードが含まれます。たとえば、サブスクリプション・エンティティを全く異なるものに変更しましょう。部屋の代わりに車とすべての属性が削除されます (空の属性要素など)。常に前の手順で subscribeContextAvailability レスポンスから得た値でコピー＆ペースト後に subscriptionId の値を置き換える必要があります。

```
(curl localhost:1026/v1/registry/updateContextAvailabilitySubscription -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Car",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "duration": "P1M",
    "subscriptionId": "52a745e011f5816465943d59"
}
EOF                                                                                   
```
レスポンスは subscribeContextAvailability リクエストのレスポンスと非常に似ています :

```
{
    "duration": "P1M",
    "subscriptionId": "52a745e011f5816465943d59"
}
```

現在のところ、車の実体が登録されていないとすれば、初期通知はありません。それでは、2つの車を登録しましょう : speed という属性名の Car1 と location という属性名の Car2 を登録しましょう。

```
(curl localhost:1026/v1/registry/registerContext -s -S  --header 'Content-Type: application/json' \
    --header 'Accept: application/json'  -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car1"
                }
            ],
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
}
EOF
```

```
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car2"
                }
            ],
            "attributes": [
                {
                    "name": "location",
                    "type": "ISO6709",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
}
EOF
```
両方のレジストレーションが updateContextAvailabilitySubscription で使用されているエンティティと属性と一致するので、accumulator-server.py で確認できるように、それぞれのレジストレーションに対して通知が届きます :

```
POST http://localhost:1028/accumulate
Content-Length: 529
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Car",
                        "isPattern": "false",
                        "id": "Car1"
                    }
                ],
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
```

```
POST http://localhost:1028/accumulate
Content-Length: 535
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Car",
                        "isPattern": "false",
                        "id": "Car2"
                    }
                ],
                "attributes": [
                    {
                        "name": "location",
                        "type": "ISO6709",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
```

最後に、リクエスト・ペイロードで subscriptionId を使用するだけで、NGSI9 の unsubscribeContextAvailability オペレーションを使用してサブスクリプションを取り消すことができます。前の手順の subscribeContextAvailability レスポンスで受け取ったサブスクリプション ID 値をコピーして置き換えます

```
(curl localhost:1026/v1/registry/unsubscribeContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "subscriptionId": "52a745e011f5816465943d59"
}
EOF
```
 
このレスポンスは、取り消しが成功したという結果です。

```
{
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    },
    "subscriptionId": "52a745e011f5816465943d59"
}
```

キャンセル後、新しい車 (例えば Car3) を登録して、accumulator-server.py に新しい通知が送信されないことを確認することができます。

[トップ](#top)

<a name="summary-of-ngsi9-standard-operations-urls"></a>
#### NGSI9 標準オペレーション URLs のサマリ

各標準オペレーションには一意の URL があります。それらのすべては POST メソッドを使用します。概要は次のとおりです :

-   <host:port>/v1/registry/registerContext
-   <host:port>/v1/registry/discoverContextAvailability
-   <host:port>/v1/registry/subscribeContextAvailability
-   <host:port>/v1/registry/updateContextAvailabilitySubscription
-   <host:port>/v1/registry/unsubscribeContextAvailability

[トップ](#top)

<a name="ngsi9-convenience-operations"></a>
### NGSI9 コンビニエンス・オペレーション

次のセクションでは、Orion Context Broker がサポートしている FIWARE NGSI REST API NGSI9 の一部として説明されているさまざまなコンビニエンス・オペレーションについて説明し、リクエストとレスポンスの例を示します。コンビニエンス・オペレーションは、OMA NGSI 仕様で定義されている標準オペレーションを補完する NGSI 実装の使用を容易にするために、FIWARE プロジェクトによって定義された一連のオペレーションです。

**このチュートリアルを開始する前に、[このドキュメントで前述](#starting-the-broker-for-the-tutorials)したように broker を再起動することを忘れないでください。**

このセクションの最後では、[前のセクション](#tutorial-on-ngsi9-standard-operations)で説明した標準的なオペレーションの手軽な代替手段として、コンビニエンス・オペレーションを使用する方法を学びました。2つのアプローチを比較できるようにするには、事前にこのチュートリアルを行い、register、discover などに慣れ親しむことを強くお勧めします

[トップ](#top)

<a name="convenience-register-context"></a>
#### コンビニエンスなコンテキストの登録

まず、次のコマンドを使用して Room1 と Room2 に温度と気圧の属性を登録します :

``` 
(curl localhost:1026/v1/registry/contextEntities/Room1/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
```
(curl localhost:1026/v1/registry/contextEntities/Room1/attributes/pressure -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \ 
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
```
(curl localhost:1026/v1/registry/contextEntities/Room2/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
```
(curl localhost:1026/v1/registry/contextEntities/Room2/attributes/pressure -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \ 
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
  
では、標準の registerContext オペレーションと比較してどのような違いがありますか？

-   標準的なオペレーションでは1回のリクエストではなく、4回のリクエストが必要でした
-   より多くのオペレーションを使用していますが、各オペレーションで使用されるペイロードははるかに簡単です。このペイロードは、duration とアプリケーションの提供だけを含む、registerContext 内のペイロードの簡略版です
-   URl の "/type/Room/id/Room1" で "Room1" を置き換えて、型を定義することができます。一般的には "/type//id/" です
-   OrionContextBroker の観点からは、4つの独立したレジストレーション (つまり、4つの異なる registration IDs)がすべての効果([更新](updating_regs_and_subs.md#updating-registrations)、[期間 (duration) の延長など)](duration.md#extending-duration) にあります
-   /v1/registry/contextEntities/Room1(属性部分なし)を使用することは可能です。その場合、属性を持たないエンティティを登録しています。registerProviderRequest 要素で属性を指定することはできません

これらのリクエストのそれぞれに対するレスポンスは、標準の registerContext に対するレスポンスと同じです (4つのリクエストごとに異なる ID を持つ1つのレスポンス) :

```
{
    "duration": "P1M",
    "registrationId": "51c1f5c31612797e4fe6b6b6"
}
```                               

[トップ](#top)

<a name="only-type-entity-registrations-using-convenience-operations"></a>
#### コンビニエンス・オペレーションを使用した型のみによるエンティティのレジストレーション

NGSI9 の "contextEntityTypes" コンビニエンス・オペレーションを使用して、特定の ID なしでエンティティ型を登録することができます。例を挙げて説明しましょう。

"Funny" というエンティティ型を登録しましょう。エンティティ ID を指定していないことに注意してください :

```
(curl localhost:1026/v1/registry/contextEntityTypes/Funny -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration": "P1M",
  "providingApplication": "http://mysensors.com/Funny"
}
EOF
```

さあ、その型について見てみましょう :

```
curl localhost:1026/v1/registry/contextEntityTypes/Funny -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

そのレスポンスは :

```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "id": "",
                        "isPattern": "false",
                        "type": "Funny"
                    }
                ],
                "providingApplication": "http://mysensors.com/Funny"
            }
        }
    ]
}
```

ご覧のとおり、ID 要素は空です。レジストレーション時に ID を指定しなかったので意味があります。

さらに、これらのレジストレーションに属性を登録することもできます。例 :

```
(curl localhost:1026/v1/registry/contextEntityTypes/MoreFunny/attributes/ATT -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration": "P1M",
  "providingApplication": "http://mysensors.com/Funny"
}
EOF
```

```
curl localhost:1026/v1/registry/contextEntityTypes/MoreFunny/attributes/ATT -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "ATT",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "",
                        "isPattern": "false",
                        "type": "MoreFunny"
                    }
                ],
                "providingApplication": "http://mysensors.com/Funny"
            }
        }
    ]
}
```

[トップ](#top)

<a name="convenience-discover-context-availability"></a>
#### コンビニエンスなコンテキスト・アベイラビリティのディスカバー

コンビニエンス・オペレーションを使用すると、単一のエンティティまたはエンティティと属性のペアのレジストレーション情報をディスカバーできます。たとえば、ルーム1のレジストレーションを検索するには (属性に関係なく) :

```
curl localhost:1026/v1/registry/contextEntities/Room1 -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```

次のレスポンスが生成されます :
```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        },
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "pressure",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
```
さて、Room2-temperature のレジストレーションを見てみましょう :

```
curl localhost:1026/v1/registry/contextEntities/Room2/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```

レスポンスは次のとおりです :

```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room2",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        },
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "pressure",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room2",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
```

登録されていない要素 (例 : Room5 または Room1 の湿度) を検出すると、エラーが発生します。例 :

```
curl localhost:1026/v1/registry/contextEntities/Room3 -s -S  \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
```
curl localhost:1026/v1/registry/contextEntities/Room2/attributes/humidity -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
次のエラーレスポンスが生成されます :

```
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context element found"
    }
}
```

[標準の discoverContextAvailability オペレーション](#discover-context-availability-operation)と比較して :

-   コンビニエンス・オペレーションでは、リクエストにペイロードを必要とせずに GET メソッドを使用します。標準オペレーションよりも簡単です。ただし、コンテンツには2つの違いがあります。第1に、[前に説明した](#convenience-register-context)ように、各属性が異なるレジストレーションに対応するので、各属性は常に異なる contextRegistrationResponse 要素に入ります。第2に、コンビニエンス・オペレーションを使用して行われたレジストレーションが型付けされていないので、型フィールドはエンティティおよび属性に対して空です
-   URl の "/type/Room/id/Room1" で "Room1" を置き換えて、型を定義することができます。一般的には "/type//id/" です
-   エンティティのリスト、エンティティのパターン、属性のリストを発見するためにコンビニエンス・オペレーションを使用することはできません

以下に示すように、同じ型に属するすべてのエンティティ、すべての属性、または特定のエンティティによってディスカバーすることもできます。まず、標準の registerContext オペレーションを使用して Car 型の2つのエンティティを登録します。前のセクションで説明したように、コンビニエンス・オペレーションを使用してエンティティを型に登録することはできません :

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car1"
                }
            ],
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
}
EOF
``` 
``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car2"
                }
            ],
            "attributes": [
                {
                    "name": "fuel",
                    "type": "float",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
} 
EOF
```

属性を指定しないリクエスト :

```
curl localhost:1026/v1/registry/contextEntityTypes/Car -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
レスポンス :

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "speed",
                        "type": "integer"
                    }
                ],
                "entities": [
                    {
                        "id": "Car1",
                        "isPattern": "false",
                        "type": "Car"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        },
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "fuel",
                        "type": "float"
                    }
                ],
                "entities": [
                    {
                        "id": "Car2",
                        "isPattern": "false",
                        "type": "Car"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
``` 

1つの属性 (速度など) を指定するリクエスト :

```
curl localhost:1026/v1/registry/contextEntityTypes/Car/attributes/speed -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
レスポンス :

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "speed",
                        "type": "integer"
                    }
                ],
                "entities": [
                    {
                        "id": "Car1",
                        "isPattern": "false",
                        "type": "Car"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
``` 

デフォルトでは 20個のレジストレーションのみが返されることに注意してください。このチュートリアルでは問題ありませんが、実際の使用シナリオではないと思われます。この動作を変更するには、このマニュアルの [ページネーションに関するセクション](pagination.md#pagination)を参照してください。

[トップ](#top)

<a name="convenience-operations-for-context-availability-subscriptions"></a>
#### コンテキスト・アベイラビリティのサブスクリプションのためのコンビニエンス・オペレーション

コンテキスト・アベイラビリティのサブスクリプションを管理するには、次のコンビニエンス・オペレーションを使用できます :

- POST/v1/registry/contextAvailabilitySubscriptions : [標準の susbcribeAvailabilityContext オペレーション](#context-availability-subscriptions) と同じペイロードを使用してサブスクリプションを作成します
- PUT/v1/registry/contextAvailabilitySubscriptions/{subscriptionID} : [標準の updateContextAvailabilitySubscription オペレーション](#context-availability-subscriptions) と同じペイロードを使用して、{subscriptionID} によって識別されるサブスクリプションを更新します。ペイロードの ID は、URL の ID と一致する必要があります
- DELETE/v1/registry/contextAvailabilitySubscriptions/{subscriptionID} : {subscriptionID} によって識別されるサブスクリプションをキャンセルします。この場合、ペイロードは使用されません

[トップ](#top)

<a name="summary-of-ngsi9-convenience-operations-urls"></a>
#### NGSI9 コンビニエンス・オペレーション URLs のサマリ

コンビニエンス・オペレーションでは、URL を使用してリソースを識別し、HTTP verb を使用して通常の REST 規則に従いそのリソースのオペレーションを識別します : GET は情報の取得に使用され、POST は新しい情報の作成に使用され、PUT は情報の更新に使用され、DELETE は情報を削除するために使用されます。

[次のドキュメント](https://docs.google.com/spreadsheets/d/1f4m624nmO3jRjNalGE11lFLQixnfCENMV6dc54wUCCg/edit#gid=0)に概要があります。
<span style="color:red">.

[Top](#top)
