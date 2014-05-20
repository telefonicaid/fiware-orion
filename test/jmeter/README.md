## JMeter scripts

This is the Jmeter scripts repository for the Orion Context Broker, used for Performance tests.

#### Pre-conditions:
	1- "Jmeter" app exists in Launcher VM - (/root/test/apache-jmeter-2.11/bin)
	2- "ServerAgent" app exist in Launcher VM, in ContextBroker VM and Nginx VM (only in cluster case) - (/root/test/apache-jmeter-2.11)	
	3- have a account in Loadosophia - (http://loadosophia.org)
	4- "nginx" app exists in Balancer VM (only in cluster case)
	6- Verify nginx configuration for each scenario (only in cluster case)
	7- verify contextBroker MD5 in each Node (http://wikis.hi.inet/fi-ware/index.php/Orion_QA#Instances_under_Test:
	8- verify mongoDB 2.4.9 in each Node	
	
#### Steps:
	1- launch "ServerAgent" in ContextBroker VM (/root/test/apache-jmeter-2.11/ServerAgent/) in Balancer and each Node VM 
		nohup sh startAgent.sh --udp-port 0 --tcp-port 3450 > ~/monitor.log &
	2- launch jmeter script "orionPerfTest_cluster_v1.2.jmx" in Launcher VM (/root/test/apache-jmeter-2.11/bin)
		~/test/apache-jmeter-2.11/bin/jmeter.sh -n -t ~/test/script/jmeter/orionPerformanceTest.jmx -JPROJECT="???" -JTEST_NAME="???" -JHOST=X.X.X.X -JNODE_1=X.X.X.X -JPORT=1026 -JTHREADS=???? -JRAMPUP=??? -JRUNTIME=??? >> /<path>/jmeter_report_YYYYMMDD.log
	3- move /tmp/Loadosophia_xxxxxxxxxxxxxxxxxxxxx.jtl.gz file into /tmp/JMeter_result/result_<today>-<now> folder
	4- copy folder in local
	5- upload in Loadosophia Loadosophia_xxxxxxxxxxxxxxxxxxxxx.jtl.gz and perfmon_xxxxxxxxxxxxxxxxxxxx.jtl.gz
	6- fill the report in Google doc

#### Scripts:
1. **orionPerformanceTest.jmx**  (used by Max Performance, Mongo Impact, Scale UP, Scale OUT). It can used for one standalone VM or a balanced cluster of VM with 4 nodes maximum.
	+ Properties:
		* PROJECT    - project name (DEFAULT by default)
		* TESTNAME   - test name (orionPerformanceTest by default)
		* RUNTIME    - test duration time (30 Sec by default)
		* THREADS    - number of concurrent threads (1 by default)
		* RAMPUP     - the amount of time for creating the total number of threads (0 by default)
		* ITERATIONS - number of repetitions of each thread (1 by default)
		* HOST       - IP or hostname (in case of clusters is Nginx) (127.0.0.1 by default)
		* Node_1     - IP or hostname for Node 1, only for monitoring (127.0.0.1 by default)
		* Node_2     - IP or hostname for Node 2, only for monitoring (127.0.0.1 by default)		
		* Node_3     - IP or hostname for Node 3, only for monitoring (127.0.0.1 by default)		
		* Node_4     - IP or hostname for Node 4, only for monitoring (127.0.0.1 by default)
		* PORT       - ContextBroker port (1026 by default)
		* MON_PORT   - serverAgent port for Monitoring (3450 by default)
		
#### Comments:
    /tmp/error_xxxxxxxxxxxxxxxxxxx.html is created, because does not have access at loadosophia, the token is wrong. We only store datas manually when finished test
	
2. **multiplesAttributes_bug_memory_leak.jmx** (used by simulate memory leak issue) (http://stackoverflow.com/questions/23386094/orion-context-broker-possible-memory-leak)
	+Properties:
		* OPERATION - SUBSCRIBER or nothing (default) (used only one time for create subscriptions)
		* HOST      - IP or hostname of ContextBroker VM (130.206.80.132 by default)
		* PORT      - ContextBroker port (1026 by default)
		* MON_PORT  - serverAgent port for Monitoring (3450 by default)
	
	Scenario:
		I have 19 entities of type "House" with 
		* 8-9 attributes that are updated every 10 seconds
		* 15 attributes that are updated every 5 minutes
		* 2 attributes that are updated between 2 seconds to 20 minutes.

		I have 1 entity of type "House" with 
		* 6 attributes that are updated every 10 seconds
		* 15 attributes that are updated every 5 minutes
		* 2 attributes that are updated between 2 seconds to 20 minutes. 

		I have 1 entity of type "House" with 
		* 31 attributes that are updated every 10 seconds
		* 3 attributes that are updated every 5 minutes

		I have 17 entities of type "Vehicle" with 2 (2 seconds to very long time) 
		* 2 attributes that are updated every 2 seconds to infinite

		
