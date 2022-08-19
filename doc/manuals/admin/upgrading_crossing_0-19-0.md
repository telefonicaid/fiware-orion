# Upgrading to 0.19.0 and beyond from a pre-0.19.0 version

This procedure has to be done always as long as your database contains
at least one entity document.

-   Stop contextBroker
-   Remove previous contextBroker version

        yum remove contextBroker

-   [Take a backup of your
    DBs](database_admin.md#backup) (this is just a
    safety measure in the case some problem occurs, e.g the
    fix\_default\_sp.py script gets interrupted before finished and your
    database data ends in a incoherent status)
-   Download the following script:
    [fix\_default\_sp.py](https://github.com/telefonicaid/fiware-orion/blob/0.19.0-FIWARE-4.2.2/scripts/managedb/fix_default_sp.py)
-   Install pymongo (it is a script dependency) in the case you haven't
    it previously installed

        pip-python install pymongo

-   Apply the script to your DBs, using the following (where 'db' is the
    database name). Note that if you are using
    [multitenant/multiservice](database_admin.md#multiservicemultitenant-database-separation)
    you need to apply the procedure to each per-tenant/service database.
    The script can take a while (e.g. around 30 minutes in a database
    with around 50.000 entities), you can monitor progress using the
    following command at mongo shell:
    *db.entities.count({"\_id.servicePath": null})* (you should see the
    count decreasing while the script works).

        python fix_default_sp.py orion

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

-   Due to "duplicated" entities (i.e. in the database they have the
    same id and type, one of it with servicePath "/" and the other with
    abscense of servicePath). This shouldn't happen if you have executed
    the process described above before upgrading to 0.19.0 (or beyond)
    but may happen if you upgraded first and run the fix\_default\_sp.py
    script after that, in the case that between the upgrade and the
    execution of the script some update entity operation took place
    using the Orion API. This situation is problematic (e.g. may cause
    problems updating entities, as described
    [here](http://stackoverflow.com/questions/28498460/orion-cb-does-not-change-the-value-of-an-attribute))
    and should be fixed as soon as possible.
    The symptom of this problem is having the following counter in the
    fix\_default\_sp.py output with a result value greater than 0:

        duplicated cases of entities with "/" and null "/" service path (NEED FIX)

    The solution is to run the
    [merge\_default\_sp\_dups.py](https://github.com/telefonicaid/fiware-orion/blob/0.19.0-FIWARE-4.2.2/scripts/managedb/merge_default_sp_dups.py)
    script to consolidate each pair of duplicated documents into a
    single entity document. The scripts uses as parameter the database
    to fix (which has to be the same than the one used
    for fix\_default\_sp.py).

        python merge_default_sp_dups.py orion

    After executing merge\_default\_sp\_dups.py, it is highly
    recommented to run again fix\_default\_sp.py in order to check that
    the NEED FIX counter is 0 and the "WARNING: some problem was found
    during the process" message is not printed.

    Note that merge\_default\_sp\_dups.py is not able to solve
    all situations. In very rare cases some pairs cannot be
    consolidated, in which case you see a "skipped" value greater than 0
    in the summary printed at the end of script execution.

        ----- processed in total:  540 (total time: 0:00:44.143435)
        ----- dup processed:       481
        ----- skipped:             7

    The merge\_default\_sp\_dups.py skips pair consolidation in the
    following situations:

    -   Several attributes with the same name in some of the documents
        in the pair. This may correspond to entities created with Orion
        previous to 0.17.0, which were using the name-type combination
        to identify attributes. This is no longer used in 0.19.0, so you
        need to remove duplicated attributes. The procedure to remove
        duplicate attributes is documented [in this post at
        StackOverflow](http://stackoverflow.com/questions/30242731/fix-duplicate-name-situation-due-to-entities-created-before-orion-0-17-0/30242791#30242791).
    -   Modification time in the element of the pair corresponding to
        null servicePath is greater than modification time in the
        element of the pair corresponding to servicePath "/". This would
        be a very abnormal situation, as the data in the servicePath "/"
        document have to be fresher than the data in null
        servicePath document. Please, tell us about this if happens.

    Finally, you could get DB errors such as "ERROR pushing attributes
    at DB" or "ERROR removing dup at DB". They follow the same treatment
    than the one described for DB errors with fix\_default\_sp.py
    described below.

-   Due to some unexpected problem during DB updating. The symptom of
    this problem is getting errors in the fix\_default\_sp.py output
    like this one:

        * Error editing entity at DB:

    There is no a general solution for this problem, it has to be
    analyzed case by case. If this happens to you, please have a look to
    the [existing questions in StackOverflow about
    fiware-orion](http://stackoverflow.com/questions/tagged/fiware-orion)
    in order to check if your problem is solved there, otherwise create
    your own question (don't forget to include the "fiware-orion" tag
    and the exact "Error editing entity at DB" message in your case).