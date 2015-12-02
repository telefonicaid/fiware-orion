#<a name="top"></a>Logs

* [Log file](#log-file)
* [Log format](#log-format)
* [Error and warning types](#error-and-warning-types)
* [Log rotation](#log-rotation)

## Log file

The default log file is `/tmp/contextBroker.log`. Remember that the directory where the log file is stored (`/tmp` by default) can be changed using the `--logDir` command line option.

When starting the Orion context broker, if a previous log file exist:

-   If **-logAppend** is used, then the log is appended to the
    existing file.
-   If **-logAppend** is not used, then the existing file is renamed,
    appending the text ".old" to its name.

The `-logLevel` option allows to chose which error messages will be printed in the log:

- NONE: no log at all
- ERROR: only ERROR messages are logged
- WARNING: WARNING and ERROR messages are logged
- INFO: INFO, WARNING and ERROR messages are logged
- DEBUG: DEBUG, INFO, WARNING and ERROR messages are logged

When Orion runs in foreground (i.e. with the `-fg` [CLI argument](cli.md)), it also prints the same log traces
(but in a simplified way) on the standard output.

[Top](#top)

## Log format

The log format is designed to be processed by tools like
[Splunk](http://www.splunk.com/) or [Fluentd](http://www.fluentd.org/).

Each line in the log file is composed by several key-value fields,
separed by the pipe character (`|`). Example:

    time=2014-07-18T16:39:06.265CEST | lvl=INFO | trans=N/A | function=main | comp=Orion | msg=contextBroker.cpp[1217]: Orion Context Broker is running
    time=2014-07-18T16:39:06.266CEST | lvl=INFO | trans=N/A | function=mongoConnect | comp=Orion | msg=MongoGlobal.cpp[122]: Successful connection to database
    time=2014-07-18T16:39:06.266CEST | lvl=INFO | trans=N/A | function=mongoInit | comp=Orion | msg=contextBroker.cpp[1055]: Connected to mongo at localhost:orion
    time=2014-07-18T16:39:06.341CEST | lvl=INFO | trans=N/A | function=recoverOntimeIntervalThreads | comp=Orion | msg=MongoGlobal.cpp[421]: Database Operation Successful ({ conditions.type: "ONTIMEINTERVAL" })
    time=2014-07-18T16:39:06.452CEST | lvl=INFO | trans=N/A | function=main | comp=Orion | msg=contextBroker.cpp[1290]: Startup completed
    ...
    time=2014-07-18T16:39:22.920CEST | lvl=INFO | trans=1405694346-265-00000000001 | function=connectionTreat | comp=Orion | msg=rest.cpp[615]: Starting transaction from 0.0.0.0:48372/NGSI10/updateContext
    time=2014-07-18T16:39:22.922CEST | lvl=INFO | trans=1405694346-265-00000000001 | function=processContextElement | comp=Orion | msg=MongoCommonUpdate.cpp[1499]: Database Operation Successful (...)
    time=2014-07-18T16:39:22.922CEST | lvl=INFO | trans=1405694346-265-00000000001 | function=createEntity | comp=Orion | msg=MongoCommonUpdate.cpp[1318]: Database Operation Successful (...)
    time=2014-07-18T16:39:22.923CEST | lvl=INFO | trans=1405694346-265-00000000001 | function=addTriggeredSubscriptions | comp=Orion | msg=MongoCommonUpdate.cpp[811]: Database Operation Successful (...)
    time=2014-07-18T16:39:22.923CEST | lvl=INFO | trans=1405694346-265-00000000001 | function=addTriggeredSubscriptions | comp=Orion | msg=MongoCommonUpdate.cpp[811]: Database Operation Successful (...)
    time=2014-07-18T16:39:22.923CEST | lvl=INFO | trans=1405694346-265-00000000001 | function=connectionTreat | comp=Orion | msg=rest.cpp[745]: Transaction ended
    ...
    time=2014-07-18T16:39:35.415CEST | lvl=INFO | trans=1405694346-265-00000000002 | function=connectionTreat | comp=Orion | msg=rest.cpp[615]: Starting transaction from 0.0.0.0:48373/NGSI10/queryContext
    time=2014-07-18T16:39:35.416CEST | lvl=INFO | trans=1405694346-265-00000000002 | function=entitiesQuery | comp=Orion | msg=MongoGlobal.cpp[877]: Database Operation Successful (...)
    time=2014-07-18T16:39:35.416CEST | lvl=INFO | trans=1405694346-265-00000000002 | function=connectionTreat | comp=Orion | msg=rest.cpp[745]: Transaction ended
    ...
    time=2014-07-18T16:41:34.254CEST | lvl=INFO | trans=N/A | function=mongoGetContextSubscriptionInfo | comp=Orion | msg=mongoOntimeintervalOperations.cpp[58]: Database Operation Successful (findOne _id: 53c8c29d09b4f403b165ff61)
    time=2014-07-18T16:41:34.255CEST | lvl=INFO | trans=N/A | function=entitiesQuery | comp=Orion | msg=MongoGlobal.cpp[877]: Database Operation Successful (...)
    time=2014-07-18T16:41:34.255CEST | lvl=INFO | trans=1405694174-239-00000000015 | function=doNotification | comp=Orion | msg=onTimeIntervalThread.cpp[89]: Starting transaction to http://localhost:1028/accumulate
    time=2014-07-18T16:41:34.255CEST | lvl=INFO | trans=1405694174-239-00000000015 | function=mongoUpdateCsubNewNotification | comp=Orion | msg=mongoOntimeintervalOperations.cpp[175]: Database Operation Successful (...)
    time=2014-07-18T16:41:34.255CEST | lvl=INFO | trans=1405694174-239-00000000015 | function=sendHttpSocket | comp=Orion | msg=clientSocketHttp.cpp[376]: Notification Successfully Sent to 10.0.0.1:1028/accumulate
    time=2014-07-18T16:41:34.255CEST | lvl=INFO | trans=1405694174-239-00000000015 | function=sendHttpSocket | comp=Orion | msg=clientSocketHttp.cpp[377]: Transaction ended
    ...
    time=2014-07-18T16:44:53.541CEST | lvl=INFO | trans=N/A | function=sigHandler | comp=Orion | msg=contextBroker.cpp[968]: Signal Handler (caught signal 2)
    time=2014-07-18T16:44:53.541CEST | lvl=INFO | trans=N/A | function=sigHandler | comp=Orion | msg=contextBroker.cpp[974]: Orion context broker exiting due to receiving a signal

The different fields in each line are as follows:

-   **time**. A timestamp corresponding to the moment in which the log
    line was generated.
-   **lvl (level)**. There are four level types:
    -   ERROR: This level designates error events. There is a severe
        problem that should be fixed. A subclass of ERROR is FATAL,
        which designates very severe error events that will
        presumably lead the application to abort. The process can no
        longer work.
    -   WARNING: This level designates potentially harmful situations.
        There is a minor problem that should be fixed.
    -   INFO: This level designates informational messages that
        highlight the progress of Orion.
    -   DEBUG: This level designates fine-grained informational events
        that are most useful to debug an application. Only shown when
        tracelevels are in use (set with the -t command line option.
-   **trans (transaction id)**. Can be either "N/A" (for log messages
    "out of transaction", as the ones corresponding to Orion Context
    Broker startup) or a string in the
    format "1405598120-337-00000000001". The transaction id generation
    logic ensures that every transaction id is unique, also for Orion
    instances running in different VMs (which is useful in the case you
    are aggregating logs from different sources), except if they are
    started in the exactly same millisecond. There are two types of
    transactions in Orion:
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
-   **function**. The function in the source code that generated the
    log message. This information is useful for developers only.
-   **comp (component)**. Current version always uses "Orion" in
    this field.
-   **msg (message)**. The actual log message. The text of the message
    include the name of the file and line number generating the trace
    (this information is useful mainly for Orion developers).

[Top](#top)
    
## Error and warning types

-   FATAL Fatal Error. This error causes the Context Broker to stop.
    This type of errors should be reported to the Orion development team
    using the appropriate channel (StackOverflow with the
    tag "fiware-orion").

    time=2014-07-18T08:33:19.389CEST | lvl=FATAL | trans=N/A | function=restStart | comp=Orion | msg=rest.cpp[861]: Fatal Error (error starting REST interface)

-   ERROR Runtime Error. This error may cause the Context Broker
    to fail. This type of errors should be reported to the Orion
    development team using the appropriate channel (StackOverflow with
    the tag "fiware-orion").

    time=2014-07-18T08:33:19.389CEST | lvl=ERROR | trans=1405607677-567-00000000002 | function=sendHttpSocket | comp=Orion | msg=clientSocketHttp.cpp[262]: Runtime Error (HTTP request to send is too large: 2048000000 bytes)

-   ERROR Database Error. This error is due to problems with
    the database. This type of errors are associated to alarms (see
    section on alarms).

    time=2014-07-21T18:38:45.897CEST | lvl=ERROR | trans=1405960644-549-00000000004 | function=entitiesQuery | comp=Orion | msg=MongoGlobal.cpp[886]: Database Error (...)

-   WARNING Bad input. This error is due to problems in the API usage.
    This type of warnings are associated to alarms (see section
    on alarms).

    time=2014-07-18T08:42:29.925CEST | lvl=WARNING | trans=1405665739-109-00000000001 | function=badRequest | comp=Orion | msg=badRequest.cpp[46]: Bad Input (service '/versionxxxx' not found)

-   WARNING notification connection error. This type of warnings are due
    to problems connecting to notification receivers. They are
    associated to alarms (see section on alarms).

    time=2014-07-18T16:36:44.245CEST | lvl=WARNING | trans=1405694174-239-00000000004 | function=socketHttpConnect | comp=Orion | msg=clientSocketHttp.cpp[98]: Notification failure for localhost:1028 (connect: Connection refused)

Alarm conditions
----------------

| Alarm ID   | Severity   |   Detection strategy                                                                          | Stop condition                                                                                                                                                                                                                                           | Description                                                                                                  | Action
|:---------- |:----------:|:--------------------------------------------------------------------------------------------- |:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |:------------------------------------------------------------------------------------------------------------ |:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 1          | CRITICAL   | A FATAL trace is found                                                                        | The following INFO text trace appear: "Startup completed""                                                                                                                                                                                               | A problem has happend at Orion Context Broker startup. The FATAL 'msg' field details the particular problem. | Solving the issue that is precluding Orion Context Broker startup, e.g. if the problem was due to the listening port is being used the solution would be either changing Orion listening port or ending the process that is already using the port.
| 2          | CRITICAL   | The following ERROR text appears in the 'msg' field: "Database Error (...)"                   | The following INFO text appears in the 'msg' field: "Database Operation Successful (...)"                                                                                                                                                                | Database Error. The text within parenthesis in the 'msg' field containts the detailed information.           | Orion is unable to access MongoDB database and/or MongoDB database is not working properly. Check database connection and database status. Once repaired the database and/or its connection from Orion, the problem should disapear (Orion service restart is not needed). No specific fixing action has to be done at Orion Context Broker service.
| 3          | CRITICAL   | The following ERROR text appears in the 'msg' field: "Runtime Error (...)"                    | N/A                                                                                                                                                                                                                                                      | Runtime Error. The text within parenthesis in the 'msg' field containts the detailed information.            | Restart Orion Context Broker. If it persits (e.g. new Runtime Errors appear within the next hour), scale up the problem to development team.
| 4          | WARNING    | The following WARNING text appear in the 'msg' field: "Bad Input (...)"                       | A transaction comming from the same client (identified by IP and port) is processed correctly, without any "Bad Input (...)" message between the "Starting transaction from" message to the "Transaction ended" message for that particular transaction. | Bad Input. The text within parenthesis in the 'msg' field containts the detailed information.                | The client has sent a request to Orion that doesn't conform to the API specification, e.g. bad URL, bad payload, syntax/semantic error in the request, etc. Depending on the IP, it could correspond to a platform client or to an external third-party client. In any case, the client owner should be reported in order to know and fix the issue. No specific fixing action has to be done at Orion Context Broker service.
| 5          | WARNING    | The following WARNING text appears in the 'msg' field: "Notification failure for <url> (...)" | The following INFO text appears in the 'msg' field: "Notification Successfully Sent to <url>", where <url> is the same one that triggered the alarm.                                                                                                     | Notification Failure. The text within parenthesis in the 'msg' field containts the detailed information.     | Orion is trying to send the notification to a given receiver and some problem has happended. It could be due to a problem on the network connectivy or on the receiver e.g. the receiver is down. In the second case, the owner of the receiver of the owner should be reported. No specific fixing action has to be done at Orion Context Broker service.

[Top](#top)

## Log rotation

Logrotate is installed as RPM dependency along with contextBroker. The
system is configure to rotate every day and whenever the log file size
is greater than 100MB (checked very 30 minutes by default):

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
settings. In this sense, take into account that every transaction can
consume around 1-2 KB (measured with Orion 0.14.1), e.g. if your
expected load is around 200 TPS, then your log file will grow 200-400 KB
per second.

[Top](#top)
