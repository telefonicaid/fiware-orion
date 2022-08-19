# Orion Context Broker Test Scripts

Here there are a set of test Scripts created to cover the needs showed during the performance test.


You find all the information about Orion Context Broker and its tests in the FI-WARE Catalogue:

http://catalogue.fiware.org/enablers/publishsubscribe-context-broker-orion-context-broker


##Scripts:

* nodeAccumulator.js
* SubscriptionTest_ONCHANGE.sh
* SubscriptionTest_ONTIMEINTERVAL.sh

### nodeAccumulator.js

* What is this:

    It is a node server to take the advantage of the event-loop non-blocking.

* How I use it:

    So if it's not installed just need to "yum install nodejs"
    By default the server will listen at ports 1028, 1029, 1030 and 1031 (The last one has an intentional delay of two seconds)
    Make sure your machine ports allows traffic on these ports.
    
    ```
    If you need to use different ports or different amount of servers just specify the parameter you need:
       $ > node nodeAccumulator.js [-u -v --version --silence -s -p -ds -dp]
    where:
     '-u' : Shows this usage info
     '-sX' : amount of servers to run (without delay). '-d5' will launch 5 servers
     '-pX' : Port Server, X is the first port number to setup the server, the rest of listener servers will be added in the next port numbers
     '-dsY' : amount of servers tu run with delay. '-ds3' will launch 3 servers with delay
     '-dpY' : Port Delayed Server, Y is the first port number to setup the Delayed server, the rest of listener servers will be added in the next port numbers
     '--silence' : no info showed in log
     '--version' : shows the version of this script
     '-v' : verbose mode, by default OFF
     ```

* Default execution:

   ```
   $ node accumulatorServer.js
   ### ACCUMULATOR SERVER STARTED ### Thu Feb 27 2014 12:03:41 GMT+0100 (CET)
   Req: 0
   Req: 0
   ```


* Parametrized execution:

    ```
    xxx@xlan:~/git/fiware-orion/scripts/testScripts$ node nodeAccumulator.js -s5 -p1030 -ds5 -dp1040 -v
    Setup ports starting at :  1030
    Setup Delayed ports starting at :  1040
    # Verbosity activated
    ### ACCUMULATOR SERVER  #1 STARTED at port 1030 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR SERVER  #2 STARTED at port 1031 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR SERVER  #3 STARTED at port 1032 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR SERVER  #4 STARTED at port 1033 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR SERVER  #5 STARTED at port 1034 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR Delayed SERVER #1 + STARTED at port 1040 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR Delayed SERVER #2 + STARTED at port 1041 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR Delayed SERVER #3 + STARTED at port 1042 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR Delayed SERVER #4 + STARTED at port 1043 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    ### ACCUMULATOR Delayed SERVER #5 + STARTED at port 1044 ### Thu Apr 03 2014 17:00:34 GMT+0200 (CEST)
    Req: 0
    Req: 0
    ```

Now accumulator server is ready to receive requests.

### SubscriptionTest_ONx.sh

* What is this:

    They are just a bash script to send massive amount of subscriptions to ContextBroker.
    It makes easier to launch the soak tests.

* How I use it: 

    ### SubscriptionTest_ONCHANGE.sh
    Usage:  ./subscriptionTest_ONCHANGE.sh endpoint:port amountOfSubscriptions conditionValue notificationEndpoint
    Example ./subscriptionTest_ONCHANGE.sh 127.0.0.1:1026 100 pressure http://127.0.0.1:1028/accumulate

    ### SubscriptionTest_ONTIMEINTERVAL.sh
    Usage: subscriptionTest_ONTIMEINTERVAL.sh endpoint:port amountOfSubscriptions updateTime notificationEndpoint
    Example ./subscriptionTest_ONTIMEINTERVAL.sh 127.0.0.1:1026 60 60 http://127.0.0.1:1028/accumulate

