# <a name="top"></a>Metrics API

* [Introduction](#introduction)
* [Operations](#operations)
    * [Get metrics](#get-metrics)
    * [Reset metrics](#reset-metrics)
    * [Get and reset](#get-and-reset)
* [Metrics](#metrics)

## Introduction

Orion implements a REST-based API that can be used to get relevant operational metrics. This
API is a complement to the [statistics API](statistics.md) which is more low-level and aimed at
debugging.

Note that this API can be switched off to avoid overhead (gathering of metrics is a bit costly,
as system calls and semaphores are involved) using the `-disableMetrics` [CLI parameter](cli.md).

[Top](#top)

## Operations

#### Get metrics

```
GET /admin/metrics
```

The response payload is a multi-level JSON tree storing the information in an structured way. It is
based on the [service](../user/multitenancy.md) and [subservice](../user/service_path.md) (sometimes
refered to as "service path"). At any point of the tree, the value of a key could be `{}` to mean that
there isn't actual information associated to that key.

At the first level there are two keys: **services** and **sum**. In sequence, **services** value is
an object whose keys are service names and whose values are objects with information about the corresponding
service. The **sum** value is an object with information for the aggregated information for all services.

```
{
  "services": {
    "service1": <service 1 info>,
    "service2": <service 2 info>,
    ...
    "serviceN": <service N info>
  }
  "sum": <aggregated info for all services>
}
```

Regarding service information objects, they use two keys: **subservs** and **sum**. In sequence, **subservs**
value is an object whose keys are subservice names and whose values are objects with information about
the corresponding subservice. The **sum** value is an object with information for the aggregated information
for all subservices in the given services.

```
{
  "subservs": {
    "subservice1": <subservice 1 info>,
    "subservice2": <subservice 2 info>,
    ...
    "subserviceN": <subservice N info>
  }
  "sum": <aggregated info for all subservice in the given service>
}
```

Subservice names in the above structure are shown without the initial slash. E.g. if the subservice
name is (as used in the `Fiware-ServicePath` header) `/gardens` then the key used for it would be
`gardens` (without `/`). Others slashes apart from the initial one are not removed, e.g. `/gardens/north`
uses the key `gardens/north`.

Regarding subservice information object, keys are the name of the different metrics.

```
{
  "metric1": <metric 1>,
  "metric2": <metric 2>,
  ...
  "metricN": <metric N>
}
```

The list of metrics is provided in [metrics section](#metrics).

Some additional remarks:

* Requests corresponding to invalid services or subservices (i.e. the ones that don't follow the syntax rules
  described [here](../user/multitenancy.md) and [here](../user/service_path.md)) are not included in the
  payload (i.e. their associated metrics are just ignored).
* The default service uses **default-service** as service key. Note that it cannot collide with
  regular services as the `-` character is not allowed in regular services.
* The root subservice (`/`) uses **root-subserv** as subservice key. Note that it cannot collide
  with regular subservices as the `-` character is not allowed in regular subservices.
* Requests using an enumeration of subservices (e.g. `Fiware-ServicePath: /A, /B`) are associated to the
  first element of the list, i.e. `A`.
* Requests using "recursive subservice" (e.g. `Fiware-ServicePath: /A/#`) are associated to the subservice
  without considering recursivity, i.e. `A`.

[Top](#top)

### Reset metrics

```
DELETE /admin/metrics
```

This operation resets all metrics, as if Orion would had just been started.

[Top](#top)

### Get and reset

```
GET /admin/metrics?reset=true
```

This operation (in fact, a variant of [get metrics](#get-metrics)) get results and, at the same time
in an atomical way, resets metrics.

[Top](#top)

## Metrics

* **incomingTransactions**: number of requests consumed by Orion. All kind of transactions
  (no matter if they are ok transactions or error transactions) count for this metric.
* **incomingTransactionRequestSize**: total size (bytes) in requests associated to incoming transactions
  ("in" from the point of view of Orion). All kind of transactions (no matter if they are ok transactions
  or error transactions) count for this metric.
* **incomingTransactionResponseSize**: total size (bytes) in responses associated to incoming transactions
  ("out" from the point of view of Orion). All kind of transactions (no matter if they are ok transactions
  or error transactions) count for this metric.
* **incomingTransactionErrors**: number of incoming transactions resulting in error.
* **serviceTime**: average time to serve a transaction. All kind of transactions (no matter if they are ok
  transactions or error transactions) count for this metric.
* **outgoingTransactions**: number of requests sent by Orion (both notifications and forward requests to CPrs).
  All kind of transactions (no matter if they are ok transactions or error transactions) count for this metric.
* **outgoingTransactionRequestSize**: total size (bytes) in requests associated to outgoing transactions
  ("out" from the point of view of Orion). All kind of transactions (no matter if they are ok transactions
  or error transactions) count for this metric.
* **outgoingTransactionResponseSize**: total size (bytes) in responses associated to outgoing transactions
  ("in" from the point of view of Orion). All kind of transactions (no matter if they are ok transactions
  or error transactions) count for this metric.
* **outgoingTransactionErrors**: number of outgoing transactions resulting in error.

[Top](#top)
