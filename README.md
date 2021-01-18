# <a name="top"></a>Orion Context Broker

<!-- Documentation badge line is processed by release.sh. Thus, if the structure of the URL changes,
     release.sh needs to be changed also -->

[![FIWARE Core Context Management](https://nexus.lab.fiware.org/repository/raw/public/badges/chapters/core.svg)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/telefonicaid/fiware-orion.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion.svg)](https://hub.docker.com/r/fiware/orion/)
[![SOF support badge](https://nexus.lab.fiware.org/repository/raw/public/badges/stackoverflow/orion.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI v2](https://nexus.lab.fiware.org/repository/raw/public/badges/specifications/ngsiv2.svg)](http://fiware-ges.github.io/orion/api/v2/stable/)
<br>
[![Documentation badge](https://img.shields.io/readthedocs/fiware-orion.svg)](https://fiware-orion.rtfd.io)
![Compliance Tests](https://github.com/telefonicaid/fiware-orion/workflows/Compliance%20Tests/badge.svg)
![Unit Tests](https://github.com/telefonicaid/fiware-orion/workflows/Unit%20Tests/badge.svg)
![Functional Tests](https://github.com/telefonicaid/fiware-orion/workflows/Functional%20Tests/badge.svg)
![Status](https://nexus.lab.fiware.org/static/badges/statuses/orion.svg)

The Orion Context Broker is an implementation of the Publish/Subscribe Context
Broker GE, providing an
[NGSI](https://swagger.lab.fiware.org/?url=https://raw.githubusercontent.com/Fiware/specifications/master/OpenAPI/ngsiv2/ngsiv2-openapi.json)
interface. Using this interface, clients can do several operations:

-   Query context information. The Orion Context Broker stores context
    information updated from applications, so queries are resolved based
    on that information. Context information consists on *entities* (e.g. a car)
    and their *attributes* (e.g. the speed or location of the car).
-   Update context information, e.g. send updates of temperature
-   Get notified when changes on context information take place (e.g. the
    temperature has changed)
-   Register context provider applications, e.g. the provider for the temperature sensor within a
    room

This project is part of [FIWARE](https://www.fiware.org/). For more information
check the FIWARE Catalogue entry for
[Core Context Management](https://github.com/Fiware/catalogue/tree/master/core).

| :books: [Documentation](https://fiware-orion.rtfd.io) | :mortar_board: [Academy](https://fiware-academy.readthedocs.io/en/latest/core/orion) | :whale: [Docker Hub](https://hub.docker.com/r/fiware/orion/) | :dart: [Roadmap](doc/roadmap.md) |
|---|---|---|---|

## Content

-   [Background](#background)
    -   [Description](#gei-overall-description)
    -   [Introductory presentations](#introductory-presentations)
-   [Install](#install)
-   [Running](#running)
-   [Usage](#usage)
-   [API](#api)
-   [Reference Documentation](#reference-documentation)
-   [Testing](#testing)
    -   [End-to-end tests](#end-to-end-tests)
    -   [Unit Tests](#unit-tests)
-   [Advanced topics](#advanced-topics)
-   [Support](#support)
-   [License](#license)

## Background

You can find the User & Programmer's Manual and the Installation &
Administration Manual on [readthedocs.io](https://fiware-orion.readthedocs.io)

For documentation previous to Orion 0.23.0 please check the manuals at FIWARE
public wiki:

-   [Orion Context Broker - Installation and Administration Guide](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide)
-   [Orion Context Broker - User and Programmers Guide](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide)

Any feedback on this documentation is highly welcome, including bugs, typos or
things you think should be included but aren't. You can use
[github issues](https://github.com/telefonicaid/fiware-orion/issues/new) to
provide feedback.

[Top](#top)

### Description

Orion is a C++ implementation of the NGSIv2 REST API binding developed as a part
of the FIWARE platform.

Orion Context Broker allows you to manage the entire lifecycle of context
information including updates, queries, registrations and subscriptions. It is
an NGSIv2 server implementation to manage context information and its availability.
Context information consists on *entities* (e.g. a car) and their *attributes*
(e.g. the speed or location of the car).

Using the Orion Context Broker, you are able to create context elements and manage
them through updates and queries. In addition, you can subscribe to context
information so when some condition occurs (e.g. the context elements have changed)
you receive a notification. These usage scenarios and the Orion Context Broker
features are described in this documentation.

If this is your first contact with the Orion Context Broker, it is highly
recommended to have a look to the brief
[Quick Start guide](doc/manuals/quick_start_guide.md).

[Top](#top)

### Introductory presentations

-   Orion Context Broker
    [(en)](https://www.slideshare.net/fermingalan/orion-context-broker-20201029)
    [(jp)](https://www.slideshare.net/fisuda/orion-contextbroker20201029)
-   NGSIv2 Overview for Developers That Already Know NGSIv1
    [(en)](https://www.slideshare.net/fermingalan/orion-context-broker-ngsiv2-overview-for-developers-that-already-know-ngsiv1-20201028)
    [(jp)](https://www.slideshare.net/fisuda/orion-contextbrokerngsiv2overviewfordevelopersthatalreadyknowngsiv120201028)

[Top](#top)

## Install

Build and Install documentation for Orion Context Broker can be found at
[the corresponding section of the Admin Manual](doc/manuals/admin/install.md).

[Top](#top)

## Running

How to run Orion Context Broker can be found at
[the corresponding section of the Admin Manual](doc/manuals/admin/running.md).

[Top](#top)

## Usage

In order to create an entity (Room1) with two attributes (temperature and
pressure):

```console
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

```console
    curl <orion_host>:1026/v2/entities/Room2 -s -S --header 'Accept: application/json' | python -mjson.tool
```

In order to update one of the entity atributes (temperature):

```console
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

```console
curl <orion_host>:1026/v2/entities/Room2/attrs/temperature/value -s -S \
    --header 'Content-Type: text/plain' \
    -X PUT -d 26.3
```

Please have a look at the [Quick Start guide](doc/manuals/quick_start_guide.md)
if you want to test these operations in an actual public instance of Orion
Context Broker. In addition, have a look to the API Walkthrough and API
Reference sections below in order to know more details about the API
(subscriptions, registrations, etc.).

[Top](#top)

## API

-   FIWARE NGSI v2 [(en)](doc/manuals/user/walkthrough_apiv2.md)
    [(jp)](doc/manuals.jp/user/walkthrough_apiv2.md) (Markdown)
-   FIWARE NGSI v2
    [(en)](http://telefonicaid.github.io/fiware-orion/api/v2/stable/cookbook)
    [(jp)](https://open-apis.letsfiware.jp/fiware-orion/api/v2/stable/cookbook)
    (Apiary)
    -   See also NGSIv2 implementation notes
        [(en)](doc/manuals/user/ngsiv2_implementation_notes.md)
        [(jp)](doc/manuals.jp/user/ngsiv2_implementation_notes.md)

[Top](#top)

## Reference Documentation

API Reference Documentation:

-   FIWARE NGSI v2
    [(en)](http://telefonicaid.github.io/fiware-orion/api/v2/stable)
    [(jp)](https://open-apis.letsfiware.jp/fiware-orion/api/v2/stable/) (Apiary)
    -   See also NGSIv2 implementation notes
        [(en)](doc/manuals/user/ngsiv2_implementation_notes.md)
        [(jp)](doc/manuals.jp/user/ngsiv2_implementation_notes.md)

Orion Reference Documentation:

-   Orion Manuals in RTD [(en)](https://fiware-orion.readthedocs.org)
    [(jp)](https://fiware-orion.letsfiware.jp/)

[Top](#top)

## Testing

### End-to-end tests

The functional_test makefile target is used for running end-to-end tests:

    make functional_test INSTALL_DIR=~

Please have a look to the section
[on building the source code](doc/manuals/admin/build_source.md) in order to get
more information about how to prepare the environment to run the functional_test
target.

### Unit Tests

The unit_test makefile target is used for running the unit tests:

    make unit_test

Please have a look to the section
[on building the source code](doc/manuals/admin/build_source.md) in order to get
more information about how to prepare the environment to run the unit_test
target.

[Top](#top)

## Advanced topics

-   Advanced Programming [(en)](doc/manuals/user/README.md)
    [(jp)](doc/manuals.jp/user/README.md)
-   Installation and administration [(en)](doc/manuals/admin/README.md)
    [(jp)](doc/manuals.jp/admin/README.md)
-   Container-based deployment
    -   Docker [(en)](docker/README.md) [(jp)](docker/README.jp.md)
    -   Docker Swarm and HA [(en)](docker/docker_swarm.md)
        [(jp)](docker/docker_swarm.jp.md)
-   Development Manual [(en)](doc/manuals/devel/README.md)
    [(jp)](doc/manuals.jp/devel/README.md)
-   Sample code contributions [(en)](doc/manuals/code_contributions.md)
    [(jp)](doc/manuals.jp/code_contributions.md)
-   Contribution guidelines [(en)](doc/manuals/contribution_guidelines.md)
    [(jp)](doc/manuals.jp/contribution_guidelines.md), especially important if
    you plan to contribute with code to Orion Context Broker
-   Deprecated features [(en)](doc/manuals/deprecated.md)
    [(jp)](doc/manuals.jp/deprecated.md)

[Top](#top)

## Support

Ask your thorough programming questions using
[stackoverflow](http://stackoverflow.com/questions/ask) and your general
questions on [FIWARE Q&A](https://ask.fiware.org). In both cases please use the
tag `fiware-orion`

[Top](#top)

---

## License

Orion Context Broker is licensed under [Affero General Public License (GPL)
version 3](./LICENSE).

© 2021 Telefonica Investigación y Desarrollo, S.A.U

### Are there any legal issues with AGPL 3.0? Is it safe for me to use?

There is absolutely no problem in using a product licensed under AGPL 3.0. Issues with GPL
(or AGPL) licenses are mostly related with the fact that different people assign different
interpretations on the meaning of the term “derivate work” used in these licenses. Due to this,
some people believe that there is a risk in just _using_ software under GPL or AGPL licenses
(even without _modifying_ it).

For the avoidance of doubt, the owners of this software licensed under an AGPL-3.0 license
wish to make a clarifying public statement as follows:

> Please note that software derived as a result of modifying the source code of this
> software in order to fix a bug or incorporate enhancements is considered a derivative
> work of the product. Software that merely uses or aggregates (i.e. links to) an otherwise
> unmodified version of existing software is not considered a derivative work, and therefore
> it does not need to be released as under the same license, or even released as open source.

