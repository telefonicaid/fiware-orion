# Running Orion from command line

You can run the broker by typing the following command:

    contextBroker

The broker runs in the background by default, so you will need to stop
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

To get more information on the options (including default values and limits), use:

    contextBroker -U

The list of available options is the following:

-   **-u** and **-U**. Shows usage in brief or long
    format, respectively.
-   **--help**. Shows help (very similar to previous).
-   **--version**. Shows version number
-   **-port <port>**. Specifies the port that the broker listens to.
    Default port is 1026.
-   **-ipv4**. Runs broker in IPv4 only mode (by default, the broker
    runs in both IPv4 and IPv6). Cannot be used at the same time
    as -ipv6.
-   **-ipv6**. Runs broker in IPv6 only mode (by default, the broker
    runs in both IPv4 and IPv6). Cannot be used at the same time
    as -ipv4.
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
-   **-rplSet <replicat_set>**. If used, Orion CB connnects to a
    MongoDB replica set (instead of a stand-alone MongoDB instance).
    The name of the replica set to use is the value of the parameter. In
    this case, the -dbhost parameter can be a list of hosts (separated
    by ",") which are used as seed for the replica set.
-   **-dbTimeout <interval>**. Only used in the case of using replica
    set (-rplSet), ignored otherwise. It specifies the timeout in
    milliseconds for connections to the replica set.
