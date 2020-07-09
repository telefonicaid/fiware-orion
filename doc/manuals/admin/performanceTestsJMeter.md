# Performance Test with Apache JMeter

Performance tests with Apache JMeter are performed to test the behavior of services provided by the **Orion Linked Data (LD) context broker**.

## How to make the test

### Requeriments

- Install [Apache JMeter](https://jmeter.apache.org/download_jmeter.cgi) on Linux OS (recommend) or Windows OS, in a machine
- Define the Apache JMeter as Path Variable
- In another machine, with Linux OS, install the Orion LD broker
- Install the [Plugins Manager](https://jmeter-plugins.org/install/Install/) on Apache JMeter;
- Install the plugin: *jpgc - Standard Set*, which will allow to use graph generators listeners, about:
  * Transactions per seconds,
  * Performance metrics,
  * and many others.
  However, to use the performance metrics listener, it is necessary to use the
  [Server Agent tool](https://github.com/undera/perfmon-agent/blob/master/README.md) on both machines used,
  to get help monitoring the CPU, RAM and others parameters of the machine which has the broker installed.
  The usage of the Server Agent tool is easy:
  In the machine with Apache JMeter, execute `startAgent.[bat|sh]` (`.bat` for Windows, `.sh` for Linux)
  and in the broker's machine, execute `startAgent.sh`.
  This process establishes the communication between both machines.
  In the JMeter machine - collect data about CPU and RAM usage.
  In the Broker machine - generate a real time graph, ready to save as an image or a csv file.

### Performance test script structure
This script has been developed in the Python programming language. The parameters used are: 
* Number of users/threads
* Loop count (represents the repetition quantity of users doing requests)
* Ramp time (represents the time which all threads/users must be ready to make requests)
* Domain/IP
* Port
* The broker service to be tested
* HTTP Verb (GET, POST, PATCH and DELETE).

### How to use the test performance script
* Define the parameters
* Choose one of the following options:
	* Generate complete report (.csv + HTML)
	* Generate .csv file
	* Results on screen

test/jMeter/output/
test/jMeter/*.log
