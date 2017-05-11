# Upgrading to 1.5.0 and beyond from a pre-1.5.0 version

This procedure has to be done *only* if your DB contains attributes with [ID metadata](../user/metadata.md#metadata-id-for-attributes).

-   Stop contextBroker
-   Remove previous contextBroker version

        yum remove contextBroker

-   [Take a backup of your
    DBs](database_admin.md#backup) (this is just a
    safety measure in case any problem occurs, e.g some script gets
    interrupted before finished and your database data ends in an
    incoherent state)
-   Download the following script:
    -   [change_attr_id_separator.py](https://github.com/telefonicaid/fiware-orion/blob/1.5.0/scripts/managedb/change_attr_id_separator.py)
-   Install pymongo (it is a script dependency) in case you don't have
    it previously installed

        pip-python install pymongo

-   Apply the change_attr_id_separator.py script to your DBs, using the
    following command (where 'db' is the database name). Note that if you are
    using
    [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation)
    you need to apply the procedure to each per-tenant/service database.
    The script can take a while, so an interactive progress counter
    is shown.

        python change_attr_id_separator.py orion

-   If you get any of the following messages, there is some problem that needs
    to be solved before going to the next step. Check the
    "Troubleshooting mdsvector2mdsobject" section below.

        WARNING: some problem was found during the process.
        ERROR: document ... change attempt failed

-   Install new contextBroker version (Sometimes the commands fails due
    to yum cache. In that case, run "yum clean all" and try again)

        yum install contextBroker

-   Start contextBroker

Note that the rpm command demands superuser privileges, so you have to
run it as root or using the sudo command.

## Troubleshooting

Three different kinds of problems may happen:

-   You get the following error message:

        OperationFailed: Sort operation used more than the maximum 33554432 bytes of RAM. Add an index, or specify a smaller limit.

    Try to create a suitable index with the following command in the mongo shell, then run the script again.

        db.entities.createIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

    Once migration has ended, you can remove the index with the following command:

        db.entities.dropIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

-   Due to some unexpected problem during DB updating. The symptom of
    this problem is getting errors like this one:

        - <n>: ERROR: document <...> change attempt failed!

    There is no a general solution for this problem, it has to be
    analyzed case by case. If this happens to you, please have a look at
    the [existing questions in StackOverflow about
    fiware-orion](http://stackoverflow.com/questions/tagged/fiware-orion)
    in order to check whether your problem is solved there. Otherwise create
    your own question (don't forget to include the "fiware-orion" tag
    and the exact error message in your case).

