# <a name="top"></a>Performance tuning

* [MongoDB configuration](#mongodb-configuration)
* [Database indexes](#database-indexes)
* [Write concern](#write-concern)
* [Notification modes and performance](#notification-modes-and-performance)
* [HTTP server tuning](#http-server-tuning)
* [Orion thread model and its implications](#orion-thread-model-and-its-implications)
* [File descriptors sizing](#file-descriptors-sizing)
* [Identifying bottlenecks looking at semWait statistics](#identifying-bottlenecks-looking-at-semwait-statistics)
* [Log impact on performance](#log-impact-on-performance)
* [Metrics impact on performance](#metrics-impact-on-performance)
* [Mutex policy impact on performance](#mutex-policy-impact-on-performance)
* [Outgoing HTTP connections timeout](#outgoing-http-connections-timeout)
* [Subscription cache](#subscription-cache)
* [Geo-subscription performance considerations](#geo-subscription-performance-considerations)

##  MongoDB configuration

From a performance point of view, it is recommended to use MongoDB 3.6 with WireTiger, especially
in update-intensive scenarios.

In addition, take into account the following information from the official MongoDB documentation, as it may have
impact on performance:

* Check that ulimit settings in your system are ok. MongoDB provides [the following recomendations](https://docs.mongodb.org/manual/reference/ulimit)
  As described in that document, in RHEL/CentOS you have to create a /etc/security/limits.d/99-mongodb-nproc.conf file,
  in order to set soft/hard process limit to at least 32000 (check details in the cited document).
* You should also disable Transparent Huge Pages (HTP) to increment the performance as explain 
  in [this document](https://docs.mongodb.org/manual/tutorial/transparent-huge-pages/).

[Top](#top)

## Database indexes

Orion Context Broker doesn't create any index in any database collection (with two exceptions,
described at the end of this section), to give flexibility to database administrators. Take
into account that index usage involves a tradeoff between read efficiency (usage of indexes generally
speeds up reads) and write efficiency (the usage of indexes slows down writes) and storage (indexes
consume space in database and mapped RAM memory) and it is the administrator (not Orion) who has
to decide what to prioritize.

However, in order to help administrators in this task, the following indexes are recommended:

* Collection [entities](database_model.md#entities-collection)
    * `_id.id`
    * `_id.type`
    * `_id.servicePath`
    * `attrNames`
    * `creDate`

In the case of using `orderBy` queries (i.e. `GET /v2/entities?orderBy=A`), it is also recommended 
to create indexes for them. In particular, if you are ordering by a given attribute 'A' in ascending order
(i.e. `orderBy=A`) you should create an index `{attrs.A.value: 1}`. In the case of ordering by a given
attribute 'A' in descending order (i.e. `orderBy=!A`) you should create an index `{attrs.A.value: -1}`.

The only indexes that Orion Context Broker actually ensure are the following ones. Both are ensured on 
Orion startup or when entities are created.

* A "2dsphere" index for the `location.coords` field in the entities collection, due to functional needs of the [geo-location functionality](../user/geolocation.md).
* An index with `expireAfterSeconds: 0` for the `expDate` field in the entities collection, due to functional needs of the [transient entities functionality](../user/transient_entities.md).

You can find an analysis about the effect of indexes in [this document](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/admin/extra/indexes_analysis.md), although
it is based on an old Orion version, so it is probably outdated.

[Top](#top)

## Write concern

[Write concern](https://docs.mongodb.org/manual/core/write-concern/) is a parameter for MongoDB write
operations. By default, Orion uses "acknowledged" write concern which means that Orion waits until
MongoDB confirms it has applied the operation in memory. You can change this behavior with the
[`-writeConcern` CLI option](cli.md). When "unacknowledged" write concern is configured, Orion doesn't
wait to get confirmation, so it can execute write-operations much faster.

Note however that there is a tradeoff between performance and reliability. Using "unacknowledged" write
concern you get better performance, but the risk to lose information is higher (as Orion doesn't get any
confirmation that the write operation was successful).

[Top](#top)

## Notification modes and performance

Orion can use different notification modes, depending on the value of [`-notificationMode`](cli.md).

Default mode is 'transient'. In this mode, each time a notification is sent, a new thread is created to deal
with the notification. Once the notification is sent and the response is received, the thread with its connection context
is destroyed. This is the recommended mode for low load scenarios. In high level cases, it might lead to
a [thread exhaustion problem](#orion-thread-model-and-its-implications).

Permanent mode is similar, except that the connection context is not destroyed at the end. Thus,
new notifications associated to the same connection context (i.e. the same destination URL) can
reuse the connection and save the HTTP connection time (i.e. TCP handshake, etc.). Of course,
this requires that the server (i.e. the component that receives the notification) also maintains the connection
open. Note that while in some cases the permanent mode could improve performance (as it saves
the time required to create and destroy HTTP connections), in others it may cause notifications
associated to the same connection context to have to wait (as only one of them may use the
connection at a time). In other words, only one notification thread can use the connection at a time, so if the
notification request/response transmission time exceeds the notification inter-triggering time
then threads will block. You can detect this situation when the `connectionContext` value
[in statistics](statistics.md#semwait-block) is abnormally high.

Finally, threadpool mode is based on a queue for notifications and a pool of worker threads that
take notifications from the queue and actually send them on the wire, as shown in the figure below.
This is the recommended mode for high load scenarios, after a careful tuning on the queue length
and the number of workers. A good starting point is to set the number of workers to the number of expected
concurrent clients that send updates, and the queue-limit as N times the number of workers (e.g. N equal
to 10, although it could be more or less depending on the expected update burst length). The statistics
on [the `notifQueue` block](statistics.md#notifqueue-block) may help you to tune.

![](notif_queue.png "notif_queue.png")

[Top](#top)

## HTTP server tuning

The REST API that Orion implements is provided by an HTTP server listening on port 1026 by default
(this can be overridden by the [`-port` CLI parameter](cli.md)). You can tune its behavior using the
following CLI parameters (see details in the corresponding document):

* **connectionMemory**. Sets the size of the connection memory buffer (in kB) per connection used
  internally by the HTTP server library. Default value is 64 kB.

* **maxConnections**. Maximum number of simultaneous connections. Default value is 1020, for legacy reasons,
  while the lower limit is 1 and there is no upper limit (limited by max number of file descriptors of the
  operating system).

* **reqPoolSize**. Size of thread pool for incoming connections. Default value is 0, meaning no thread pool at all,
  i.e., a new thread is created to manage each new incoming HTTP request and destroyed after its use. Thread pool
  mode uses internally the `epoll()` system call, which is more efficient than the one used when no
  thread pool is used (`poll()`). Some performance information regarding this can be found in [the documentation of the
  HTTP server library itself](https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#Thread-modes-and-event-loops).

* **reqTimeout**. The inactivity timeout in seconds before a connection is closed. Default value is 0 seconds, 
which means infinity. This is the recommended behaviour and setting it to a non-infinite timeout could cause Orion 
to close the connection before completing the request (e.g. a query request involving several CPr forwards can 
take a long time). This could be considered an "HTTP-unpolite" behaviour from the point of view of the server 
(Orion) given that, in this case, it should be the client who decides to close the connection.
However, this parameter may be used to limit resource consumption at server side (Orion). Use it with care.

Given that thread creation and destruction are costly operations, it is recommend to use `-reqPoolSize` in
high load scenarios. In particular, according to [MHD feedback](http://lists.gnu.org/archive/html/libmicrohttpd/2016-12/msg00023.html),
the pool should be sized with a value equal or close to number of available CPU cores. If you set `-reqPoolSize` to a value higher than
number of CPU cores then you'll most probably experience performance decrease.

The other three parameters (`-reqTimeout`, `-maxConnections` and `-connectionMemory`) usually work well with their default values.

![](requests_queue.png "requests_queue.png")

[Top](#top)

## Orion thread model and its implications

Orion is a multithread process. With default starting parameters and in idle state (i.e. no load),
Orion consumes 4 threads:

* Main thread (the one that starts the broker, then sleeps forever)
* Subscription cache synchronization thread (if `-noCache` is used then this thread is not created)
* Listening thread for the IPv4 server (if `-ipv6` is used then this thread is not created)
* Listening thread for the IPv6 server (if `-ipv4` is used then this thread is not created)

In busy state, the number of threads will be higher. With default configuration, Orion creates a new thread
for each incoming request and for each outgoing notification. These threads are destroyed once their
work finalizes.

The default configuration is fine for low to medium load scenarios. In high load scenarios you may have
a large number of simultaneous requests and notifications so the number of threads may reach the per process
operating system level. This is known as the *thread exhaustion problem* and will cause Orion to
not work properly, being unable to deal with new incoming request and outgoing notifications. You can detect
that situation by two symptoms.

* First, a number of threads associated to the process very close to the per process operating system limit.
* Second, error messages like this appearing in the logs:

  ```
  Runtime Error (error creating thread: ...)
  ```

In order to avoid this problem, Orion supports thread pools. Using thread pools you can statically set
the number of threads that the Orion process uses, removing the dynamics of thread creation/destruction
the thread exhaustion problem is avoided. In other words, pools make the behavior of Orion more
predictable, as a way of guaranteeing that the Orion process doesn't go beyond the per process
operating system thread limit.

There are two pools that can be configured independently:

* Incoming requests pool. Set by the `-reqPoolSize c` parameter, being `c` the number of threads
  in this pool. See [HTTP server tuning section](#http-server-tuning) in this page for more information.
* Notifications pool. Set by `-notificationMode threadpool:q:n`, being `n` the number of threads in this pool.
  See [notification modes and performance section](#notification-modes-and-performance) in this page.

Using both parameters, in any situation (either idle or busy) Orion consumes a fixed number of threads:

* Main thread (the one that starts the broker, then sleeps forever)
* Subscription cache synchronization thread (if `-noCache` is used then this thread is not created)
* `c` listening threads for the IPv4 server (if `-ipv6` is used then these threads are not created)
* `c` listening threads for the IPv6 server (if `-ipv4` is used then these threads are not created)
* `n` threads corresponding to the workers in the notification thread pool.

Apart from avoiding the thread exhaustion problem, there is a trade-off between using thread pools and
not. On the one side, using thread pools is beneficial as it saves thread creation/destruction time.
On the other hand, setting thread pools is a way of "capping" throughput. If the thread workers are busy
all the time, at the end the queue saturates and you will end up losing ongoing notifications.

[Top](#top)

## File descriptors sizing

The following inequity ensures the number of file descriptors used by Orion is below the operating system limit:

```
max fds > 5 * n + max cons + db pool size + extra
```

where

* **max fds** is the per process file descriptors limit, i.e. the output of the `ulimit -n` command. It can
  be changed with `ulimit -n <new limit>`.
* **n**, number of threads in the notification thread pool. The factor 5 is due to that each thread can
  hold up to 5 connections ([libcurl pool](https://curl.haxx.se/libcurl/c/CURLOPT_MAXCONNECTS.html)).
* **max cons** is the size of the thread pool for incoming connections, configured with `-reqPoolSize`
  [CLI parameter](cli.md). Note that if you don't use this parameter, default is not using any pool for incoming
  connections. Thus, a burst of incoming connections large enough could exhaust in theory all 
  available file descriptors.
* **db pool size** is the size of the DB connection pool, configured with `-dbPoolSize` [CLI parameter](cli.md),
  which default value is 10.
* **extra** an amount of file descriptors used by log files, listening sockets and file descriptors used by libraries.
  There isn't any general rule for this value, but one in the range of 100 to 200 must suffice most of the cases.

If the above inequity doesn't hold, you may have *file descriptors exhaustion problem* and Orion Context Broker
will not work properly. In particular, it may happen that Orion is unable to accept new incoming connections and/or
send notifications due to lack of file descriptors.

Note that having a large number of client connections at Orion in CLOSE_WAIT status is not a problem. This
is part of the libcurl connection cache strategy, in order to save time by reusing connections.
From [libcurl email discussion about this topic](https://curl.haxx.se/mail/tracker-2011-05/0006.html):

> The CLOSE_WAIT sockets are probably the ones that libcurl has in its connection cache but that have been
> "closed" (a FIN was sent) by the server already but not yet by libcurl. They are not there "indefinitely"
> (and they really can't be) since the connection cache has a limited size so eventually the old connections
> should get closed.

[Top](#top)

## Identifying bottlenecks looking at semWait statistics

The [semWait section](statistics.md#semwait-block) in the statistics operation output includes valuable
information that can be used to detect potential bottlenecks.

* **connectionContext**. An abnormally high value in this metric may be due to that many notifications want
  to use the same permanent connection. In that case, stop using permanent notification mode and use
  transient or threadpool instead (note that the value of this metric is always 0 if permanent notification
  mode is not used).

* **dbConnectionPool**. Orion keeps a DB connection pool (which size is established with [`-dbPoolSize`](cli.md)).
  An abnormally high value of this metric means that Orion threads wait too much to get a connection from
  the pool. This could be due to the size of the pool is insufficient (in that case, increase the value of `-dbPoolSize`)
  or that there is some other bottleneck with the DB (in that case, review your DB setup and configuration).

* **request**. An abnormally high value in this metric means that threads wait too much before entering
  the internal logic module that processes the request. In that case, consider to use the "none" policy
  (note that the value of this metric is always 0 if "none" policy is used). Have a look at
  [the section on mutex policy](#mutex-policy-impact-on-performance).

Other metrics (timeStat, transaction and subCache) are for internal low-level semaphores. These metrics
are mainly for Orion developers, to help to identify bugs in the code. Their values shouldn't be too high.

[Top](#top)

## Log impact on performance

[Logs](logs.md) can have a severe impact on performance. Thus, in high level scenarios, it is recommended to use `-logLevel`
ERROR or WARN. We have found in some situations that the saving between `-logLevel WARN` and `-logLevel INFO`
can be around 50% in performance.

[Top](#top)

## Metrics impact on performance

Metrics measurement may have an impact on performance, as system calls and semaphores are involved. You can disable
this feature (thus improving performance) using the `-disableMetrics` [CLI parameter](cli.md).

[Top](#top)

## Mutex policy impact on performance

Orion supports four different policies (configurable with `-reqMutexPolicy`):

* "all", which ensures that not more than one request is being processed by the internal logic module at the same time

* "read", which ensures that in a given CB node not more than one read request is being processed by the internal logic module at the same time - write requests can execute concurrently.

* "write", which ensures that in a given CB node not more than one write request is being processed by the internal logic module at the same time, read requests can execute concurrently

* "none", which allows all the requests to be executed concurrently.

Default value is "all", mainly due to legacy reasons (a leftover of the times in which some race condition issues may occur).
However, for the time being, "none" can safely be used, leading to a better performance (as no thread is blocked waiting 
for others at the internal logic module entry). In fact, in Active-Active Orion configuration, using something different 
than "none" doesn't provide any advantage (as the mutex policy is local to the Orion process).

[Top](#top)

## Outgoing HTTP connection timeout

It may happen that a given notification receiver or a context provider (to which a query/update has been forwarded) takes
too long to repond to an HTTP request. In some cases, the receiver is not even listening, so a long timeout (the default one
established by the operating system) has to pass before the request can be considered a failure and the sending thread unblocks.
This may have a significant impact.

In the case of notifications, it causes that the thread (either transients, persistent or in the thread pool) is blocked. 
In transient or persistent mode, it involves an idle thread inside the process, counting toward the maximum per-process thread limit but doing
no effective work (this can be especially severe in the case of persistent mode, as it will block other notifications trying to send to the same URL).
In the second case, it means there are workers in the pool that cannot take on new work while waiting.

In the case of queries/updates forwarded to context providers, the effect is that the original client will take a long time
to get the answer. In fact, some clients may give up and close the connection.

In this kind of situations, the `-httpTimeout` [CLI parameter](cli.md) may help to control how long Orion should wait for
outgoing HTTP connections, overriding the default operating system timeout.

[Top](#top)

## Subscription cache

Orion implements a context subscription cache in order to speed up notification triggering. In the current
version (this may change in the future), context availability subscriptions doesn't use any cache.

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

## Geo-subscription performance considerations

Current support of georel, geometry and coords expression fields in NGSIv2 subscriptions (aka geo-subscriptions)
relies on MongoDB geo-query capabilities. While all other conditions associated to subscriptions (e.g. query filter,
etc.) are evaluated on a memory image of the updated entity, the ones related with the georel, geometry and coords of
a given subscription need a query in the DB.

However, note that the impact on performance shouldn't be too heavy (the operation invoked in MongoDB is `count()`
which is relatively light).

Our [future plan](https://github.com/telefonicaid/fiware-orion/issues/2396) is to implement geo-subscription matching in memory (as the rest of the conditions), but this is not a priority at the moment.

[Top](#top)
