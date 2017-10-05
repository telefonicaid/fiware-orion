# <a name="top"></a>Logs

* [Log file](#log-file)
* [Log format](#log-format)
* [Alarms](#alarms)
* [Summary traces](#summary-traces)
* [Log rotation](#log-rotation)

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

When Orion runs in foreground (i.e. with the `-fg` [CLI argument](cli.md)), it also prints the same log traces
(but in a simplified way) on the standard output.

The log level can be changed (and retrieved) in run-time, using the [admin API](management_api.md) exposed by Orion.

[Top](#top)

## Log format

The log format is designed to be processed by tools like
[Splunk](http://www.splunk.com/) or [Fluentd](http://www.fluentd.org/).

Each line in the log file is composed by several key-value fields,
separed by the pipe character (`|`). Example:

    time=2014-07-18T16:39:06.265Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1217]:main | msg=Orion Context Broker is running
    time=2014-07-18T16:39:06.266Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=MongoGlobal.cpp[122]:mongoConnect | msg=Successful connection to database
    time=2014-07-18T16:39:06.266Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1055]:mongoInit | msg=Connected to mongo at localhost:orion
    time=2014-07-18T16:39:06.452Z | lvl=INFO | corr=N/A | trans=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[1290]:main | msg=Startup completed
    ...
    time=2014-07-18T16:39:22.920Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=pending | srv=pending | subsrv=pending | comp=Orion | op=rest.cpp[615]:connectionTreat | msg=Starting transaction from 10.0.0.1:v1/v1/updateContext
    time=2014-07-18T16:39:22.922Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[1499]:processContextElement | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.922Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[1318]:createEntity | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.923Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[811]:addTriggeredSubscriptions | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.923Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=MongoCommonUpdate.cpp[811]:addTriggeredSubscriptions | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:22.923Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000001 | from=10.0.0.1 | srv=s1 | subsrv=/A | comp=Orion | op=rest.cpp[745]:connectionTreat | msg=Transaction ended
    ...
    time=2014-07-18T16:39:35.415Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000002 | from=pending | srv=pending | subsrv=pending | comp=Orion | op=rest.cpp[615]:connectionTreat | msg=Starting transaction from 10.0.0.2:48373/v1/queryContext
    time=2014-07-18T16:39:35.416Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000002 | from=10.0.0.2 | srv=s1 | subsrv=/A | comp=Orion | op=MongoGlobal.cpp[877]:entitiesQuery | msg=Database Operation Successful (...)
    time=2014-07-18T16:39:35.416Z | lvl=INFO | corr=2b60beba-fff5-11e5-bc30-643150a45f86 | trans=1405694346-265-00000000002 | from=10.0.0.2 | srv=s1 | subsrv=/A | comp=Orion | op=rest.cpp[745]:connectionTreat | msg=Transaction ended
    ...
    time=2014-07-18T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[968]:sigHandler | msg=Signal Handler (caught signal 2)
    time=2014-07-18T16:44:53.541Z | lvl=INFO | corr=N/A | trans=N/A | corr=N/A | from=N/A | srv=N/A | subsrv=N/A | comp=Orion | op=contextBroker.cpp[974]:sigHandler | msg=Orion context broker exiting due to receiving a signal

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
        highlight the progress of Orion.
    -   DEBUG: This level designates fine-grained informational events
        that are most useful to debug an application. Only shown when
        tracelevels are in use (set with the -t command line option.
    -   SUMMARY: This is a special level used by log summary traces,
        enabled with the `-logSummary` CLI option. Have a look at [the
        section on summary traces](#summary-traces) for details.
-   **corr (correlator id)**. Can be either "N/A" (for log messages
    "out of transaction", e.g. log lines corresponding to Orion Context
    Broker startup), or it is a string in the UUID format.
    An example:
      "550e8400-e29b-41d4-a716-446655440000".
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
-   **srv**. Service associated to the transaction, or "pending" if the
    transaction has started but the service has not been yet obtained.
-   **subsrv**. Subservice associated to the transaction, or "pending" if the
    transaction has started but the subservice has not been yet obtained.
-   **comp (component)**. Current version always uses "Orion" in
    this field.
-   **op**. The function in the source code that generated the
    log message. This information is useful for developers only.
-   **msg (message)**. The actual log message. The text of the message
    include the name of the file and line number generating the trace
    (this information is useful mainly for Orion developers).

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
