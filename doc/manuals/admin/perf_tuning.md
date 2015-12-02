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
* [Outgoing HTTP connections timeout](#outgoing-http-connetions-timeout)
* [Subscription cache](#subscription-cache)

##  MongoDB version

Since version 0.21.0, Orion supports both MongoDB 2.6 or MongoDB 3.0 without differences from a functional
point of view. However, MongoDB 2.6 implements a per-collection lock, while MongoDB 3.0 (when configured
to use WireTiger storage engine) implements per-document lock. Thus, the lock system in MongoDB3.0
(with WireTiger) is less restrictive than the one used by MongoDB 2.6.

In sum, from a performance point of view, it is recommended to use MongoDB 3.0 with WireTiger, especially
in update-intensive scenarios.

[Top](#top)

## Database indexes

Orion Context Broker doesn't create any index in any database collection except for one exception,
described at the end of this section) in order to let flexibility to database administrators. Take
into account that index usage involves a tradeoff between read efficiency (usage of indexes generally
speeds up reads) and write efficiency (the usage of indexes slow down writes) and storage (indexes
consume space in database and mapped RAM memory) and is the administrator (not Orion) who has
to decide what to priorize.

However, in order to help administrator in that task, the following indexes are recommended:

* Collection [entities](database_model.md#entities-collection)
  * `_id.id`
  * `_id.type`
  * `_id.servicePath`
  * `attrNames`
  * `creDate`

The only index that Orion Context Broker actually ensures is the "2dsphere" one in the `location.coords`
field in the entities collection, due to functional needs [geo-location functionality](../user/geolocation.md)to work.
The index is ensured at Orion startup or when entities are created for the first time.

You can find an analysis about indexes effect in [this document](https://github.com/telefonicaid/fiware-orion/blob/develop/doc/manuals/admin/extra/indexes_analysis.md), although
it is based in an old Orion version, so probably outdated.

[Top](#top)

## Write concern

[Write concern](https://docs.mongodb.org/manual/core/write-concern/) is a parameter for MongoDB write
operations. By default, Orion uses "acknowledgue" write concern that means that the Orion waits until
MongoDB confirm it has applyed the operation in memory. You can change this behavior with the
[`-writeConcern` CLI option](cli.md). When "unacknowledgue" write concern is configured, Orion doesn't
wait to get confirmation, so it can execute operation involving wring much faster.

Note, however, that there is a tradeoff between performance and reliability. Using "unacknowledgue" write
concern you will get better performance, but the risk to lost information (as Orion doesn't get any
indication regarding if the write operation were fine or not) is higher.

[Top](#top)

## Notification modes and performance

Orion can use different notifications modes, depending on the [`-notificationMode`](cli.md) value.

Default mode is transient. In this mode, each time a notification is sent, a new thread is created to deal
with it. Once the notification is sent and the response is received, the thread and the connection context
are destroyed. This is the recommended mode for low load scenarios. In high level cases it could lead to
a [thread exhaustion problem](#thread-pool-considerations).

Permanent mode is similar, except in that the connection context is not destroyed at the end. Thus,
new notifications associated to the same connection context (i.e. the same destination URL) could
reuse the connection and save the HTTP connection time (i.e. TCP handshake, etc.). Of course,
this requires that the server (i.e. the component received the notification) maintains also the connection
opened. Note that while in some cases the permanent mode could improve performance (as it saves
the time required to create and destroy HTTP connections), but in others it make cause that notifications
associated to the same connection context have to wait (as only one at the same time may use the
connection). In other words, only one notification thread can use the connection at a time, so if the
notification request/response transmission time is longer than notification inter-triggering time
then threads will block- You can detect this situation when the `connectionContext` value
[in statistics](statistics.md#semwait-block) is abnormally high.

Finally, threadpool mode is based on a queue for notifications and a pool of worker threads that
take notifications from the queue and actually send them on the wire, as shown in the figure below.
This is the recommended mode for high load scenarios, after a careful tuning on the queue length
and workers number. A good starting point is to set the number of workers to the number of expected
concurrent clients sending updates and queue limit as N times the number of workers (e.g. N equal
to 10, although it could be more or less depending on the expected update burst length). The statistics
on [the `notifQueue` block](statistics.md#notifqueue-block) could help you to tune.

![](notif_queue.png "notif_queue.png")

[Top](#top)

## HTTP server tuning

The REST API which Orion implements is provided by an HTTP server listening on port 1026 by default
(this can be overridden by the [`-port` CLI parameter](cli.md)). You can tune its behavior using the
following CLI parameters (see details at the corresponding document):

* **connectionMemory**. Sets the size of the connection memory buffer (in Kb) per connection used
  internally by the HTTP server library. Default value is 64 Kb.

* **maxConnections**. Maximum number of simultaneous connections. Default value is "unlimited" (limited
   by max file descriptors of operating system).

* **reqPoolSize**. Size of thread pool for incoming connections. Default value is 0, meaning no thread pool,
  i.e. a new thread is created to manage each new incoming HTTP request and destroyed after use.

Given that thread creation and destruction is a costly operation, it is recommend to use `-reqPoolSize` in
high load scenarios. The other two parameters use to work well with the default values.

![](requests_queue.png "requests_queue.png")

[Top](#top)

## Thread pool considerations

Orion can use thread pools at two different points: to process incoming HTTP requests at the API endpoint
(managed by `-reqPoolSize`) and to process outgoing notifications (using the threadpool notification mode).

There is a tradeoff between using or not using thread pools. On the one side, using thread pools is
beneficial as it saves thread creation/destruction times. In addition, they make the behavior of Orion more
predictable, taking into account that operating system use to stablish a thread limit per process and
threadpools is a way of guarantee that the Orion process doesn't goes beyond that limit (typically, you
can change the limit with the ulimit command).

On the other hand, setting thread pools is a way of "capping" throughput. If the thread workers are busy
all the time, at the end the queue saturates and you will end losing incoming request or ongoing
notifications.

Finally, you may have thread exhaustion problems if don’t use threadpools. You can detect that
situation by two symptoms. First, an unexpectedly high number of thread associated to the process.
Second, error messages like this appear in the logs:

```
Runtime Error (error creating thread: ...)
```

[Top](#top)

## Identifying bootlenecks looking at semWait statistics

The [semWait section](statistics.md#semwait-block) in the statistics operation output includes valuable
information that can be used to detect potential bottlenecks.

* **connectionContext**. An abnormally high value in this metric may be due to many notifications wants
  to use the same permanent connection. In that case, stop using permanent notification mode and use
  transient or threadpool instead (in fact note its value of this metric always 0 if permanent notification
  mode is not used).

* **dbConnectionPool**. Orion keeps a DB connections pool (which size is established with [`-dbPoolSize`](cli.md)).
  An abnormally high value of this metric means that Orion threads wait too much to get a connection from
  the pool. That could be due to the size is insufficient (in that case, increase the value of `-dbPoolSize`)
  or that there is some other bottleneck with the DB (in that case, review your DB set up and configuration).

* **request**. An abnormally high value in this metric means that threads waits to much before entering in
  the internal logic module that process the request. In that case, consider to use "none" policy
  (in fact, note that the value of this metric is always 0 if "none" policy is used). A a look to
  [the section on mutex policy](#mutex-policy-impact-in-performance).

Other metrics (timeStat, transaction and subCache) are for internal low-level semaphores. These metrics
are mainly for Orion developers, to help to identify bugs in the code. Their values shouldn't be too high.

[Top](#top)

## Log impact in performance

[Logs](logs.md) can have a severe impact in performance. Thus, in high level scenarios, it is recommended to use `-logLevel`
ERROR or WARNING. We have found in some situations that the saving between `-logLevel WARNING` and `-logLevel INFO`
could be around 50% in performance.

[Top](#top)

## Mutex policy impact in performance

Orion allows four different policies (configurable with `-reqMutextPolicy`):

* "all", which ensures that as much as 1 request is being processed by the internal logic module at the same time

* "read", which ensures that in a given CB node as much as 1 read request is being processed by the internal logic module
  at the same time, write requests can execute concurrently.

* "write", which ensures that in a given CB node as much as 1 write request is being processed by the internal logic module
   at the same time, read requests can execute concurrently

* "none", which allows all the requests being executed concurrently.

Default value is "all", due mainly to legacy reasons (a leftover of the times in which some race condition issues may occur).
However, by the time being "none" can be safely used, leading to a better performance (as no thread is blocked waiting for
others at the internal logic module entry). In fact, in Active-Active Orion configuration using something different from
"none" doesn't provide any advantage (as the mutex policy is local to the Orion process).

[Top](#top)

## Outgoing HTTP connections timeout

It may happen that a given notification receiver or a context provider (to which a query/update has been forwarded) takes
too long to answer the HTTP request. In some cases, the receiver is not even listening, so a long timeout (the default one
established by the operating system) has to pass before the request can be considered as failed and the sending thread unblocks.
This may have a significant impact.

In the case of notifications, it cause that the thread (either transients or in the thread pool) is blocked. In transien t case,
it involves a thread inside the process, counting toward the maximum per-process thread limit but doing no effective work. In
the second case, it means there is worker in the pool that cannot take new work while it is waiting.

In the case  of queries/updates forwarded to context providers, the effect is that the original client will take a long to get
the answer. In fact, some clients may give up and close the connection.

In this kind of situation, the `-httpTimeout` [CLI parameter](cli.md) may help to control how much Orion must wait for at
outgoing HTTP connection, overriding the default operating system timeout.

[Top](#top)

## Subscription cache

Orion implements a context subscriptions (NGSI10) cache in order to speed up notification triggering. In the current
version (this may change in the future), context availability subscriptions (NGSI9) doesn't use cache.

Cache synchronization period is controlled by the `-subCacheIval` (by default it is 60 seconds). Synchronization involves two
different tasks:

* Reading for changes in the context subscriptions collection at database and update the local cache based on it. Note that
  in a multi-CB configuration one node may modify the context subscriptions collection, so this is the way other nodes get
  aware of it.

* Writing some transient information associated to each subscription into the database. This means that even in mono-CB
  configurations, you should use a `-subCacheIval` different from 0 (`-subCacheIval 0` is allowed, but not recommended).

Note that in multi-CB configuration with load balancing, it may pass a time between (which upper limit is the cache refresh
interval) a given client send a notification and all CB nodes get aware of that. During that period, only one CB
(the one which processed the subscription and have it in the cache) will trigger notifications based on it. Thus,
CB implements "eventual consistency" regarding this.

Note also that there is a tradeoff on the cache refresh interval. Short intervals mean that changes regarding context
subscriptions are propagated faster between CB nodes (i.e. it has to pass less time to pass from "eventual consistency"
to full consistency) but there is more stress at CB and DB. Large CB intervals mean that changes take more time to
propagate, but the stress at CB and DB is lower.

As final note, you can disable cache completely using the `-noCache` CLI option, but that is not a recommended configuration.

[Top](#top)
