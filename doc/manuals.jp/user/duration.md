# レジストレーションとサブスクライブのための期間 (Duration for registration and subscriptions)

## デフォルト期間

registerContext, subscribeContext または subscribeContextAvailability で期間 (duration) を指定しない場合、デフォルトの24時間が使用されます。これらのケースでは、レスポンスの中で、例えば registerContext のための期間の確認を得るでしょう :

      {
      "duration": "PT24H",
         "registrationId": "52f38a64261c371af12b8565"
      }

## 期間の延長

レジストレーションとサブスクリプション (コンテキストとコンテキスト・アベイラビリティの両方に) は期間があることがわかりました。有効期限は、次の式を使用して計算されます :

-   有効期限 (expriration) = 現在の時間 (current-time) + 期間 (duration) :

有効期限が切れた要素に関する broker の動作は次のとおりです :

-   レジストレーションの場合 : 期限切れレジストレーションは、discoverContextAvailability リクエスト処理では考慮されませんが、[更新可能](../admin/database_admin.md#updating-registrations)です。
-   サブスクリプション : 期限切れサブスクリプションは、それに基づいて新しい通知を送信するために考慮されませんが、updateContextSubscription/updateContextSubscriptionAvailability を使用して更新可能であり、unsubscribeContext/unsubscribeContextAvailability を使用してキャンセルすることができます。

Orion Context Broker は、期限切れの要素をデータベースから削除しませんが、[管理マニュアルに記載](../admin/database_admin.md#deleting-expired-documents)されているとおりにそれらを簡単に削除することができます。

最後に、期限は *拡張 (expanded)* ではなく更新で *再計算 (recalculated)* されることを考慮してください。例を挙げてこれを明確にしましょう。18時30分に PT1H 期間 (つまり1時間) のサブスクリプションをしたとしましょう。したがって、19:30 に有効期限が切れます。次に、19:00 に PT1H を再度使用して更新を行います。その時間帯は 19:30 (前の有効期限) に追加されず、19:00 (現在の時刻) に追加されます。したがって、新しい有効期限は 20:00 です。
