# Performance tests with Apache JMeter

This python script help to measure performance of any REST application using Apache JMeter tool

## How to make the test

### Requeriments

- Install [Apache JMeter](https://jmeter.apache.org/download_jmeter.cgi) on Linux OS (recommend) or Windows OS, in a machine
- Define the Apache JMeter as Path Variable

```bash
nano .bashrc

# Add this line:
export PATH="$PATH":"$HOME/apache-jmeter-5.3/bin"

# Save and run this command:
source .bashrc
```

- In another machine, with Linux OS, install the Orion LD broker
- Install the [Plugins Manager](https://jmeter-plugins.org/install/Install/) on Apache JMeter and put it into lib/ext directory, then restart JMeter;
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
* Create json test file

  ``` bash
  {
    "NUM_OF_THREADS": 1,
    "LOOPS": 1,
    "RAMP_TIME": 1,
    "DOMAIN_IP": "localhost",
    "PORT": 1026,
    "PATH_SERVICE": "path/name",
    "HTTP_VERB": "GET",
    "HEADERS": {
        "Content-Type": "application/json",
        "client-id": "app"
    },
    "BODY_DATA": {
        "raw": "username=manager&password=1234"
    }
  }
  ```

  * **"BODY_DATA"** can be **"raw"** for **"application/x-www-form-urlencoded"** or **"json"** for **"application/json"**

  * You can use a counter variable:

    ``` bash
      "raw": "username=manager${counter_value}&password=1234"
    ```

  * You can use, too, a random variable:

    ``` bash
      "raw": "username=manager${__UUID()}&password=1234"
    ```

* Choose one of the following options:
	* Generate complete report (.csv + HTML)
	* Generate .csv file
	* Only results on screen

* Run command:

  ``` bash
  # For specific test case
  python generateTestJMeter.py test_name.json
  
  # For all test cases created
  python generateTestJMeter.py
  ```
