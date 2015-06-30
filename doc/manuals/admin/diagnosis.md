# Problem diagnosis procedures

The Diagnosis Procedures are the first steps that a System Administrator
will take to locate the source of an error in Orion. Once the nature of
the error is identified with these tests, the system admin will often
have to resort to more concrete and specific testing to pinpoint the
exact point of failure and a possible solution. Such specific testing is
out of the scope of this section.

Please report any bug or problem with Orion Context Broker by [opening and issue in github.com](https://github.com/telefonicaid/fiware-orion/issues/new).

## Sanity check procedures

The Sanity Check Procedures are the steps that a System Administrator
will take to verify that an installation is ready to be tested. This is
therefore a preliminary set of tests to ensure that obvious or basic
malfunctioning is fixed before proceeding to unit tests, integration
tests and user validation.

### Checking Orion is up and running

-   Start context broker in default port (1026)
-   Run the following command

```
curl localhost:1026/version
```

-   Check that you get the version number as output (along with uptime
    information and compilation environment information):

```
<orion>
  <version>0.22.0-next</version>
  <uptime>0 d, 1 h, 34 m, 25 s</uptime>
  <git_hash>6e2aca5ebe287083efa306fc84d213fa24309a63</git_hash>
  <compile_time>Fri Jun 19 10:34:41 CEST 2015</compile_time>
  <compiled_by>fermin</compiled_by>
  <compiled_in>debvm</compiled_in>
</orion>
```

### List of Running Processes

A process named "contextBroker" should be up and running, e.g.:

```
$ ps ax | grep contextBroker
 8517 ?        Ssl    8:58 /usr/bin/contextBroker -port 1026 -logDir /var/log/contextBroker -pidpath /var/log/contextBroker/contextBroker.pid -dbhost localhost -db orion
```

### Network interfaces Up & Open

Orion Context Broker uses TCP 1026 as default port, although it can be
changed using the -port command line option.

### Database server

The Orion Context Broker uses a MongoDB database, whose parameters are
provided using the command line options `dbhost`, `-dbuser`, `-dbpwd`
and `-db`. Note that `-dbuser` and `-dbpwd` are only used if MongoDB
runs using authentication, i.e. with `--auth`.

You can check that the database is working using the mongo console:

```
mongo <dbhost>/<db>
```

You can check the different collections used by the broker using the
following commands in the mongo console. However, note that the broker
creates a collection the first time a document is to be inserted in it,
so if it is the first time you run the broker (or if database was
cleaned) and the broker hasn't received any request yet no collection
exists. Use `show collections` to get the actual collections list in any
given moment.

```
> db.registrations.count()
> db.entities.count()
> db.csubs.count()
> db.casubs.count()
```

## Diagnose database connection problems

The symptoms of a database connection problem are the following ones:

-   At start time. The broker doesn't start and the following message
    appears in the log file:

` X@08:04:45 main[313]: MongoDB error`

-   During broker operation. Error message like the following ones
    appear in the responses sent by the broker.

```
      ...
      <errorCode>
        <code>500</code>
        <reasonPhrase>Database Error</reasonPhrase>
        <details>collection: ... - exception: Null cursor</details>
      </errorCode>
      ...

      ...
      <errorCode>
        <code>500</code>
        <reasonPhrase>Database Error</reasonPhrase>
        <details>collection: ... - exception: socket exception [CONNECT_ERROR] for localhost:27017</details>
      </errorCode>
      ...

      ...
      <errorCode>
        <code>500</code>
        <reasonPhrase>Database Error</reasonPhrase>
        <details>collection: ... - exception: socket exception [FAILED_STATE] for localhost:27017</details
      </errorCode>
      ...

      ...
      <errorCode>
        <code>500</code>
        <reasonPhrase>Database Error</reasonPhrase>
        <details>collection: ... - exception: DBClientBase::findN: transport error: localhost:27017 ns: orion.$cmd query: { .. }</details>
      </errorCode>
      ...
```

In both cases, check that the connection to MonogDB is correctly
configured (in particular, the BROKER\_DATABASE\_HOST if you are running
Orion Context Broker [as a service](#As_system_service "wikilink") or
the "-dbhost" option if you are running it [from the command
line](#From_the_command_line "wikilink")) and that the mongod or mongos
process (depending if you are using sharding or not) is up and running.

If the problem is that MongoDB is down, note that Orion Context Broker
is able to reconnect to the database once it gets ready again. In other
words, you don't need to restart the broker in order to re-connect to
the database.

## Diagnose spontaneous binary corruption problems**

The symptoms of this problem are:

-   Orion Context Broker send empty responses to REST requests (e.g.
    with curl the message is typically "empty response from server").
    Note that it could happen that request on some URLs work
    normally (e.g. /version) while in others the symptom appears.
-   The MD5SUM of /usr/bin/contextBroker binary (that can be get with
    "md5sum /usr/bin/contextBroker" is not the right one (check list for
    particular versions at the end of this section).
-   The prelink package is installed (this can be checked running the
    command "rpm -qa | grep prelink")

The cause of this problem is
[prelink](http://en.wikipedia.org/wiki/Prelink), a program that modifies
binaries to make them starting faster (which is not very useful for
binary implementing long running services as contextBroker is) but that
is known to be incompatible with some libraries (in particular, it seems
to be incompatible with some of the libraries used by Context Broker).

The solution fo this problems is:

-   Disable prelink, either implementing one of the following
    alternatives:
    -   Remove the prelink software, typically running (as root or using
        sudo): `rpm -e prelink`
    -   Disable the prelink processing of contextBroker binary, creating
        the /etc/prelink.conf.d/contextBroker.conf file with the
        following content (just one line)

```
-b /usr/bin/contextBroker
```

-   Re-install the contextBroker package, typically running (as root or
    using sudo):

```
yum remove contextBroker
yum install contextBroker
```