-   **-dbuser <user>**. The MongoDB user to use. If your MongoDB doesn't
    use authorization then this option must be avoided. See [database
    authorization section](database_admin.md#database-authorization).
-   **-dbpwd <pass>**. The MongoDB password to use. If your MongoDB
    doesn't use authorization then this option must be avoided. See [database
    authorization section]( database_admin.md#database-authorization).
-   **-dbAuthMech <mechanism>**. The MongoDB authentication mechanism to use in the case
    of providing `-dbuser` and `-dbpwd`. Alternatives are SCRAM-SHA-1 or MONGODB-CR.
    Default (in the case of omitting this field) is SCRAM-SHA-1.
-   **-dbAuthDb <database>**. Specifies the database to use for authentication in the case
    of providing `-dbuser` and `-dbpwd`. Default is the same as `-db` in the case of not
    using `-multiservice` or `"admin"` in the case of using `-multiservice`.
-   **-dbSSL**. Enable SSL in the connection to MongoDB. You have to use this option if your
    MongoDB server or replica set is using SSL (or, the other way around, you have not to use
    this option if your MongoDB server or replicat set is not using SSL).
-   **-dbPoolSize <size>**. Database connection pool. Default size of
    the pool is 10 connections.
-   **-writeConcern <0|1>**. Write concern for MongoDB write operations:
    acknowledged (1) or unacknowledged (0). Default is 1.
-   **-https**. Work in secure HTTP mode (See also `-cert` and `-key`).
-   **-cert**. Certificate file for https. Use an absolute
    file path. Have a look at [this
    script](https://github.com/telefonicaid/fiware-orion/blob/master/test/functionalTest/httpsPrepare.sh)
    for an example on how to generate this file.
-   **-key**. Private server key file for https. Use an absolute
    file path. Have a look at [this
    script](https://github.com/telefonicaid/fiware-orion/blob/master/test/functionalTest/httpsPrepare.sh)
    for an example on how to generate this file.
-   **-logDir <dir\>**. Specifies the directory to use for the contextBroker log file.
-   **-logAppend**. If used, the log lines are appended to the existing
    contextBroker log file, instead of starting with an empty log file.
-   **-logLevel**. Select initial logging level, supported levels:
    - NONE    (suppress ALL log output, including fatal error messages),
    - FATAL   (show only fatal error messages),
    - ERROR   (show only error messages),
    - WARN    (show error and warning messages - this is the default setting),
    - INFO    (show error, warning and informational messages),
    - DEBUG   (show ALL messages).
    Note that the log level can be modified in run-time, using the [admin API](management_api.md).
-   **-t <trace level>**. Specifies the initial trace levels
    for logging. You can use a single value (e.g. "-t 70"), a
    range (e.g. "-t 20-80"), a comma-separated list (e.g. "-t 70,90") or
    a combination of them (e.g. "-t 60,80-90"). If you want to use all
    trace levels for logging, use "-t 0-255". Note that trace levels can
    be changed dynamically using the [management REST
    interface](management_api.md). Details of the
    available tracelevels and their values can be found
    [here](https://github.com/telefonicaid/fiware-orion/blob/master/src/lib/logMsg/traceLevels.h)
    (as a C struct).
-   **-fg**. Runs broker in foreground (useful for debugging). Log output is printed on standard output
    (in addition to the log file, but using a simplified format).
-   **-localIp <ip>**. Specifies on which IP interface the broker
    listens to. By default it listens to all the interfaces.
-   **-pidpath <pid_file>**. Specifies the file to store the PID of the
    broker process.
-   **-httpTimeout <interval>**. Specifies the timeout in milliseconds
    for forwarding messages and for notifications.
-   **-reqTimeout <interval>**. Specifies the timeout in seconds
    for REST connections. Note that the default value is zero, i.e., no timeout (wait forever).
-   **-cprForwardLimit**. Maximum number of forwarded requests to Context Providers for a single client request
    (default is no limit). Use 0 to disable Context Providers forwarding completely.
-   **-corsOrigin <domain>**. Enables Cross-Origin Resource Sharing,
    specifing the allowed origin (use `__ALL` for `*`). More information about CORS support in Orion can be found
    in [the users manual](../user/cors.md).
-   **-corsMaxAge <time>**. Specifies the maximum time (in seconds) preflight requests are allowed to be cached. Defaults
    to 86400 if not set. More information about CORS support in Orion can be found in [the users manual](../user/cors.md).
-   **-reqMutexPolicy <all|none|write|read>**. Specifies the internal
    mutex policy. See [performance tuning](perf_tuning.md#mutex-policy-impact-on-performance) documentation
    for details.
-   **-subCacheIval**. Interval in seconds between calls to subscription cache refresh. A zero
    value means "no refresh". Default value is 60 seconds, apt for mono-CB deployments (see more details on
    the subscriptions cache in [this document](perf_tuning.md#subscription-cache)).
-   **-noCache**. Disables the context subscription cache, so subscriptions searches are
    always done in DB (not recommended but useful for debugging).
-   **-notificationMode** *(Experimental option)*. Allows to select notification mode, either:
    `transient`, `permanent` or `threadpool:q:n`. Default mode is `transient`.
    * In transient mode, connections are closed by the CB right after sending the notification.
    * In permanent connection mode, a permanent connection is created the first time a notification
      is sent to a given URL path (if the receiver supports permanent connections). Following notifications to the same
      URL path will reuse the connection, saving HTTP connection time.
    * In threadpool mode, notifications are enqueued into a queue of size `q` and `n` threads take the notifications
      from the queue and perform the outgoing requests asynchronously. Please have a look at the
      [thread model](perf_tuning.md#orion-thread-model-and-its-implications) section if you want to use this mode.
-   **-simulatedNotification**. Notifications are not sent, but recorded internally and shown in the
    [statistics](statistics.md) operation (`simulatedNotifications` counter). This is not aimed for production
    usage, but it is useful for debugging to calculate a maximum upper limit in notification rate from a CB
    internal logic point of view.
-   **-connectionMemory**. Sets the size of the connection memory buffer (in kB) per connection used internally
    by the HTTP server library. Default value is 64 kB.
-   **-maxConnections**. Maximum number of simultaneous connections. Default value is 1020, for legacy reasons,
    while the lower limit is 1 and there is no upper limit (limited by max file descriptors of the operating system).
-   **-reqPoolSize**. Size of thread pool for incoming connections. Default value is 0, meaning *no thread pool*.
-   **-inReqPayloadMaxSize**. Max allowed size for incoming requests payloads, in bytes. Default value is 1MB.
-   **-outReqMsgMaxSize**. Max allowed total size for request *outgoing message*, in bytes. Default value is 8MB.
-   **-statCounters**, **-statSemWait**, **-statTiming** and **-statNotifQueue**. Enable statistics
    generation. See [statistics documentation](statistics.md).
-   **-logSummary**. Log summary period in seconds. Defaults to 0, meaning *Log Summary is off*. Min value: 0. Max value: one month (3600 * 24 * 31 == 2678400 seconds).
    See [logs documentation](logs.md#summary-traces) for more detail.
-   **-relogAlarms**. To see *every* possible alarm-provoking failure in the log-file, even when an alarm is already active, use this option. See [logs documentation](logs.md#alarms)
    for more detail.
-   **-disableCustomNotifications**. Disabled NGSIv2 custom notifications. In particular:
    * `httpCustom` is interpreted as `http`, i.e. all sub-fields except `url` are ignored
    * No `${...}` macro substitution is performed.
-   **-logForHumans**. To make the traces to standard out formated for humans (note that the traces in the log file are not affected)
-   **-disableMetrics**. To turn off the 'metrics' feature. Gathering of metrics is a bit costly, as system calls and semaphores are involved.
    Use this parameter to start the broker without metrics overhead.
-   **-insecureNotif**. Allow HTTPS notifications to peers which certificate cannot be authenticated with known CA certificates. This is similar
    to the `-k` or `--insecure` parameteres of the curl command.

