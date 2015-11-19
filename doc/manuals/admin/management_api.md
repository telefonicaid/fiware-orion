# Management REST interface

Apart from the NGSI 9/10 interface, Orion Context Broker exposes a REST
API for management that allows to change the trace and verbosity levels
(which initial value is set using "-t" command line options).

In order to manage the trace level:

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

'PUT-requests' overwrites the previous log settings. So, in order to ADD
a trace level, a GET /log/trace must be issued first and after that the
complete trace string to be sent in the PUT request can be assembled.
