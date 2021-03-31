# Temporal Representation of Entities
Starting with the Orion-LD release [Beta 2](https://github.com/FIWARE/context.Orion-LD/releases/), Orion-LD implements Temporal Representation of Entities (TRoE), in an experimental state.
The feature is minimally tested and not production-ready, but more or less working and the development team would be more than happy to have it tested, and any found bugs reported.

The sink used for TRoE is Postgres, with PostGIS and TimescaleDB extensions.

While Orion-LD takes care of populating the TRoE databases, another component handles the queries of temporal data - [Mintaka](https://github.com/FIWARE/Mintaka).

So, for queries of the temporal data, instead of sending the requests to Orion-LD, on (default) port 1026, the queries are sent to Mintaka, on (default) port xxxx.

## Database setup

For running orion-ld with TroE enabled, you need to setup a PostgreSQL with PostGIS and TimescaleDB. For local installations, we can recommend the 
[timescaledb-postgis image](https://hub.docker.com/layers/timescale/timescaledb-postgis/latest-pg12/images/sha256-40be823de6035faa44d3e811f04f3f064868ee779ebb49b287e1c809ec786994?context=explore).

You can start it with 
```
docker run -e POSTGRES_USER=orion -e POSTGRES_PASSWORD=orion -e POSTGRES_HOST_AUTH_METHOD=trust timescale/timescaledb-postgis:latest-pg12
```

## Database migration

You can find the databse migration scripts ins the [database-folder](../../database)   
The file [initial.sql](../../database/sql/initial.sql) does contain the sql script for the timescale database. It holds the schema at the state of orion-ld version 0.7.0.

### Migration

For database migration, we are providing a liquibase changelog, that can be used to migrate existing databases to the new schema. The 
changesets use the following ID schema: ```v<MIGRATION_NR>_step_<STEP_NR>```
The ```<MIGRATION_NR>``` is a continuously incrementing number. All steps of one migration to a certain orion version should use the same  ```<MIGRATION_NR>```.

### Run the migration

The database migration can be executed via a liquibase docker container. It needs access to the database. You can use the following script:

```
docker run 
           -v <DATABASE_FOLDER>:/liquibase/changelog
           liquibase/liquibase 
           --driver=org.postgresql.Driver 
           --url="jdbc:postgresql://<TIMESCALE_HOST>:<TIMESCALE_PORT>/<DATABSE_NAME>"
           --changeLogFile=timescale-changelog.xml 
           --username=<TIMESCALE_USERNAME>
           --password=<TIMESCALE_PASSWORD>
           update
```
(Replace ```<DATABASE_FOLDER>``` with a path [database](../../database))

You need to run the script for every tenant. Every tenant gets its own database, with the naming schema 
```<DEFAULT_DATABASE_NAME>_<TENANT_ID>```, for example orion_mytenant. 

### Version history

| Version | Description |
| ----------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| v1 | Change primary keys to include the timestamp in preparation of timescale-hypertable support, add index for (sub-)attributes. |