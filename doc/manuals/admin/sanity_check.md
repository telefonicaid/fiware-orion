# <a name="top"></a>Sanity check procedures

* [End to End Testing](#end-to-end-testing)
* [List of Running Processes](#list-of-running-processes)
* [Network Interfaces Up and Open](#network-interfaces-up-and-open)
* [Databases](#databases)

The Sanity Check Procedures are the steps that a System Administrator will take to verify that an installation is
ready to be tested. This is therefore a preliminary set of tests to ensure that obvious or basic malfunctioning
is fixed before proceeding to unit tests, integration tests and user validation.

## End to End Testing

-   Start context broker in default port (1026)
-   Run the following command

```
curl --header 'Accept: application/json' localhost:1026/version
```

-   Check that you get the version number as output (along with uptime
    information and compilation environment information):

```
{
  "orion" : {
    "version" : "2.4.0-next",
    "uptime" : "0 d, 0 h, 2 m, 30 s",
    "git_hash" : "f2a3d436b2b507c5fd1611492ad7fad386901952",
    "compile_time" : "Thu Oct 29 16:56:16 CEST 2020",
    "compiled_by" : "fermin",
    "compiled_in" : "debvm",
    "release_date" : "Thu Oct 29 16:56:16 CEST 2020",
    "doc" : "https://fiware-orion.rtfd.io/",
    "libversions": ...
  }
}
```

[Top](#top)

## List of Running Processes

A process named "contextBroker" should be up and running, e.g.:

```
$ ps ax | grep contextBroker
 8517 ?        Ssl    8:58 /usr/bin/contextBroker -port 1026 -logDir /var/log/contextBroker -pidpath /var/log/contextBroker/contextBroker.pid -dbhost localhost -db orion
```

[Top](#top)

## Network Interfaces Up and Open

Orion Context Broker uses TCP 1026 as default port, although it can be
changed using the -port command line option.

[Top](#top)

## Databases

The Orion Context Broker uses a MongoDB database, whose parameters are
provided using the command line options:

* `-dbhost`
* `-db`
* `-dbuser`
* `-dbpwd`
* `-dbAuthMech`
* `-dbAuthDb`
* `-dbSSL`
* `-dbDisableRetryWrites`
* `-dbTimeout`
* `-dbPoolSize`
* `-writeConcern`

Note that `-dbuser`, `-dbpwd`, `-dbAuthMech` and `-dbAuthDb`.
are only used if MongoDB runs using authentication, i.e. with `--auth`.

You can check that the database is working using the mongo console:

```
mongo <dbhost>/<db>
```

You can check the different collections used by the broker using the
following commands in the mongo console. However, note that the broker
creates a collection the first time a document is to be inserted in it,
so if it is the first time you run the broker (or if database was
cleaned) and the broker hasn't received any request yet no collection
exists. Use `show collections` to get the actual collections list in any
given moment.

```
> db.registrations.count()
> db.entities.count()
> db.csubs.count()
```

[Top](#top)
