# Database indexes analysis

The following analyzis shows the TPS (transation per second) and storage
consumption figures for different indexes configuration and number of
entities in the Orion Context Broker database. We have used Orion 0.14.0
in this analysis. Each transaction comprises one entity (either creating
it, querying for it or updating it).

Please, take into account that this information is provided only as a
hint to guide your decision about which indexes to use in your
particular set up, but the results in your particular environment may
differ depending on hardware profile, the particular entities being used
for the test, set up situation, etc. In this particular case, the
resources of the system under test (a VMware-based VM) are: 2 vCPU (on a
physical host based on Intel Xeon E5620@2.40GHz) and 4GB RAM. Both Orion
and MongoDB run in the same VM. The tool to generate load is JMeter
using the configuration that can be found at [the following
location](https://github.com/telefonicaid/fiware-orion/tree/develop/test/LoadTest)
(orionPerformanceOnlyQueries\_v2.0.jmx,
orionPerformanceOnlyAppends\_v2.0.jmx and
orionPerformanceAppendsAndUpdates\_v2.0.jmx) and running in a separated
VM (but in the same subnet, i.e. L2 connectivity with the system under
test.

Test cases are entity query, entity creation and mixing creation and update.

Throughput:


| Case - Indexes                                           |  10,000 entities  | 100,000 entities   | 1,000,000 entities |
|:---------------------------------------------------------|:----------------- |:------------------ |:------------------ |
| Query - none                                             | 115.3             | 12.2               | 2                  |
| Query - `_id.id`                                         | 2271.2            | 2225.7             | 2187.7             |
| Query - `_id.type`                                       | 40                | 4.6                | 1.8                |
| Query - separated `_id.id` and `_id.type`                | 2214.7            | 2179.3             | 2197.1             |
| Query - compound `{_id.id,_id.type}`                     | 2155.5            | 2174.4             | 2084.4             |
| Creation - none                                          | 64.5              | 17.8               | 2.4                |
| Creation - `_id.id`                                      | 748.2             | 672.9              | 698.3              |
| Creation - `_id.type`                                    | 33.5              | 4.9                | 2.1                |
| Creation - separated `_id.id` and `_id.type`             | 774.4             | 703.9              | 691.9              |
| Creation - compound `{_id.id,_id.type}`                  | 784.6             | 721.1              | 639.2              |
| Creation and update - none                               | 102.1             | 15.5               | 3.3                |
| Creation and update - `_id.id`                           | 1118.1            | 798.1              | 705.5              |
| Creation and update - `_id.type`                         | 32.6              | 4.8                | 1.8                |
| Creation and update - separated `_id.id` and `_id.type`  | 1145.3            | 746.4              | 706.5              |
| Creation and update - compound `{_id.id,_id.type}`       | 1074.7            | 760.7              | 636.1              |

Storage:

| Case - Indexes                                         | Index size (MB) |  Index size / DB file size   |
|:-------------------------------------------------------|:--------------- |:---------------------------- |
| 10,000 entities - none (\*)                            | 0.88            | 0.004                        |
| 10,000 entities - `_id.id`                             | 1.17            | 0.006                        |
| 10,000 entities - `_id.type`                           | 1.11            | 0.005                        |
| 10,000 entities - separated `_id.id` and `_id.type`    | 1.40            | 0.007                        |
| 10,000 entities - compound `{_id.id`,`_id.type}`       | 1.23            | 0.006                        |
| 100,000 entities - none (\*)                           | 8               | 0.041                        |
| 100,000 entities -  `_id.id`                           | 11              | 0.057                        |
| 100,000 entities -  `_id.type`                         | 10              | 0.052                        |
| 100,000 entities -  separated `_id.id` and `_id.type`  | 13              | 0.067                        |
| 100,000 entities -  compound `{_id.id`,`_id.type}`     | 12              | 0.062                        |
| 1,000,000 entities - none (\*)                         | 124             | 0.077                        |
| 1,000,000 entities - `_id.id`                          | 154             | 0.076                        |
| 1,000,000 entities - `_id.type`                        | 145             | 0.073                        |
| 1,000,000 entities - separated `_id.id` and `_id.type` | 175             | 0.088                        |
| 1,000,000 entities - compound (`_id.id`,`_id.type}`    | 161             | 0.079                        |

(\*) Althought we don't set any index, note that the MongoDB always set
up an index in `_id`. Thus, some ammount of space is always allocated to
indexes.

**Hint:** considering the above information, it is hihgly recommended to
set up an index on `_id.id` in the entities collection.
