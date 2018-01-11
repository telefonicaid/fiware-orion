# Running Orion as system service

Once installed, there are two ways of running Orion Context Broker: manually from the command line or as a system service (the later only available if Orion was installed as RPM package). It is not recommended to mix both ways (e.g. start the context broker from the command line, then use `/etc/init.d/contextBroker status` to check its status). This section assumes you are running Orion as system service. From command line alternative, check [this document](cli.md).

You will typically need superuser privileges to use Orion Context Broker
as a system service, so the following commands need to be run as root or
using the sudo command.

In order to start the broker service, run:

```
/etc/init.d/contextBroker start
```

Then, to stop the context broker, run:

```
/etc/init.d/contextBroker stop
```

To restart, run:

```
/etc/init.d/contextBroker restart
```

You can use chkconfig command to make contextBroker automatically
start/stop when your system boots/shutdowns (see [chkconfig
documentation](http://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-services-chkconfig.html)
for details).

## Configuration file

The configuration used by the contextBroker service is stored in the
/etc/sysconfig/contextBroker file, which typical content is:

```
# BROKER_USER - Who to run orion-broker as. Note that you may need to use root if you want
# to run Orion in a privileged port (<1024)
BROKER_USER=orion

# BROKER_PORT - the port/socket where orion-broker will listen for connections
BROKER_PORT=1026

# BROKER_LOG_DIR - Where to log to
BROKER_LOG_DIR=/var/log/contextBroker

# BROKER_LOG_LEVEL - Log File Level
BROKER_LOG_LEVEL=WARN

# BROKER_PID_FILE - Where to store the pid for orion-broker
BROKER_PID_FILE=/var/run/contextBroker/contextBroker.pid

## Database configuration for orion-broker
BROKER_DATABASE_HOST=localhost
BROKER_DATABASE_NAME=orion

## Replica set configuration. Note that if you set this parameter, the BROKER_DATABASE_HOST
## is interpreted as the list of host (or host:port) separated by commas to use as
## replica set seed list (single element lists are also allowed). If BROKER_DATABASE_RPL_SET
## parameter is unset, Orion CB assumes that the BROKER_DATABASE_HOST is an stand-alone
## mongod instance
#BROKER_DATABASE_RPLSET=orion_rs

# Database authentication (not needed if MongoDB doesn't use --auth)
#BROKER_DATABASE_USER=orion
#BROKER_DATABASE_PASSWORD=orion

# Use the following variable if you need extra ops
#BROKER_EXTRA_OPS="-t 0-255"
```

All the fields except BROKER\_USER and BROKER\_EXTRA\_OPS map to *one*
of the options described in [command line
options](cli.md#command-line-options), as follows:

-   BROKER\_USER doesn't map to CLI option but is used to the init.d script to set the owner of the contextBroker process
-   BROKER\_PORT maps to -port
-   BROKER\_LOG\_DIR maps to -logDir
-   BROKER\_LOG\_LEVEL maps to -logLevel
-   BROKER\_PID\_FILE maps to -pidpath
-   BROKER\_DATABASE\_HOST maps to -dbhost
-   BROKER\_DATABASE\_NAME maps to -db
-   BROKER\_DATABASE\_RPLSET maps to -rplSet
-   BROKER\_DATABASE\_USER maps to -dbuser
-   BROKER\_DATABASE\_PASSWORD maps to -dbpwd

Regarding BROKER\_EXTRA\_OPS, it is used to specify other options not
covered by the fields above, as a string that is appended to the broker
command line at starting time. Note that this string is "blindly"
appended, i.e. the service script doesn't do any check so be careful
using this, ensuring that you are providing valid options here and you
are not duplicating any option in other BROKER\_\* field (e.g. not set
BROKER\_EXTRA\_OPS="-port 1026" as BROKER\_PORT is used for that).

Regarding BROKER\_USER, it is the user that will own the contextBroker
process upon launching it. By default, the RPM installation creates a
user named 'orion'. Note that if you want to run the broker in a
privileged port (i.e. 1024 or below) you will need to use 'root' as
BROKER\_USER.

## Checking status

In order to check the status of the broker, use the following command
with superuser privileges (using the root user or the sudo command):

```
/etc/init.d/contextBroker status
```

If broker is running you will get:

```
Checking contextBroker...                         Running
```

If broker is not running you will get:

```
Checking contextBroker...                         Service not running
```
