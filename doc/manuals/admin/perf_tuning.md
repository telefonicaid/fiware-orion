#<a name="top"></a>Performance tuning

* [MongoDB version](#mongodb-version)
* [Database indexes](#database-indexes)
* [Write concern](#write-concern)
* [Notification modes and performance](#notification-modes-and-performance)
* [HTTP server tuning](#http-server-tuning)
* [Thread pool considerations](#thread-pool-considerations)
* [Identifying bootlenecks looking at semWait statistics](#identifying-bootlenecks-looking-at-semwait-statistics)
* [Log impact in performance](#log-impact-in-performance)
* [Mutex policy impact in performance](#mutex-policy-impact-in-performance)
* [Outgoing HTTP connections timeout](#outgoing-http-connections-timeout)
* [Subscription cache](#subscription-cache)

##  MongoDB version

Since version 0.21.0, Orion supports both MongoDB 2.6 or MongoDB 3.0 without differences from a functional
point of view. However, MongoDB 2.6 implements a per-collection lock, while MongoDB 3.0 (when configured
to use WireTiger storage engine) implements per-document lock. Thus, the lock system in MongoDB 3.0
(with WireTiger) is less restrictive than the one used by MongoDB 2.6.

From a performance point of view, it is recommended to use MongoDB 3.0 with WireTiger, especially
in update-intensive scenarios.

[Top](#top)

## Database indexes

Orion Context Broker doesn't create any index in any database collection, with one exception
(described at the end of this section), to give flexibility to database administrators. Take
into account that index usage involves a tradeoff between read efficiency (usage of indexes generally
speeds up reads) and write efficiency (the usage of indexes slows down writes) and storage (indexes
consume space in database and mapped RAM memory) and it is the administrator (not Orion) who has
to decide what to priorize.

However, in order to help administrator in that task, the following indexes are recommended:

* Collection [entities](database_model.md#entities-collection)
  * `_id.id`
  * `_id.type`
  * `_id.servicePath`
  * `attrNames`
  * `creDate`

The only index that Orion Context Broker actually ensures is the "2dsphere" in the `location.coords`
field in the entities collection, due to functional needs [geo-location functionality](../user/geolocation.md)to work.
The index is ensured at Orion startup or when entities are created for the first time.

You can find an analysis about the effect of indexes in [this document](https://github.com/telefonicaid/fiware-orion/blob/develop/doc/manuals/admin/extra/indexes_analysis.md), although
it is based on an old Orion version, so it is probably outdated.

[Top](#top)

## Write concern

[Write concern](https://docs.mongodb.org/manual/core/write-concern/) is a parameter for MongoDB write
operations. By default, Orion uses "acknowledge" write concern which means that Orion waits until
MongoDB confirms it has applied the operation in memory. You can change this behavior with the
[`-writeConcern` CLI option](cli.md). When "unacknowledged" write concern is configured, Orion doesn't
wait to get confirmation, so it can execute write-operations much faster.

Note however that there is a tradeoff between performance and reliability. Using "unacknowledged" write
concern you get better performance, but the risk to lose information (as Orion doesn't get any
indication whether the write operation was successful) is higher.

[Top](#top)

## Notification modes and performance

Orion can use different notification modes, depending on the [`-notificationMode`](cli.md) value.

Default mode is 'transient'. In this mode, each time a notification is sent, a new thread is created to deal
with the notification. Once the notification is sent and the response is received, the thread and the connection context
are destroyed. This is the recommended mode for low load scenarios. In high level cases, it could lead to
a [thread exhaustion problem](#thread-pool-considerations).

Permanent mode is similar, except that the connection context is not destroyed at the end. Thus,
new notifications associated to the same connection context (i.e. the same destination URL) could
reuse the connection and save the HTTP connection time (i.e. TCP handshake, etc.). Of course,
this requires that the server (i.e. the component that receives the notification) also maintains the connection
open. Note that while in some cases the permanent mode could improve performance (as it saves
the time required to create and destroy HTTP connections), in others it may cause notifications
associated to the same connection context to have to wait (as only one of them may use the
connection at the same time). In other words, only one notification thread can use the connection at a time, so if the
notification request/response transmission time is longer than notification inter-triggering time
then threads will block. You can detect this situation when the `connectionContext` value
[in statistics](statistics.md#semwait-block) is abnormally high.

Finally, threadpool mode is based on a queue for notifications and a pool of worker threads that
take notifications from the queue and actually send them on the wire, as shown in the figure below.
This is the recommended mode for high load scenarios, after a careful tuning on the queue length
and number of workers. A good starting point is to set the number of workers to the number of expected
concurrent clients that send updates, and the queue-limit as N times the number of workers (e.g. N equal
to 10, although it could be more or less depending on the expected update burst length). The statistics
on [the `notifQueue` block](statistics.md#notifqueue-block) may help you to tune.

![](notif_queue.png "notif_queue.png")

[Top](#top)

## HTTP server tuning

The REST API that Orion implements is provided by an HTTP server listening on port 1026 by default
(this can be overridden by the [`-port` CLI parameter](cli.md)). You can tune its behavior using the
following CLI parameters (see details in the corresponding document):

* **connectionMemory**. Sets the size of the connection memory buffer (in Kb) per connection used
  internally by the HTTP server library. Default value is 64 Kb.

* **maxConnections**. Maximum number of simultaneous connections. Default value is "unlimited" (limited
   by max file descriptors of the operating system).

* **reqPoolSize**. Size of thread pool for incoming connections. Default value is 0, meaning no thread pool at all,
  i.e., a new thread is created to manage each new incoming HTTP request and destroyed after use.

Given that thread creation and destruction are costly operations, it is recommend to use `-reqPoolSize` in
high load scenarios. The other two parameters usually work well with their default values.

![](requests_queue.png "requests_queue.png")

[Top](#top)

## Thread pool considerations

Orion can use thread pools in two different points: to process incoming HTTP requests at the API endpoint
(managed by `-reqPoolSize`) and to process outgoing notifications (using the threadpool notification mode).

There is a tradeoff between using thread pools or not. On the one side, using thread pools is
beneficial as it saves thread creation/destruction time. In addition, they make the behavior of Orion more
predictable, taking into account that the operating system establishes a thread limit per process and
threadpools is a way of guaranteeing that the Orion process doesn't go beyond that limit (typically, you
can change the limit with the 'ulimit' command).

On the other hand, setting thread pools is a way of "capping" throughput. If the thread workers are busy
all the time, at the end the queue saturates and you will end up losing incoming request or ongoing
notifications.

Finally, you may have thread exhaustion problems if you don’t use threadpools. You can detect that
situation by two symptoms. First, an unexpectedly high number of threads associated to the process.
Second, error messages like this appearing in the logs:

```
Runtime Error (error creating thread: ...)
```

[Top](#top)

## Identifying bootlenecks looking at semWait statistics

The [semWait section](statistics.md#semwait-block) in the statistics operation output includes valuable
information that can be used to detect potential bottlenecks.

* **connectionContext**. An abnormally high value in this metric may be due to that many notifications want
  to use the same permanent connection. In that case, stop using permanent notification mode and use
  transient or threadpool instead (in fact note that the value of this metric is always 0 if permanent notification
  mode is not used).

* **dbConnectionPool**. Orion keeps a DB connection pool (which size is established with [`-dbPoolSize`](cli.md)).
  An abnormally high value of this metric means that Orion threads wait too much to get a connection from
  the pool. This could be due to the size of the pool is insufficient (in that case, increase the value of `-dbPoolSize`)
  or that there is some other bottleneck with the DB (in that case, review your DB setup and configuration).

* **request**. An abnormally high value in this metric means that threads wait too much before entering
  the internal logic module that processes the request. In that case, consider to use the "none" policy
  (in fact, note that the value of this metric is always 0 if "none" policy is used). Have a look at
  [the section on mutex policy](#mutex-policy-impact-in-performance).

Other metrics (timeStat, transaction and subCache) are for internal low-level semaphores. These metrics
are mainly for Orion developers, to help to identify bugs in the code. Their values shouldn't be too high.

[Top](#top)

## Log impact in performance

[Logs](logs.md) can have a severe impact in performance. Thus, in high level scenarios, it is recommended to use `-logLevel`
ERROR or WARNING. We have found in some situations that the saving between `-logLevel WARNING` and `-logLevel INFO`
can be around 50% in performance.

[Top](#top)

## Mutex policy impact on performance

Orion supports four different policies (configurable with `-reqMutexPolicy`):

* "all", which ensures that not more than one request is being processed by the internal logic module at the same time

* "read", which ensures that in a given CB node not more than one read request is being processed by the internal logic module at the same time - write requests can execute concurrently.

* "write", which ensures that in a given CB node n ot more than one write request is being processed by the internal logic module at the same time, read requests can execute concurrently

* "none", which allows all the requests to be executed concurrently.

Default value is "all", mainly due to legacy reasons (a leftover of the times in which some race condition issues may occur).
However, for the time being, "none" can be safely used, leading to a better performance (as no thread is blocked waiting 
for others at the internal logic module entry). In fact, in Active-Active Orion configuration, using something different 
from "none" doesn't provide any advantage (as the mutex policy is local to the Orion process).

[Top](#top)

## Outgoing HTTP connection timeout

It may happen that a given notification receiver or a context provider (to which a query/update has been forwarded) takes
too long to answer the HTTP request. In some cases, the receiver is not even listening, so a long timeout (the default one
established by the operating system) has to pass before the request can be considered a failure and the sending thread unblocks.
This may have a significant impact.

In the case of notifications, it causes that the thread (either transients, persistent or in the thread pool) is blocked. 
In transient or persistent mode, it involves an idle thread inside the process, counting toward the maximum per-process thread limit but doing
no effective work (this can be specially severe in the case of persistent mode, as it will block other notification trying to sent to the same URL).
In the second case, it means there are workers in the pool that cannot take on new work while waiting.

In the case of queries/updates forwarded to context providers, the effect is that the original client will take a long time
to get the answer. In fact, some clients may give up and close the connection.

In this kind of situations, the `-httpTimeout` [CLI parameter](cli.md) may help to control how much Orion should wait for
outgoing HTTP connections, overriding the default operating system timeout.

[Top](#top)

## Subscription cache

Orion implements a context subscription (NGSI10) cache in order to speed up notification triggering. In the current
version (this may change in the future), context availability subscriptions (NGSI9) doesn't use any cache.

The cache synchronization period is controlled by the `-subCacheIval` (by default it is 60 seconds).
Synchronization involves two different tasks:

* Reading for changes in the context subscription collection in the database and update the local cache based on it.
  Note that in a multi-CB configuration, one node may modify the context subscription collection, so this is the way other
  nodes get aware of the modification.

* Writing some transient information associated to each subscription into the database. This means that even in mono-CB
  configurations, you should use a `-subCacheIval` different from 0 (`-subCacheIval 0` is allowed, but not recommended).

Note that in multi-CB configurations with load balancing, it may pass some time between (whose upper limit is the cache
refresh interval) a given client sends a notification and all CB nodes get aware of it. During this period, only one CB
(the one which processed the subscription and have it in its cache) will trigger notifications based on it. Thus,
CB implements "eventual consistency" regarding this.

Note also that there is a tradeoff with the cache refresh interval. Short intervals mean that changes regarding context
subscriptions are propagated faster between CB nodes (i.e. it has to pass less time to pass from "eventual consistency"
to full consistency) but there is more stress on CB and DB. Large intervals mean that changes take more time to
propagate, but the stress on CB and DB is lower.

As a final note, you can disable cache completely using the `-noCache` CLI option, but that is not a recommended configuration.

[Top](#top)
