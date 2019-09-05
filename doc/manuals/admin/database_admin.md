# <a name="top"></a>Database administration

* [Introduction](#introduction)
* [Backup](#backup)
* [Restore](#restore)
* [Database authorization](#database-authorization)
* [Multiservice/multitenant database separation](#multiservicemultitenant-database-separation)
* [Delete complete database](#delete-complete-database)
* [Setting indexes](#setting-indexes)
* [Database management scripts](#database-management-scripts)
    * [Deleting expired documents](#deleting-expired-documents)
    * [Latest updated document](#latest-updated-document)
* [Orion Errors due to Database](#orion-errors-due-to-database)
	  
## Introduction

We assume that the system administrator has knowledge of MongoDB (there
are very good and free courses at [MongoDB education
site](https://education.mongodb.com/)). Otherwise, we recommend to be very careful with the procedures described in this section.

## Backup

The usual procedure for MongoDB databases is used.

Use mongobackup command to get a backup of the Orion Context Broker
database. It is strongly recommended that you stop the broker before
doing a backup.

```
mongodump --host <dbhost> --db <db>
```

This will create the backup in the dump/ directory.

Note that if you are using
[multitenant/multiservice](#multiservicemultitenant-database-separation)
you need to apply the procedures to each per-tenant/service database

[Top](#top)

## Restore

The usual procedure for MongoDB databases is used.

Use the mongorestore command to restore a previous backup of the Orion
Context Broker database. It is strongly recommended that you stop the
broker before doing a backup and to remove (drop) the database used by
the broker.

Let's assume that the backup is in the dump/<db> directory. To restore
it:

```
mongorestore --host <dbhost> --db <db> dump/<db>
```

Note that if you are using
[multitenant/multiservice](#multiservicemultitenant-database-separation)
you need to apply the procedures to each per-tenant/service database.

[Top](#top)

## Database authorization

MongoDB authorization is configured with the `-db`, `-dbuser` and `-dbpwd`
options ([see section on command line
options](cli.md)). There are a few different cases
to take into account:

-   If your MongoDB instance/cluster doesn't use authorization,
    then do not use the `-dbuser` and `-dbpwd` options.
-   You can specify authentication mechanism with `-dbAuthMech`.
-   If your MongoDB instance/cluster uses authorization , then:
    -   If you run Orion in single service/tenant mode (i.e.
        without `-multiservice`) then you are using only one database
        (the one specified by the -db option) and the authorization is
        done with `-dbuser` and `-dbpwd` in that database.
    -   If you run Orion in multi service/tenant mode (i.e.
        with `-multiservice`) then the authorization is done at `admin`
        database using `-dbuser` and `-dbpwd`. As described [later in this
        document](#multiservicemultitenant-database-separation),
        in multi service/tenant mode, Orion uses several databases
        (which in addition can potentially be created on the fly), thus
        authorizing on `admin` DB ensures permissions in all of them.
    -   Anyway, you can override the above default with `-dbAuthDb` and
        specify the authentication DB you want.

Let's consider the following example. If your MongoDB configuration is so you typically access to it
using:

```
mongo "mongodb://example1.net:27017,example2.net:27017,example3.net:27017/orion?replicaSet=rs0" --ssl --authenticationDatabase admin --username orion --password orionrules
```

Then the equivalent connection in Context Broker CLI parameters will be:


```
-dbhost examples1.net:27017,example2.net:27017,example3.net:27017 -rplSet rs0 -dbSSL -dbAuthDb admin -dbuser orion -dbpwd orionrules
```


     
[Top](#top)

## Multiservice/multitenant database separation

Normally, Orion Context Broker uses just one database at MongoDB level
(the one specified with the `-db` command line option, typically "orion").
However, when [multitenant/multiservice](#multiservicemultitenant-database-separation) is used
the behaviour is different and the following databases are used (let
`<db>` be the value of the `-db` command line option):

-   The database `<db>` for the default tenant (typically, `orion`)
-   The database `<db>-<tenant>` for service/tenant `<tenant>` (e.g. if
    the tenant is named `tenantA` and default `-db` is used, then the
    database would be `orion-tenantA`.

Per-service/tenant databases are created "on the fly" as the first
request involving tenant data is processed by Orion.

Finally, in the case of per-service/tenant databases, all collections
and administrative procedures (backup, restore, etc.) are associated to
each particular service/tenant database.

[Top](#top)

## Delete complete database

This operation is done using the MongoDB shell:

```
mongo <host>/<db>
> db.dropDatabase()
```
[Top](#top)

## Setting indexes

Check [database indexes section](perf_tuning.md#database-indexes) in the
performance tuning documentation.

[Top](#top)

## Database management scripts

Orion Context Broker comes with a few scripts that can be used for
browsing and administrative activities in the database, installed in
the `/usr/share/contextBroker` directory.

In order to use these scripts, you need to install the pymongo driver (version
2.5 or above), typically using (run it as
root or using the sudo command):

` pip-python install pymongo`

[Top](#top)

### Deleting expired documents

NGSI specifies an expiration time for registrations and subcriptions
(both context and context availability subscriptions). Orion Context Broker doesn't
delete the expired documents (they are just ignored) as
expired registrations/subscription can be "re-activated" using a subscription update request,
modifying their duration.

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
  "expired": 1,
  ...
}
```

The garbage-collector.py program takes as arguments the collection to be
analyzed. E.g. to analyze csubs and casubs, run:

```
garbage-collector.py csubs casubs
```

After running garbage-collector.py you can easily remove the expired
documents using the following commands in the mongo console:

```
mongo <host>/<db>
> db.registrations.remove({expired: 1})
> db.csubs.remove({expired: 1})
> db.casubs.remove({expired: 1})
```
[Top](#top)

### Latest updated document

You can take an snapshot of the latest updated entities and attributes
in the database using the latest-updates.py script. It takes up to four
arguments:

-   Either "entities" or "attributes", to set the granularity level in
    the updates.
-   The database to use (same as the -db parameter and
    BROKER\_DATABASE\_NAME used by the broker). Note that the mongod
    instance has to run in the same machine where the script runs.
-   The maximum number of lines to print
-   (Optional) A filter for entity IDs, interpreted as a regular
    expression in the database query.

Ej:

    # latest-updates.py entities orion 4
    -- 2013-10-30 18:19:47: Room1 (Room)
    -- 2013-10-30 18:16:27: Room2 (Room)
    -- 2013-10-30 18:14:44: Room3 (Room)
    -- 2013-10-30 16:11:26: Room4 (Room)

[Top](#top)

## Orion Errors due to Database

If you are retreiving entities using a large offset value and get this error:

```
GET /v2/entities?offset=54882

{
    "description": "Sort operation used more than the maximum RAM. You should create an index. Check the Database Administration section in Orion documentation.",
    "error": "InternalServerError"
}
```

then the DB has raised an error related to sorting operation failure due to lack of resources. You can
check that the Orion log file contains an ERROR trace similar to this one:

```
Raising alarm DatabaseError: nextSafe(): { $err: "Executor error: OperationFailed Sort operation used more than the maximum 33554432 bytes of RAM. Add an index, or specify a smaller limit.", code: 17144 }
```

The typical solution to this is to create an index in the field used for sorting. In particular,
if you are using the default entities ordering (based on creation date) you can create the
index with the following command at mongo shell:

```
db.entities.createIndex({creDate: 1})
```

[Top](#top)
