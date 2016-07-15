# Upgrading to 1.3.0 and beyond from a pre-1.3.0 version

This procedure has to be done always as long as your database contains
at least one entity document.

-   Stop contextBroker
-   Remove previous contextBroker version

        yum remove contextBroker

-   [Take a backup of your
    DBs](database_admin.md#backup) (this is just a
    safety measure in case any problem occurs, e.g some script gets
    interrupted before finished and your database data ends in an
    incoherent state)
-   Download the following script:
    -   [mdsvector2mdsobject.py](https://github.com/telefonicaid/fiware-orion/blob/1.3.0/scripts/managedb/mdsvector2mdsobject.py)
-   Install pymongo (it is a script dependency) in case you don't have
    it previously installed

        pip-python install pymongo

-   Apply the mdsvector2mdsobject.py to your DBs, using the
    following (where 'db' is the database name). Note that if you are
    using
    [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation)
    you need to apply the procedure to each per-tenant/service database.
    The script can take a while, so an interactive progress counter
    is shown.

        python mdsvector2mdsobject.py orion

-   If you get the following message, there is some problem that needs
    to be solved before going to the next step. Check the
    Troubleshooting section below.

        WARNING: some problem was found during the process.

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

    Try to create a suitable index with the following command in the mongo shell, then run mdsvector2mdsobject.py again.

        db.entities.createIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

    Once migration has ended, you can remove the index with the following command:

        db.entities.dropIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

-   Due to duplicated metadata names in a given entity while
    running mdsvector2mdsobject.py. The symptom of this problem is
    getting errors like this one:

        - <n>: dupplicate metadata detected in entity {"type": "...", "id": "...", "servicePath": "..."} (attribute: <attrName>): <MetadataName>. Skipping

    This may correspond to entities created with old Orion versions,
    which were using the name-type combination to identify metadata.
    The solution is to remove the duplicate metadata and re-run
    mdsvector2mdsobject.py. The procedure to remove duplicate
    metadata is similar to the one documented for duplicated attributes [in this post at
    StackOverflow](http://stackoverflow.com/questions/30242731/fix-duplicate-name-situation-due-to-entities-created-before-orion-0-17-0/30242791#30242791).
    In the case of doubts, please submit a question in StackOverflow (using "fiware-orion" tag to label your question).

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
