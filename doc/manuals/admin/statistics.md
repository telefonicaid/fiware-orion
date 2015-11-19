## Statistics

**Warning**: This section of the manual is yet work on progress, so maybe the current Orion implementation
is not fully aligned with it. In addition, take into account that statistics API is by the momennt
in beta status, so changes in the name of the different counters or the JSON structure could take
place in the future.

Orion Context Broker provides a set of statistics (mainly aimed at testing and debugging) through the
`GET /statistics` and `GET /cache/statistics` operations. These operations only supports JSON encoding.

At the present moment, statistics accuracy is not guaranteed under high local conditions. Atomic
counters are not used in all places, so race conditions on request running at the same time could
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
  "uptime_in_secs" : "65697",
  "measuring_interval_in_secs" : "65697"
}
```

Conditional blocks are enabled using `-statXXX` flags at the command line (in the future, maybe
we will develop the posibility of activatinig/deactiving them using the [management API](management_api.md).
This is due to two reasons. First, to avoid too verbose statistics output for users that are not interested
in want some parts. Second, measure some of the information could involve a performance penalty
(in general, to measure time always needs additional system calls) so explicit activation is needed.

* "counters" (enabled with the `-statCounters`)
* "semWait" (enabled with the `-statSemWait`)
* "timing" (enabled with the `-statTiming`)
* "notifQueue" (enabled with the `-statNotifQueue`)

Unconditional fields are:

* `uptime_in_secs`, Orion uptime in seconds.
* `measuring_interval_in_secs`, statistics measuring time in seconds. It is set to 0 each time statistics
  are reset. If statistics have not been reset since Orion start, this field matches `uptime_in_secs`.

### Counter block

It provides counters information for the time a particular request type has been received (also for
notifications being sent), e.g:

```
{
  ...
  "counters" : {
    "jsonRequests" : "75916",
    "queries" : "3698",
    "updates" : "2416",
    "subscriptions" : "138",
    "unsubscriptions" : "6",
    "notificationsReceived" : "216936",
    "notificationsSent" : "579542",
    "individualContextEntity" : "360",
    "allContextEntitiesRequests" : "3",
    "versionRequests" : "1109",
    "statisticsRequests" : "13",
    "invalidRequests" : "2"
  },
  ...
}
```

If a particular request type has not been received, its corresponding counter is not shown.

### SemWait block

It provides accumulated waiting time for main internal semaphores. It can be useful to detect bottelnecks, e.g.
if your DB is two slow or your DB pool is undersized, then `dbConnectionPool` time would be anormaly high
(see section on [performance tunning](perf_tuning.md) for more details).

```
{
  ...
  "semWait" : {
    "request" : "0.000000000",
    "dbConnectionPool" : "2.917002794",
    "transaction" : "0.567478849",
    "subCache" : "0.784979145",
    "curlContext" : "0.000000000",
    "timeStat" : "0.124000605"
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
      "jsonV1Parse": "7.860908311",
      "mongoBackend": "416.796091597",
      "mongoReadWait": "4656.924425628",
      "mongoWriteWait": "259.347915990",
      "mongoCommandWait": "0.514811318",
      "render": "108.162782114",
      "request": "186476.593504743",
      "xmlParse": "44.490766332"
        },
    "last": {
      "mongoBackend": "0.014752309",
      "mongoReadWait": "0.012018445",
      "mongoWriteWait": "0.000574611",
      "render": "0.000019136",
      "request": "0.015148915",
      "xmlParse": "0.000153878"
      }
  }
  ...
}
```

The block includes two main sections:

* `last`: times corresponding to the last processed request. If that request didn't used a particular module
  (e.g. a GET request doesn't use parsing), then that counter is 0 and it is not shown.
* `accumulated`: accumulated time corresponding to all requests.

The particular counters are as follow:

* `request`: processing time for the whole request, excluding the time that the HTTP library
  takes for request/response dispatching (pseudo end-to-end time)
* `jsonV1Parse`: time passed in NGSIv1 JSON parsing module (pseudo self-time)
* `jsonV2Parse`: time passed in NGSIv2 JSON parsing module (pseudo self-time)
* `xmlParser`: time passed in NGSIv1 XML module (pseudo self-time)
* `mongoBackend`: time passed in mongoBackend module (pseduo self-time)
* `render`: time passed in rendering module (pseudo self-time)
* `mongo*Wait`: time passed waiting for MongoDB for `Read`, `Write` or `Cmd` operations. Note that if
  a given request involves several read/write/cmd calls to MongoDB, the time shown in `mongo*Wait` under
  `last` includes the accumulation for all them.

Times are measured since a particular thread request start using a module and ends using it. Thus, if the
thread is stoped for any reason (e.g. the kernel decides to give priority to another thread based on its
scheculing policy) the time is not accurate. That is what we say *pseudo* selt/end-to-end time. However,
under low load conditions this situation is not expected to have a significative impact.

### NotifQueue block

Provides information related with the notification queue used in the thread pool notification mode. Thus,
it is only shown if `-notificationMode` uses threadpool.

```
{
  ...
  "notifQueue" : {
    "in" : "579619",
    "out" : "579619",
    "reject" : "0",
    "sentOk" : "579543",  // Probably will be generalized for all transmision modes at the end
    "sentError" : "76",   // Probably will be generalized for all transmision modes at the end
    "timeInQueue" : "44.884263230",
    "size" : "0"
  }
  ...
}
```

The particular counters are as follow:

* `in`: number of notifications that get in the queue
* `out`: numbers of notifications that get out the queue
* `reject`: number of notifications that get rejected, due to queue was full.
* `sentOk`: number of successfully sent notifications
* `sentError`: number of not successfully sent notifications
* `timeInQueue`: accumulated time of notifications waiting in queue
* `size`: current size of the queue


## GET /cache/statistics

Provides counter for the context subscription cache operations (refres, insert, remove and update), along
with the current number of cache items.

```
{
  ...
  "refreshs" : "1",
  "inserts" : "1433",
  "removes" : "6",
  "updates" : "0",
  "items" : "1427"
  ...
}
```

## Reseting statistics

To reset counters (note that fields that come from the status of the system are not reset, e.g. subs cache items
or notification queue size) just invoke the DELETE operation on the statistics URL:

* DELETE /statistics
* DELETE /

Resetting statistics yields the following response:

```
{
  "message" : "All statistics counter reset"
}
```
