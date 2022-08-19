# Oneshot サブスクリプション

Oneshot サブスクリプションは、一度の通知のためだけにエンティティを
サブスクライブするオプションを提供します。コンシューマがステータス
"oneshot" のサブスクリプションを作成すると、
[通常のサブスクリプション](walkthrough_apiv2.md#subscriptions)・
リクエストと同じようにわずかな差異でサブスクリプションが作成されます。

通常のケースでは、サブスクリプションが削除されるか、サブスクリプションの
更新後にそのステータスが非アクティブになるまで、エンティティが更新されると、
コンシューマは初期通知および継続的な通知を受け取ります。

ワンショット・サブスクリプションの場合、サブスクリプションの作成後に
エンティティが更新されたときに、コンシューマに通知されるのは1回だけです。
通知がトリガーされると、サブスクリプションは "status": "inactive"
に移行します。この状態になると、コンシューマは "oneshot" でステータスを
更新して、同じ動作を繰り返すことができます。
すなわち、1回限りの通知を再度得ることができます。

![](../../manuals/user/oneshot_subscription.png "oneshot_subscription.png")

* Room1 という ID で Room 型のエンティティがすでにデータベースに存在している
  と仮定します。

コンテキスト・コンシューマは、以下のように status "oneshot"
を持つそのエンティティのサブスクリプションを作成できます :

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
  "status" : "oneshot"
}
EOF
```

気圧の属性の値が更新されると、コンテキスト・コンシューマは温度属性の通知を
受け取り、このサブスクリプションのステータスは自動的に非アクティブになり、
コンシューマが次のようにして再び "oneshot" に更新するまで、
それ以上の通知はトリガされません :

```
curl localhost:1026/v2/subscriptions/<subscription_id> -s -S \
    -X PATCH -H 'Content-Type: application/json' -d @- <<EOF
{
  "status": "oneshot"
}
EOF
```

ステータスが再び "oneshot" に更新されると、エンティティが更新されたときに、
コンシューマは再度通知を受け取り、サブスクリプション・ステータスは自動的に
"inactive" に変更されます。
