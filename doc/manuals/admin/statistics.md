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

The counter block provides information about counters for the times a particular request type has been received (also for notifications being sent), e.g:

```
{
  ...
  "counters" : {
    "jsonRequests" : 75916,
    "queries" : 3698,
    "updates" : 2416,
    "subscriptions" : 138,
    "registrationRequest": 11,
    "registrations": 41,
    "unsubscriptions" : 6,
    "notificationsReceived" : 216936,
    "notificationsSent" : 579542,
    "individualContextEntity" : 360,
    "allContextEntitiesRequests" : 3,
    "versionRequests" : 1109,
    "statisticsRequests" : 13,
    "invalidRequests" : 2
  },
  ...
}
```

If a particular request type has not been received, its corresponding counter is not shown.

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
      "mongoBackend": 416.796091597,
      "mongoReadWait": 4656.924425628,
      "mongoWriteWait": 259.347915990,
      "mongoCommandWait": 0.514811318,
      "render": 108.162782114,
      "total": 6476.593504743
     },
    "last": {
      "mongoBackend": 0.014752309,
      "mongoReadWait": 0.012018445,
      "mongoWriteWait": 0.000574611,
      "render": 0.000019136,
      "total": 0.015148915
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

Times are measured from the point in time in which a particular thread request starts using a module until it finishes using it.
Thus, if the thread is stopped for some reason (e.g. the kernel decides to give priority to another thread based on its
scheculing policy) the time that the thread was sleeping, waiting to execute again is included in the measurement and thus, the measurement is not accurate. That is why we say *pseudo* selt/end-to-end time. However,
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
