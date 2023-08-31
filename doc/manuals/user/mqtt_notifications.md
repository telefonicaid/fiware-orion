## Introduction

Apart from HTTP notifications, Orion is able to notify using MQTT. In this case, a MQTT message
is published in a given MQTT broker specified at subscription time each time a notification
is triggered.

![](mqtt-notifications.png "mqtt-notifications.png")

From an operational point of view, MQTT subscriptions are like HTTP ones,
as described in [this section of the documentation](walkthrough_apiv2.md#subscriptions) and in the
[Orion API specification](../orion-api.md) (e.g. the notification
payload is the same, you can set an expiration date, a filtering expression, etc.) but they use `mqtt`
instead of `http` in the `notification` object.

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

The following elements can be used within `mqtt`:

* `url` to specify the MQTT broker endpoint to use. URL must start with `mqtt://` and never contains
  a path (i.e. it only includes host and port)
* `topic` to specify the MQTT topic to use
* `qos`: to specify the MQTT QoS value to use in the notifications associated to the subscription
  (0, 1 or 2). This is an optional field, if omitted then QoS 0 is used.
* `retain`: to specify the MQTT retain value to use in the notifications associated to the subscription
  (`true` or `false`). This is an optional field, if omitted then retain `false` is used.
* `user` and `passwd`: optional fields, to be used in the case MQTT broker needs user/password based
  authentication. If used, both fields have to be used together. Note that for security reasons,
  the password is always offuscated when retrieving subscription information (e.g. `GET /v2/subscriptions`).

Another difference between MQTT and HTTP subscriptions in that the former don't include the following
fields:

* `lastSuccessCode`. There is no equivalence to HTTP response codes in MQTT case
* `lastFailureReason`. The only failure reason that Orion is able to detect is connection fail to the
  corresponding MQTT broker. Thus, there is no need of providing extra detail.

However, note that `lastSuccess` and `lastFailure` fields (which specify the timestamp of the last
success/failure) are supported in MQTT subscriptions in the same way than in HTTP subscriptions.

## Custom notifications

Custom notifications (described in [this section of the Orion API specification](../orion-api.md#custom-notifications))
in MQTT subscriptions work the same as in HTTP subscriptions, taking into account the following:

* `mqttCustom` is used instead of `httpCustom`
* The same fields used in `mqtt` can be used in `mqttCustom`.
* `headers`, `qs` and `method`cannot be used, as they doesn’t have equivalence in MQTT
* Macro replacement is performed in `topic` and `payload` fields. `url`, `qos`, `retain`, `user` and `passwd` are fixed values

## Connection management

The endpoint of the MQTT broker associated to a subscription is specified in the `url` field at subscription time,
but the connection to it is done first time a MQTT notification is published. 

Once established, connection is kept opened while it is being used, i.e. MQTT notifications are published. If
a connections is not used (i.e. no MQTT is published) Orion will close it after a predefined keepalive time
(specified with the `-mqttMaxAge` [CLI parameter](../admin/cli.md), one hour by default). Orion also closes
connection in case of MQTT notification errors (e.g. MQTT broker is down), so it will be re-created next
time a successfully MQTT notification gets published.

## MQTT cheatsheet

The following commands can be useful to test and debug MQTT notifications (using [mosquitto_sub](https://mosquitto.org/man/mosquitto_sub-1.html)
and [mosquito_pub](https://mosquitto.org/man/mosquitto_pub-1.html)).

To subscribe with QoS 2:

```
mosquitto_sub --disable-clean-session --id 1 -q 2 -d -h <host> -p 1883 -t '#'
```

To create a shared subscription (cluster name “g1”)

```
mosquitto_sub -h <host> -p 1883 -t '$share/g1/#'
```

To publish using TLS (not yet supported by Orion, pending on [this issue](https://github.com/telefonicaid/fiware-orion/issues/3915)):

```
mosquitto_pub -d --insecure --cafile file.pem -h <host> -p 1883 -u <username> -P <password> -t '/topic' -m 'payload'
```


To clear all mosquitto broker retained messages:

```
sudo service mosquitto stop
sudo rm /var/lib/mosquitto/mosquitto.db
sudo systemctl start mosquitto.service
```
