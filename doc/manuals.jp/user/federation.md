# Context Broker のフェデレーション (Context Broker Federation)

このセクションでは、1つの Orion インスタンスによって送信された notifyContext が他の Orion インスタンスによって処理されるという意味で、"push" フェデレーションについて説明しました。しかしながら、[コンテキスト・プロバイダのレジストレーションおよびリクエスト転送](context_providers.md)機能を使用して、1つの Orion インスタンスが別の Orion インスタンスへのクエリ/更新を誘導する、ある種の "pull" フェデレーションを実装できます。2つのアプローチの違いは、"push" モードではすべての Orion インスタンスがローカル状態を更新し、"pull" アプローチではすべての中間の Orion インスタンスがデータをローカルに格納せずに "proxy" として機能することです。.

Orion Context Broker は、updateContext および registerContext (通常はクライアントアプリケーションによって発行される) の処理とは別に、notifyContextRequest および notifyContextAvailabilityRequest を同じセマンティクスで処理できます。これは、興味深いフェデレーション・シナリオへの扉を開きます (1つの例は [FIWARE Lab context management platform](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FIWARE_Lab_Context_Management_Platform)です)。

![](../../manuals/user/Federation.png "Federation.png")

## NGSIv2 ベースのフェデレーション

(これは基本的に以前のバージョンで説明したのと同じですが、古いバージョンの CB API を使用しています)

同じマシンで実行されている 3つの Context Broker のインスタンス (もちろん、これは必須ではありませんが、この機能をテストするのは簡単です) をポート 1030,1031, 1032 で実行し、A, B, C という名前のそれぞれ異なるデータベースを使用して説明します。各インスタンスを起動してみましょう。各コマンドは別々の端末で実行してください :

    contextBroker -fg -port 1030 -db orion1030
    contextBroker -fg -port 1031 -db orion1031
    contextBroker -fg -port 1032 -db orion1032

次に、B が A の更新をサブスクライブするよう、A でサブスクリプションの作成を送信してみましょう。参照で使用される URL は "/v2/op/notify" である必要があります :

```
curl localhost:1030/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "temperature"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1031/v2/op/notify"
    }
  }
}
EOF
```

次に、C が B の更新をサブスクライブするよう、B でサブスクリプションの作成を送信してみましょう
。サブスクリプションは基本的に同じですが、curl と リファレンス要素のポートだけが異なります。

```
curl localhost:1031/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "temperature"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1032/v2/op/notify"
    }
  }
}
EOF
```

ここで、Context Broker A でエンティティを作成しましょう。

```
curl localhost:1030/v2/entities -s -S -H'Content-Type: application/json' -d @- <<EOF
{
  "id": "Room1",
  "type": "Room",
  "temperature": {
    "value": 23,
    "type": "Number"
  }
}
EOF
```

サブスクリプションが適切に設定されていると、A から B への通知が自動的に送信されます。B ではそのイベントは、C に通知が送信されます。最後に、A でエンティティを作成すると、同じ属性値を持つエンティティが C に追加されます。C にクエリすることで、これを確認できます :

```
curl localhost:1032/v2/entities -s -S H-header 'Accept: application/json' -d @- | python -mjson.tool
```

そのレスポンスは :

```
[
  {
    "id": "Room1",
    "type": "Room",
    "temperature": {
      "value": 23,
      "type": "Number"
    }
  }
]
```

通知リクエストのセマンティクスは、`POST /v2/entities?options=upsert` は同じです。したがって、エンティティが存在する場合は、更新されます。エンティティが存在しない場合は作成されます。したがって、フェデレーションは正確なミラーリングを提供しません : 最初の Context Broker でエンティティが削除されると、エンティティは 2番目の Context Broker で削除されません。

フェデレーションのサブスクリプションが完了すると、Orion Context Broker が [初期通知](initial_notification.md)を送信できることに注意してください。場合によっては、この初期通知は受信側の Context Broker によって処理不能になる可能性があります。特に、初期通知で合法的に許可されているよりも多くの要素がサービスパス・ヘッダに含まれている場合があります ([サービスパスに関するドキュメント](service_path.md)を参照してください)。したがって、`"too many service paths - a maximum of ten service paths is allowed"` エラーが発生します。ただし、この初期通知だけが無視されることに注意してください。定期的な通知にはこの問題がなく、受信した Context roker によって正しく処理されます。

## NGSIv1 ベースのフェデレーション

(これは基本的に以前のバージョンで説明したのと同じですが、古いバージョンの CB API を使用しています)

次の設定を考えてみましょう : 同じマシンス (もちろん、これは必須ではありませんが、この機能をテストするのは簡単です) 上でポート 1030,1031,1032 でそれぞれ異なるデータベース (簡潔に A,B,C とする) を使用している3つの Context Broker のインスタンスです。各インスタンスを起動してみましょう (各コマンドは別々の端末で実行してください) :

    contextBroker -fg -port 1030 -db orion1030
    contextBroker -fg -port 1031 -db orion1031
    contextBroker -fg -port 1032 -db orion1032

次に、A に subscribeContext を送信してみましょう (B が A で行われた更新をサブスクライブするようにします)。"reference" で使用される URL は "/v1/notifyContext" である必要があります :

```
(curl localhost:1030/v1/subscribeContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "reference": "http://localhost:1031/v1/notifyContext",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "temperature"
            ]
        }
    ],
    "throttling": "PT5S"
}
EOF
```

次に、subscribecontext を B に送りましょう(C が B の更新をサブスクライブするようにします)。サブスクリプションは基本的に同じですが、curl の行のポートと "releerenece" 要素だけが異なります。

```
(curl localhost:1031/v1/subscribeContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "reference": "http://localhost:1032/v1/notifyContext",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "temperature"
            ]
        }
    ],
    "throttling": "PT5S"
}
EOF
```

ここで、Context Broker A でエンティティを作成しましょう。

```
(curl localhost:1030/v1/updateContext -s -S --header 'Content-Type: application/json' \
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
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```

特定のサブスクリプションが適切に設定されていると、notifyContextRequest が A から B に自動的に送信されます。B のイベントでは、notifyContextRequest が C に送信されます。したがって、最後に A のエンティティを作成すると、エンティティ (同じ属性値を持つ) を C に入れます。これを確認するには、C に queryContext を実行します :

```
(curl localhost:1032/v1/queryContext -s -S --header 'Content-Type: application/json' \
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

そのレスポンスは :

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

現在の Context Broker のバージョンでは、[nofityContextRequest のセマンティクスは updateContext APPEND](walkthrough_apiv1.md#update-context-elements)と同じか、コンテキスト要素がすでに存在する場合は updateContext UPDATE のセマンティクスになります。したがって、フェデレーションは正確なミラーリングを提供しません : 1つの Context Broker に対する updateContext DELETE は、フェデレーションした Context Broker で同じ効果を生じません。

フェデレーション・サブスクリプションが完了すると、Orion Context Broker は [初期通知](initial_notification.md)を送信できることに注意してください。場合によっては、この初期通知は受信者の Context Broker によって処理不能になる可能性があります。特に、初期通知で合法的に許可されているよりも多くの要素がサービスパスヘッダに含まれている場合があり ([サービスパスに関するドキュメント](service_path.md)を参照)、`"too many service paths - a maximum of ten service paths is allowed"` エラーが発生します。ただし、この初期通知のみが無視され、その後の通常の通知でこの問題が発生せず、受信した Context Broker によって正しく処理されることに注意してください。

このメカニズムは、registerContext および subscribeContextAvailability と同様に機能します。この場合、"reference" 要素の URL は "/v1/registry/notifyContextAvailability" です。
