## JMeter scripts

This is the Jmeter scripts repository for the Orion Context Broker, used for Performance tests.

#### Pre-conditions:

* "Jmeter" app (2.11 or higher) is required in Launcher VM (http://jmeter.apache.org)
* "jmeter-plugin" is required (http://jmeter-plugins.org)
* "ServerAgent" is required in ContextBroker and/or balancer hosts (http://jmeter-plugins.org/wiki/PerfMonAgent) (Java is required)
* have a account in Loadosophia (optional) - (http://loadosophia.org)
* "nginx" app exists in Balancer VM (only in cluster case)
* Verify nginx configuration for each scenario (only in cluster case)
* Verify mongoDB version (3.4.x)
	
#### Steps:

* Launch "ServerAgent" in ContextBroker VM or/and in Balancer. 
```
nohup sh startAgent.sh --udp-port 0 --tcp-port 4444 > monitor.log &
```
* Launch jmeter script 
* get reports in `/tmp/jmeter_result/<date>__<proyect_name>/` folder
* (Optional) Upload in Loadosophia Loadosophia_xxxxxxxxxxxxxxxxxxxxx.jtl.gz and perfmon_xxxxxxxxxxxxxxxxxxxx.jtl.gz (where "xxxxxxxxxxxxxxxxxxx" is a hash value).

Comments:

  - if someone would like to disable the listener to get metrics from host (jp@gc - Perfom Metrics Collector), it is very easy, in GUI or in file directly (change enabled="false"). Avoiding scripts duplicate.
   Review if the testname is correct.
   ```
   <kg.apc.jmeter.perfmon.PerfMonCollector guiclass="kg.apc.jmeter.vizualizers.PerfMonGui" testclass="kg.apc.jmeter.perfmon.PerfMonCollector" testname="PerfMon Metrics Collector" enabled="true">
   ``` 

  -  /tmp/error_xxxxxxxxxxxxxxxxxxx.html is created, because does not have access at loadosophia, the token is wrong intentionally, because the jmeter do not constantly access and penalizes the test times. We only store datas manually when finished test. So "xxxxxxxxxxxxxxxxxxx" is a hash value.

  

#### Scripts:

**orion_updates_ngsiv2.jmx**: script used to send update requests using ngsi V2 API and subscriptions. This script only use one host.
 
 - If the ATTRIBUTES property is bigger than 20, the ATTRIBUTES property is replaced by 20.
 - The entity id is random (between 1 and 50000), with this format ex: E00001 or E50000.
 - Suffix of attributes are random (between 1 and 20), with this format ex: A01, A07, A20.
 - Attribute values are random values (12 characters).
 - It is possible to include metadata or not in each attribute.
 - It is possible to include TimeInstant attribute (in zulu mode) or not in each entity.
 - **WARN: It is a requisite very important that all entities (50000) with the format A00000 exist in db, because the PATCH request is used.**
 
Properties:
```
		* HOST                - CB host or balancer (default: localhost)
		* PORT                - CB port (default: 1026)
		* THREADS             - number of concurrent threads (default: 1)
		* RAMP_UP             - the amount of time for creating the total number of threads (default: 1)
		* TEST_TIME           - test duration time in seconds (default:30)
		* SERVICE             - service header (default: no service is used, i.e. Fiware-Service header is omitted) 
		* SERVICE_PATH        - service path header (default: /)	    
		* ATTRIBUTES          - number of attributes per entity (default:1)
		* METADATA            - if true is appended a metadata in each attribute (default: false)
		* TIME_INSTANT        - if true is appended a "timeInstant" attribute in each entity (default: true)
		* RANDOM_TIME_INSTANT - if this is greater than 0 is generated a string of N random characters, else it is generated a date with zulu format. (default: 0)
		* SUBSC               - number of subscriptions (default: 0)
		* SUBSC_DURATION      - Duration of the subscriptions in seconds (default: 60)
		* LISTEN_HOST         - host to receive notifications (mock) (default:localhost)
		* LISTEN_PORT         - port to receive notifications (mock) (default:8090)				
```

Report files (listeners):
```
  * tps_<date>.csv: this graph shows the number of transactions per second for each sampler (jp@gc - Transactions per Second).
  * reports_<date>.csv: which displays values for all request made during the test and an individual row for each named request in your test(Aggregate Report).
  * perfmon_<date>.csv: Some metric types (CPU, Memory, TCP) are show in graphic (jp@gc - Perfom Metrics Collector)
  * errors__<date>.csv:  shows a tree of all sample responses, allowing you to view the response for any sample (only errors are stored) (View Results Tree).
```

Example:
```
./jmeter.sh -n -t orion_updates_ngsiv2.jmx -JHOST=10.10.10.1 -JTHREADS=100 -JTEST_TIME=60 -JATTRIBUTES=3
```

**orion_soak_test_ngsiv1.jmx** and **orion_soak_test_ngsiv1_ngsiv2.jmx**: scripts used to soak tests using NGSI v1 or NGSI v2. This script only use one host.
Soak Testing is a type of performance test that verifies a system's stability and performance characteristics over an extended period of time (three days in this case).
Script flow using NGSI v1:
  - One thread that creates a subscription and returns the total of subcriptions each 30 secs with a random "notifyConditions" attribute (between A00 and A20).
  - One thread that get subcriptionIds and remove all subcriptions each 7200s (2hrs).
  - N threads that send random entity updates (between E0000 and E9999) with random attributes name (between A00 and A20) and try to get values of a  random entity (between E0000 and E9999) if it does exist. 
 
Properties:
```
		* HOST                - CB host (default: localhost)
		* PORT                - CB port (default: 1026)
		* THREADS             - number of concurrent threads (default: 1)
		* RAMP_UP             - the amount of time for creating the total number of threads (default: 1)
		* TEST_TIME           - test duration time in seconds (default:30)
		* SERVICE             - service header (default: no service is used, i.e. Fiware-Service header is omitted) 
		* SERVICE_PATH        - service path header (default: /)	    
		* ATTRIBUTES          - number of attributes per entity (default:1)
		* METADATA            - if true is appended a metadata in each attribute (default: false)
		* TIME_INSTANT        - if true is appended a "timeInstant" attribute in each entity (default: true)
		* RANDOM_TIME_INSTANT - if this is greater than 0 is generated a string of N random characters, else it is generated a date with zulu format. (default: 0)
		* SUBSC_REFERENCE     - host  and port to receive notifications (mock) (default: http//localhost:8090/notify)
		* SUBSC_DURATION      - Duration of the subscriptions in seconds (default: 60)	
		* VERSION_PERCENTAGE  - determine the version percentage, where "0" is V1 and "100" es V2 (only used in orion_soak_test_ngsiv1_ngsiv2.jmx script)	
```

Report files (listeners):
```
  * reports_<date>.csv: which displays values for all request made during the test and an individual row for each named request in your test(Aggregate Report).
  * perfmon_<date>.csv: Some metric types (CPU, Memory, TCP) are show in graphic (jp@gc - Perfom Metrics Collector)
  * errors__<date>.csv:  shows a tree of all sample responses, allowing you to view the response for any sample (only errors are stored) (View Results Tree).
```

Example:
```
 ./jmeter.sh -n -t orion_soak_test_ngsiv1.jmx -JHOST=localhost -JTEST_TIME=259200 -JTHREADS=100 -JSERVICE="soak_test" -JATTRIBUTES=5 -JSUBSC_DURATION=8000
 ./jmeter.sh -n -t orion_soak_test_ngsiv1_ngsiv2.jmx -JHOST=localhost -JTEST_TIME=172800 -JTHREADS=100 -JSERVICE="soak_test" -JATTRIBUTES=15 -JSUBSC_REFERENCE="http://localhost:8090/notify" -JSUBSC_DURATION=8000 -JVERSION_PERCENTAGE=100`
```



**orionPerformanceTest_v1.0.jmx**   (used by Max Performance, Mongo Impact, Scale UP, Scale OUT). It can used for one standalone VM or a balanced cluster of VM with 4 nodes maximum.
	 
        Properties:

		* PROJECT    - project name (DEFAULT by default)
		* TESTNAME   - test name (orionPerformanceTest by default)
		* RUNTIME    - test duration time (30 Sec by default)
		* THREADS    - number of concurrent threads (1 by default)
		* RAMPUP     - the amount of time for creating the total number of threads (0 by default)
		* ITERATIONS - number of repetitions of each thread (1 by default)
		* HOST       - IP or hostname (in case of clusters is Nginx)  (127.0.0.1 by default)
		* Node_1     - IP or hostname for Node 1, only for monitoring (127.0.0.1 by default)
		* Node_2     - IP or hostname for Node 2, only for monitoring (127.0.0.1 by default)		
		* Node_3     - IP or hostname for Node 3, only for monitoring (127.0.0.1 by default)		
		* Node_4     - IP or hostname for Node 4, only for monitoring (127.0.0.1 by default)
		* PORT       - ContextBroker port (1026 by default)
		* MON_PORT   - serverAgent port for Monitoring (3450 by default)

Example:
```
./jmeter.sh -n -t orionPerformanceTest_v1.0.jmx -JPROJECT="XXX" -JTEST_NAME="XXXX" -JHOST=X.X.X.X -JNODE_1=X.X.X.X -JPORT=1026 -JTHREADS=XXXX -JRAMPUP=XXX -JRUNTIME=XXXX >> /<path>/jmeter_report__`date +%Y%m%d%H`.log
```
  		 
**orionPerformanceOnlyQueries_v2.0.jmx**   (used by Max Performance, Mongo Impact, Scale UP, Scale OUT). It is used for generate only queries.
	 
        Properties:

		* PROJECT    - project name (DEFAULT by default)
		* TESTNAME   - test name (onlyQueriesTest by default)
		* RUNTIME    - test duration time (3 Sec by default)
		* THREADS    - number of concurrent threads (1 by default)
		* RAMPUP     - the amount of time for creating the total number of threads (0 by default)
		* ITERATIONS - number of repetitions of each thread (1 by default)
		* HOST       - IP or hostname (in case of clusters is Nginx)  (127.0.0.1 by default)
		* Node_1     - IP or hostname for Node 1, only for monitoring (127.0.0.1 by default)
		* Node_2     - IP or hostname for Node 2, only for monitoring (127.0.0.1 by default)		
		* Node_3     - IP or hostname for Node 3, only for monitoring (127.0.0.1 by default)		
		* Node_4     - IP or hostname for Node 4, only for monitoring (127.0.0.1 by default)
		* PORT       - ContextBroker port (1026 by default)
		* MON_PORT   - serverAgent port for Monitoring (3450 by default)

Example:
```
./jmeter.sh -n -t orionPerformanceOnlyQueries_v2.0.jmx -JPROJECT="XXX" -JTEST_NAME="XXXX" -JHOST=X.X.X.X -JNODE_1=X.X.X.X -JPORT=1026 -JTHREADS=XXXX -JRAMPUP=XXX -JRUNTIME=XXXX >> /<path>/jmeter_report__`date +%Y%m%d%H`.log
```
 
**orionPerformanceOnlyAppends_v2.0.jmx**   (used by Max Performance, Mongo Impact, Scale UP, Scale OUT). It is used for generate only appends.
	 
        Properties:

		* PROJECT    - project name (DEFAULT by default)
		* TESTNAME   - test name (onlyQueriesTest by default)
		* RUNTIME    - test duration time (3 Sec by default)
		* THREADS    - number of concurrent threads (1 by default)
		* RAMPUP     - the amount of time for creating the total number of threads (0 by default)
		* ITERATIONS - number of repetitions of each thread (1 by default)
		* HOST       - IP or hostname (in case of clusters is Nginx)  (127.0.0.1 by default)
		* Node_1     - IP or hostname for Node 1, only for monitoring (127.0.0.1 by default)
		* Node_2     - IP or hostname for Node 2, only for monitoring (127.0.0.1 by default)		
		* Node_3     - IP or hostname for Node 3, only for monitoring (127.0.0.1 by default)		
		* Node_4     - IP or hostname for Node 4, only for monitoring (127.0.0.1 by default)
		* PORT       - ContextBroker port (1026 by default)
		* MON_PORT   - serverAgent port for Monitoring (3450 by default)

Example:
```
./jmeter.sh -n -t orionPerformanceOnlyAppends_v2.0.jmx -JPROJECT="XXX" -JTEST_NAME="XXXX" -JHOST=X.X.X.X -JNODE_1=X.X.X.X -JPORT=1026 -JTHREADS=XXXX -JRAMPUP=XXX -JRUNTIME=XXXX >> /<path>/jmeter_report__`date +%Y%m%d%H`.log
```

**orionPerformanceOnlyAppendsAndUpdates_v2.0.jmx**   (used by Max Performance, Mongo Impact, Scale UP, Scale OUT). It is used for generate appends and updates.
	 
        Properties:

		* PROJECT    - project name (DEFAULT by default)
		* TESTNAME   - test name (onlyQueriesTest by default)
		* RUNTIME    - test duration time (3 Sec by default)
		* THREADS    - number of concurrent threads (1 by default)
		* RAMPUP     - the amount of time for creating the total number of threads (0 by default)
		* ITERATIONS - number of repetitions of each thread (1 by default)
		* HOST       - IP or hostname (in case of clusters is Nginx)  (127.0.0.1 by default)
		* Node_1     - IP or hostname for Node 1, only for monitoring (127.0.0.1 by default)
		* Node_2     - IP or hostname for Node 2, only for monitoring (127.0.0.1 by default)		
		* Node_3     - IP or hostname for Node 3, only for monitoring (127.0.0.1 by default)		
		* Node_4     - IP or hostname for Node 4, only for monitoring (127.0.0.1 by default)
		* PORT       - ContextBroker port (1026 by default)
		* MON_PORT   - serverAgent port for Monitoring (3450 by default)

Example:
```
./jmeter.sh -n -t orionPerformanceAppendsAndUpdates_v2.0.jmx -JPROJECT="XXX" -JTEST_NAME="XXXX" -JHOST=X.X.X.X -JNODE_1=X.X.X.X -JPORT=1026 -JTHREADS=XXXX -JRAMPUP=XXX -JRUNTIME=XXXX >> /<path>/jmeter_report__`date +%Y%m%d%H`.log
```

**populateDB_v1.0.sh** (used by populate a mongoDB wint N context and with a step)
   
```
It is necesary to define url, tenant, elements_number and step
usage:  populateDB_v1.0.sh <http://hostname:port> <tenant> <100000> <10>
```
	
**multiplesAttributes_bug_memory_leak.jmx** (used by simulate some problems reported by users, see http://stackoverflow.com/questions/23386094/orion-context-broker-possible-memory-leak)

	    Properties:
	
		* OPERATION: SUBSCRIBER or nothing (default) (used only one time for create subscriptions)
		* HOST     : IP or hostname of ContextBroker VM (127.0.0.1 by default)
		* PORT     : ContextBroker port (1026 by default)
		* MON_PORT : serverAgent port for Monitoring (3450 by default)
	
	Scenario:

		I have 19 entities of type "House" with 
		* 8-9 attributes that are updated every 10 seconds
		* 15 attributes that are updated every 5 minutes
		* 2 attributes that are updated between 2 seconds to 20 minutes randomly.

		I have 1 entity of type "House" with 
		* 6 attributes that are updated every 10 seconds
		* 15 attributes that are updated every 5 minutes
		* 2 attributes that are updated between 2 seconds to 20 minutes randomly. 

		I have 1 entity of type "House" with 
		* 31 attributes that are updated every 10 seconds
		* 3 attributes that are updated every 5 minutes

		I have 17 entities of type "Vehicle" with  
		* 2 attributes that are updated every 2 seconds 

**ngsiv1_vs_ngsi2.jmx**: this is an earlier version of orion_soak_test_ngsiv1_ngsiv2.jmx. Although it has not been mantained since time ago, it could be useful to inject entities with/without subscriptions in NGSIv1 and NGSIv2 using the same script (and avoiding the randomness using the the soak version).

## `perf` directory

It contains some Python scripts that may be useful to set up CB in order to perform performance tests on it (entity population, subscription creation, etc.).
