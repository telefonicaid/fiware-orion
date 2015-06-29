# Management REST interface

Apart from the NGSI 9/10 interface, Orion Context Broker exposes a REST
API for management that allows to change the trace and verbosity levels
(which initial value is set using "-t" command line options).

In order to manage the trace level:

```
curl --request DELETE <host>:<port>/log/trace
curl --request DELETE <host>:<port>/log/trace/t1
curl --request DELETE <host>:<port>/log/trace/t1-t2
curl --request DELETE <host>:<port>/log/trace/t1-t2,t3-t4
curl --request GET <host>:<port>/log/trace
curl --request PUT <host>:<port>/log/trace/t1
curl --request PUT <host>:<port>/log/trace/t1-t2
curl --request PUT <host>:<port>/log/trace/t1-t2,t3-t4

'PUT-requests' overwrites the previous log settings. So, in order to ADD
a trace level, a GET /log/trace must be issued first and after that the
complete trace string to be sent in the PUT request can be assembled.

## Statistics

The Orion context broker maintains a set of counters for the incoming
messages and such. The information is accessed via REST, of course:

` curl `<host>`:`<port>`/statistics`

A sample <i>statistics</i> response:

```
 <orion>
   <xmlRequests>16</xmlRequests>
   <jsonRequests>1</jsonRequests>
   <registrations>5</registrations>
   <discoveries>10</discoveries>
   <statisticsRequests>2</statisticsRequests>
 </orion>
```

To reset the statistics, the verb DELETE is used:

```
curl -X DELETE `<host>`:`<port>`/statistics
```

Resetting the statistics counters yields the following response:

```
<orion>
  <message>All statistics counter reset</message>
</orion>
```

There are a lot of counters, but only those that have been used since
the last reset are included in the response to the *statistics*
REST request.

The `-mutexTimeStat` CLI parameter activates recording of waiting time in the different internal semaphores, e.g:

```
      <requestSemaphoreWaitingTime>0.000000000</requestSemaphoreWaitingTime>
      <dbConnectionPoolWaitingTime>0.000000230</dbConnectionPoolWaitingTime>
      <transactionSemaphoreWaitingTime>0.000001050</transactionSemaphoreWaitingTime>
```
