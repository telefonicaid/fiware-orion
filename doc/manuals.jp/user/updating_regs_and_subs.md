# レジストレーションの更新 (Updating registrations)

コンテキストのレジストレーション・リクエスト ([標準](walkthrough_apiv1.md#register_context_operation)と[コンビニエンス](walkthrough_apiv1.md#convenience_context_operation)の両方) に対するレスポンスには、レジストレーション ID (24桁の16進数) が含まれています :

``` 
{
    "duration": "PT24H",
    "registrationId": "51bf1e0ada053170df590f20"
}
``` 

この ID を使用してレジストレーションを更新することができます。レジストレーションを更新する特別なオペレーションはありません (この意味では、updateContextSubscription オペレーションと updateContextAvailabilitySubscription オペレーションを持つコンテキスト・サブスクリプションとコンテキスト・アベイラビリティ・サブスクリプションとは異なります)。*registerId* を設定して、新しい registerContextRequest を発行して更新を行います :

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
                    "id": "Room8"
                }
            ],
            "attributes": [
                {
                    "name": "humidity",
                    "type": "percentage",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M",
    "registrationId": "51bf1e0ada053170df590f20"
}
EOF
```
この "update registration" は、その ID に関連付けられた既存のレジストレーションを、[期限切れ再計算](duration.md#extending-duration)を含む新しい内容で置き換えます。

驚いたことに、NGSI にレジストレーションを取り消す方法はありません。この問題を回避するには、存在しないエンティティと期間 (duration) 0で更新してください。実際の削除を行うには、データベースからレジストレーションを削除する必要があります。[データベースの管理についての管理マニュアル](../admin/database_admin.md#database-administration)を参照してください。

# サブスクリプションの更新

このドキュメントでは、[コンテキスト・サブスクリプション](walkthrough_apiv1.md#register-context-operation)と[コンテキスト・アベイラビリティ・サブスクリプション](walkthrough_apiv1.md#convenience-register-context)を更新できることを以前に見てきました。しかし、registerContext とは異なり、すべてを更新することはできません。サブスクリプションの種類によって、これを詳しく見ていきましょう。

## コンテキスト・サブスクリプションで更新できるものは何ですか？

updateContextSubscription のペイロードは、subscribeContext リクエストのペイロードに似ています。ただし、すべてを更新することはできないため、すべてのフィールドを含めることはできません。特に、次のフィールドは更新できません :

-   subscriptionId (サブスクリプションを参照するには updateContextSubscription に含める必要があります)
-   entities 
-   attributes
-   reference

ただし、次のフィールドは変更できます :

-   notifyConditions
-   throttling
-   duration
-   restriction

## コンテキスト・アベイラビリティ・サブスクリプションで更新できるものは何ですか？

updateContextAvailabilitySubscription によって使用されるペイロードは、subscribeContextAvailability リクエストのものとかなり類似しています。ただし、すべてが更新可能であるわけではないので、すべてのフィールドを含めることはできません。特に、以下は更新可能ではありません :

-   subscriptionId (サブスクリプションを参照するには updateContextSubscription に含める必要があります)
-   reference

したがって、以下が更新可能です :

-   entities (実際には、すべての updateContextAvailabilitySubscription で必須です)
-   attributes
-   duration
