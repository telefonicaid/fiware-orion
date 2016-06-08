# Management REST interface

## Log and Trace levels
Apart from the NGSI 9/10 interface, Orion Context Broker exposes a REST
API for management that allows to change the log level and the trace levels
(whose initial value is set using `-t` and `-logLevel` command line options).

To change the log level:

```
curl --request PUT <host>:<port>/admin/log?level=<fatal|error|warning|warn|info|debug|none>
```



To manage the trace level:

```
curl --request DELETE <host>:<port>/log/trace
curl --request DELETE <host>:<port>/log/trace/t1
curl --request DELETE <host>:<port>/log/trace/t1-t2
curl --request DELETE <host>:<port>/log/trace/t1-t2,t3-t4
curl --request GET <host>:<port>/log/trace
curl --request PUT <host>:<port>/log/trace/t1
curl --request PUT <host>:<port>/log/trace/t1-t2
curl --request PUT <host>:<port>/log/trace/t1-t2,t3-t4
```

'PUT-requests' overwrite the previous log settings. So, in order to ADD
a trace level, a GET /log/trace must be issued first and after that the
complete trace string to be sent in the PUT request can be assembled.


## Semaphores
Another useful (especially if the broker stops responding correctly) REST API is
the semaphore listing offered:


```
curl <host>:<port>/admin/sem
```

The response is a listing of information of all the broker's semaphores:

```
{
    "alarmMgr": {
        "status": "free"
    },
    "connectionContext": {
        "status": "free"
    },
    "connectionSubContext": {
        "status": "free"
    },
    "dbConnection": {
        "status": "free"
    },
    "dbConnectionPool": {
        "status": "free"
    },
    "logMsg": {
        "status": "free"
    },
    "request": {
        "status": "free"
    },
    "subCache": {
        "status": "free"
    },
    "timeStat": {
        "status": "free"
    },
    "transaction": {
        "status": "free"
    }
}
```

Short explanation of the semaphores:
* **alarmMgr**, protects the data of the Alarm Manager 
* **connectionContext**, protects the curl context for sending HTTP notifications/forwarded messages
* **connectionEndpoints**, protects the curl endpoints when sending HTTP notifications/forwarded messages.
                           Note that the value for this field is actually the number of taken individual
                           semaphores used for each connection.
                           If no individual semaphores are taken, the string "free" is used as value of this field.
                           If one or more individual semaphores are taken, then a string describing the number of
                           taken semaphores is used as value.

* **dbConnectionPool**, protects mongo connection pool
* **dbConnection**, protects the set of connections of the mongo connection pool
* **logMsg**, makes sure that not two messages are written simultaneously to the log-file
* **request**, makes sure there are not two simultaneous requests to mongodb 
* **subCache**, protects the Subscription Cache
* **timeStat**, protects the data for timing statistics
* **transaction**, protects the Transaction ID (for the log-file)

The information supplied for each of the semaphores is:
* status:  `free` or `taken`

[ For now only one item per semaphore but the idea is to add more information in the future ]
