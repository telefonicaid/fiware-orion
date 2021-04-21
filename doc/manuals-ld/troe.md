# Temporal Representation of Entities
Starting with the Orion-LD release [Beta 2](https://github.com/FIWARE/context.Orion-LD/releases/), Orion-LD implements Temporal Representation of Entities (TRoE), in an experimental state.
The feature is minimally tested and not production-ready, but more or less working and the development team would be more than happy to have it tested, and any bugs reported.

The sink used for TRoE is Postgres, with PostGIS and TimescaleDB extensions.

While Orion-LD takes care of populating the TRoE databases, another component handles the queries of temporal data - [Mintaka](https://github.com/FIWARE/Mintaka).

So, for queries of the temporal data, instead of sending the requests to Orion-LD, on (default) port 1026, the queries are sent to Mintaka, on (default) port 8080.

## Database setup
To run Orion-LD with TroE enabled, a PostgreSQL with PostGIS and TimescaleDB is needed.
For local installations, this [timescaledb-postgis image](https://hub.docker.com/layers/timescale/timescaledb-postgis/latest-pg12/images/sha256-40be823de6035faa44d3e811f04f3f064868ee779ebb49b287e1c809ec786994?context=explore) is recommended.

To start it, use:
```
docker run -e POSTGRES_USER=orion -e POSTGRES_PASSWORD=orion -e POSTGRES_HOST_AUTH_METHOD=trust timescale/timescaledb-postgis:latest-pg12
```

## Database Migration
A database migration scripts is found in the [database-folder](../../database)
The file [initial.sql](../../database/sql/initial.sql) contains the SQL script for the timescale database. It holds the schema at the state of Orion-LD version 0.7.0.

### Migration
For database migration, a liquibase changelog is provided. This changelog can be used to migrate existing databases to the new schema.
The  changesets use the following ID schema: ```v<MIGRATION_NR>_step_<STEP_NR>```
The ```<MIGRATION_NR>``` is a continuously incrementing number.
All steps of one migration to a certain Orion-LD version should use the same  ```<MIGRATION_NR>```.

### Running the Migration

The database migration can be executed via a liquibase docker container. It needs access to the database.
The following script can be used:
```
docker run 
           -v <DATABASE_FOLDER>:/liquibase/changelog
           liquibase/liquibase 
           --driver=org.postgresql.Driver 
           --url="jdbc:postgresql://<TIMESCALE_HOST>:<TIMESCALE_PORT>/<DATABSE_NAME>"
           --changeLogFile=timescale-changelog_master.xml 
           --username=<TIMESCALE_USERNAME>
           --password=<TIMESCALE_PASSWORD>
           update
```
(Replace ```<DATABASE_FOLDER>``` with a path [database](../../database))

The script needs to be executed for every tenant, as each tenant in held in a database of its own.
The naming schema of the "tenant databases" is:
```<DEFAULT_DATABASE_NAME>_<TENANT_ID>```, e.g., "orion_mytenant".

Every migration has its own changelog file, they are postfixed with the given version. When running on top of a database initialized with the 
[initial.sql](../../database/sql/initial.sql), the [timescale-changelog_master.xml](../../database/timescale-changelog_master.xml) can be used to apply 
all changesets at once. If the db is already updated to a certain version, only the changelogs after that version should be used.

> :warning: Be aware that the runtime of an update can be substantial(minutes to hours) when executed at a huge database. The existence of a an up-to-date 
> backup should be assured!

### Version history
| Version | Description |
| ----------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| v1 | Changed primary keys to include the timestamp in preparation of timescale-hypertable support, added index for (sub-)attributes. |
| v2 | Added the datasetId to the combined primary key and optimizes its datatype. |
