# Orion Context Broker

This is the code repository for the Orion Context Broker, the reference implementation of the Publish/Subscribe Context Broker GE.

This project is part of [FIWARE](http://www.fiware.org). Check also the [FIWARE Catalogue entry for Orion](http://catalogue.fiware.org/enablers/publishsubscribe-context-broker-orion-context-broker)

Any feedback on this documentation is highly welcome, including bugs, typos
or things you think should be included but aren't. You can use [github issues](https://github.com/telefonicaid/fiware-orion/issues/new) to provide feedback.

For documentation previous to Orion 0.23.0 please check the manuals at FIWARE public wiki:

* [Orion Context Broker - Installation and Administration Guide](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide)
* [Orion Context Broker - User and Programmers Guide](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide)

## GEi overall description

Orion is a C++ implementation of the NGSI9/10 REST API binding developed as a part of the FIWARE platform.

Orion Context Broker allows you to manage all the whole lifecycle of context information including updates, queries, registrations and subscriptions. It is an NGSI9/10 server implementation to manage context information and its availability. Using the Orion Context Broker, you are able to register context elements and manage them through updates and queries. In addition, you can subscribe to context information so when some condition occurs (e.g. an interval of time has passed or the context elements have changed) you receive a notification. These usage scenarios and the Orion Context Broker features are described in this document.

If this is your first approach with Orion Context Broker, it is highly recommended to have a look to the brief [Quick Start guide](doc/manuals/quick_start_guide.md).

## Build and Install

The recommended procedure is to install using RPM packages in CentOS 6.x. If you are interested in
building from sources, check [this document](doc/manuals/admin/build_source.md).

### Requirements

* System resources: see [these recommendations](doc/manuals/admin/resources.md#resources-recommendations)
* Operating system: CentOS/RedHat. The reference operating system is CentOS 6.3  
but it should work also in any later CentOS/RedHat 6.x version.
* Database: MongoDB is required running either in the same system where Orion Context Broker is going to be installed or in a different host accessible through the network. The recommended MongoDB version is 2.6.9 (although it should work with later MongoDB 2.6.x and 3.0.x versions). It is not recommended using MongoDB 2.4.x., as some [geolocated queries](doc/manuals/user/geolocation.md) may not work.
    * Note that the officially supported MongoDB version is 2.6. In the case of using MongoDB 3.0 with its new authentication mechanism
      (SCRAM_SHA1) you may need to compile from sources using special switches for the MongoDB driver. See [this issue](https://github.com/telefonicaid/fiware-orion/issues/1061) for details.
* RPM dependencies (some of these packages could not be in the official CentOS/RedHat repository but in EPEL, in which case you have to configure EPEL repositories, see <http://fedoraproject.org/wiki/EPEL>):
    * The contextBroker package (mandatory) depends on the following packages: boost-filesystem, boost-thread, libmicrohttpd, logrotate, libcurl and boost-regex.
    * The contextBroker-test package (optional) depends on the following packages: python, python-flask, python-jinja2, curl, libxml2, libxslt, nc, mongo-10gen and contextBroker. The mongo-10gen dependency needs to configure MongoDB repository, check [this piece of documentation about that](http://docs.mongodb.org/manual/tutorial/install-mongodb-on-red-hat-centos-or-fedora-linux/).

### Installation

#### Using yum (recommended)

Configure the FIWARE yum repository ([as described in this post](http://stackoverflow.com/questions/24331330/how-to-configure-system-to-use-the-fi-ware-yum-repository/24510985#24510985)). Then you can install doing (as root):

    yum install contextBroker

Sometimes the above commands fails due to yum cache. In that case, run
`yum clean all` and try again.

#### Using RPM file

Download the package from the [FIWARE Files area](https://forge.fiware.org/frs/?group_id=7). Look for the "DATA-OrionContextBroker" entry.

Next, install the package using the rpm command (as root):

    rpm -i contextBroker-X.Y.Z-1.x86_64.rpm

#### Optional packages

Apart from the mandatory RPM described above, you can install the contextBroker-test package, which contain utility tools:

    yum install contextBroker-test

or

    rpm -i contextBroker-test-X.Y.Z-1.x86_64.rpm

### Upgrading from a previous version

Upgrade procedure depends on whether the *upgrade path* (i.e. from the installed Orion version to the target one to upgrade) crosses a version number that requires:

* Upgrading MongoDB version
* Migrating the data stored in DB (due to a change in the DB data model).

#### Upgrading MongoDB version

You only need to pay attention to this if your upgrade path crosses 0.11.0 or 0.22.0. Otherwise, you can skip this section.

* Orion versions previous to 0.11.0 recommend MongoDB 2.2
* Orion version from 0.11.0 to 0.20.0 recommend MongoDB 2.4. Check [the 2.4 upgrade procedure in the oficial MongoDB documentation.](http://docs.mongodb.org/master/release-notes/2.4-upgrade/)
* Orion version from 0.21.0 on recommend MongoDB 2.6 o 3.0. check [the 2.6 upgrade procedure](http://docs.mongodb.org/master/release-notes/2.6-upgrade/) or
[the 3.0 upgrade procedure](http://docs.mongodb.org/master/release-notes/3.0-upgrade/) in the oficial MongoDB documentation.  

#### Migrating the data stored in DB

You only need to pay attention to this if your upgrade path crosses 0.14.1, 0.19.0 or 0.21.0. Otherwise, you can skip this section. You can also skip this section if your DB are not valuable (e.g. debug/testing environments) and you can flush your DB before upgrading.

* [Upgrading to 0.14.1 and beyond from a pre-0.14.1 version](doc/manuals/admin/upgrading_crossing_0-14-1.md)
* [Upgrading to 0.19.0 and beyond from a pre-0.19.0 version](doc/manuals/admin/upgrading_crossing_0-19-0.md)
* [Upgrading to 0.21.0 and beyond from a pre-0.21.0 version](doc/manuals/admin/upgrading_crossing_0-21-0.md)

If your upgrade cover several segments (e.g. you are using 0.13.0 and
want to upgrade to 0.19.0, so both "upgrading to 0.14.1 and beyond from
a pre-0.14.1 version" and "upgrading to 0.19.0 and beyond from a
pre-0.19.0 version" applies to the case) you need to execute the
segments in sequence (the common part are done only one time, i.e. stop
CB, remove package, install package, start CB). In the case of doubt,
please [ask using StackOverflow](http://stackoverflow.com/questions/ask)
(remember to include the "fiware-orion" tag in your questions).

#### Standard upgrade 

If you are using yum, then you can upgrade doing (as root):

` yum install contextBroker`

Sometimes the above commands fails due to yum cache. In that case, run
`yum clean all` and try again.

If you are upgrading using the RPM file, then first download the new package from the [FIWARE Files area](https://forge.fiware.org/frs/?group_id=7). Look for the "DATA-OrionContextBroker" entry.

Then upgrade the package using the rpm command (as root):

    rpm -U contextBroker-X.Y.Z-1.x86_64.rpm

## Running

Once installed, there are two ways of running Orion Context Broker: manually from the command line or as a system service (the later only available if Orion was installed as RPM package). It is not recommended to mix both ways (e.g. start the context broker from the command line, then use `/etc/init.d/contextBroker status` to check its status). This section assumes you are running Orion as system service. From command line alternative, check [this document](doc/md/admin/cli.md).

You will typically need superuser privileges to use Orion Context Broker
as a system service, so the following commands need to be run as root or
using the sudo command.

In order to start the broker service, run:

    /etc/init.d/contextBroker start

Then, to stop the context broker, run:

    /etc/init.d/contextBroker stop

To restart, run:

    /etc/init.d/contextBroker restart

You can use chkconfig command to make contextBroker automatically
start/stop when your system boots/shutdowns (see [chkconfig
documentation](http://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-services-chkconfig.html)
for details).

### Configuration file

The configuration used by the contextBroker service is stored in the
/etc/sysconfig/contextBroker file, which typical content is:

    # BROKER_USER - What user to run orion-broker as
    BROKER_USER=orion
    
    # BROKER_PORT - the port/socket where orion-broker will listen for connections
    BROKER_PORT=1026
    
    # BROKER_LOG_DIR - Where to log to
    BROKER_LOG_DIR=/var/log/contextBroker
    
    # BROKER_PID_FILE - Where to store the pid for orion-broker
    BROKER_PID_FILE=/var/log/contextBroker/contextBroker.pid
    
    ## Database configuration for orion-broker
    BROKER_DATABASE_HOST=localhost
    BROKER_DATABASE_NAME=orion
    
    ## Replica set configuration. Note that if you set this parameter, the BROKER_DATBASE_HOST
    ## is interpreted as the list of host (or host:port) separated by commas to use as
    ## replica set seed list (single element lists are also allowed). If BROKER_DATABASE_RPL_SET
    ## parameter is unset, Orion CB assumes that the BROKER_DATABASE_HOST is an stand-alone
    ## mongod instance
    #BROKER_DATABASE_RPLSET=orion_rs
    
    # Database authentication (not needed if MongoDB doesn't use --auth)
    #BROKER_DATABASE_USER=orion
    #BROKER_DATABASE_PASSWORD=orion
    
    # Use the following variable if you need extra command line options
    #BROKER_EXTRA_OPS="-t 0-255"

All the fields except BROKER\_USER and BROKER\_EXTRA\_OPS map to *one*
of the options described in [command line
options](#Command_line_options), as follows:

-   BROKER\_PORT maps to -port
-   BROKER\_LOG\_DIR maps to -logDir
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

### Checking status

In order to check the status of the broker, use the following command
with superuser privileges (using the root user or the sudo command):

    /etc/init.d/contextBroker status

If broker is running you will get:

    Checking contextBroker...                         Running

If broker is not running you will get:

    Checking contextBroker...                         Service not running

## API Overview

In order to create an entity (Room1) with two attributes (temperature and pressure):

    (curl <orion_host>:1026/v1/contextEntities/Room1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF
    {
      "attributes" : [
    {
      "name" : "temperature",
      "type" : "float",
      "value" : "23"
    },
    {
      "name" : "pressure",
      "type" : "integer",
      "value" : "720"
    }
      ]
    }
    EOF

In order to query the entity:

    curl <orion_host>:1026/v1/contextEntities/Room1 -s -S --header 'Accept: application/json' | python -mjson.tool

In order to update one of the entity atributes (temperature):

    (curl <orion_host>:1026/v1/contextEntities/Room2/attributes/temperature -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -X PUT -d @- | python -mjson.tool) <<EOF
    {
       "value" : "26.3"
    }
    EOF

Please have a look to the [Quick Start guide](doc/manuals/quick_start_guide.md) if you want to test these operations in an actual public instance of Oriion Context Broker. In addition, have a look to the API Walkthrough and API Reference sections below in order to know more details about the API (subscriptions, registrations, etc.).

## API Walkthrough

* [FIWARE NGSI v1](doc/manuals/user/walkthrough_apiv1.md) (Markdown)
* [FIWARE NGSI v2](http://telefonicaid.github.io/fiware-orion/api/v2/cookbook) (Apiary) - *ongoing*

## API Reference Documentation

* [FIWARE NGSI v1](http://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FI-WARE_NGSI:_publicly_available_documents) (XSD and PDF)
* [FIWARE NGSI v2](http://telefonicaid.github.io/fiware-orion/api/v2/) (Apiary) - *ongoing*

## Testing

### Ent-to-end tests

The functional_test makefile target is used for running end-to-end tests:

    make functional_test INSTALL_DIR=~

Please have a look to the section [on building the source code](#from-sources) in order to get more information about how to prepare the environment to run the functional_test target.

### Unit tets

The unit_test makefile target is used for running the unit tests:

    make functional_test INSTALL_DIR=~

Please have a look to the section [on building the source code](#from-sources) in order to get more information about how to prepare the environment to run the unit_test target.

## Advanced topics:

* Installation and administration
        * [Building from sources](doc/manuals/admin/build_source.md)
        * [Running Orion from command line](doc/manuals/admin/cli.md)
	* [Database administration](doc/manuals/admin/database_admin.md)
	* [Logs](doc/manuals/admin/logs.md)
	* [Watchdog](doc/manuals/admin/watchdog.md)
	* [Rush relayer](doc/manuals/admin/rush.md)
	* [Management REST inferface](doc/manuals/admin/management_api.md)   
	* [Data model](doc/manuals/admin/database_model.md)
	* [Resources & I/O Flows](doc/manuals/admin/resources.md) 
	* [Problem diagnosis procedures](doc/manuals/admin/diagnosis.md)
* API
	* [Pagination](doc/manuals/user/pagination.md)
	* [Geolocation ](doc/manuals/user/geolocation.md)
        * [Structured values for attributes](doc/manuals/user/structured_attribute_valued.md)
        * [Context Providers registration and request forwarding](doc/manuals/user/context_providers.md)
	* [Attribute metadata](doc/manuals/user/metadata.md)
	* [Filtering results](doc/manuals/user/filtering.md)
	* [Multy tenancy](doc/manuals/user/multitenancy.md)
	* [Entity service paths](doc/manuals/user/service_path.md)
	* [Adding and removing attributes and entities with APPEND and DELETE in updateContext](doc/manuals/user/append_and_delete.md)
	* [Updating registrations and subscriptions](doc/manuals/user/updating_regs_and_subs.md)	 
	* [Context broker federation](doc/manuals/user/federation.md)
	* [Forbidden characters](doc/manuals/user/forbidden_characters.md)
	* [Duration (for registration and subscriptions)](doc/manuals/user/duration.md)
	* [Using empty types](doc/manuals/user/empty_types.md)
	* [Mixing standard and convenience operations](doc/manuals/user/std_conv_mix.md)
	* [HTTP and NGSI response codes](doc/manuals/user/http_and_ngsi_sc.md)
	* [Security considerations](doc/manuals/user/security.md)
	* [Known limitations](doc/manuals/user/known_limitations.md)	
* Container-based deployment
	* [Vagrant](doc/manuals/vagrant.md) 
* [Sample code contributions](doc/manuals/code_contributions.md)
* [Deprecated features](doc/manuals/deprecated.md)

## License

Orion Context Broker is licensed under Affero General Public License (GPL) version 3.
