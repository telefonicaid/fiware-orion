# <a name="top"></a>FIWARE NGSI APIv2 ウォークスルー

* [イントロダクション](#introduction)
* [始める前に ...](#before-starting)
    * [例](#example-case)
    * [チュートリアル用の broker の起動](#starting-the-broker-for-the-tutorials)
    * [アキュムレータ・サーバの起動](#starting-accumulator-server)
    * [broker にコマンドを発行](#issuing-commands-to-the-broker)
* [コンテキスト管理](#context-management)
    * [エンティティの作成](#entity-creation)
    * [エンティティのクエリ](#query-entity)
    * [すべてのエンティティの取得とフィルタリング](#getting-all-entities-and-filtering)
    * [エンティティの更新](#update-entity)
    * [サブスクリプション](#subscriptions)
    * [すべての型と型に関する詳細情報のブラウズ](#browsing-all-types-and-detailed-information-on-a-type)
    * [バッチ処理](#batch-operations)
* [コンテキスト・アベイラビリティ管理](#context-availability-management)

<a name="introduction"></a>
## イントロダクション

このウォークスルーでは、読者が Orion Context Broker に慣れ親しんでプロセスを楽しむのに役立つと期待される実践的アプローチを採用しています。

ウォークスルーは NGSIv2 仕様に基づいています。[ここ](http://telefonicaid.github.io/fiware-orion/api/v2/stable)で Apiary 形式で確認できます。また、[NGSIv2 実装ノート](ngsiv2_implementation_notes.md)を見てください。

メイン・セクションは [コンテキスト管理](#context-management)です。コンテキスト管理 (車の温度などのエンティティに関する情報) のための Context Broker の基本的な機能について説明します。[コンテキスト・アベイラビリティ管理](#context-availability-management) (エンティティ自体についてではなく、その情報の提供者についての情報) は、このドキュメントの一部としても記述されています。

開始する前に、または途中で紛失して最初から開始する必要がある場合も :)、[チュートリアルの broker の開始](#starting-the-broker-for-the-tutorials)で説明したように、Orion Context Broker を再起動します。

開始する前に NGSI モデルが基礎としている理論的概念に精通することをお勧めします。例えば、エンティティ、属性など。これについての FIWARE のドキュメントをご覧ください。例 : [このプレゼンテーション](http://bit.ly/fiware-orion)

[トップ](#top)

<a name="before-starting"></a>
## 始める前に ...

始める前に、チュートリアルで使用されている事例と、Orion Context Broker を実行してオペレーションする方法を紹介しましょう。

[トップ](#top)

<a name="example-case"></a>
#### 例

Orion Context Broker を使用してコンテキスト情報を管理したいと考えています。部屋は Room1, Room2, Room3, Room4 で、各部屋には温度と大気圧の2つのセンサがあります。Room4 は大気圧センサのみです。速度とGPS センスで位置を測定できるセンサを備えた2台の車 (Car1 と Car2) を持っていると考えてみましょう。

![](../../manuals/user/Orion-example-case.png "Orion-example-case.png")

<!--

Most of the time we will use Room1 and Room2 in the tutorials. Room3,
Room4, Car1 and Car2 will be used only in the section regarding [context
availability
subscriptions](#context-availability-subscriptions).

-->

Orion Context Broker は、センサ情報を提供するコンテキスト・プロデューサ・アプリケーションと、グラフィカル・ユーザ・インタフェースで表示するためにその情報を処理するコンテキスト・コンシューマのアプリケーションと対話します。チュートリアルでは、両方の種類のアプリケーションの役割を果たします。

[トップ](#top)

<a name="starting-the-broker-for-the-tutorials"></a>
### チュートリアル用の broker を起動

始める前に、"[インストールおよび管理ガイド](../admin/install.md)" の説明に従って broker をインストールする必要があります 。

チュートリアルでは、Orion Context Broker データベースに以前のコンテンツがないことを前提としています。これを行うには、[データベースの削除手順](../admin/database_admin.md#starting-the-broker-for-the-tutoria)に従います。

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

サブスクリプションと通知に関連するチュートリアルの一部では、通知を受け取ることができるコンシューマ・アプリケーションの役割を果たすためにいくつかのプロセスが必要です。そのためには、[GitHub](https://github.com/telefonicaid/fiware-orion/blob/master/scripts/accumulator-server.py)で入手可能なアキュムレータ・スクリプトをダウンロードしてください。これは非常に単純な "ダミー" アプリケーションであり、与えられた URL (以下の例では、localhost:1028/accumulate を使用していますが、別のホストやポートを指定することができます) を単にリッスンし、それが実行されるターミナル・ウィンドウで受け取ったものをエコーします。次のコマンドを使用して実行します :

```
# cd /dir/where/accumulator-server/is/downloaded
# chmod a+x accumulator-server.py
# ./accumulator-server.py --port 1028 --url /accumulate --host ::1 --pretty-print -v
```

[トップ](#top)

<a name="issuing-commands-to-the-broker"></a>
### broker にコマンドを発行

broker にリクエストを発行するには、`curl` コマンド・ラインツールを使用します。GNU/Linux システムでもほぼどこにでもあり、簡単にコピーして貼り付けることができるこのドキュメントの例を含めて簡素化するで、`curl` を選択しました。もちろん、それを使用することは必須ではなく、代わりに REST クライアント・ツールを使用することができます ([RESTClient](http://restclient.net/)ど)。実際には、おそらく、アプリケーションの REST クライアント部分を実装するプログラミング言語ライブラリを使用して Orion Context Broker とやりとりすることになるでしょう。

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

-   PATCH の場合 :
```
curl localhost:1026/<operation_url> -s -S [headers] -X PATCH -d @- <<EOF
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
curl ... -H 'Accept: application/json' ...
```

-   リクエスト (POST, PUT, PATCH など) でペイロードを使用する場合のみ、Context-Type ヘッダを使用してフォーマット (JSON) を指定する必要があります

```
curl ... -H 'Content-Type: application/json' ...
```

いくつかの追加意見 :

-   ほとんどの場合、EOF を使用して複数行のブロック (*here-documents*) の先頭と最後をマークするために、マルチライン・シェル・コマンドを使用して curl への入力を提供しています。場合(GET と DELETE)によっては、`-d @-` をペイロードを使用しないので省略します

-   例では、broker がポート1026をリッスンしていると仮定しています。異なる設定を使用している場合は、curl コマンドラインでこれを調整してください

-   レスポンスで JSON をきれいに印刷するために、Python を msjon.tool とともに使用することができます (チュートリアルの例ではこのスタイルを使用しています) :

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

<a name="context-management"></a>
## コンテキスト管理

**このチュートリアルを開始する前に、[このドキュメントで前述した](#broker)ように broker を再起動することを忘れないでください。**

このセクションの最後には、コンテキスト管理オペレーションで Orion Context Broker を使用して、コンテキスト・プロデューサとコンシューマの両方のアプリケーションを作成するための基本知識があります。

<a name="entity-creation"></a>
### エンティティの作成

Orion Context Broker は空の状態から開始します。まず、特定のエンティティの存在を認識させる必要があります。特に、2つの属性 (温度と気圧) を持つ Room1 と Room2 エンティティを "作成" します。この `POST /v2/entities` オペレーションを使用してこれを行います。

まず、Room1 を作成します。エンティティ作成時に、Room1 の温度と気圧がそれぞれ 23 ºC と 720mmHg であるとします。

```
curl localhost:1026/v2/entities -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "Room1",
  "type": "Room",
  "temperature": {
    "value": 23,
    "type": "Float"
  },
  "pressure": {
    "value": 720,
    "type": "Integer"
  }
}
EOF
```

別に `id` と `type` フィールド (ID 及びエンティティの型を定義)、ペイロードは、属性のセットを含みます。各属性には値とが含まれています。

Orion Context Broker は、型のチェックを実行しません。例えば コンテキスト・プロデューサ・アプリケーションが温度の値を更新したときに、この値が `25.5` や `-40.23` のような浮動小数点でフォーマットされていて、`hot` のようなものではないことをチェックしません。

このリクエストを受け取ると、broker は内部データベースにエンティティを作成し、その属性の値を設定し、201 Created HTTP code でレスポンスします。

次に、同様の方法で Room2 を作成します。この場合、温度と気圧をそれぞれ 21ºC と 711mmHg に設定します。

```
curl localhost:1026/v2/entities -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "Room2",
  "type": "Room",
  "temperature": {
    "value": 21,
    "type": "Float"
  },
  "pressure": {
    "value": 711,
    "type": "Integer"
  }
}
EOF
```

属性値の JSON データ型 (数値、文字列、ブール値など) に対応する単純な値以外に、複雑な構造やカスタム・メタデータを使用することもできます。これらは高度なトピックで、この[セクション](structured_attribute_valued.md#structured-attribute-values)と[他のセクション](metadata.md#custom-attribute-metadata)でそれぞれ説明します。

[トップ](#top)

<a name="query-entity"></a>
### エンティティのクエリ

次に、コンシューマ・アプリケーションの役割を果たすために、Orion Context Broker が格納しているコンテキスト情報にアクセスして興味深いことをしたいとしましょう。グラフィカル・ユーザ・インタフェースで室温を表示するなどです。例えば、Room1 のためのコンテキスト情報を取得する場合には、`GET /v2/entities/{id}` リクエストが使用されます。 :

```
curl localhost:1026/v2/entities/Room1?type=Room -s -S -H 'Accept: application/json' | python -mjson.tool
```

実際には、型を指定する必要はありません。この場合、ID だけを使用して曖昧さがないので、次のようにすることもできます :

```
curl localhost:1026/v2/entities/Room1 -s -S -H 'Accept: application/json' | python -mjson.tool
```

どちらの場合も、レスポンスには Room1 に属するすべての属性が含まれており、温度と気圧が updateContext (23 ºC と 720mmHg) を使用してエンティティ作成時に設定した値を持つことを確認できます。

```
{
    "id": "Room1",
    "pressure": {
        "metadata": {},
        "type": "Integer",
        "value": 720
    },
    "temperature": {
        "metadata": {},
        "type": "Float",
        "value": 23
    },
    "type": "Room"
}

```

属性値だけ含むのでなく、よりコンパクトで簡単な表現を得るために、`keyValues` オプションを使うことができます :


```
curl localhost:1026/v2/entities/Room1?options=keyValues -s -S -H 'Accept: application/json' | python -mjson.tool
```

そのレスポンスは :


```
{
    "id": "Room1",
    "pressure": 720,
    "temperature": 23,
    "type": "Room"
}
```

この `values` オプションを使用して、属性値のリストに対応するよりコンパクトな表現を得ることもできます。この場合、`attrs` URL パラメータを使用して注文を指定する必要があります。例えば、最初に温度を取得し、次に気圧を取得します :

```
curl 'localhost:1026/v2/entities/Room1?options=values&attrs=temperature,pressure' -s -S  \
    -H 'Accept: application/json' | python -mjson.tool
```

どのレスポンスが

```
[
    23,
    720
]
```

同じオペレーションと比較しますが、`attrs` リストを逆にします。最初に気圧、次に気圧 :

リクエスト :

```
curl 'localhost:1026/v2/entities/Room1?options=values&attrs=pressure,temperature' -s -S  \
    -H 'Accept: application/json' | python -mjson.tool
```

レスポンス :

```
[
    720,
    23
]

```

この `GET /v2/entities/{id}/attrs/{attrsName}` オペレーションを使用して、単一の属性をリクエストすることもできます。たとえば、温度のみを取得するには :

```
curl localhost:1026/v2/entities/Room1/attrs/temperature -s -S -H 'Accept: application/json' | python -mjson.tool
```

そのレスポンスは以下の通りです :

```
{
    "metadata": {},
    "type": "Float",
    "value": 23
}
```

`GET /v2/entities/{id}/attrs/{attrsName}/value` オペレーションを使用して値のみを取得することもできます。この場合、その種類の属性の値として `Accept: text/plain` を使用する必要があることに注意してください。


```
curl localhost:1026/v2/entities/Room1/attrs/temperature/value -s -S -H 'Accept: text/plain' | python -mjson.tool
```

そのレスポンスは単純です :

```
23.0
```

最後に、以下のように、存在しないエンティティまたは属性をクエリしようとするとエラーが発生することに注意してください。

リクエスト :

```
curl localhost:1026/v2/entities/Room5 -s -S -H 'Accept: application/json' | python -mjson.tool
```

レスポンス :

```
{
    "description": "The requested entity has not been found. Check type and id",
    "error": "NotFound"
}
```

リクエスト :

```
curl localhost:1026/v2/entities/Room1/attrs/humidity -s -S -H 'Accept: application/json' | python -mjson.tool
```

レスポンス :

```
{
    "description": "The entity does not have such an attribute",
    "error": "NotFound"
}
```

どちらの場合も、HTTP レスポンス・コード (示されず) は404 Not Found です。

[トップ](#top)

<a name="getting-all-entities-and-filtering"></a>
### すべてのエンティティの取得とフィルタリング

`GET /v2/entities` オペレーションを使用してすべてのエンティティを取得できます。

```
curl localhost:1026/v2/entities -s -S -H 'Accept: application/json' | python -mjson.tool
```

このチュートリアルの場合、Room1 と Room2 の両方が返されます :

```
[
    {
        "id": "Room1",
        "pressure": {
            "metadata": {},
            "type": "Integer",
            "value": 720
        },
        "temperature": {
            "metadata": {},
            "type": "Float",
            "value": 23
        },
        "type": "Room"
    },
    {
        "id": "Room2",
        "pressure": {
            "metadata": {},
            "type": "Integer",
            "value": 711
        },
        "temperature": {
            "metadata": {},
            "type": "Float",
            "value": 21
        },
        "type": "Room"
    }
]
```

`GET /v2/entities/{id}` と同様に `keyValues` と `values` オプションも使用できます。

エンティティ全体を提供するだけでなく、このオペレーションでは、取得したエンティティのリストを必要なものに調整するためのフィルタリング機能が実装されています。特に :

* `type` URL パラメータを使用して、型別にフィルタリングできます。たとえば、型がすべてのエンティティを `Room` 取得するには (この場合、Room1 と Room2 の両方を取得する)、次のようにします :

```
curl localhost:1026/v2/entities?type=Room -s -S  -H 'Accept: application/json' | python -mjson.tool
```

* `idPattern` URL パラメータ (値が正規表現) を使用して、エンティティ ID パターンを使用してフィルタすることができます。たとえば、ID が `Room` で始まり、2～5 の範囲 (この場合、Room2 を取得) の番号が続くすべてのエンティティを取得するには (括弧で囲まれた問題を避けるために、curl コマンドラインの `-g` に注意してください) :
```
curl localhost:1026/v2/entities?idPattern=^Room[2-5] -g -s -S -H 'Accept: application/json' | python -mjson.tool
```

* `q` URL パラメータを使用して、属性フィルタを使用してフィルタ処理できます。全体の説明については、[NGSv2 sepecficaiton](http://telefonicaid.github.io/fiware-orion/api/v2/stable)の "Simple Query Language" のセクションを参照してください。たとえば、温度が 22を超えるすべてのエンティティ (この場合は Room1 を取得) を取得するには、次のものを使用します :
```
curl 'localhost:1026/v2/entities?q=temperature>22' -s -S  -H 'Accept: application/json' | python -mjson.tool
```

* 地理的な場所でフィルタリングすることができます。これは高度なトピックで、[このセクション](geolocation.md#)で説明します

最終的なコメントとして、この例は非常にシンプル (2つのエンティティのみ) ですが、実際の配置では数百万のエンティティを管理できます。したがって、デフォルトでは、20個のエンティティしか返されません。このチュートリアルでは問題ありませんが、実際の使用シナリオではおそらくありません。大量のエンティティをページ単位で取得する方法については、[このマニュアルのページネーション](pagination.md#pagination)に関するセクションを参照してください。

[トップ](#top)

<a name="update-entity"></a>
### エンティティの更新

`PATCH /v2/entities/{id}/attrs` オペレーションを使用してエンティティ属性の値を更新できます。これは、属性がすでにエンティティに存在することを前提としています。

ここで、コンテキスト・プロデューサ・アプリケーション、すなわちコンテキスト情報のソースの役割を果たす。このアプリケーションでは、Room1 の温度と気圧をそれぞれ 26.5ºC と 763mmHg に設定したいと考えているので、次のリクエストを出します :

```
curl localhost:1026/v2/entities/Room1/attrs -s -S -H 'Content-Type: application/json' -X PATCH -d @- <<EOF
{
  "temperature": {
    "value": 26.5,
    "type": "Float"
  },
  "pressure": {
    "value": 763,
    "type": "Float"
  }
}
EOF
```

ご覧のように、リクエストの構造は、エンティティ ID と型がペイロードに含まれていないことを除いて、[エンティティ作成のオペレーション](#entity-creation)で使用されるものと非常に似ています。

このリクエストを受け取ると、broker は内部データベース内のエンティティ属性の値を更新し、204 No Content でレスポンスします。

これで、[エンティティのクエリ・オペレーション](#query-entity)を使用して Room1 が実際に更新されたことを確認できます。

この `PUT /v2/entities/{id}/attrs/{attrName}/value` オペレーションを使用して、属性の値を実際にコンパクトな方法で更新し、属性の型をそのままにすることもできます。たとえば、Room1 の温度を `28.4` に更新するには (ここの Content-Type は `text/plain` で値 `28.4` に対応しています。ここでは JSON は関係ありません ...) :

```
curl localhost:1026/v2/entities/Room1/attrs/temperature/value -s -S -H 'Content-Type: text/plain' -X PUT -d 28.5
```

最後に、`PUT /v2/entities/{id}/attrs` オペレーションは、指定されたエンティティのすべての属性を置き換える、つまり既存のエンティティを削除するために使用できます。

エンティティ作成の場合と同様に、属性値の JSON データ型 (数値、文字列、ブール値など) に対応する単純な値は別として、複雑な構造やカスタム・メタデータを使用することもできます。これらは高度なトピックで、[このセクション](structured_attribute_valued.md#structured-attribute-values)と[別のセクション](metadata.md#custom-attribute-metadata)でそれぞれ説明します。

属性の追加/削除の詳細について は、マニュアルの [このセクション](update_action_types.md)を参照してください。

[トップ](#top)

<a name="subscriptions"></a>
### サプスクリプション

エンティティの作成、クエリおよび更新のために現在までに精通しているオペレーションは、同期コンテキスト・プロデューサおよびコンテキストコンシューマ・アプリケーションの基本的なビルディング・ブロックです。しかし、Orion Context Broker には、"何か" が発生したときにコンテキスト情報をサブスクライブする機能 ("何か" のさまざまなケースについて説明します) を使用すると、アプリケーションで非同期通知が取得されるという強力な機能があります。このようにして、クエリ・リクエスト (ポーリングなど) を継続して繰り返す必要はなく、Orion Context Broker は情報が来たときにそれを知ることができます。

この機能を使用する前に、[アキュムレータ・サーバを起動して](#starting-accumulator-server)通知を取得してください。

サブスクリプションを作成するために、以下の `POST /v2/subscriptions` オペレーションが使用されます。次の例を考えてみましょう。後で説明するように、レスポンスで Location ヘッダを取得するために `-v` を使用しています :

```
curl -v localhost:1026/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "A subscription to get info about Room1",
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "pressure"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/accumulate"
    },
    "attrs": [
      "temperature"
    ]
  },
  "expires": "2040-01-01T14:00:00.00Z",
  "throttling": 5
}
EOF
```

ペイロードに含まれるさまざまな要素を詳細に検討してみましょう :

-   `notifications` の中の `entities` と `attrs` は、通知メッセージの内容を定義します。この例では、Room1 エンティティの温度属性を通知に含める必要があることを示しています
-   通知を送信する URL は、`url` サブフィールドで定義されます。ここでは、以前に起動した accumulator-server.py プログラムの URL を使用しています。1つのサブスクリプションにつき1つの URL のみを含めることです。ただし、同一のコンテキスト要素。つまり、同じエンティティと属性)にいくつかのサブスクリプションを問題なく置くことができます
-   サブスクリプションの有効期限 (`expires` フィールド) は、[ISO 8601](https://en.wikipedia.org/wiki/ISO_8601) 標準フォーマットで指定できます。サブスクリプションがその日付よりもオーバーフローすると、サブスクリプションは単に無視されますただし、broker データベースにはまだ格納されており、[管理マニュアル](../admin/database_admin.md#deleting-expired-documents)に記載されている手順を使用してパージする必要があります。サブスクリプションを更新することで期間を延長することができます。このチュートリアルを実行している間、サブスクリプションが期限切れにならないように、十分に離れた日付 (2040年) を使用しています :)
-   恒久的なサブスクリプションも可能です。`expires` フィールドを省略してください
-   `conditions` 要素は、サブスクリプションの "トリガー" を定義します。この `attrs` フィールドには、属性名のリストが含まれています。これらの名前は、" トリガする属性 "、すなわち、[エンティティの作成](#entity-creation)または [更新](#update-entity)によって作成/変更が通知をトリガする属性を定義します
-  ルールは、`conditions.attrs` リストの属性の少なくとも1つが変更された場合 (ある種の "OR" 条件など)、通知が送信されます。たとえば、この場合、Room1 の気圧が変化すると、Room1 の温度値が通知されますが、気圧自体は通知されません。気圧を通知したい場合は、`notifications.attrs` リスト内に "気圧" を含める必要があります。または、"エンティティのすべての属性" を意味する空の属性ベクトルを使用する必要があります。ここでの、気圧の値が変化するたびに温度の値が通知されるこの例はあまり有用ではないかもしれません。この例は、サブスクリプションの巨大な柔軟性を示すためにのみ、この方法が選択されています
-   エンティティ属性の変更時に通知トリガーを作成するには、`conditions.attrs` を空のままにしておくことができます (属性の名前に関係なく)
-   通知には、通知をトリガする更新オペレーションを処理した後の属性値が含まれます。ただし、Orion に以前の値も含めさせることができ ます。これはメタデータを使用して実現されます。[次のドキュメント](metadata.md#metadata-in-notifications)を見てください
-   また、"いくつかの属性を除くすべての属性に通知する" サブスクリプション (一種のブラック・リスト機能) を設定することもできます。この場合、`notifications` 内の `attrs` の代わり `exceptAttrs` を使用してください
-   `conditions` に中にフィルタリング式をに含めることができます。たとえば、気圧が変化するだけでなく、700-800 の範囲で変化するかどうかを通知するためにも使用します。これは高度なトピックです。[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "サブスクリプション" を参照して ください
-   `throttling` 要素は、通知間の最小到着時間を指定するために使用されます。したがって、上記の例のように5秒にスロットリングを設定すると、その期間に実際に何回変更が行われても、前の通知が5秒前に送信された場合に通知が送信されなくなります。これは、通知レセプターに、属性値を頻繁に更新するコンテキスト・プロデューサに対して自分自身を保護する手段を与えることです。multi-CB 構成では、最後の通知手段が各 CB ノードに対してローカルであることを考慮に入れます。各ノードは DB と定期的に同期してより新しい値を取得しますが ([ここ](../admin/perf_tuning.md#subscription-cache)ではこれ以上)、特定のノードに古い値があることがありますので、スロットリングは 100％ 正確ではありません

そのリクエストに対応するレスポンスは、201 HTTP レスポンス・コードとして作成されたものを使用します。サブスクリプション ID (サブスクリプションの更新とキャンセルに使用される24桁16進数) を保持する Location ヘッダが含まれています。このチュートリアルの後半で必要になるので、書き留めておいてください。

```
< HTTP/1.1 201 Created
< Connection: Keep-Alive
< Content-Length: 0
< Location: /v2/subscriptions/57458eb60962ef754e7c0998
< Fiware-Correlator: 9ac7bbba-2268-11e6-aaf0-d48564c29d20
< Date: Wed, 25 May 2016 11:05:35 GMT
```

accumulator-server.py を見てみましょう。次のような notifyContextRequest が表示されます。どれだけ待っていても、ちょうど1つだけです :

```
POST http://localhost:1028/accumulate
Content-Length: 141
User-Agent: orion/1.1.0-next libcurl/7.38.0
Ngsiv2-Attrsformat: normalized
Host: localhost:1028
Accept: application/json
Content-Type: application/json; charset=utf-8
Fiware-Correlator: 3451e5c2-226d-11e6-aaf0-d48564c29d20

{
    "data": [
        {
            "id": "Room1",
            "temperature": {
                "metadata": {},
                "type": "Float",
                "value": 28.5
            },
            "type": "Room"
        }
    ],
    "subscriptionId": "57458eb60962ef754e7c0998"
}
```

Orion Context Broker は、POST HTTP メソッド (サブスクリプションの URL 上) を使用してコンテキスト・サブスクリプションに通知します。ペイロードには、サブスクリプション ID への参照と `data`、エンティティの実際のデータが含まれます。エンティティ表現形式は、`GET /v2/entities` オペレーションに対するレスポンスで使用されるものと同じであることに注意してください 。

実際に更新を行わないと、accumulator-server.py がこのメッセージを受け取る理由は不思議に思うかもしれません。これは *初期通知* によるもので、詳細については [ここ](initial_notification.md)で説明します。

[エンティティの更新](#update-entity)から知っていることに基づいて、次の4つの更新を順番に実行して、更新間隔を5秒以上 (スロットリングによる通知の損失を避けるため) にします :

-   Room1 温度を27に更新 : 温度がトリガ属性ではないため、何も起こりません
-   Room1 の気圧を765に更新 : Room1 の現在の値 (27) を通知します
-   Room1 の気圧を765に更新 : broker が賢明であるため、updateContext リクエストの前の値も765なので、実際の更新は行われず、したがって通知は送信されません
-   リクエスト URL に `?options=forcedUpdate` を追加し、Room1 の気圧を765に更新 :
    この場合、`forcedUpdate` URI オプションにより、Broker は通知を送信します。
    [ここ](ngsiv2_implementation_notes.md#forcedupdate-option) に
    `forcedUpdate` URI オプションの詳細が記述されています。
-   Room2 の気圧を740に更新 : サブスクリプションは Room1 で Room2 ではないため、何も起こりません

次に、スロットリングがどのように強制されているかを確認します。5秒経過することなく Room1 の気圧を速やかに更新すると、2番目の通知が accumulator-server.py に到着しないことがわかります。

サブスクリプションは、`GET /v2/subscriptions` (リスト全体を提供し、リストが大きすぎる場合はページネーションが必要) または `GET /v2/subscriptions/{subId}` (サブスクリプションを1つだけ取得する) を使用して取得できます。さらに、サブスクリプションは `PATCH /v2/subscription/{subId}` オペレーションを使用して更新することができます。最後に、`DELETE /v2/subscriptions/{subId}` オペレーションを使用して削除することができます。

いくつかの暫定的な考慮事項 :

* サブスクリプションを一時停止することができます。これを行うには、`status` 属性を "inactive" に設定するだけです (サブスクリプションを再開するには、"active" に戻します) :
```
curl localhost:1026/v2/subscriptions/57458eb60962ef754e7c0998 -s -S \
    -X PATCH -H 'Content-Type: application/json' -d @- <<EOF
{
  "status": "inactive"
}
EOF
```

* 通知はいくつかの方法でカスタマイズできます。まず、`notification` の中の `attrsFormat` フィールド内のフィールドを使用して、通知のエンティティ表現形式を調整できます。次に、カスタム通知 HTTP verb (PUT など)、カスタム HTTP ヘッダ、カスタム URL クエリパラメータ、カスタムペイロード (必ずしも JSON ではなく) を使用できます。[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Notification Messages" と "Custom Notifications" を見て ください。

[トップ](#top)

<a name="browsing-all-types-and-detailed-information-on-a-type"></a>
### すべての型と型に関する詳細情報のブラウズ

Orion Context Broker に存在するすべてのエンティティ型のリストを取得するには、次のオペレーションを使用できます。たとえば、Room 型の3つのエンティティと、Car 型の2つのエンティティがあるとします :

```
curl localhost:1026/v2/types -s -S -H 'Accept: application/json' | python -mjson.tool
```

レスポンスは次のようになります :

```
[
    {
        "attrs": {
            "fuel": {
                "types": [
                    "Percentage"
                ]
            },
            "speed": {
                "types": [
                    "Float"
                ]
            }
        },
        "count": 2,
        "type": "Car"
    },
    {
        "attrs": {
            "pressure": {
                "types": [
                    "Integer"
                ]
            },
            "temperature": {
                "types": [
                    "Float"
                ]
            }
        },
        "count": 3,
        "type": "Room"
    }
]
```

ご覧のとおり、各型の属性情報が提供されています。いくつかの重要な意見 :

-   NGSI が与えられた型のすべてのエンティティに同じ属性のセット (すなわち、同じ型のエンティティは、異なる属性セットを有することができる) を強制しないとすれば、このオペレーションによって返される型ごとの属性セットは、その型に属する各エンティティの属性セットの和集合です
-   さらに、異なるエンティティで同じものを持つ属性は、異なる型を持つことがあります。したがって、各属性に関連付けられた `types` フィールドはリストです

エンティティ型のリストのみが必要な場合 (追加の属性の詳細なし)、次を使用できます :

```
curl localhost:1026/v2/types?options=values -s -S -H 'Accept: application/json' | python -mjson.tool
```

取得する :

```
[
    "Car",
    "Room"
]

```
[ページネーション・メカニズム](pagination.md#pagination)は、上記で述べた `GET /v2/types` オペレーションでも機能することに注意してください。

さらに、次のオペレーションを使用して、単一の型の詳細情報を取得することもできます :

```
curl localhost:1026/v2/types/Room -s -S -H 'Accept: application/json' | python -mjson.tool
```

レスポンスは次のようになります :

```
{
    "attrs": {
        "pressure": {
            "types": [
                "Integer"
            ]
        },
        "temperature": {
            "types": [
                "Float"
            ]
        }
    },
    "count": 3
}
```

[トップ](#top)

<a name="batch-operations"></a>
### バッチ処理

これまでに説明したエンティティを管理するための RESTful なオペレーションとは別に、NGSIv2 は場合によっては有用な "バッチ" オペレーションも含みます。特に、バッチ更新オペレーション (`POST /v2/op/update`) とバッチクエリオペレーション (`POST /v2/op/query`) があります。

バッチ更新では、1回のオペレーションで複数のエンティティを作成または更新できます。たとえば、Room3 (温度 21.2 と気圧722) と Room4 (温度 31.8 と気圧712) を作成するには、次のものを使用します :

```
curl -v localhost:1026/v2/op/update -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "actionType": "append",
  "entities": [
    {
      "type": "Room",
      "id": "Room3",
      "temperature": {
        "value": 21.2,
        "type": "Float"
      },
      "pressure": {
        "value": 722,
        "type": "Integer"
      }
    },
    {
      "type": "Room",
      "id": "Room4",
      "temperature": {
        "value": 31.8,
        "type": "Float"
      },
      "pressure": {
        "value": 712,
        "type": "Integer"
      }
    }
  ]
}
EOF
```

レスポンスは HTTP レスポンス・コード 204 No Content を使用します。ここでは、エンティティと属性を追加するための `append` `actionType` を使用しています。また、`update` を使用して、あるエンティティ (Room3 の温度) と他の属性 (Room4 の気圧) を変更し、他の属性は変更しません。

```
curl -v localhost:1026/v2/op/update -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "actionType": "update",
  "entities": [
    {
      "type": "Room",
      "id": "Room3",
      "temperature": {
        "value": 29.9,
        "type": "Float"
      }
    },
    {
      "type": "Room",
      "id": "Room4",
      "pressure": {
        "value": 709,
        "type": "Integer"
      }
    }
  ]
}
EOF
```

`append` と `update` 以外にも、`delete`, `appendStrict` などのアクション・タイプがあります 。詳細は [このセクション](update_action_types.md)を参照してください。

最後に、`POST /v2/op/query` は、ペイロードに指定されたクエリ条件に一致するエンティティを取得できます。これは `GET /v2/entities` に非常によく似ています (実際には、レスポンス・ペイロードは同じで、同じ方法でページネーションもサポートしています)。しかし `POST /v2/op/query` は、`GET /v2/entities` ができないクエリ (例えば、異なる型のエンティティのリスト) を表現することができます。

例えば、温度が 40より大きく、40.31, -3.75 の座標から 20km のところにある Room 型、または車のすべてのエンティティの属性温度と気圧を取得するには、次のオペレーションを使用できます :

```
curl -v localhost:1026/v2/op/query -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "entities": [
    {
      "idPattern": ".*",
      "type": "Room"
    },
    {
      "id": ".*",
      "type": "Car"
    }
  ],
  "attrs": [
    "temperature",
    "pressure"
  ],
  "expression": {
    "q": "temperature>40",
    "georel": "near;maxDistance:20000",
    "geometry": "point",
    "coords": "40,31,-3.75"
  }
}
EOF
```

[上](#top)

<a name="context-availability-management"></a>
## コンテキスト・アベイラビリティ管理

コンテキスト管理は *エンティティおよび属性* (作成、更新、取得など) に関するものですが、
コンテキスト・アベイラビリティ管理は、*エンティティと属性のソース* に関するものです。コンテキスト・アベイラビリティ管理の基本的な概念は、*レジストレーション* のリソースです。 レジストレーションには、"コンテキスト・プロバイダ" と名付けられている情報ソースと、そのソースによってエンティティおよび属性が提供されます。

簡単なレジストレーションを作成する例を示してみましょう。 Room5 の温度と気圧の属性は、URL http://mysensors.com/Rooms のコンテキスト・プロバイダによって提供されています :

```
curl -v localhost:1026/v2/registrations -s -S -H 'Content-Type: application/json' -d @-  <<EOF
{
  "description": "Registration for Room5",
  "dataProvided": {
    "entities": [
      {
        "id": "Room5",
        "type": "Room"
      }
    ],
    "attrs": [
      "temperature",
      "pressure"
    ]
  },
  "provider": {
    "http": {
      "url": "http://mysensors.com/Rooms"
    },
    "legacyForwarding": true
  }
}
EOF
```

そのリクエストに対応するレスポンスは、201 HTTP レスポンス・コードとして作成されたものを使用します。さらに、それはレジストレーション ID を保持する、`Location header` を含んでいます : レジストレーションを更新し、削除するのに使用される24桁の16進数です。このチュートリアルの後半で必要になるので、書き留めておいてください。

```
< HTTP/1.1 201 Created
< Connection: Keep-Alive
< Content-Length: 0
< Location: /v2/registrations/5a82be3d093af1b94ac0f730
< Fiware-Correlator: e4f0f334-10a8-11e8-ab6e-000c29173617
< Date: Tue, 13 Feb 2018 10:30:21 GMT
```

次のレジストレーションを使用して、既存レジストレーションのリストを取得することができます :

```
curl localhost:1026/v2/registrations -s -S -H 'Accept: application/json' | python -mjson.tool
```

この特定のケースでは、以前に1つしか作成していないので、1つだけを取得しますが、レスポンスには `[...]` が使用されるため、完全なリストが存在することに注意してください。

さらに、次のリクエストを使用して単一のレジストレーションを取得することもできます。実際のレジストレーション ID で `5a82be3d093af1b94ac0f730` を置き換えてください :

```
curl localhost:1026/v2/registrations/5a82be3d093af1b94ac0f730 -s -S -H 'Accept: application/json' | python -mjson.tool
```
Orion は、エンティティ/属性のソースに関する情報をレジストレーションの形で保存するだけでなく、これらのレジストレーションを使用してクエリ/更新の転送を実装します。 つまり、レジストレーションが行われている間、Orion はその情報を使用して、ローカルで管理されていないエンティティのクエリ/更新を解決します。ただし、これはこのチュートリアルの範囲外の高度なトピックです。 詳細については、[コンテキスト・プロバイダと転送のドキュメント](context_providers.md)をご覧ください。

最後に、次のリクエストで既存のレジストレーションを削除することができます。実際のレジストレーション ID で `5a82be3d093af1b94ac0f730` を置き換えてください :

```
curl -X DELETE localhost:1026/v2/registrations/5a82be3d093af1b94ac0f730 -s -S -H 'Accept: application/json' | python -mjson.tool
```

[トップ](#top)
