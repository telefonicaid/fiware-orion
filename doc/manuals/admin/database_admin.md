# Database administration 

We assume that the system administrator has knowledge of MongoDB (there
are very good and free courses at [MongoDB education
site](https://education.mongodb.com/)). Otherwise, we recommend to be very careful with the procedures described in this section.

## Backup

The usual procedure for MongoDB databases is used.

Use mongobackup command to get a backup of the Orion Context Broker
database. It is strongly recommended that you stop the broker before
doing a backup.

```
mongodump --host <dbhost> --db <db>
```

This will create the backup in the dump/ directory.

Note that if you are using
[multitenant/multiservice](../user/multitenancy.md)
you need to apply the procedures to each per-tenant/service database

## Restore

The usual procedure for MongoDB databases is used.

Use the mongorestore command to restore a previous backup of the Orion
Context Broker database. It is strongly recommended that you stop the
broker before doing a backup and to remove (drop) the database used by
the broker.

Let's assume that the backup is in the dump/<db> directory. To restore
it:

```
mongorestore --host <dbhost> --db <db> dump/<db>
```

Note that if you are using
[multitenant/multiservice](../user/multitenancy.md)
you need to apply the procedures to each per-tenant/service database

## Database authorization

MongoDB authorization is configured with the `-db`, `-dbuser` and `-dbpwd`
options ([see section on command line
options](#Command_line_options "wikilink")). There are different cases
to take into account:

-   If your MongoDB instance/cluster doesn't use authorization at all,
    then don't use the `-dbuser` and `-dbpwd` options.
-   If your MongoDB instance/cluster uses authorization , then:
    -   If you run Orion in single service/tenant mode (i.e.
        without `-multiservice`) then you are using only one database
        (the one specified by the -db option) and the authorization is
        done with `-dbuser` and `-dbpwd` in that database.
    -   If you run Orion in multi service/tenant mode (i.e.
        with + -multiservice`) then the authorization is done at `admin`
        database using `-dbuser` and `-dbpwd`. As described [later in this
        document](#Multiservice/multitenant_database_separation "wikilink"),
        in multi service/tenant mode, Orion uses several databases
        (which in addition can potentially be created on the fly), thus
        authorizing on `admin` DB ensures permisions in all them.

## Multiservice/multitenant database separation

Normally, Orion Context Broker uses just one database at MongoDB level
(the one specified with the `-db` command line option, typically "orion").
However, when [multitenant/multiservice is
used](Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide#Multi_service_tenancy "wikilink")
the behaviour is different and the following databases are used (let
`<db>` be the value of the `-db` command line option):

-   The database `<db>` for the default tenant (typically, `orion`)
-   The database `<db>-<tenant>` for service/tenant `<tenant>` (e.g. if
    the tenant is named `tenantA` and default `-db` is used, then the
    database would be `orion-tenantA`.

Per-service/tenant databases are created "on the fly" as the first
request involving tenant data is processed by Orion. Note that there is
a limitation in MongoDB current versions of 24,000 namespaces (each
collection or index in a database consumes a namespace). Orion currently
uses 5 collections per database, thus taking into account each
collection involves also at least the \_id index, that will end in a
2,400 services/tenants limit (less if you have more indexes in place).

Finally, in the case of per-service/tenant databases, all collections
and administrative procedures (backup, restore, etc.) are associated to
each particular service/tenant database.

## Delete complete database

This operation is done using the MongoDB shell:

```
mongo <host>/<db>
> db.dropDatabase()
```

## Setting indexes

Orion Context Broker doesn't ensure any index in the database collection
(except for one exception, described at the end of this section) in
order to let flexibility to database administrators. Take into account
that index usage involves a tradeoff between read efficiency (usage of
indexes generally speeds up reads) and write efficiency (the usage of
indexes slow down writes) and storage (indexes consume space in database
and mapped RAM memory) and that is the administrator (not Orion) who has
to decide what to priorize.

However, in order to help administrator in that task, the following
indexes could be recommended:

-   Collection [entities](#entities_collection "wikilink")
    -   \_id.id (used by queryContext and related
        convenience operations)
    -   \_id.type (used by queryContext and related
        convenience operations)
    -   \_id.servicePath (used by queryContext and related
        convenience operations)
    -   creDate (used to provided ordered results in queryContext and
        related convenience operations)
-   Collection [registrations](#registrations_collection "wikilink")
    -   \_id (used to provided ordered results in
        discoverContextAvailability and related convenience operations).
        We include this index here for the sake of completeness, but the
        administrator doesn’t need to explicitly ensure it, given that
        MongoDB automatically provides a mandatory index for \_id in
        every collection.

The only index that Orion Context Broker actually ensures is the
"2dsphere" one in the location.coords field in the entities collection,
due to functional needs (in order [geo-location
functionality](Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide#Geolocation_capabilities "wikilink")
to work). The index is ensured at Orion startup or when entities are
created for first time.

### Analysis

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

```
  Test                                Indexes   10,000 entities   100,000 entities   1,000,000 entities
  ----------------------------------- --------- ----------------- ------------------ --------------------
  Entity query                        None      115.3             12.2               2
  \_id.id                             2271.2    2225.7            2187.7
  \_id.type                           40        4.6               1.8
  separated \_id.id and \_id.type     2214.7    2179.3            2197.1
  compound (\_id.id, \_id.type)       2155.5    2174.4            2084.4
  Entity creation                     None      64.5              17.8               2.4
  \_id.id                             748.2     672.9             698.3
  \_id.type                           33.5      4.9               2.1
  separated \_id.id and \_id.type     774.4     703.9             691.9
  compound (\_id.id, \_id.type)       784.6     721.1             639.2
  Mixing entity creation and update   None      102.1             15.5               3.3
  \_id.id                             1118.1    798.1             705.5
  \_id.type                           32.6      4.8               1.8
  separated \_id.id and \_id.type     1145.3    746.4             706.5
  compound (\_id.id, \_id.type)       1074.7    760.7             636.1

  Case                              Indexes     Index size (MB)   Index size / DB file size
  --------------------------------- ----------- ----------------- ---------------------------
  10,000 entities                   None        0.88 (\*)         0.004
  \_id.id                           1.17        0.006
  \_id.type                         1.11        0.005
  separated \_id.id and \_id.type   1.40        0.007
  compound (\_id.id, \_id.type)     1.23        0.006
  100,000 entities                  None (\*)   8                 0.041
  \_id.id                           11          0.057
  \_id.type                         10          0.052
  separated \_id.id and \_id.type   13          0.067
  compound (\_id.id, \_id.type)     12          0.062
  1,000,000 entities                None (\*)   124               0.077
  \_id.id                           154         0.076
  \_id.type                         145         0.073
  separated \_id.id and \_id.type   175         0.088
  compound (\_id.id, \_id.type)     161         0.079

(\*) Althought we don't set any index, note that the MongoDB always set
up an index in \_id. Thus, some ammount of space is always allocated to
indexes.
```

**Hint:** considering the above information, it is hihgly recommended to
set up an index on `_id.id` in the entities collection.

## Database management scripts

Orion Context Broker comes along with some scripts that can be use to do
some browsing and administrative actions in the database, installed in
the `/usr/share/contextBroker` directory.

In order to use them, you need to install the pymongo driver (version
2.5 or above) as a requirement to run it, typically using (run it as
root or using the sudo command):

` pip-python install pymongo`

### Deleting expired documents

NGSI specifies an expiration time for registrations and subcriptions
(both NGSI9 and NGSI10 subscriptions). Orion Context Broker doesn't
delete the expired documents (it just ignores them) due to the fact that
expired registrations/subscription can be "re-activated" by an update of
their duration.

However, expired registrations/subscriptions consume space in the
database, so they can be "purged" from time to time. In order to help
you in that task, the garbage-collector.py script is provided along with
the Orion Context Broker (in
/usr/share/contextBroker/garbage-collector.py after installing the RPM).

The garbage-collector.py looks for expired documents in registrations,
csubs and casubs collection, "marking" them with the following field:

```
{
  ...,
  "expired": 1,
  ...
}
```

The garbage-collector.py program takes as arguments the collection to be
analyzed, e.g. to analyze csubs and casubs, run:

```
garbage-collector.py csubs casubs
```

After running garbage-collector.py you can easily remove the expired
documents using the following commands in the mongo console:

```
mongo <host>/<db>
> db.registrations.remove({expired: 1})
> db.csubs.remove({expired: 1})
> db.casubs.remove({expired: 1})
```

### Latest updated document

You can take an snapshot of the lastest updated entities and attributes
in the database using the lastest-updates.py script. It takes up to four
arguments:

-   Either "entities" or "attributes", to set the granularity level in
    the updates.
-   The database to use (same than the -db parameter and
    BROKER\_DATABASE\_NAME used by the broker). Note that the mongod
    instance has to run in the same machine where the script runs.
-   The maximum number of lines to print
-   (Optional) A filter for entity IDs, interpreted as a regular
    expression in the database query.

Ej:

    # lastest-updates.py entities orion 4
    -- 2013-10-30 18:19:47: Room1 (Room)
    -- 2013-10-30 18:16:27: Room2 (Room)
    -- 2013-10-30 18:14:44: Room3 (Room)
    -- 2013-10-30 16:11:26: Room4 (Room)


