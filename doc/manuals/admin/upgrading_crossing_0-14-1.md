# Upgrading to 0.14.1 and beyond from any pre-0.14.1 version

In the case you have previous information in the database used by Orion
Context Broker and that database contains geo-located entities you need
to apply the following procedure. Otherwise, you can use the default
upgrading procedure.

-   Stop contextBroker
-   Remove previous contextBroker version

        yum remove contextBroker

-   [Take a backup of your
    DBs](database_admin.md#backup) (this is just a
    safety measure in the case some problem occurs, e.g the
    swap\_coords.js script gets interrupted before finished and your
    database data ends in a incoherent status)
-   Download the following script:
    [swap\_coords.js](https://github.com/telefonicaid/fiware-orion/tree/0.14.1-FIWARE-3.5.1/scripts/managedb/swap_coords.js)
-   Apply the script to your DBs, using the following (where 'db' is the
    database name). Note that if you are using
    [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation)
    you need to apply the procedure to each per-tenant/service database.

        mongo <db> swap_coords.js

-   Install new contextBroker version (Sometimes the commands fails due
    to yum cache. In that case, run "yum clean all" and try again)

        yum install contextBroker

-   Start contextBroker

Note that the rpm command demand superuser privileges, so you have to
run them as root or using the sudo command.