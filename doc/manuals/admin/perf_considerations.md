<a name="top"></a>Performance and Scalability consideration
===========================================================

In order to better understand how to deploy Orion in production, we report here the results of non functional tests carried out by the FIWARE QA team and in other research initiatives.

# Orion Stress tests


For the execution of the tests it was configured a test environment consists of two physical machines with the same configuration, one for the deployment of Orion Context Broker (Version 1.6.0) and its database and another for client test and monitoring applications.

         CPU: 1 Intel(R) Xeon(R) E31230 
         CPU-GHz: 3.20
         Core: 4 cores and 8 threads
         RAM: 4GB DIMM DDR3 Synchronous 1333 MHz
         Cache: 128KB L1, 1MB L2, 8MB L3
         Hard-Disk: 128GB
         Operating-System: CentOS release 6.7 - 64 bits
         NICs: 2 interfaces 82574L Gigabit Network Connection

The performance tests executed are composed by nine scenarios:

1.  [Update Stress Scenario (1-20 attributes)](#scenario-1)
2.  [Update Stress Scenario (1-6 attributes)](#scenario-2)
3.  [Convenience Update Stress Scenario](#scenario-3)
4.  [NGSIv2 Update Stress Scenario](#scenario-4)
5.  [Update Stress with notifications](#scenario-5)
6.  [NGSIv2 Update Stress with notifications](#scenario-6)
7.  [Stability Scenario](#scenario-7)
8.  [Optimized Stability Scenario](#scenario-8)
9.  [No-cache Optimized Stability Scenario](#scenario-9)

Scenarios 1,2, 3 and 6 are the same than in the first Performance Testing launched over Orion in which it is adding a thread every six seconds to reach 300, for the Context Broker Update service. From a data set of 5000 entities are updated entities in a number of attributes ranging from 1 to 20. The scenario stops 10 minutes after the 300 concurrent threads are reached. The execution of this scenario begins with the database with the subscriptions necessary to perform the updates.

Scenarios 4 and 5 are updated scenarios over the NGSIv2 API instead NGSIv1. Scenario 2 is the same than scenario 1, but the number of attributes is lower, in order to compare the results with scenario 2 (NGSIv1 vs NGSIv2).

Scenarios 4 and 5 are the same than scenarios 1 and 3, but a previous load of 1000 subscriptions has been launched over the database, in order to makes Orion to generate notifications.

Scenario 7 Stability scenario, in which it is adding 3 thread every six seconds to reach 30:

-   1 for the Context Broker Subscription service,
-   1 for the Context Broker Update service and
-   1 for the Context Broker Convenience Update service.

The scenario stops afeter 10 hours. The goal of this scenario is to check if the system is degraded with a moderate load for a long period of time.

Scenario 8 is the same than scenario 6, with the optimizations recommended by Orion’s developer:

1 - Added the next parameters to Orion startup:

        -reqMutexPolicy none -writeConcern 0 -logLevel ERROR -notificationMode threadpool:q:n

2 - Created four indexes over the entities collection in the database:

        db.entities.createIndex( { "_id.id": 1 } )
        db.entities.createIndex( { "_id.type": 1 } )
        db.entities.createIndex( { "_id.servicePath": 1 } )
        db.entities.createIndex( { "attrNames": 1 } )

Finally scenario 9 is the same than scenario 8, previously disabling the cache.

The obtained results were:

[Top](#top)

## Scenario 1


From Update Stress Scenario (1-20 attributes), we can get:

-   Orion can handle near 514 update request per second, updating 1-20 attributes in each request.
-   In these conditions, response times are about 0.363 second.
-   The throughput is about 861 KBytes/sec
-   Reliability is 100% (there were no errors).
-   The CPU and the memory usage is stable.

## Scenario 2



From Update Stress Scenario (1-6 attributes), we can get:

-   Orion can handle near 700 update request per second, updating 1-6 attributes in each request.
-   In these conditions, average response time is about 267 milliseconds.
-   The throughput is about 660 KBytes/sec
-   The performance of the update requests is less proportional to the number of attributes updated in it (updating more attributes in a request is now better).
-   Reliability is 100% (there were no errors).
-   The CPU and the memory usage is stable.

## Scenario 3


From Convenience Update Stress Scenario, we can get:

-   Orion can handle about 183 convenience update request per second, updating 1-20 attributes in each request.
-   In these conditions, response times are about 1 second.
-   The throughput is about 275 KBytes/sec
-   Reliability is 100% (there were no errors).
-   The CPU usage is stable, and the memory usage is more stable.

## Scenario 4


From NGSIv2 Update Stress Scenario, we can get:

-   Orion can handle about 500 update request per second, updating 1-6 attributes in each request.
-   In these conditions, response times are about 373 milliseconds.
-   The throughput was 82KB/s, which is about the 10% of NGSIv1 case’s throughput.
-   Reliability is 100% (there were no errors).
-   The CPU and memory usages are stable.-

Response times are 75% higher, and the requests per second rate is 43% lower. We can conclude that NGSIv1 case can handle a higher data volume than NGSIv2, but the network usage is much higher (lower performance), then NGSIv1 has better data handling performance, but lower network usage performance.

## Scenario 5


From Update Stress with notifications, we can get:

-   Orion can handle almost 304 update request per second, updating 1-6 attributes in each request.
-   In these conditions, average response time is about 617 milliseconds.
-   The request per second rate is about a 27 lower due to the notifications generation. The average response time is about 35% higher.
-   Reliability is 100% (there were no errors).
-   17523 notifications were generated and sent.
-   The CPU and memory usages are stable.

## Scenario 6


From NGSIv2 Update Stress with notifications Scenario, we can get:

-   Orion can handle about 170 update request per second, updating 1-6 attributes in each request, which is 60% lower due to the notifications generation.
-   111063 notifications were generated and sent.
-   In these conditions, response times are about 1,110 milliseconds.
-   The throughput was 27KB/s, which is 60% lower than the case without notifications. Response times are about 50% higher.
-   Reliability is 100% (there were no errors).
-   The CPU and memory usages are stable.

## Scenario 7


From Stability Scenario, we can get:

-   With 30 threads, 10 for each operation, Orion handles around 6 requests per second.
-   In these conditions, average response time is similar for the three operations (around 4,9 seconds).
-   Reliability is 100% (there were no errors).
-   The CPU usage is stable, but the memory usage increases in the time. The system ran out of memory, but it didn’t fall.

## Scenario 8


From Optimized Stability Scenario, we can get:

-   With 30 threads, 10 for each operation, Orion handles around 27,7 requests per second.
-   In these conditions, average response time is around 0,483 seconds for subscriptions, 3,173 for convenience updates and 2,340 for updates.
-   The performance is much better. Now it can handle about 361% more requests than without the optimizations, and the response times, in average now are better too.
-   The CPU usage is stable, but the memory usage increases in the time, as it happened without optimizations, but due to the higher load handling, the system ran out of memory before, and fell after 3 hours, due to lack of memory.

## Scenario 9


From no-Cache Optimized Stability Scenario, we can get:

-   With 30 threads, 10 for each operation, Orion handles around 9 requests per second.
-   In these conditions, average response time is around 0,015 seconds for subscriptions, 22,541 for convenience updates and 23,630 for updates.
-   The performance is much worse. Now it can handle about a third of requests than with the cache enabled, and the response times are extremely high for updates and convenience updates operations.
-   The memory management problem doesn’t appear in this case, and the memory usage keeps stable along all the test.

# Scalability Test


The scalability tests performed for the Orion Context Broker concerned the most important functionality: the attribute update.
In order to measure the scalability of Context Broker three types of scenarios has been executed:

1.  [Scenario 1](#scalability-scenario-1)Context Broker nodes horizontal scaling; these tests were executed gradually increasing the number of CB nodes

2.  MongoDB shards horizontal scaling: these tests were executed gradually increasing the number of MongoDB shards

3.  Context Broker database connections configuration: these tests were executed gradually increasing the number of CB connections to the database

## Scalability Scenario 1


In this scenario, a single MongoDB node has been used, increasing the CB number of nodes gradually. Each node is in a separate virtual machine. It also has an Apache Balancer on a separate node. The following diagram shows the sample tested infrastructure.

![scalingcb1](Scaling_CB.png "Scaling_CB.png")

below are the results obtained for the three configurations obtained by adding a Context Broker at a time. We can see that there is no increase in throughput, but only a slight improvement in response time.




![scalingcb2](ScalingCBres.png "ScalingCBres.png")

## Scalability Scenario 2

![shardingMDB1](Sharding_MDB.png "Sharding_MDB.png")

below are the results obtained for the three configurations obtained by adding a Context Broker at a time. We can see that there is no increase in throughput, but only a slight improvement in response time.




![shardingMDB2](ShardingMDBres.png "ShardingMDBres.png")

# Latency ?

