# <a name="top"></a>Logs

* [Log file](#log-file)
* [Log format](#log-format)
* [INFO level in detail](#info-level-in-detail)
* [Alarms](#alarms)
* [Summary traces](#summary-traces)
* [Log rotation](#log-rotation)
* [Log examples for notification transactions](#log-examples-for-notification-transactions)
* [Command line options related with logs](#command-line-options-related-with-logs)

## Log file

The default log file is `/tmp/contextBroker.log`. Remember that the directory where the log file is stored (`/tmp` by default) can be changed using the `-logDir` command line option.

When starting the Orion context broker, if a previous log file exists:

-   If **-logAppend** is used, then the log is appended to the existing file.
-   If **-logAppend** is not used, then the existing file is renamed, appending the text ".old" to its name.

The `-logLevel` option allows to choose which error messages are printed in the log:

- NONE: no log at all
- FATAL: only FATAL ERROR messages are logged
- ERROR: only ERROR messages are logged
- WARN (default): WARN and ERROR messages are logged
- INFO: INFO, WARN and ERROR messages are logged
- DEBUG: DEBUG, INFO, WARN and ERROR messages are logged

When Orion runs in foreground (i.e. with the `-fg` [CLI argument](cli.md)), it also prints the same log traces on the standard output.

The log level can be changed (and retrieved) in run-time, using the [admin API](management_api.md) exposed by Orion.

[Top](#top)

## Log format

The log format is designed to be processed by tools like
[Splunk](http://www.splunk.com/) or [Fluentd](http://www.fluentd.org/).

Each line in the log file is composed by several key-value fields,
separed by the pipe character (`|`). Example:

    time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[986]:main | msg=start command line <contextBroker -fg -logLevel INFO>
    time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[873]:logEnvVars | msg=env var ORION_PORT (-port): 1026
    time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1054]:main | msg=Orion Context Broker is running
    time=2020-10-26T09:45:17.301Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=MongoGlobal.cpp[247]:mongoInit | msg=Connected to mongo at localhost/orion (poolsize: 10)
    time=2020-10-26T09:45:17.304Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1180]:main | msg=Startup completed
    ...
    time=2020-10-26T10:27:02.619Z | lvl=INFO | corr=c99e4592-1775-11eb-ad30-000c29df7908 | trans=1603707992-318-00000000001 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[79]:logInfoRequestWithoutPayload | msg=Request received: GET /v2/entities?type=Device, response code: 200
    ...
    time=2020-10-26T10:32:41.724Z | lvl=INFO | corr=93bdc5b4-1776-11eb-954d-000c29df7908 | trans=1603708355-537-00000000002 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities, request payload (34 bytes): {  "id": "Room1",  "type": "Room"}, response code: 201
    ...
    time=2020-10-26T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[968]:sigHandler | msg=Signal Handler (caught signal 2)
    time=2020-10-26T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[974]:sigHandler | msg=Orion context broker exiting due to receiving a signal

The different fields in each line are as follows:

-   **time**. A timestamp corresponding to the moment in which the log
    line was generated in [ISO8601](https://es.wikipedia.org/wiki/ISO_8601) format.
    Orion prints timestamps in UTC format.
-   **lvl (level)**. There are six levels:
    -   FATAL: This level designates severe error events
        that lead the application to exit.
        The process can no longer work.
    -   ERROR: This level designates error events.
        There is a severe problem that must be fixed.
    -   WARN: This level designates potentially harmful situations.
        There is a minor problem that should be fixed.
    -   INFO: This level designates informational messages that
        highlight the progress of Orion. More info on this level
        in [this specific section](#info-level-in-detail).
    -   DEBUG: This level designates fine-grained informational events
        that are most useful to debug an application. Only shown when
        tracelevels are in use (set with the `-t` command line option.
    -   SUMMARY: This is a special level used by log summary traces,
        enabled with the `-logSummary` CLI option. Have a look at [the
        section on summary traces](#summary-traces) for details.
-   **corr (correlator id)**. Can be either "N/A" (for log messages
    "out of transaction", e.g. log lines corresponding to Orion Context
    Broker startup), or it is a string in the UUID format.
    An example: `550e8400-e29b-41d4-a716-446655440000`. Sometimes, this UUID string
    may include a sufix, e.g. `550e8400-e29b-41d4-a716-446655440000; cbnotif=2`
    or `550e8400-e29b-41d4-a716-446655440000; cbfwd=1` (more information on this
    in the [section about the INFO level](#info-level-in-detail)).
    This 'correlator id' is either transferred from an incoming request,
    or, if the incoming request doesn't carry any HTTP header "Fiware-Correlator",
    the correlator is generated by the Orion context broker and then used in
    the log file (as well as sent as HTTP header in forwarding messages, notifications and responses).
    The correlator id is a common identifier among all applications involved in the
    'message chain' for one specific request.
-   **trans (transaction id)**. Can be either "N/A" (for log messages
    "out of transaction", as the ones corresponding to Orion Context
    Broker startup) or a string in the format "1405598120-337-00000000001".
    The transaction id generation logic ensures that every transaction id is unique,
    also for Orion instances running in different VMs (which is useful in the case
    you are aggregating logs from different sources), except if they are
    started in the exact same millisecond. 
    Note that transaction ID is independent of correlator ID.
    Transaction ID has a local nature while correlator ID is meaningful end-to-end,
    involving other software componentes apart from Context Broker itself.
    There are two types of transactions in Orion:
    -   The ones initiated by an external client invoking the REST API
        exposed by Orion. The first message on these transactions use
        the pattern "Starting transaction **from** *url*", where *url*
        includes the IP and port of the client invoking the operation
        and the path is the actual operation invoked at Orion. The last
        message on these transaction is "Transaction ended".
    -   The ones that Orion initiates when it sends a notification. The
        first message on these transactions use the pattern "Starting
        transaction **to** *url*", where *url* is the URL used in the
        reference element of the subscription, i.e. the URL of the
        callback to send the notification. The last message of both
        transaction types is "Transaction ended".
-   **from**. Source IP of the HTTP request associated to the transaction, except
    if the request includes `X-Forwarded-For` header (which overrides the former)
    or `X-Real-IP` (which overrides `X-Forwarded-For` and source IP).
-   **srv**. Service associated to the transaction. If request didn't include
    a service (i.e. `fiware-service` header was missing) then `<none>` is used.
-   **subsrv**. Subservice associated to the transaction. If request didn't include
    subservice (i.e. `fiware-servicepath` header was missing) then `<none>` is used.
-   **comp (component)**. Current version always uses "Orion" in
    this field.
-   **op**. The function in the source code that generated the
    log message. This information is useful for developers only.
-   **msg (message)**. The actual log message. The text of the message
    include the name of the file and line number generating the trace
    (this information is useful mainly for Orion developers).

[Top](#top)  

## INFO level in detail

**NOTE:** the INFO level was redesigned in Orion version 2.5.0 so some traces has
been removed from the sake of simplicity. If you want to see those old INFO traces,
use the trace level 240 in DEBUG mode, (e.g. `-logLevel DEBUG -t 240`).

At boot time, INFO level shows traces about how Orion has been started (both CLI
and environment vars) and information about the connection to the MongoDB database, e.g:

```
time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[986]:main | msg=start command line <contextBroker -fg -logLevel INFO>
time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[873]:logEnvVars | msg=env var ORION_PORT (-port): 1026
time=2020-10-26T09:45:17.225Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1054]:main | msg=Orion Context Broker is running
time=2020-10-26T09:45:17.301Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=MongoGlobal.cpp[247]:mongoInit | msg=Connected to mongo at localhost/orion (poolsize: 10)
time=2020-10-26T09:45:17.304Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1180]:main | msg=Startup completed
```

At runtime, the INFO level shows relevant information about client request, notifications and
forwarded requests. In particular:

* For each client request *without payload*, Orion shows a trace like this when the processing of the
  request finishes:

```
time=2020-10-26T10:27:02.619Z | lvl=INFO | corr=c99e4592-1775-11eb-ad30-000c29df7908 | trans=1603707992-318-00000000001 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[79]:logInfoRequestWithoutPayload | msg=Request received: GET /v2/entities?type=Device, response code: 200
```

* For each client request *with paylaod*, Orion shows a trace like this when the processing of the
  request finishes:

```
time=2020-10-26T10:32:41.724Z | lvl=INFO | corr=93bdc5b4-1776-11eb-954d-000c29df7908 | trans=1603708355-537-00000000002 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities, request payload (34 bytes): {  "id": "Room1",  "type": "Room"}, response code: 201
```

* For each notification sent, Orion shows a trace like this. Note that Orion adds the `cbnotif=` suffix
  to the "root correlator" associated to the client request (in this example, the client request uses
  `corr=87f708a8-1776-11eb-b327-000c29df7908`). The value of this suffix is an auto increment counter (starting
  with 1 for the first notification) so every notification triggered by the same update has an strictly
  different value for the correlator.

```
time=2020-10-26T10:32:22.145Z | lvl=INFO | corr=87f708a8-1776-11eb-b327-000c29df7908; cbnotif=1 | trans=1603707992-318-00000000003 | from=0.0.0.0 | srv=s1| subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f914177334436ea590f6edb): POST localhost:1028/accumulate, response code: 200
```

* For each forwarded request to a [Context Provider](../user/context_providers.md) (either queries or updates),
  Orion shows a trace like this. Note that Orion adds the `cbfwd=` suffix  to the "root correlator" associated
  to the client request (in this example, the client request uses `eabce3e2-149f-11eb-a2e8-000c29df7908`). The value
  of this suffix is an auto increment counter (starting with 1 for the first forwarded request) so every forwarded
  request triggered by the same update has a strictly different value for the correlator.

```
time=2020-10-22T19:51:03.565Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=1 | trans=1603396258-520-00000000007 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da0697f): POST http://localhost:9801/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A1"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A1":{"type":"Text","value":"A1 in CP1","metadata":{}}}], response code: 200
```

Some additional considerations:

* The `-logInfoPayloadMaxSize` setting is used to specify the maximum size that the payloads in the
  above traces may have. If the payload overpasses this limit, then only the first `-logInfoPayloadMaxSize`
  bytes are printed (and an ellipsis in the form of `(...)` is shown). Default value: 5 Kbytes.
* The status code in notifications and forwarding traces can be either a number (corresponding to the
  HTTP response code of the notification or forwarded request) or a string when some connectivity problem
  occurs. For instance:

```
time=2020-10-26T10:32:22.145Z | lvl=INFO | corr=87f708a8-1776-11eb-b327-000c29df7908; cbnotif=1 | trans=1603707992-318-00000000003 | from=0.0.0.0 | srv=s1| subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f914177334436ea590f6edb): POST localhost:1028/accumulate, response code: Couldn't connect to server
```

* When a client request triggers forwarding to Context Providers, a `Starting forwarding for <client request URL>`
  trace is printed before starting with the first forwarded request. Thus, a full forwarding block (e.g. a client query resulting in querying 5 context providers for individual attributes of the entity) would be as follows. Note all
  them use the same root correlator and the `cbfwd=1` to `cbfwd=5` suffixes in the log traces associated to the
  forwarded requests.

```
time=2020-10-22T19:51:03.556Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908 | trans=1603396258-520-00000000006 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[146]:logInfoFwdStart | msg=Starting forwarding for GET /v2/entities/E1?type=T1
time=2020-10-22T19:51:03.565Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=1 | trans=1603396258-520-00000000007 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da0697f): POST http://localhost:9801/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A1"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A1":{"type":"Text","value":"A1 in CP1","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.573Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=2 | trans=1603396258-520-00000000008 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06980): POST http://localhost:9802/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A2"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A2":{"type":"Text","value":"A2 in CP2","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.584Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=3 | trans=1603396258-520-00000000009 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06981): POST http://localhost:9803/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A3"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A3":{"type":"Text","value":"A3 in CP3","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.593Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=4 | trans=1603396258-520-00000000010 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06982): POST http://localhost:9804/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A4"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A4":{"type":"Text","value":"A4 in CP4","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.601Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908; cbfwd=5 | trans=1603396258-520-00000000011 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[212]:logInfoFwdRequest | msg=Request forwarded (regId: 5f91e2a719595ac73da06983): POST http://localhost:9805/v2/op/query, request payload (53 bytes): {"entities":[{"id":"E1","type":"T1"}],"attrs":["A5"]}, response payload (80 bytes): [{"id":"E1","type":"T1","A5":{"type":"Text","value":"A5 in CP5","metadata":{}}}], response code: 200
time=2020-10-22T19:51:03.602Z | lvl=INFO | corr=eabce3e2-149f-11eb-a2e8-000c29df7908 | trans=1603396258-520-00000000006 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[79]:logInfoRequestWithoutPayload | msg=Request received: GET /v2/entities/E1?type=T1, response code: 200
```

[Top](#top)  

## Alarms

Alarm conditions:

| Alarm ID   | Severity   |   Detection strategy                                                                                              | Stop condition                                                                                                                                                                                                                            | Description                                                                                                   | Action
|:---------- |:----------:|:----------------------------------------------------------------------------------------------------------------- |:----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |:------------------------------------------------------------------------------------------------------------- |:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 1          | CRITICAL   | A FATAL trace is found                                                                                            | N/A                                                                                                                                                                                                                                       | A problem has occurred at Orion Context Broker startup. The FATAL 'msg' field details the particular problem. | Solving the issue that is precluding Orion Context Broker startup, e.g. if the problem was due to the listening port is being used, the solution would be either changing Orion listening port or ending the process that is already using the port.
| 2          | CRITICAL   | The following ERROR text appears in the 'msg' field: "Runtime Error (`<detail>`)"                                 | N/A                                                                                                                                                                                                                                       | Runtime Error. The `<detail>` text containts the detailed information.                                        | Restart Orion Context Broker. If it persists (e.g. new Runtime Errors appear within the next hour), scale up the problem to development team.
| 3          | CRITICAL   | The following ERROR text appears in the 'msg' field: "Raising alarm DatabaseError: `<detail>`"                    | The following ERROR text appears in the 'msg' field: "Releasing alarm DatabaseError". Orion prints this trace when it detects that DB is ok again."                                                                                       | Database Error. The `<detail>` text contains the detailed information.                                        | Orion is unable to access MongoDB database and/or MongoDB database is not working properly. Check database connection and database status. Once the database is repaired and/or its connection to Orion, the problem should disappear (Orion service restart is not needed). No specific action has to be performed at Orion Context Broker service.
| 4          | WARNING    | The following WARN text appear in the 'msg' field: "Raising alarm BadInput `<ip>`: `<detail>`".                   | The following WARN text appears in the 'msg' field: "Releasing alarm BadInput `<ip>`", where <uip> is the same one that triggered the alarm. Orion prints this trace when it receives a correct request from that client.                 | Bad Input. The `<detail>` text contains the detailed information.                                             | The client has sent a request to Orion that doesn't conform to the API specification, e.g. bad URL, bad payload, syntax/semantic error in the request, etc. Depending on the IP, it could correspond to a platform client or to an external third-party client. In any case, the client owner should be reported in order to know and fix the issue. No specific action has to be performed at Orion Context Broker service.
| 5          | WARNING    | The following WARN text appears in the 'msg' field: "Raising alarm NotificationError `<url>`: `<detail>`".        | The following WARN text appears in the 'msg' field: "Releasing alarm NotificationError <url>", where <url> is the same one that triggered the alarm. Orion prints this trace when it successfully sent a notification to that URL.        | Notification Failure. The `<detail>`text contains the detailed information.                                   | Orion is trying to send the notification to a given receiver and some problem has occurred. It could be due to a problem with the network connectivity or on the receiver, e.g. the receiver is down. In the second case, the owner of the receiver of the notification should be reported. No specific action has to be performed at Orion Context Broker service.

By default, Orion only traces the origin (i.e. raising) and end (i.e. releasing) of an alarm, e.g:


```
time=... | lvl=ERROR | ... Raising alarm DatabaseError: collection: orion.entities - query(): { ... } - exception: ....
time=... | lvl=ERROR | ... Releasing alarm DatabaseError
...
time=... | lvl=WARN  | ... Raising alarm BadInput 10.0.0.1: JSON Parse Error: <unspecified file>(1): expected object or array
time=... | lvl=WARN  | ... Releasing alarm BadInput 10.0.0.1
...

time=... | lvl=WARN  | ... Raising alarm NotificationError localhost:1028/accumulate: (curl_easy_perform failed: Couldn't connect to server)
time=... | lvl=WARN  | ... Releasing alarm NotificationError localhost:1028/accumulate
```

This means that if the condition that triggered the alarm (e.g. a new invalid request from the 10.0.0.1 client) occurs again between the raising
and releasing alarm messages, it wouldn't be traced again. However, this behaviour can be changed using the `-relogAlarms` [CLI parameter](cli.md). When
`-relogAlarms` is used, a log trace is printed every time a triggering conditions happens, e.g:

```
time=... | lvl=WARN | ... Raising alarm BadInput 10.0.0.1: JSON parse error
time=... | lvl=WARN | ... Repeated BadInput 10.0.0.1: JSON parse error
time=... | lvl=WARN | ... Repeated BadInput 10.0.0.1: JSON parse error
time=... | lvl=WARN | ... Repeated BadInput 10.0.0.1: service '/v2/entitiesxx' not found
time=... | lvl=WARN | ... Releasing alarm BadInput 0.0.0.0
```

Log traces between "Raising" and "Releasing" messages use "Repeated" in the message text. Note that the details part of the message is not necesarily the same
in all traces, so re-logging alarms could be a means to get extra information when debugging problems. In the example above, it could correspond to a client
that after fixing the problem with the JSON payload now has a new problem with the URL of the Orion API operation.


[Top](#top)

## Summary traces

You can enable log summary traces with the `-logSummary` [CLI parameter](cli.md) which value is the summary reporting period in seconds. For
example `-logSummary 5` involves that summary traces will be print each 5 seconds (no matter which log level has been set with `-logLevel`).

Four traces are printed each time, as follows (the lines have been abbreviated, omitting some fields, for the sake of clarity):

```
time=... | lvl=SUMMARY | ... Transactions: 2345 (new: 45)
time=... | lvl=SUMMARY | ... DB status: ok, raised: (total: 0, new: 0), released: (total: 0, new: 0)
time=... | lvl=SUMMARY | ... Notification failure active alarms: 0, raised: (total: 0, new: 0), released: (total: 0, new: 0)
time=... | lvl=SUMMARY | ... Bad input active alarms: 5, raised: (total: 12, new: 1), released: (total: 7, new: 2)
```

* First line (Transactions) shows the current number of transactions and the new ones in the last summary reporting period.
* Second line is about [DB alarms](#alarms). It shows the current DB status (either "ok" or "erroneous"), the number of raised DB alarms (both total
  since Orion started and in the last summary reporting period) and the number of released DB alarms (both total since Orion started and in the last summary reporting period).
* Third line is about [notification failure alarms](#alarms). It shows the current number of active notification failure alarms, the number of
  raised notification failure alarms (both total since Orion started and in the last summary reporting period) and the number of released notification failure alarms
  (both total since Orion started and in the last summary reporting period).
* Fourth line is about [bad input alarms](#alarms). It shows the current number of bad input alarms, the number of raised bad input alarms (both total since Orion
  started and in the last summary reporting period) and the number of released bad input alarms (both total since Orion started and in the last summary reporting period).

[Top](#top)

## Log rotation

Logrotate is installed as an RPM dependency along with the contextBroker.
The system is configured to rotate once a day, or more, in case the log file size
exceeds 100MB (checked very 30 minutes by default):

-   For daily rotation: `/etc/logrotate.d/logrotate-contextBroker-daily`:
    which enables daily log rotation
-   For size-based rotation:
    -   `/etc/sysconfig/logrotate-contextBroker-size`: in addition to the
        previous rotation, this file ensures log rotation if the log
        file grows beyond a given threshold (100 MB by default)
    -   `/etc/cron.d/cron-logrotate-contextBroker-size`: which ensures the
        execution of etc/sysconfig/logrotate-contextBroker-size at a
        regular frecuency (default is 30 minutes)

Depending on your expected load you would need to adjust the default
settings. In this sense, take into account that in INFO log level every transaction can
consume around 1-2 KB (measured with Orion 0.14.1), e.g. if your
expected load is around 200 TPS, then your log file will grow 200-400 KB
per second.

[Top](#top)

## Log examples for notification transactions

This section illustrates some log examples corresponding to notification transactions.
Note this has been generated with Orion 2.5.0 release and although no big changes are
expected in the future, the exact log traces could be slightly different in newer versions.

For this test, ContextBroker was started this way:

```
contextBroker -fg -httpTimeout 10000 -logLevel INFO -notificationMode threadpool:100:10 -multiservice -subCacheIval 180
```

Successful sent (response code 200):

```
time=2020-10-26T14:48:37.192Z | lvl=INFO | corr=54393a44-179a-11eb-bb87-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000006 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e174b14e7532482ac794): POST localhost:1028/accumulate, response code: 200
```

Notification endpoint response with 400 (a WARN trace is printed):

```
time=2020-10-26T14:49:34.619Z | lvl=WARN | corr=7689f6ba-179a-11eb-ac4c-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000009 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=httpRequestSend.cpp[583]:httpRequestSendWithCurl | msg=Notification response NOT OK, http code: 400
time=2020-10-26T14:49:34.619Z | lvl=INFO | corr=7689f6ba-179a-11eb-ac4c-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000009 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e1fdb14e7532482ac795): POST localhost:1028/giveme400, response code: 400
```

Notification endpoint response with 404 (a WARN trace is printed):

```
time=2020-10-26T14:51:40.764Z | lvl=WARN | corr=c1b8e9c0-179a-11eb-9edc-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000012 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=httpRequestSend.cpp[583]:httpRequestSendWithCurl | msg=Notification response NOT OK, http code: 404
time=2020-10-26T14:51:40.764Z | lvl=INFO | corr=c1b8e9c0-179a-11eb-9edc-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000012 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e27cb14e7532482ac796): POST localhost:1028/giveme404, response code: 404
```

Notification endpoint response with 500 (a WARN trace is printed)

```
time=2020-10-26T14:53:04.246Z | lvl=WARN | corr=f37b5024-179a-11eb-9ce6-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000015 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=httpRequestSend.cpp[583]:httpRequestSendWithCurl | msg=Notification response NOT OK, http code: 500
time=2020-10-26T14:53:04.247Z | lvl=INFO | corr=f37b5024-179a-11eb-9ce6-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000015 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e2cfb14e7532482ac797): POST localhost:1028/giveme500, response code: 500

```

Endpoint not responding within 10 seconds timeout or some other connection error (alarm is raised in WARN level):

```
time=2020-10-26T14:54:15.996Z | lvl=WARN | corr=184b8b80-179b-11eb-9c52-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000018 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError localhost:1028/givemeDelay: notification failure for queue worker: Timeout was reached
time=2020-10-26T14:54:15.996Z | lvl=INFO | corr=184b8b80-179b-11eb-9c52-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000018 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e30db14e7532482ac798): POST localhost:1028/givemeDelay, response code: Timeout was reached
```

Endpoint in not responding port, e.g. localhost:9999 (alarm is raised in WARN log level):

```
time=2020-10-26T15:01:50.659Z | lvl=WARN | corr=2d3e4cfc-179c-11eb-b667-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000030 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError localhost:9999/giveme: notification failure for queue worker: Couldn't connect to server
time=2020-10-26T15:01:50.659Z | lvl=INFO | corr=2d3e4cfc-179c-11eb-b667-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000030 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e4deb14e7532482ac79c): POST localhost:9999/giveme, response code: Couldn't connect to server
```

Endpoint in unresolvable name, e.g. foo.bar.bar.com (alarm is raised in WARN log level):

```
time=2020-10-26T15:03:54.258Z | lvl=WARN | corr=769f8d8e-179c-11eb-960f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000033 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError foo.bar.bar.com:9999/giveme: notification failure for queue worker: Couldn't resolve host name
time=2020-10-26T15:03:54.258Z | lvl=INFO | corr=769f8d8e-179c-11eb-960f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000033 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e559b14e7532482ac79d): POST foo.bar.bar.com:9999/giveme, response code: Couldn't resolve host name
```

Endpoint in unreachable IP, e.g. 12.34.56.87 (alarm is raised in WARN log level):

```
time=2020-10-26T15:06:14.642Z | lvl=WARN | corr=c4a3192e-179c-11eb-ac8f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000036 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=AlarmManager.cpp[328]:notificationError | msg=Raising alarm NotificationError 12.34.56.78:9999/giveme: notification failure for queue worker: Timeout was reached
time=2020-10-26T15:06:14.642Z | lvl=INFO | corr=c4a3192e-179c-11eb-ac8f-000c29df7908; cbnotif=1 | trans=1603722272-416-00000000036 | from=0.0.0.0 | srv=s1 | subsrv=/A | comp=Orion | op=logTracing.cpp[63]:logInfoNotification | msg=Notif delivered (subId: 5f96e5dbb14e7532482ac79e): POST 12.34.56.78:9999/giveme, response code: Timeout was reached
```

[Top](#top)

## Command line options related with logs

Apart from the ones already described in this document (`-logDir`, `-logAppend`, `-logLevel`, `-t`, `-logInfoPayloadMaxSize`, `-relogAlarms` and `-logSummary`), the following command line options are related with logs:

* `-logForHumans`
* `-logLineMaxSize`

Please have a look to the [command line options documentation](cli.md) for more information about them.

[Top](#top)