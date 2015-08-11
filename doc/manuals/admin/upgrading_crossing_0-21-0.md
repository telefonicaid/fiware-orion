# Upgrading to 0.21.0 and beyond from a pre-0.21.0 version

This procedure has to be done always as long as your database contains
at least one entity document.

-   Stop contextBroker
-   Remove previous contextBroker version

        yum remove contextBroker   

-   [Take a backup of your
    DBs](database_admin.md#backup) (this is just a
    safety measure in the case some problem occurs, e.g some script gets
    interrupted before finished and your database data ends in a
    incoherent status)
-   Download the following scripts:
    -   [fix\_location\_gjson.py](https://github.com/telefonicaid/fiware-orion/blob/0.21.0/scripts/managedb/fix_location_gjson.py)
    -   [attrsvector2attrsobject.py](https://github.com/telefonicaid/fiware-orion/blob/0.21.0/scripts/managedb/attrsvector2attrsobject.py)
-   Install pymongo (it is a script dependency) in the case you haven't
    it previously installed

        pip-python install pymongo

-   Apply the fix\_location\_gjson.py to your DBs, using the following
    (where 'db' is the database name). Note that if you are using
    [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation)
    you need to apply the procedure to each per-tenant/service database.
    The script can take a while, an interactive progress counter
    is shown.

        python fix_location_gjson.py orion

-   If you get the following message, there is some problem that needs
    to be solved before going to the next step. Check the
    Troubleshooting section below.

        WARNING: some problem was found during the process.

-   Apply the attrsvector2attrsobject.py to your DBs, using the
    following (where 'db' is the database name). Note that if you are
    using
    [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation)
    you need to apply the procedure to each per-tenant/service database.
    The script can take a while, an interactive progress counter
    is shown.

        python attrsvector2attrsobject.py orion

-   If you get the following message, there is some problem that needs
    to be solved before going to the next step. Check the
    Troubleshooting section below.

        WARNING: some problem was found during the process.

-   Install new contextBroker version (Sometimes the commands fails due
    to yum cache. In that case, run "yum clean all" and try again)

        yum install contextBroker

-   Start contextBroker

Note that the rpm command demand superuser privileges, so you have to
run them as root or using the sudo command.

## Troubleshooting

Two different kind of problems may happen:

-   Due to duplicated attribute names in a given entity while
    running attrsvector2attrsobject.py. The symptom of this problem is
    getting errors like this one:

        - <n>: dupplicate attribute detected in entity {"type": "...", "id": "...", "servicePath": "..."}: <AttrName>. Skipping

    This may correspond to entities created with Orion 0.17.0 or before,
    which were using the name-type combination to identify attributes.
    The solution is to remove the duplicate attribute and re-run
    attrsvector2attrsobject.py again. The procedure to remove duplicate
    attributes is documented [in this post at
    StackOverflow](http://stackoverflow.com/questions/30242731/fix-duplicate-name-situation-due-to-entities-created-before-orion-0-17-0/30242791#30242791).

-   Due to some unexpected problem during DB updating. The symptom of
    this problem is getting errors like this one:

        - <n>: ERROR: document <...> change attempt failed!

    There is no a general solution for this problem, it has to be
    analyzed case by case. If this happens to you, please have a look to
    the [existing questions in StackOverflow about
    fiware-orion](http://stackoverflow.com/questions/tagged/fiware-orion)
    in order to check if your problem is solved there, otherwise create
    your own question (don't forget to include the "fiware-orion" tag
    and the exact error message in your case).