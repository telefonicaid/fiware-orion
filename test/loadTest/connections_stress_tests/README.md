## Connections Stress Test

Verify that ContextBroker works properly with a large number of stablished connections.

#### Tests procedure:
- drop the database in mongo
- create 5000 subscriptions with subject.entities.idPattern: .*
- verify/modify the notification listener with a delay of 10 minutes before answering (recomend to use `notif_listener_with_delay_in_response` script).
- modify the ContextBroker config with: `"-httpTimeout 600000 -notificationMode threadpool:60000:5000"` and restart it.
- launch an entity update, that it triggers all subscriptions.
- launch indefinitely a "/version" request per second and:
     - report that its response is correct.
     - report the number of established connections (if `-noEstablished` param is used this column is ignored)
     - report the queue size into ContextBroker (if `-noQueueSize` param is used this column is ignored)

**Note**: the contextBroker should be started by command (`/usr/bin/contextBtoker ...`) instead of service (service contextBroker start), because the VMs are limited the number of threads (1024) in all users except in `root`.
     
#### Mehod of use:
- firstly, launch established connections listener (rpyc_classic.py) in CB machine. 
    - download from `https://pypi.python.org/pypi/rpyc`
    - install the dependencies: `pip install rpyc psutil`. If you have problem with the `psutil` installation, use `yum install python-devel python-psutil` to install it on your CentOS system.
    - unzip and execute `python bin/rpyc_classic.py`.
- after, launch the notifications listener `./notif_listener_with_delay_in_response`.
- ContextBroker configuration recommended:
```
    BROKER_EXTRA_OPS="-reqMutexPolicy none -writeConcern 0 -httpTimeout 600000 -notificationMode threadpool:60000:5000 -statTiming -statSemWait -statCounters -statNotifQueue -multiservice -subCacheIval 5"
```   
- finally, execute in local the `connections_stress_test.py` script.
   
#### Usage
       Parameters:                                                                                                  
          -host=<host>         : CB host (OPTIONAL) (default: localhost).                                           
          -u                   : show this usage (OPTIONAL).                                                        
          -v                   : verbose with all responses (OPTIONAL) (default: False). 
          -noEstablished       : is used to ignore the established connections (OPTIONAL) (default: False).                         
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

#### notif_listener_with_delay_in_response script
It is a server to receive all notifications sent from CB, the port used is `8090` and any path is allowed. 
The delay is the 10 minutes.

#### Logging
All the information is logged in `connections_stress_tests.log` in the same path.
