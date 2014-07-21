## Index Behavior test

How it affects the use of indexes with a lot of entities and several concurrent  users  to the Time response and the TPS average.

#### Pre-conditions:

* "Jmeter" app exists in Launcher VM
* "ServerAgent" app exist in Launcher VM and ContextBroker VM
* "populateDB_v1.0.sh" script exists in Launcher VM
* "orionPerformanceOnlyQueries_v2.0.jmx" file in Launcher VM
* "orionPerformanceOnlyAppends_v2.0.jmx" file in Launcher VM
* "orionPerformanceAppendsAndUpdates_v2.0.jmx" file in Launcher VM
* Have a account in Loadosophia (http://loadosophia.org)
* verify contextBroker MD5 (http://wikis.hi.inet/fi-ware/index.php/Orion_QA#Instances_under_Test:)
* verify mongoDB 2.4 or higher
	
#### Steps:

1- verify that "ServerAgent" is listenning in Launcher VM and ContextBroker VM
```
nohup sh startAgent.sh --udp-port 0 --tcp-port 3450 > ~/monitor.log &
```
2- drop and populate the DB with "populateDB_v1.0.sh" script
```
db.dropDatabase()            -- drop a Db existent              
db.repairDatabase('DB')      -- reclaim disk space 
./populateDB_v1.0.sh http://hostname:port tenant max_element step 
    (we recommend to make a dump after the script execution)
```
3- Verify BD size with data
```
db.stats (1024) response in kBytes
```
4- Create or drop indexes according to the test performed
```
db.collection.ensureIndex({"<index>" : 1})    -- create a new index
db.collection.dropIndex ("<index>")           -- drop a index existent 
db.collection.getIndexes()                    -- return index existent
```
5- Verify BD size with data and indexes

6- launch jmeter scripts file in Launcher VM without/with indexes
```
./jmeter.sh -n -t  <script>.jmx  -JHOST=X.X.X.X -JTHREADS=100 -JRAMPUP=10 -JMAXAPPENDS=10000  \
-JRUNTIME=180 >> /<log_path>/jmeter_report_YYYYMMDD.log
```
7- Generate report with summary of tests groups

8- upload data in Loadosophia

9- Collect results

#### Scenarios:

* without indexes and only queries with DB dropped and populated
* with _id.id index only and only queries with DB dropped and populated
* with _id.type index only and only queries with DB dropped and populated
* with _id.id and _id.type indexes and only queries with DB dropped and populated
* without indexes and only appends with DB dropped and populated
* with _id.id index only and only appends with DB dropped and populated
* with _id.type index only and only appends with DB dropped and populated
* with _id.id and _id.type indexes and only appends with DB dropped and populated
* without indexes and appends and updates with DB dropped and populated
* with _id.id index only and appends and updates with DB dropped and populated
* with _id.type index only and appends and updates with DB dropped and populated
* with _id.id and _id.type indexes and appends and updates with DB dropped and populated