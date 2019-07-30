# Performance Test with Apache JMeter

Performance tests with Apache JMeter were performed to test the behavior of services provided by the **Orion Linked Data (LD) broker**.

## How to make the test?

**Requeriments**

- Install [Apache JMeter](https://jmeter.apache.org/download_jmeter.cgi) on Linux OS (recommend) or Windows OS, in a machine;
- Define the Apache JMeter as Path Variable;
- In other machine, with Linux OS, install the Orion LD broker;
- Install the [Plugins Manager](https://jmeter-plugins.org/install/Install/) on Apache JMeter;
- Install the plugin: *jpgc - Standard Set*, which will allow to use graph generators listeners , about: Transactions per seconds, Performance metrics and many others. However, to use the performance metricts listener, is necessary to use the [Server Agent tool](https://github.com/undera/perfmon-agent/blob/master/README.md) on both machines used, for help to monitoring the CPU, RAM and others parameters of the machine which is with broker installed. The usage of this tool is easy. On machine with Apache JMeter execute the file: `startAgent` (`.bat` for Windows, `.sh` for Linux) and on broker's machine execute the `startAgent.sh` file. This process establish the communication with both machines, doing with which the machine with JMeter running, catch the data about: CPU, RAM usage, of machine with Orion LD Broker, generating a real time graph, ready for save as Image or csv file.

**Performance test script structure**
The script was developed in Python language. The parameters used are: 
- Users/threads quantity; 	
- Loop count (represent the repetition quantity of users doing requests);
- Ramp time (represent the time which all threads/users must be stay ready for make requests);
- Domain/IP;
- Port;
- The broker service to be tested;
- HTTP Verb (GET, POST, PATCH and DELETE).

**How to use the test performance script**
    
- Define the parameters;
- Choose the one of the options:
	- Generate complete report (.csv + HTML);
	- Generate only .csv file;
	- Only results on terminal.

test/jMeter/output/
test/jMeter/*.log