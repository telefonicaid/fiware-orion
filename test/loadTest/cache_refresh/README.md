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
- 1 node for mongoDB (associated to CBs)
- 1 node for launch listener and viewer
- 1 node for launch jmeter


#### Scripts used:

- **append_entities_long_time_v1.0.jmx**: Jmeter script used to generate N Append Requests with an estable TPS. Additionally is possible to create a subscription before all append requests.
     
  Parameters:
  ```
     HOST: CB or balancer host (default: localhost)
     PORT: CB or balancer port (default: 1026)
     THREADS: number of threads used in the test (default: 1)
     TESTTIME: duration time of the test (default: 60)
     SERVICE: service header used in requests (default: entities_append_long_time)
     SERVICE_PATH: service path header used in requests (default: /test)
     ENTITIES: number of consecutive entities in each thread (default: 10)
     SUBSCRIPTION: optionally is possible to create a subscription before all appends requests, in case of you would like to create use CREATE as value (default: nothing)
     SUBS_DURATION: duration time of the subscription, used if the subscription is create previously (default: 60)
     LISTEN_HOST: listen host used per receive notifications, used if the subscription is create previously (default: localhost)
     LISTEN_PORT: listen por used per receive notifications, used if the subscription is create previously  (default: 8090)
  ```
  
  Example:
  ```
  sh jmeter -n  -t <path>/append_entities_long_time_v1.0.jmx  -JHOST=<balancer_host>  -JENTITIES=1000 -JTESTTIME=50000 -JTHREADS=100 -JTPS=100
  ```
 
- **requests_listener.py**: listener used to counter all POST request received.
  
  Parameters:
  ```
     -u: show the usage (Optional)
     -hide: hide info by console (current body and total of requests) (Optional) 
     -p: change of mock port (by default 8090) (Optional)
  ```              
  
  API requests:     
   ```
      GET /receive - returns total requests and TPS since first request to last request received.    
      GET /reset   - reset requests counter
  ```
  
  Example:
  ```
      python requests_listener.py -p=4567 -hide
  ```
  
- **reqs_x_secs.sh**: show in table mode all requests received each second in the listener, with the total requests and the TPS between the first until the last request received.
  
  Parameters:
  ```
     <url>: host and port used by requests_listener.py script (MANDATORY)
     -reset: it is used to reset the listener before starting the test (total requests and TPS)  (Optional)
  ```
  
  Example:
  ```
     ./reqs_x_secs..sh localhost:4567 -reset  
  ```
  
- **subscription_manager.py**: create, update or delete a subscription associated to appends generated with Jmeter script `append_entities_long_time_v1.0.jmx`.
                               It is necessary to import `requests` library. `pip install requests`. 
  Parameters:
  ```                                                                       
       <host>               : CB or balancer host (MANDATORY).
       -u                   : show this usage (Optional).  
       -operation=<value>   : operations allowed: create | update | delete (default: create) (Optional).       
       -duration=<value>    : used only in create and update operations (default: 1000S) (Optional)           
       -throttling=<value>  : used only in create and update operations (default: it is not used) (Optional)  
       -subs_id=<value>     : used only in delete and update operation (default: NotDefined) (Optional)                                
       -service=<value>     : service header (default: entities_append_long_time) (Optional)                                                          
       -service_path=<value>: service path header (default: /test)(Optional)                                                     
       -refer_url=<value>   : reference url used to notifications in create operation (default: http://localhost:9999/notify (Optional)                                                                  
  ```
  
  Examples:
  ```
       python subscription_manager.py localhost -operation=create -duration=1000S -throttling=20S               
       python subscription_manager.py localhost -operation=update -duration=1000S -subs_id=51c04a21d714fb3b37d7d5a7
       python subscription_manager.py localhost -operation=delete -subs_id=51c04a21d714fb3b37d7d5a7   
  ```
 
- **drop_database_mongo.sh**: used to delete all databases with a prefix in mongo
  
  Parameters:
  ```
    <host>: mongo host (MANDATORY)
    <prefix>: databases prefix (default: orion) (Optional)
  ```
  
  Example:
  ```
    ./drop_database_mongo.sh localhost test
  ```

      
#### Steps to execute the test:

 1- modify contextBroker config in each node with `-subCacheIval` parameter.
 
 2- update `nginx` system and restart it.
 
 3- start `requests_listener.py` script.
 
 4- start `reqs_x_secs.sh` script.
 
 5- delete all dbs with orion prefix in Mongo, with `drop_database_mongo.sh` script.
  
 6- launch jmeter script `append_entities_long_time_v1.0.jmx` with N TPS for a long time.

 
#### Test plan
 
 - Use `subscription_manager.py` script to subscriptions management.
 
 - Use `reqs_x_secs.sh` script to review the notifications received in the listener each second (by console)
 ```
1- create a subscription with duration of 1000s
   - verify that the number of notifications goes up until 50% when the subscription is created and goes up until 100%, when in the other node the cache is refreshed. 
   - remove previous subscription  
   - verify that the number of requests comes down until 50% when the subscription is deleted and comes down until 0%, when in the other node the cache is refreshed. 

2- create a subscription with duration of 50s 
   - verify that the number of notifications goes up until 50% when the subscription is created and goes up until 100%, when in the other node the cache is refreshed but at 50s, not reach more notifications 0%.
   - remove previous subscription  
   - verify that it remains at 0 % 

3- create a subscription with duration of 1000s and throttling of 20s
   - verify that begins to notify but wait 20s between notifications 
   - remove previous subscription
   - verify notifies last time and stops

4- create a subscription with duration of 50s and throttling 40s
   - verify that comes only one or two notifications and nothing else
   - remove previous subscription  
   - verify that nothing arrives

5- create a subscription with duration of 50s and throttling 40s
   - verify that comes only one or two notifications and nothing else
   - update the previous subcription with duration of 160s
   - verify that notifications are arriving every 40s and after of 160s do not reach more
   - remove previous subscription  
   - verify that nothing arrives

6- create a subscription with duration of 50s and throttling 40s 
   - verify that comes only one or two notifications and nothing else.
   - update the previous subcription with duration of 90s and throttling of 5s 
   - verify that notifications are arriving every 5s and after of 90s do not reach more
   - remove previous subscription 
   - verify that nothing arrives    
 ```
 
 
#### Summary:
These tests are very complicated to automate, because the system should be balanced and the responses (total requests each seconds) are not the same in each execution. So that, these tests should be executed manually.
