# Running Orion from command line

You can run the broker just typing the following command:

    contextBroker

The broker will run in background by default, so you will need to stop
it using signals.

You can use command line arguments, e.g. to specify the port in which
Orion Context Broker listens, using the -port option:

    contextBroker -port 5057

To know all the possible options, have a look at the next section.

## Command line options

Command line options can be used directly (in the case of running from
the command line) or indirectly
through the different fields in /etc/sysconfig/contextBroker (in the
case of running [as a system service](running.md)).
To obtain a list of available options, use:

    contextBroker -u

To get more information (including default values), use:

    contextBroker -U

The list of available options is the following:

-   **-u** and **-U**. Shows usage in brief or long
    format, respectively.
-   **--help**. Show help (very similar to previous).
-   **--version**. Shows version number
-   **-port <port>**. Specifies the port that the broker listens to.
    Default port is 1026.
-   **-ipv4**. Runs broker in IPv4 only mode (by default, the broker
    runs in both IPv4 and IPv6). Cannot be used at the same time
    that -ipv6.
-   **-ipv6**. Runs broker in IPv6 only mode (by default, the broker
    runs in both IPv4 and IPv6). Cannot be used at the same time
    that -ipv4.
-   **-rush <host:port>**. Use **rush** in *host* and
    *port*. Default behavior is to *not* use Rush. See section
    on [using Rush relayer](rush.md).
-   **-multiservice**. Enables multiservice/multitenant mode (see [multi
    service tenant section](../user/multitenancy.md)).
-   **-db <db>**. The MongoDB database to use or
    (if `-multiservice` is
    in use) the prefix to per-service/tenant databases (see section on
    [service/tenant database
    separation](../user/multitenancy.md). This field is restricted to 10 characters
    max length.
-   **-dbhost <host>**. The MongoDB host and port to use, e.g. `-dbhost
    localhost:12345`.
-   **-rplSet <replicat_set>**. If used Orion CB will connnect to a
    MongoDB replica set (instead of an stand-alone MongoDB instance).
    The name of the replica set to use is the value of the parameter. In
    this case, the -dbhost parameter can be a list of host (separated
    by ",") which are used as seed for the replica set.
-   **-dbTimeout <interval>**. Only used in the case of using replica
    set (-rplSet), ignored otherwise. It specifies the timeout in
    miliseconds for connections to the replica set.
-   **-dbuser <user>**. The MongoDB user to use. If your MongoDB doesn't
    use authorization this option must be avoided. See [database
    authorization section](database_admin.md#database-authorization).
-   **-dbpwd <pass>**. The MongoDB password to use. If your MongoDB
    doesn't use authorization this option must be avoided. See [database
    authorization section]( database_admin.md#database-authorization).
-   **-dbPoolSize <size>**. Database connection pool. Default size of
    the pool is 10 connections.
-   **-writeConcern <0|1>**. Write concern for MongoDB write operations:
    acknowledge (1) or unacknowledge (0). Default is 1.
-   **-https**. Work in secure HTTP mode (See also `-cert` and `-key`).
-   **-cert**. Certificate file for https. Use an absolute
    file path. Have a look to [this
    script](https://github.com/telefonicaid/fiware-orion/blob/master/scripts/httpsPrepare.sh)
    for an example of how generating this file.
-   **-key**. Private server key file for https. Use an absolute
    file path. Have a look to [this
    script](https://github.com/telefonicaid/fiware-orion/blob/master/scripts/httpsPrepare.sh)
    for an example of how generating this file.
-   **-logDir <dir\>**. Specifies the directory to use for the contextBroker log file.
-   **-logAppend**. If used, the log lines are appended to the
    contextBroker log file, instead of re-creating it when the
    broker starts.
-   **-logLevel**. Select logging level, supported levels:
    - NONE    (suppress ALL log output, including errors),
    - ERROR   (show only error messages),
    - WARNING (show error and warning messages),
    - INFO    (show error, warning and informational messages),
    - DEBUG   (show ALL messages - this is the default setting).
-   **-t <trace level>**. Specifies the initial trace levels
    for logging. You can use a single value (e.g. "-t 70"), a
    range (e.g. "-t 20-80"), a comma-separated list (e.g. "-t 70,90") or
    a combination of them (e.g. "-t 60,80-90"). If you want to use all
    trace levels for logging, use "-t 0-255". Note that trace levels can
    be changed dynamically using the [management REST
    interface](management_api.md). The detail of the
    available tracelevels and its numbers can be found
    [here](https://github.com/telefonicaid/fiware-orion/blob/develop/src/lib/logMsg/traceLevels.h)
    (as a C struct).
-   **-fg**. Runs broker in foreground (useful for debugging). Log output is printed on standard output
    (in addition to the log file, but using a simplified format).
-   **-localIp <ip>**. Specifies in which IP interface the broker
    listens to. By default it listens to all the interfaces.
-   **-pidpath <pid_file>**. Specifies the file to store the PID of the
    broker process.
-   **-httpTimeout <interval>**. Specifies the timeout in milliseconds
    for forwards and notifications.
-   **-corsOrigin <domain>**. Configures CORS allowed for GET requests,
    specifing the allowed origin (use `__ALL` for `*`).
-   **-reqMutextPolicy <all|none|write|read>**. Specifies the internal
    mutext policy. Possible values are: "all" (which ensures that in a
    given CB node as much as 1 request is being processed by the
    internal logic module at the same time), "read" (which ensures that
    in a given CB node as much as 1 read request is being processed by
    the internal logic module at the same time, write request can
    execute concurrently), "write" (which ensures that in a given CB
    node as much as 1 write request is being processed by the internal
    logic module at the same time, write request can
    execute concurrently) and "none" (which allows all the requests
    being executed concurrently). Default value is "all". For
    Active-Active Orion configuration "none" is recommended.
-   **-subCacheIval**. Interval in seconds between calls to subscription cache refresh. A zero
    value means "no refresh" (default value and the recommended one for mono-CB
    deployments).
-   **-noCache**. Disables context subscription cache, so subscriptions searches are
    always done at DB (not recommended but useful for debugging).
-   **-notificationMode** *(Experimental option)*. Allows to select notification mode, either:
    `transient`, `permanent` or `threadpool:q:n`. Default mode is `transient`.
    * In transient mode, connections are closed by CB just after sending the notification.
    * In permanent connection mode, a permanent connection is created the first time a notification
      is sent to a given URL path (if the receiver support permanent connections). Following notifications to the same
      URL path will reuse the connection, saving HTTP connection time.
    * In threadpool mode, notifications are enqueued into a queue of size `q` and `n` threads take the notifications
      from the queue and do the outgoing requests asynchronously.
-   **-simulatedNotification**. Notifications are not sent, but recorded internally and shown in the 
    [statistics](statistics.md) operation (`simulatedNotifications` counter). This is not aimed for production
    usage, but it may be useful for debugging to calculate a maximum upper limit in notification rate from a CB
    internal logic point of view.
-   **-connectionMemory**. Sets the size of the connection memory buffer (in Kb) per connection used internally
    by the HTTP server library. Default value is 64 Kb.
-   **-maxConnections**. Maximum number of simultaneous connections. Default value is "unlimited" (limited by 
    max file descriptors of operating system).
-   **-reqPoolSize**. Size of thread pool for incoming connections. Default value is 0, meaning *no thread pool*.
-   **-statCounters**, **-statSemWait**, **-statTiming** and **-statNotifQueue**. Enable statistics
    generation. See [statistics documentation](statistics.md).
