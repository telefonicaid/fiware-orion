# <a name="top"></a>Orion Context Broker

<!-- Documentation badge line is processed by release.sh. Thus, if the structure of the URL changes,
     release.sh needs to be changed also -->

[![License badge](https://img.shields.io/badge/license-AGPL-blue.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=1.13.0)](http://fiware-orion.readthedocs.io/en/1.13.0/?badge=1.13.0)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion.svg)](https://hub.docker.com/r/fiware/orion/)
[![Support badge]( https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)

* [Introduction](#introduction)
* [GEi overall description](#gei-overall-description)
* [Build and Install](#build-and-install)
* [Running](#running)
* [API Overview](#api-overview)
* [API Walkthrough](#api-walkthrough)
* [API Reference Documentation](#api-reference-documentation)
* [Testing](#testing)
    * [End-to-end tests](#end-to-end-tests)
    * [Unit Tests](#unit-tests)
* [Advanced topics](#advanced-topics)
* [License](#license)
* [Support](#support)
		  
## Introduction

This is the code repository for the Orion Context Broker, the reference implementation of the Publish/Subscribe Context Broker GE.

This project is part of [FIWARE](http://www.fiware.org). Check also the [FIWARE Catalogue entry for Orion](http://catalogue.fiware.org/enablers/publishsubscribe-context-broker-orion-context-broker)

Any feedback on this documentation is highly welcome, including bugs, typos
or things you think should be included but aren't. You can use [github issues](https://github.com/telefonicaid/fiware-orion/issues/new) to provide feedback.

You can find the User & Programmer's Manual and the Installation & Administration Manual on [readthedocs.io](https://fiware-orion.readthedocs.io)

For documentation previous to Orion 0.23.0 please check the manuals at FIWARE public wiki:

* [Orion Context Broker - Installation and Administration Guide](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide)
* [Orion Context Broker - User and Programmers Guide](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide)

[Top](#top)

## GEi overall description

Orion is a C++ implementation of the NGSIv2 REST API binding developed as a part of the FIWARE platform.

Orion Context Broker allows you to manage the entire lifecycle of context information including updates, queries, registrations and subscriptions. It is an NGSIv2 server implementation to manage context information and its availability. Using the Orion Context Broker, you are able to create context elements and manage them through updates and queries. In addition, you can subscribe to context information so when some condition occurs (e.g. the context elements have changed) you receive a notification. These usage scenarios and the Orion Context Broker features are described in this documentation.

If this is your first contact with the Orion Context Broker, it is highly recommended to have a look to the brief [Quick Start guide](doc/manuals/quick_start_guide.md).

[Top](#top)

## Build and Install

Build and Install documentation for Orion Context Broker can be found at [the corresponding section of the Admin Manual](doc/manuals/admin/install.md).

[Top](#top)

## Running

How to run Orion Context Broker can be found at [the corresponding section of the Admin Manual](doc/manuals/admin/running.md).

[Top](#top)

## API Overview

In order to create an entity (Room1) with two attributes (temperature and pressure):

``` 
curl <orion_host>:1026/v2/entities -s -S --header 'Content-Type: application/json' \
    -X POST -d @- <<EOF
{
  "id": "Room2",
  "type": "Room",
  "temperature": {
    "value": 23,
    "type": "Number"
  },
  "pressure": {
    "value": 720,
    "type": "Number"
  }
}
EOF
``` 
In order to query the entity:

    curl <orion_host>:1026/v2/entities/Room2 -s -S --header 'Accept: application/json' | python -mjson.tool

In order to update one of the entity atributes (temperature):
``` 
curl <orion_host>:1026/v2/entities/Room2/attrs/temperature -s -S \
    --header 'Content-Type: application/json' \
    -X PUT -d @- <<EOF
{
  "value": 26.3,
  "type": "Number"
}
EOF
``` 

or (more compact):
```
curl <orion_host>:1026/v2/entities/Room2/attrs/temperature/value -s -S \
    --header 'Content-Type: text/plain' \
    -X PUT -d 26.3
```

Please have a look at the [Quick Start guide](doc/manuals/quick_start_guide.md) if you want to test these operations in an actual public instance of Orion Context Broker. In addition, have a look to the API Walkthrough and API Reference sections below in order to know more details about the API (subscriptions, registrations, etc.).

[Top](#top)

## API Walkthrough

* [FIWARE NGSI v1](doc/manuals/user/walkthrough_apiv1.md) (Markdown)
* [FIWARE NGSI v2](doc/manuals/user/walkthrough_apiv2.md) (Markdown) - *release candidate*
* [FIWARE NGSI v2](http://telefonicaid.github.io/fiware-orion/api/v2/stable/cookbook) (Apiary) - *release candidate*
  * See also [NGSIv2 implementation notes](doc/manuals/user/ngsiv2_implementation_notes.md)

[Top](#top)

## API Reference Documentation

* [FIWARE NGSI v1](http://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FI-WARE_NGSI:_publicly_available_documents) (XSD and PDF)
* [FIWARE NGSI v2](http://telefonicaid.github.io/fiware-orion/api/v2/stable) (Apiary) - *release candidate*
  * See also [NGSIv2 implementation notes](doc/manuals/user/ngsiv2_implementation_notes.md)

[Top](#top)

## Testing

### End-to-end tests

The functional_test makefile target is used for running end-to-end tests:

    make functional_test INSTALL_DIR=~

Please have a look to the section [on building the source code](doc/manuals/admin/build_source.md) in order to get more information about how to prepare the environment to run the functional_test target.

### Unit Tests

The unit_test makefile target is used for running the unit tests:

    make unit_test

Please have a look to the section [on building the source code](doc/manuals/admin/build_source.md) in order to get more information about how to prepare the environment to run the unit_test target.

[Top](#top)

## Advanced topics

* [Advanced Programming](doc/manuals/user/README.md)
* [Installation and administration](doc/manuals/admin/README.md)
* Container-based deployment
  * [Docker](docker/README.md)
  * [Docker Swarm and HA](docker/docker_swarm.md)
* [Development Manual](doc/manuals/devel/README.md)
* [Sample code contributions](doc/manuals/code_contributions.md)
* [Contribution guidelines](doc/manuals/contribution_guidelines.md), especially important if you plan to contribute with code
  to Orion Context Broker
* [Deprecated features](doc/manuals/deprecated.md)

[Top](#top)

## License

Orion Context Broker is licensed under Affero General Public License (GPL) version 3.

[Top](#top)

## Support

Ask your thorough programming questions using [stackoverflow](http://stackoverflow.com/questions/ask)
and your general questions on [FIWARE Q&A](https://ask.fiware.org). In both cases please use the tag `fiware-orion`

[Top](#top)
