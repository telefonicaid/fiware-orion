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


## Sempahores
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
* alarmMgr               Protects the data of the Alarm Manager 
* connectionContext      Protects the curl context for sending HTTP notifications/forwarded messages
* connectionSubContext   Protects the curl context for sending HTTP notifications/forwarded messages
* dbConnectionPool       Protects mongo connection pool
* dbConnection           Protects the set of connections of the mongo connection pool
* logMsg                 Makes sure that not two messages are written simultaneously to the log-file
* request                Makes sure there are not two simultaneous requests to mongodb 
* subCache               Protects the Subscription Cache
* timeStat               Protects the data for timing statistics
* transaction            Protects the Transaction ID (for the log-file)

The information supplied for each of the semaphores is:
* status:  free or taken

[ For now only one item per semaphore but the idea is to add more information in the future ]
