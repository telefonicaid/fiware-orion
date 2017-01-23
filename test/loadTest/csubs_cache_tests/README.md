# Cache refresh Tests

These tests are used to verify that the notifications rate from ContextBroker is similar to the rate received in the listener.


#### Requirements

- Bash shell (linux)
- Jmeter (2.11 verson or higher)
- Python (2.7.x version)


#### Architecture for testing

- 1 node with a context broker installed ()
- 1 node with mongoDB (associated to the CB)
- 1 node for launch listener
- 1 node for launch jmeter

Notes: 
The listener recommended is the [listen.js](https://github.com/telefonicaid/iotqatools/blob/master/iotqatools/simulators/listen/listen.js) with this config:
```
     node listen.js -a -s1 -p8090 -ds0
```
The `startAgent.sh` of Jmeter must be installed in CB node, for monitoring the CPU, Memory and TCP.
```
     nohup ./startAgent.sh --udp-port 0 --tcp-port 4444 &
```

#### Scripts used:

- **orion_listener_check.jmx**: Jmeter script used to verify rate allowed by the listener.
     
Parameters:
```
     HOST: listener host or IP (default: localhost)
     PORT: listener port (default: 8090)
     THREADS: number of threads used in the test (default: 1)
     TESTTIME: duration time of the test (default: 60)
     RAMP_UP: the amount of time for creating the total of threads (default: 10)
```
  
  Example:
  ```
  sh jmeter -n  -t test/loadTest/csubs_cache_tests/orion_listener_check.jmx  -JHOST=<listener_host>  -JTESTTIME=500 -JTHREADS=100
  ```
  
- **populate_entities_and_subcriptions.jmx**: Jmeter script used to populate de Database with N entities and subscriptions with a relation 1:1.
  
 Parameters:
```
     HOST: CB host or IP (default: localhost)
     PORT: CB port (default: 8090)
     THREADS: number of threads used in the test (default: 1)
     TESTTIME: duration time of the test (default: 60)
     RAMP_UP: the amount of time for creating the total of threads (default: 10)
     SERVICE: service header used in requests (default: )
     SERVICE_PATH: service path header used in requests (default: /)
     SUBSC_REFERENCE: host and port to send the notification (listener) (default: ,http://localhost:8090/notify)
     TOTAL: total of subscriptions/entities created (default: 10000)
```
  
  Example:
  ```
  sh jmeter -n  -t test/loadTest/csubs_cache_tests/populate_entities_and_subcriptions.jmx -JHOST=<cb_host> -JPORT=1026 -JTHREADS=200 -JTOTAL=25000  -JSERVICE="cache_test_1_6_0_next"  -JSUBSC_REFERENCE="http://10.10.10.1:8090/notify"
  ```
   
- **orion_csubs_cache_check.jmx**: Jmeter script used to check if the notifications rate from ContextBroker is similar to the rate received in the listener.
  
 Parameters:
```
     HOST: CB host or IP (default: localhost)
     PORT: CB port (default: 8090)
     THREADS: number of threads used in the test (default: 1)
     TESTTIME: duration time of the test (default: 60)
     RAMP_UP: the amount of time for creating the total of threads (default: 10)
     SERVICE: service header used in requests (default: )
     SERVICE_PATH: service path header used in requests (default: /)
     TOTAL: total of subscriptions/entities created (default: 10000)
```
  
  Example:
  ```
  sh jmeter -n  -t test/loadTest/csubs_cache_tests/orion_csubs_cache_check.jmx -JHOST=<cb_host> -JPORT=1026 -JTHREADS=200 -JTOTAL=25000  -JSERVICE="cache_test_1_6_0_next"  
  ```
