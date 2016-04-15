# Management REST interface

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
