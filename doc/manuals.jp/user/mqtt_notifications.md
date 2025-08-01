## イントロダクション

HTTP 通知とは別に、Orion は MQTT を使用して通知できます。この場合、MQTT メッセージは、通知がトリガーされるたびに
サブスクリプション時に指定された特定の MQTT broker で公開されます。

![](mqtt-notifications.png "mqtt-notifications.png")

[ドキュメントのこのセクション](walkthrough_apiv2.md#subscriptions) および[Orion API 仕様](../orion-api.md)
で説明されているように、運用の観点からは、MQTT サブスクリプションは HTTP サブスクリプションに似ています (たとえば、通知ペイロードは同じです。
有効期限やフィルタリング式などを設定できます) が、`notification` オブジェクトで `http` の代わりに `mqtt` を使用します。

```
...
"notification": {
  "mqtt": {
    "url": "mqtt://<mqtt_host>:1883",
    "topic": "sub1"
  }
}
...
```

次の要素は `mqtt` 内で使用できます:

* 使用する MQTT broker エンドポイントを指定するための `url`。URL は `mqtt://` で始まる必要があり、
  パスを含めることはできません (つまり、ホストとポートのみが含まれます)
* `topic` に、使用する MQTT トピックを指定します
* `qos`: サブスクリプションに関連付けられた通知 (0, 1 または 2) で使用する MQTTQoS 値を指定します。
   これはオプションのフィールドです。省略した場合、QoS0 が使用されます
* `retain`: サブスクリプションに関連付けられた通知で使用する MQTT 保持値を指定します (`true` または `false`)。これはオプションのフィールドで、省略した場合は retain には `false` が使用されます
* `user` および `passwd`: MQTT broker がユーザ/パスワードベースの認証を必要とする場合に使用される
  オプションのフィールド。使用する場合は、両方のフィールドを一緒に使用する必要があります。セキュリティ上の理由から、
  サブスクリプション情報を取得するときは常にパスワードが使用されないことに注意してください (例: `GET /v2/subscriptions`)

MQTT サブスクリプションと HTTP サブスクリプションのもう1つの違いは、前者には次のフィールドが含まれていないことです: 

* `lastSuccessCode`。MQTT の場合、HTTP レスポンス・コードと同等のものはありません
* `lastSuccessCode`。Orion が検出できる唯一の失敗の理由は、対応する MQTT ブローカーへの接続の失敗です。
  したがって、追加の詳細を提供する必要はありません

ただし、`lastSuccess` フィールドと `lastFailure` フィールド (最後の成功/失敗のタイムスタンプを指定) は、
HTTP サブスクリプションと同じ方法で MQTT サブスクリプションでサポートされることに注意してください。

## カスタム通知

MQTT サブスクリプションのカスタム通知 ([Orion API 仕様のカスタム通知のセクション](../orion-api.md#custom-notifications) で説明) は、
次の点を考慮して、HTTP サブスクリプションと同じように機能します

* `httpCustom` の代わりに `mqttCustom` が使用されます
* `mqtt` で使用されているのと同じフィールドを `mqttCustom` で使用できます
* `headers`, `qs` と `method` は MQTT で同等ではないため、使用できません
* マクロ置換は `topic` および `payload` フィールドで実行されます。`url`, `qos`, `retain`, `user` と `passwd` は固定値です

## 接続管理

サブスクリプションに関連付けられた MQTT broker のエンドポイントは、サブスクリプション時に `url` フィールドで指定されますが、
MQTT 通知が最初に公開されたときに接続が行われます。

接続が一度確立されると、使用中も開いたままになります。つまり、MQTT 通知が公開されます。接続が使用されていない場合 (つまり、
MQTT が公開されていない場合)、Orion は事前定義されたキープ・アライブ時間 (`-mqttMaxAge` [CLI パラメータ](../admin/cli.md)
で指定、デフォルトで1時間) 後に接続を閉じます。Orion は、MQTT 通知エラー (MQTT broker がダウンしているなど)
が発生した場合にも接続を閉じるため、次に MQTT 通知が正常に発行されたときに再作成されます。

## MQTT チートシート

次のコマンドは、MQTT 通知 ([mosquitto_sub](https://mosquitto.org/man/mosquitto_sub-1.html) および
[mosquito_pub](https://mosquitto.org/man/mosquitto_pub-1.html) を使用) をテストおよびデバッグするのに役立ちます。

QoS2 でサブスクライブするには:

```
mosquitto_sub --disable-clean-session --id 1 -q 2 -d -h <host> -p 1883 -t '#'
```

共有サブスクリプションを作成するには (クラスター名 "g1")

```
mosquitto_sub -h <host> -p 1883 -t '$share/g1/#'
```

TLS を使用して公開するには (Orion ではまだサポートされていません。
[この Issue](https://github.com/telefonicaid/fiware-orion/issues/3915) で保留中です):

```
mosquitto_pub -d --insecure --cafile file.pem -h <host> -p 1883 -u <username> -P <password> -t '/topic' -m 'payload'
```

Mosquitto Broker に保持されているすべてのメッセージをクリアするには:

```
sudo service mosquitto stop
sudo rm /var/lib/mosquitto/mosquitto.db
sudo systemctl start mosquitto.service
```
