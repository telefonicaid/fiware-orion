## Introduction

Orion can send notifications via Kafka. In this case, each time a notification is triggered, a KAFKA message is published
to a specific KAFKA broker or KAFKA cluster specified at the time of subscription.

![](kafka-notifications.png "kafka-notifications.png")

From an operational point of view, KAFKA subscriptions are like HTTP ones,
as described in [this section of the documentation](walkthrough_apiv2.md#subscriptions) and in the
[Orion API specification](../orion-api.md) (e.g. the notification
payload is the same, you can set an expiration date, a filtering expression, etc.) but they use `kafka`
instead of `http` in the `notification` object.

```
...
"notification": {
  "kafka": {
    "url": "kafka://broker1:9092,broker2:9092,broker3:9092",
    "topic": "sub1"
  }
}
...
```

The following elements can be used within `kafka`:

* `url` to specify the KAFKA broker endpoint to use. URL must start with `kafka://` and never contains
  a path (i.e. it only includes host and port)
* `topic` to specify the KAFKA topic to use


Another difference between KAFKA and HTTP subscriptions in that the former don't include the following
fields:

* `lastSuccessCode`. There is no equivalence to HTTP response codes in KAFKA case
* `lastFailureReason`. The only failure reason that Orion is able to detect is connection fail to the
  corresponding KAFKA cluster. Thus, there is no need of providing extra detail.

However, note that `lastSuccess` and `lastFailure` fields (which specify the timestamp of the last
success/failure) are supported in kafka subscriptions in the same way than in HTTP subscriptions.

## Custom notifications

Custom notifications (described in [this section of the Orion API specification](../orion-api.md#custom-notifications))
in KAFKA subscriptions work the same as in HTTP subscriptions, taking into account the following:

* `kafkaCustom` is used instead of `httpCustom`
* The same fields used in `kafka` can be used in `kafkaCustom`.
* `qs` and `method`cannot be used, as they doesn’t have equivalence in Kafka
* Macro replacement is performed in `topic`, `payload`, `json` and `ngsi` fields. `url` is a fixed value.

## Connection management

The connection is established the first time a notification is published to a Kafka cluster.

Kafka is designed for efficiency with persistent connections (long-lived, reused).

- Every time you destroy and recreate a producer (rd_kafka_t):
- Resynchronizes cluster metadata (queries partitions, brokers, etc.).
- Re-handshakes with brokers (authentication, new TCP connections).
- Loss of internal buffers (the producer loses its optimized state).
- Performance impact: additional latency on publications.

Kafka producers have the configuration parameter `connections.max.idle.ms` to close inactive sockets (without destroying the producer).
This releases network resources without destroying the producer. If the producer needs to send a message after the socket is closed, it automatically opens a new connection.
Kafka does not “reconnect” the same closed TCP connection; it creates a new one (to the same broker).
However, the producer object (rd_kafka_t) remains the same, keeping its configuration and internal state. It does not free the producer’s memory.



