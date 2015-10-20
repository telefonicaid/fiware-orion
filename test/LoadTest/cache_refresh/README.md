# Cache refresh Tests

These tests are used to verify the cache refresh in a balanced system.


#### Requirements

- Bash shell (linux)
- Jmeter (2.11 verson or higher)
- Python (2.7.x version)
- Nginx (1.0.15 or higher)


#### Architecture for testing

- 1 node for balancer (nginx)
- 2 nodes for context broker installed
- 1 node for mongo db (associated to CBs)
- 1 node for launch listener and viewer
- 1 node for launch jmeter


#### Scripts used:

- **append_entities_long_time_v1.0.jmx**: Jmeter script used to generate N Append Requests with an estable TPS. Additionally is possible to create a subscription before all append requests.
     
  Parameters:
  ```
     HOST: CB or balancer host (by default, localhost)
     PORT: CB or balancer port (by default, 1026)
     THREADS: number of threads used in the test (by default, 1)
     TESTTIME: duration time of the test (by default, 60)
     SERVICE: service header used in requests (by default, entities_append_long_time)
     SERVICE_PATH: service path header used in requests (by default, /test)
     ENTITIES: number of consecutive entities in each thread (by default, 10)
     SUBSCRIPTION: optionally is possible to create a subscription before all appends requests, in case of you would like to create use CREATE as value (by default, nothing)
     SUBS_DURATION: duration time of the subscription, used if the subscription is create previously (by default, 60)
     LISTEN_HOST: listen host used per receive notifications, used if the subscription is create previously (by default, localhost)
     LISTEN_PORT: listen por used per receive notifications, used if the subscription is create previously  (by default, 8090)
  ```
  
  Example:
  ```
  sh jmeter -n  -t <path>/append_entities_long_time_v1.0.jmx  -JHOST=<balancer_host>  -JENTITIES=1000 -JTESTTIME=50000 -JTHREADS=100 -JTPS=100
  ```
 
- **requests_listener.py**: listener used to counter all POST request received.
  
  Parameters:
  ```
     -u: show the usage.                                
     -hide: hide info by console (current body and total of requests). 
     -p: change of mock port (by default 8090).
  ```              
  
  API requests:     
   ```
      GET /receive - returns total requests and TPS since first request to last request received.    
      GET /reset   - reset requests counter
  ```
  
  Example:
  ```
      python requests_listener.py -p=4567
  ```
  
- **reqs_x_secs.sh**: show in table mode all requests received each second in the listener, with the total requests and the TPS between the first until the last request received.
  
  Parameters:
  ```
     <url>: host and port used by requests_listener.py script (MANDATORY)
  ```
  
  Example:
  ```
     ./reqs_x_secs..sh localhost:4567    
  ```
  
- **subs_create_delete.sh**: create or delete a subscription associated to appends generated with Jmeter script `append_entities_long_time_v1.0.jmx`
  
  Parameters:
  ```
     <host>: host used to create or delete a subscription (MANDATORY)
     <subscriptionId>: used to delete a subscription (only used in delete action) (OPTIONAL)        
  ```
  
  Examples:
  ```
    ./subs_create_delete.sh localhost 
    ./subs_create_delete.sh localhost 51c04a21d714fb3b37d7d5a7
  ```
 
- **drop_database_mongo.sh**: used to delete all databases with a prefix in mongo
  
  Parameters:
  ```
    <host>: mongo host (MANDATORY)
    <prefix>: databases prefix (by default, orion) (OPTIONAL)
  ```
  
  Example:
  ```
    ./drop_database_mongo.sh localhost
  ```

      
#### Steps to execute the test:

 1- modify contextBroker config in each node with `-subCacheIval` parameter.
 
 2- update `nginx` system and restart it.
 
 3- start `requests_listener.py` script.
 
 4- start `reqs_x_secs.sh` script.
 
 5- delete all dbs with orion prefix in Mongo, with `drop_database_mongo.sh` script.
  
 6- launch jmeter script `append_entities_long_time_v1.0.jmx` for a long time
 
 7- create a subscription with `subs_create_delete.sh` script.
 
 8- review that the number of requests goes up until 50% when the subscription is created and goes up until 100%, when in the other node the cache is refreshed (in `reqs_x_secs.sh` script by console).
 
 9- delete the subscription created previously with `subs_create_delete.sh` script.
 
10- review that the number of requests comes down until 50% when the subscription is deleted and comes down until 0%, when in the other node the cache is refreshed (in `reqs_x_secs.sh` script by console).


#### Summary:
These tests are very complicated to automate, because the system should be balanced and the responses (total requests each seconds) are not the same in each execution. So that, these tests should be executed manually.
