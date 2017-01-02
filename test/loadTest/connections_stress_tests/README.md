## Connections Stress Test

The purpose of this test is to verify that ContextBroker works properly with a large number of
established connections.

### Pre-requirements

* Install [Remote Python Call (RPyC)](https://pypi.python.org/pypi/rpyc) and psutil in CB host:

```
sudo yum install python-devel python-psutil
sudo pip install rpyc psutil
```

* Build slow_listener (requires Go compiler):

```
go build slow_listener.go
chmod a+x slow_listener
```

### Test procedure

* Launch RPyC listener:

```
$ rpyc_classic.py
INFO:SLAVE/18812:server started on [0.0.0.0]:18812
```

* Launch slow_listener (e.g. 5 minutes delay, printing stats each 5 seconds). By default it
  listens on port 8090.

```
./slow_listener -delay 5m -stats 5s
```

* Check CB limits for max processes and open files. Note that the setting for the test makes the CB
  create 5000 threads for the notification pool (plus other threads dynamically created to deal
  with incoming connections) and the same number for file descriptors when creating connections.
  Thus a higher value (e.g. 8000) is recommended. The procedure can be a bit tricky, so it
  is described in [a specific section](#raising-system-limits-for-the-test).

* Launch CB as service. Use the following configuration in `/etc/sysconfig/contextBroker`:

```
BROKER_EXTRA_OPS="-reqMutexPolicy none -writeConcern 0 -httpTimeout 600000 -notificationMode threadpool:60000:5000 -statTiming -statSemWait -statCounters -statNotifQueue -multiservice -subCacheIval 5"
```

* Launch CB (alternative way, in foreground)

```
contextBroker -reqMutexPolicy none -writeConcern 0 -httpTimeout 600000 -notificationMode threadpool:60000:5000 -statTiming -statSemWait -statCounters -statNotifQueue -multiservice -subCacheIval 5 -fg
```

* Launch `python connections_stress_tests.py`. The script does the following steps:
  * Drops the database in mongo associated with the service
  * Creates 5000 subscriptions with subject.entities.idPattern: .*
  * Launches a single update, which triggers a notification per subscriptions (i.e. 5000 notifications). At this
    moment you will see the following trace in `slow_listener` output: `Received  0 Rate 0 r/s`
  * Launches for a given time (in minutes, see `-duration` param) a "GET /version" request per second and:
     * Reports that its response is correct.
     * Reports the number of established connections (if `-noConnectionInfo` param is used this column is ignored),
       both ESTABLISHED, CLOSE_WAIT and sum.
     * Reports the queue size into ContextBroker (if `-noQueueSize` param is used this column is ignored)


### Expected behaviour

The following sample output will be printed in **connections_stress_tests.log** file:

```
2016-12-19 17:01:45,191 | INFO | Test init: 2016-12-19T17:01:45.191521Z
2016-12-19 17:01:50,447 | INFO |  Reports each second:
2016-12-19 17:01:50,448 | INFO |  counter       version      queue   established   close wait    sum
2016-12-19 17:01:50,448 | INFO |                request      size    connections   connections   connections
2016-12-19 17:01:50,449 | INFO |  --------------------------------------------------------------------------------
2016-12-19 17:01:52,679 | INFO |  -------- 1 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:01:55,417 | INFO |  -------- 2 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:01:58,218 | INFO |  -------- 3 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:02:00,978 | INFO |  -------- 4 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:02:03,503 | INFO |  -------- 5 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:02:06,133 | INFO |  -------- 6 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
...
(After around 5 minutes, i.e., the delay configuration for slow_listener has passed and its output now
shows "Received  5000 Rate 0 r/s")
...
2016-12-19 17:07:13,115 | INFO |  -------- 159 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:07:15,078 | INFO |  -------- 160 -------- OK -------- 0 -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-19 17:07:17,491 | INFO |  -------- 161 -------- OK -------- 0 -------- 4643 ---------- 1670 -------- 6313 ----------
2016-12-19 17:07:20,036 | INFO |  -------- 162 -------- OK -------- 0 -------- 1119 ---------- 4501 -------- 5620 ----------
2016-12-19 17:07:22,057 | INFO |  -------- 163 -------- OK -------- 0 -------- 10 ---------- 5000 -------- 5010 ----------
2016-12-19 17:07:23,966 | INFO |  -------- 164 -------- OK -------- 0 -------- 10 ---------- 5000 -------- 5010 ----------
2016-12-19 17:07:25,779 | INFO |  -------- 165 -------- OK -------- 0 -------- 10 ---------- 5000 -------- 5010 ----------
2016-12-19 17:07:27,828 | INFO |  -------- 166 -------- OK -------- 0 -------- 10 ---------- 5000 -------- 5010 ----------
...
```

Note that 5010 corresponds to the 5000 outgoing notifications plus the 10 connections to DB (default
`-dbPoolSize`). After the delay, all the 5000 connections to slow_listener pass to CLOSE_WAIT
status, while the 10 connections corresponding to the DB pool stay in established status.

Similar findings are obtained with `netstat -nputan | grep contextBr`.

In this state, note that Orion is able to serve new incoming connections. You can check this running
(in parallel) several `telnet localhost 1026`. We will still get OK in the version column in the
script output. As we will see in the following section, failure in doing this is one of the symptoms
of a failing situation.

#### Failure pattern

The following failure pattern has been identified with versions previous to the one that includes the
fix allowing large number of established/close_wait connections (e.g. Orion 1.4.3).

The testing procedure is the same but the `connections_stress_tests.py` script has to be run with
`-noQueueSize` (the queue size is based on `GET /statistics` and in this situation, the CB is
unable to serve that request, resulting in ConnectionError exceptions):

```
python connections_stress_tests.py -noQueueSize
```

It may be a surprise to see that the script output is the same as in the working scenario. In particular,
the version check keeps showing OK. This can be explained looking at the CB file descriptor information.
The typical situation is as follows, the 5000 outgoing connections have exhausted almost (but not all) of
the fds with numbers below 1024:

```
$ lsof -p <pid cb>
...
contextBr 46790 fermin    0u   CHR   136,2      0t0       5 /dev/pts/2
contextBr 46790 fermin    1u   CHR   136,2      0t0       5 /dev/pts/2
contextBr 46790 fermin    2u   CHR   136,2      0t0       5 /dev/pts/2
contextBr 46790 fermin    3r   CHR     1,9      0t0    7609 /dev/urandom
contextBr 46790 fermin    4u   REG     8,1      330  915381 /tmp/contextBroker.log
contextBr 46790 fermin    5w   REG     8,1        5  919967 /tmp/contextBroker.pid
contextBr 16243 fermin    6u  IPv4 1316606      0t0     TCP localhost:42664->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin    7u  IPv4 1316608      0t0     TCP localhost:42665->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin    8u  IPv4 1316610      0t0     TCP localhost:42666->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin    9u  IPv4 1316612      0t0     TCP localhost:42667->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   10u  IPv4 1316614      0t0     TCP localhost:42668->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   11u  IPv4 1316616      0t0     TCP localhost:42669->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   12u  IPv4 1316618      0t0     TCP localhost:42670->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   13u  IPv4 1316620      0t0     TCP localhost:42671->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   14u  IPv4 1316622      0t0     TCP localhost:42672->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   15u  IPv4 1316624      0t0     TCP localhost:42673->localhost:27017 (ESTABLISHED)
contextBr 16243 fermin   16u  IPv4 1316626      0t0     TCP *:1026 (LISTEN)
contextBr 16243 fermin   17u  IPv6 1316631      0t0     TCP *:1026 (LISTEN)
contextBr 16243 fermin   19u  IPv6 1372321      0t0     TCP localhost:39807->localhost:8090 (CLOSE_WAIT)
contextBr 16243 fermin   20u  IPv6 1372322      0t0     TCP localhost:39808->localhost:8090 (CLOSE_WAIT)
contextBr 16243 fermin   21u  IPv6 1372357      0t0     TCP localhost:39819->localhost:8090 (CLOSE_WAIT)
...
contextBr 16243 fermin 5015u  IPv6 1401317      0t0     TCP localhost:44810->localhost:8090 (CLOSE_WAIT)
contextBr 16243 fermin 5016u  IPv6 1401320      0t0     TCP localhost:44811->localhost:8090 (CLOSE_WAIT)
contextBr 16243 fermin 5017u  IPv6 1400253      0t0     TCP localhost:44812->localhost:8090 (CLOSE_WAIT)
contextBr 16243 fermin 5018u  IPv6 1401325      0t0     TCP localhost:44813->localhost:8090 (CLOSE_WAIT)
```

Note the number of the file descriptor just after the last one used by the listening server is free
(i.e. 18 in this case). This is probably due to this fd was the one given to the update request
thas triggered the 5000 notifications, so after closing that incoming connection, the fd is available again.

Thus, you can "take" the last free fd (e.g. `telnet localhost 1026`, then hold the terminal), check it
with `lsof -p <pid cb>`, then version column will start to show NOK, as shows below:

```
2016-12-20 10:24:17,042 | INFO | Test init: 2016-12-20T10:24:17.042571Z
2016-12-20 10:24:18,517 | INFO |  Reports each second:
2016-12-20 10:24:18,517 | INFO |  counter       version      queue   established   close wait    sum
2016-12-20 10:24:18,517 | INFO |                request      size    connections   connections   connections
2016-12-20 10:24:18,517 | INFO |  --------------------------------------------------------------------------------
2016-12-20 10:24:21,685 | INFO |  -------- 1 -------- OK -------- N/A -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-20 10:24:26,019 | INFO |  -------- 2 -------- OK -------- N/A -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-20 10:24:30,021 | INFO |  -------- 3 -------- OK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:24:34,032 | INFO |  -------- 4 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:24:38,072 | INFO |  -------- 5 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:24:42,263 | INFO |  -------- 6 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:24:46,387 | INFO |  -------- 7 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:24:50,570 | INFO |  -------- 8 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:24:54,397 | INFO |  -------- 9 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
...
(Do "telnet localhost 1026" in a terminal)
...
2016-12-20 10:24:58,460 | INFO |  -------- 10 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:25:02,325 | INFO |  -------- 11 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:25:06,028 | INFO |  -------- 12 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:25:09,546 | INFO |  -------- 13 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:25:13,220 | INFO |  -------- 14 -------- NOK -------- N/A -------- 5011 ---------- 0 -------- 5011 ----------
2016-12-20 10:25:16,861 | INFO |  -------- 15 -------- NOK -------- N/A -------- 5010 ---------- 0 -------- 5010 ----------
...
(Close the "telnet localhost 1026" session, releasing its connection)
...
2016-12-20 10:25:20,339 | INFO |  -------- 16 -------- OK -------- N/A -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-20 10:25:23,676 | INFO |  -------- 17 -------- OK -------- N/A -------- 5010 ---------- 0 -------- 5010 ----------
2016-12-20 10:25:27,150 | INFO |  -------- 18 -------- OK -------- N/A -------- 5010 ---------- 0 -------- 5010 ----------
```

### Raising system limits for the test

In the case of running CB as foreground process:

* If you are not using CentOS 6.x, probably just using `ulimit -u <new limit>` and `ulimit -n <new limit>`
  to change respectively the max number of processes and the max number of open file descriptors will work.
* If you are using CentOS 6.x, edit the following file (as root): `/etc/security/limits.d/90-nproc.conf`
  as shown below, then re-login (more detail in [this reference](https://bugzilla.redhat.com/show_bug.cgi?id=919793)):

```
*          soft    nproc     10240
*          soft    nofile    10240
*          hard    nproc     10240
*          hard    nofile    10240
```

In the case of running CB as a service in CentOS:

* The contextBroker process is run by the `orion` user with default configuration. The above configuration
  in /etc/security/limits.d/90-nproc.conf should suffice but anyway it is advisable to check the limits
  with `cat /proc/<pid>/limits` once the service has been started.
* You have to run RPyC as `orion` user. Otherwise, the connection inspection done by connections_stress_tests.py
  will fail (rpyc_classic.py is the actual process that looks for CB process connections so it has to be run by the
  same user that runs CB in order to have enough privilege level).

```
$ sudo su orion -c rpyc_classic.py
INFO:SLAVE/18812:server started on [0.0.0.0]:18812
```

### Detailed `connections_stress_tests.py` usage

       Parameters:                                                                                                  
          -host=<host>         : CB host (OPTIONAL) (default: localhost).                                           
          -u                   : show this usage (OPTIONAL).                                                        
          -v                   : verbose with all responses (OPTIONAL) (default: False). 
          -noConnectionInfo    : is used to ignore ESTABLISHED and CLOSE_WAIT connections (OPTIONAL)(default: False)
          -noQueueSize         : is used to ignore the Queue size (OPTIONAL) (default: False).                         
          -service=<value>     : service header (OPTIONAL) (default: stablished_connections).                                        
          -service_path=<value>: service path header (OPTIONAL) (default: /test)                                    
          -notif_url=<value>   : url used to notifications (OPTIONAL) (default: http://localhost:8090/notify)       
          -mongo=<value>       : mongo host used to clean de bd (OPTIONAL) (default: localhost)                     
          -duration=<value>    : test duration, value is in minutes (OPTIONAL) (default: 60 minutes)                
                                                                                                                    
       Examples:                                                                                                    
        python connections_stress_tests.py -host=10.10.10.10 -notif_url=http://10.0.0.1:8090/notify duration=100 -v 
                                                                                                                    
       Note:                                                                                                        
         - the version delay is a second                                                                            
         - the number of subscriptions is 5000     

### Detailed `slow_listener` usage

```
$./slow_listener --help
Usage of ./slow_listener:
  -close=false: do not send Connection:close in response
  -delay=0: delay in write response
  -port="8090": port to listen to (8090 by default)
  -stats=5s: interval for writing stats
  -timeout=30s: timeout
```
