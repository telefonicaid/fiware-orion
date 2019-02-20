# Upgrading to 2.2.0 and beyond from a pre-2.2.0 version

Orion 2.2.0 removes the metadata ID feature and this involves a change in DB model. This procedure check if you
have such metadata ID in your data and, in that case, explains how to deal with that situation.

-   Stop contextBroker
-   Remove previous contextBroker version

        yum remove contextBroker

-   [Take a backup of your DBs](database_admin.md#backup) (this is just a safety measure in case any problem occurs,
    e.g some script gets interrupted before finished and your database data ends in an incoherent state)
-   Download the following script:
    -   [check_metadata_id.py](https://github.com/telefonicaid/fiware-orion/blob/2.2.0/scripts/managedb/upgrade-2.2.0/check_metadata_id.py)
-   Install pymongo (it is a script dependency) in case you don't have it previously installed

        pip-python install pymongo

-   Apply the `check_metadata_id.py` script to your DBs, using the following command (where 'db' is the database name). Note that
    if you are using [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation) you need to apply the
    procedure to each per-tenant/service database. The script can take a while, so an interactive progress counter is shown.

        python check_metadata_id.py orion

-   If you get some message like this one, then check the "Dealing with metadata ID" section.

        entity {u'type': u'...', u'id': u'...', u'servicePath': u'...'}: found attr <...>, metadata id <...>, value <...>

-   If you don't get the above message, then you data doesn't have any ID and you can proceed with next step-

-   If you get any of the following messages, there is some problem that needs to be solved before going to the next step. Check the
    "Troubleshooting" section below.

        WARNING: some problem was found during the process.
        ERROR: document ... change attempt failed

-   Install new contextBroker version. (Sometimes the commands fails due to yum cache. In that case, run "yum clean all" and try again)

        yum install contextBroker

-   Start contextBroker

Note that the yum command demands superuser privileges, so you have to run it as root or using the sudo command.

## Dealing with metadata ID

In the case you have metadata ID in your database, you need to remove them. There are two ways of doing so:

-   Using the Orion API with the old version of Orion you were using (recommended). You can use the
    `DELETE /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}` or
    `DELETE /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}` operations.

-   Using the script `check_metadata_id.py` in autofix mode. In order to do so, edit the script and change the line `autofix = None`
    to either `autofix = 'as_new_attrs'` or `autofix = 'as_metadata'`.
    - In `as_new_attrs` mode each attribute with ID is transformed to a new attribute in the form `<attrName>:<id>` (e.g. if you
      have an attribute `temperature` with metadata ID `id1` then it will be transformed to `temperature:id1`). In the rare case the
      destination attribute already exists in the entity then you will get a `- ERROR: attribute <...> already exist in entity ...
      Cannot be automatically fixed` message and you need to deal with it manually.
    - In `as_metadata` mode the attributes don't change their names and the ID is transformed in a regular metadata. Thus, from a point of view
      of external clients (i.e. a system doing `GET /v2/entities`) there wouldn't be any difference. However, this assumes that no attribute
      exists with more than one ID (i.e. `entities w/ at least one attr w/ more than one ID` is zero in the first pass of the script),
      otherwise it will fail.

## Troubleshooting

Two different kinds of problems may happen:

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

