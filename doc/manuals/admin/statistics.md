## Statistics

**Warning**: This section of the manual is yet work in progress, so the current Orion implementation
may not be fully aligned with it. In addition, take into account that statistics API is by the moment
in beta status, so changes in the name of the different counters or the JSON structure could take
place in the future.

Orion Context Broker provides a set of statistics (mainly for testing and debugging) through the
`GET /statistics` and `GET /cache/statistics` operations. These operations support JSON encoding only.

For now, statistics accuracy is not 100% guaranteed under high load conditions. Atomic
counters are not used in all places, so race conditions on requests running at the same time could
(theoretically) lead to losing some data. However, under low load conditions, statistics should
be accurate. There is [an open issue about this](https://github.com/telefonicaid/fiware-orion/issues/1504).

## GET /statistics

The statistics JSON is composed of four conditional blocks and two unconditional fields:

```
{
  "counters" : { ... },
  "semWait" : { ... },
  "timing" : { ... },
  "notifQueue": { ... },
  "uptime_in_secs" : 65697,
  "measuring_interval_in_secs" : 65697
}
```

Conditional blocks are enabled using `-statXXX` flags at command line (in the future, we may
implement the possibility to activate/deactivate them using the [management API](management_api.md).
This is due to two reasons. First, to avoid too verbose statistics output for users that are not interested
in some parts. Second, measuring some of the information might involve a performance penalty
(in general, to measure time always needs additional system calls) so explicit activation is desirable.

* "counters" (enabled with the `-statCounters`)
* "semWait" (enabled with the `-statSemWait`)
* "timing" (enabled with the `-statTiming`)
* "notifQueue" (enabled with the `-statNotifQueue`)

Unconditional fields are:

* `uptime_in_secs`, Orion uptime in seconds.
* `measuring_interval_in_secs`, statistics measuring time in seconds. It is set to 0 each time statistics
  are reset. If statistics have not been reset since Orion start, this field matches `uptime_in_secs`.

### Counter block

The counter block provides information about counters for the times a particular request type has been received
(also for notifications being sent), e.g:

```
{
  ...
  "counters": {
    "deprecatedFeatures": {
      "geoFormat": 2,
      "ngsiv1Forwarding": 4,
      "ngsiv1Requests": 4
    },
    "invalidRequests": 2,
    "jsonRequests": 4,
    "noPayloadRequests": 250,
    "notificationsSent": 4,
    "requests": {
      "/v2/entities": {
        "GET": 231
      },
      "/v2/entities/{id}": {
        "GET": 2,
        "PATCH": 1,
        "POST": 3
      },
      "/v2/types": {
        "GET": 1
      },
      "/admin/metrics": {
        "GET": 2
      },
      "/statistics": {
        "GET": 9
      },
      "/v2/subscriptions/{id}": {
        "GET": 1
      },
      "/v2/subscriptions/{id}": {
        "GET": 2
      }
    }
  },
  ...
}
```

If a particular request URL (or verb) has not been received, its corresponding counter is not shown, except
if the `fullCounters` option is used (i.e. `GET /statistics?options=fullCounters`).

### SemWait block

The SemWait block provides accumulates waiting time for the main internal semaphores. It can be useful to detect bottlenecks, e.g.
if your DB is too slow or your DB pool is undersized, then `dbConnectionPool` time would be abnormally high
(see section on [performance tunning](perf_tuning.md) for more details).

```
{
  ...
  "semWait" : {
    "request" : 0.000000000,
    "dbConnectionPool" : 2.917002794,
    "transaction" : 0.567478849,
    "subCache" : 0.784979145,
    "metrics": 0.000000000,
    "connectionContext" : 0.000000000,
    "timeStat" : 0.124000605
  },
  ...
}
```

### Timing block

Provides timing information, i.e. the time that CB passes executing in different internal modules.

```
{
  ...
  "timing": {
    "accumulated": {
      "jsonV1Parse": 7.860908311,
      "jsonV2Parse": 120.680244446,
      "mongoBackend": 12778.52734375,
      "mongoReadWait": 7532.301757812,
      "mongoWriteWait": 3619.282226562,
      "mongoCommandWait": 0.120559767,
      "exprJexlCtxBld": 27.092681885,
      "exprJexlEval": 124.217208862,
      "render": 44.540554047,
      "total": 25051.384765625
    },
    "last": {
      "mongoBackend": 0.003775352,
      "mongoReadWait": 0.0013743,
      "render": 0.000286864,
      "total": 0.00440685
    }
  }
  ...
}
```

The block includes two main sections:

* `last`: times corresponding to the last request processed. If the last request didn't use a particular module
  (e.g. a GET request doesn't use parsing), then that counter is 0 and it is not shown.
* `accumulated`: accumulated time corresponding to all requests since the broker was started.

The particular counters are as follows:

* `total`: processing time for the whole request, excluding the time that the HTTP library
  takes for request/response dispatching (pseudo end-to-end time)
* `jsonV1Parse`: time passed in NGSIv1 JSON parsing module (pseudo self-time)
* `jsonV2Parse`: time passed in NGSIv2 JSON parsing module (pseudo self-time)
* `mongoBackend`: time passed in mongoBackend module (pseduo self-time)
* `render`: time passed in rendering module (pseudo self-time)
* `mongo*Wait`: time passed waiting for MongoDB for `Read`, `Write` or `Cmd` operations. Note that if
  a given request involves several read/write/cmd calls to MongoDB, the time shown in `mongo*Wait` under
  `last` includes the accumulation for all of them. In the case of mongoReadWait, only the time used
  to get the results cursor is taken into account, but not the time to process cursors results (which
  is time that belongs to mongoBackend counters).
* `exprJexlCtxBld`: time passed building context for custom notification expression evaluation (see [macro substitution](../orion-api.md#macro-substitution) and [JEXL support](../orion-api.md#jexl-support))
* `exprJexlEval`: time passed evaluating custom notification expressions (see [macro substitution](../orion-api.md#macro-substitution) and [JEXL support](../orion-api.md#jexl-support))

*NOTE*: if Orion binary is build without using cjexl and only basic replacement is available, then `exprBasicCtxtBld` and `exprBasicEval`
fields appear instead of `exprJexlCtxBld` and `exprJexlEval`.

Times are measured from the point in time in which a particular thread request starts using a module until it finishes using it.
Thus, if the thread is stopped for some reason (e.g. the kernel decides to give priority to another thread based on its
scheculing policy) the time that the thread was sleeping, waiting to execute again is included in the measurement and thus, the measurement is not accurate. That is why we say *pseudo* self/end-to-end time. However,
under low load conditions this situation is not expected to have a significant impact.

### NotifQueue block

Provides information related to the notification queue used in the thread pool notification mode. Thus,
it is only shown if `-notificationMode` is set to threadpool.

```
{
  ...
  "notifQueue" : {
    "avgTimeInQueue": 0.000077437,
    "in" : 579619,
    "out" : 579619,
    "reject" : 0,
    "sentOk" : 579543,  // Probably will be generalized for all notification modes at the end
    "sentError" : 76,   // Probably will be generalized for all notification modes at the end
    "timeInQueue" : 44.884263230,
    "size" : 0
  }
  ...
}
```

The particular counters are as follows:

* `avgTimeInQueue`: average time that each notification waits in the queue (equal to `timeInQueue` divided by `out`)
* `in`: number of notifications that get into the queue
* `out`: numbers of notifications that get out of the queue
* `reject`: number of notifications that get rejected, due to queue full. In other words, notifications that are not even
  enqueued.
* `sentOk`: number of successfully sent notifications
* `sentError`: number of unsuccessful notification-attempts
* `timeInQueue`: accumulated time of notifications waiting in queue
* `size`: current size of the queue

Note that in the case of using [per-service reserved queues/pools](perf_tuning.md#notification-modes-and-performance),
the information in this section is the aggregated of every per-service queue/pool plus the
default queue/workers.

## GET /cache/statistics

Provides counters for the context subscription cache operations (refresh, insert, remove and update), along
with the current number of cached items.

```
{
  "ids": "SubscriptionId1, SubscriptionId2, ... SubscriptionIdN",
  "refreshs" : 1,
  "inserts" : 1433,
  "removes" : 6,
  "updates" : 0,
  "items" : 1427  
}
```

Note that the "ids" field could get really really long. To avoid a too long response, the broker sets a limit of the size of the 'ids' field.
If the length is longer than that limit, instead of presenting the complete list of subscription-identifiers, the text
   "too many subscriptions"
is presented instead.

## Reseting statistics

To reset the statistics counters (note that the fields that come from the state of the system are not reset, e.g. subs cache items or notification queue size) just invoke the DELETE operation on the statistics URL:

* DELETE /statistics
* DELETE /cache/statistics

Resetting statistics yields the following response:

```
{
  "message" : "All statistics counter reset"
}
```
