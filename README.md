# <a name="top"></a>Orion Context Broker

<!-- Documentation badge line is processed by release.sh. Thus, if the structure of the URL changes,
     release.sh needs to be changed also -->

[![FIWARE Core Context Management](https://img.shields.io/badge/FIWARE-Core-233c68.svg?logo=data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAVCAYAAAC33pUlAAAABHNCSVQICAgIfAhkiAAAA8NJREFUSEuVlUtIFlEUx+eO+j3Uz8wSLLJ3pBiBUljRu1WLCAKXbXpQEUFERSQF0aKVFAUVrSJalNXGgmphFEhQiZEIPQwKLbEUK7VvZrRvbr8zzjfNl4/swplz7rn/8z/33HtmRhn/MWzbXmloHVeG0a+VSmAXorXS+oehVD9+0zDN9mgk8n0sWtYnHo5tT9daH4BsM+THQC8naK02jCZ83/HlKaVSzBey1sm8BP9nnUpdjOfl/Qyzj5ust6cnO5FItJLoJqB6yJ4QuNcjVOohegpihshS4F6S7DTVVlNtFFxzNBa7kcaEwUGcbVnH8xOJD67WG9n1NILuKtOsQG9FngOc+lciic1iQ8uQGhJ1kVAKKXUs60RoQ5km93IfaREvuoFj7PZsy9rGXE9G/NhBsDOJ63Acp1J82eFU7OIVO1OxWGwpSU5hb0GqfMydMHYSdiMVnncNY5Vy3VbwRUEydvEaRxmAOSSqJMlJISTxS9YWTYLcg3B253xsPkc5lXk3XLlwrPLuDPKDqDIutzYaj3eweMkPeCCahO3+fEIF8SfLtg/5oI3Mh0ylKM4YRBaYzuBgPuRnBYD3mmhA1X5Aka8NKl4nNz7BaKTzSgsLCzWbvyo4eK9r15WwLKRAmmCXXDoA1kaG2F4jWFbgkxUnlcrB/xj5iHxFPiBN4JekY4nZ6ccOiQ87hgwhe+TOdogT1nfpgEDTvYAucIwHxBfNyhpGrR+F8x00WD33VCNTOr/Wd+9C51Ben7S0ZJUq3qZJ2OkZz+cL87ZfWuePlwRcHZjeUMxFwTrJZAJfSvyWZc1VgORTY8rBcubetdiOk+CO+jPOcCRTF+oZ0okUIyuQeSNL/lPrulg8flhmJHmE2gBpE9xrJNkwpN4rQIIyujGoELCQz8ggG38iGzjKkXufJ2Klun1iu65bnJub2yut3xbEK3UvsDEInCmvA6YjMeE1bCn8F9JBe1eAnS2JksmkIlEDfi8R46kkEkMWdqOv+AvS9rcp2bvk8OAESvgox7h4aWNMLd32jSMLvuwDAwORSE7Oe3ZRKrFwvYGrPOBJ2nZ20Op/mqKNzgraOTPt6Bnx5citUINIczX/jUw3xGL2+ia8KAvsvp0ePoL5hXkXO5YvQYSFAiqcJX8E/gyX8QUvv8eh9XUq3h7mE9tLJoNKqnhHXmCO+dtJ4ybSkH1jc9XRaHTMz1tATBe2UEkeAdKu/zWIkUbZxD+veLxEQhhUFmbnvOezsJrk+zmqMo6vIL2OXzPvQ8v7dgtpoQnkF/LP8Ruu9zXdJHg4igAAAABJRU5ErkJgggA=)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/telefonicaid/fiware-orion.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion.readthedocs.io/en/latest/?badge=latest)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion.svg)](https://hub.docker.com/r/fiware/orion/)
[![SOF support badge](https://nexus.lab.fiware.org/repository/raw/public/badges/stackoverflow/orion.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![Build badge](https://img.shields.io/travis/telefonicaid/fiware-orion.svg)](https://travis-ci.org/telefonicaid/fiware-orion/)
[![NGSI v2](https://nexus.lab.fiware.org/repository/raw/public/badges/specifications/ngsiv2.svg)](http://fiware.github.io/context.Orion/api/v2/stable/)

* [Introduction](#introduction)
* [GEi overall description](#gei-overall-description)
* [Introductory presentations](#introductory-presentations)
* [Build and Install](#build-and-install)
* [Running](#running)
* [API Overview](#api-overview)
* [API Walkthrough](#api-walkthrough)
* [Reference Documentation](#reference-documentation)
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

## Introductory presentations

* Orion Context Broker [(en)](https://www.slideshare.net/fermingalan/orion-context-broker-1150) [(jp)](https://www.slideshare.net/fisuda/orion-context-broker-1150-106271125)
* NGSIv2 Overview for Developers That Already Know NGSIv1 [(en)](https://www.slideshare.net/fermingalan/ngsiv2-overview-for-developers-that-already-know-ngsiv1-20180716) [(jp)](https://www.slideshare.net/fisuda/orion-contextbroker-ngsiv2-overview-for-developers-that-already-know-ngsiv1-20180716)

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

* FIWARE NGSI v2 [(en)](doc/manuals/user/walkthrough_apiv2.md) [(jp)](doc/manuals.jp/user/walkthrough_apiv2.md) (Markdown)
* FIWARE NGSI v2 [(en)](http://telefonicaid.github.io/fiware-orion/api/v2/stable/cookbook) [(jp)](https://open-apis.letsfiware.jp/fiware-orion/api/v2/stable/cookbook) (Apiary)
  * See also NGSIv2 implementation notes [(en)](doc/manuals/user/ngsiv2_implementation_notes.md) [(jp)](doc/manuals.jp/user/ngsiv2_implementation_notes.md)

[Top](#top)

## Reference Documentation

API Reference Documentation:

* FIWARE NGSI v2 [(en)](http://telefonicaid.github.io/fiware-orion/api/v2/stable) [(jp)](https://open-apis.letsfiware.jp/fiware-orion/api/v2/stable/) (Apiary)
  * See also NGSIv2 implementation notes [(en)](doc/manuals/user/ngsiv2_implementation_notes.md) [(jp)](doc/manuals.jp/user/ngsiv2_implementation_notes.md)

Orion Reference Documentation:

* Orion Manuals in RTD [(en)](https://fiware-orion.readthedocs.org) [(jp)](https://fiware-orion.letsfiware.jp/)

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

* Advanced Programming [(en)](doc/manuals/user/README.md) [(jp)](doc/manuals.jp/user/README.md)
* Installation and administration [(en)](doc/manuals/admin/README.md) [(jp)](doc/manuals.jp/admin/README.md)
* Container-based deployment
  * Docker [(en)](docker/README.md) [(jp)](docker/README.jp.md)
  * Docker Swarm and HA [(en)](docker/docker_swarm.md) [(jp)](docker/docker_swarm.jp.md)
* Development Manual [(en)](doc/manuals/devel/README.md) [(jp)](doc/manuals.jp/devel/README.md)
* Sample code contributions [(en)](doc/manuals/code_contributions.md) [(jp)](doc/manuals.jp/code_contributions.md)
* Contribution guidelines [(en)](doc/manuals/contribution_guidelines.md) [(jp)](doc/manuals.jp/contribution_guidelines.md), especially important if you plan to contribute with code
  to Orion Context Broker
* Deprecated features [(en)](doc/manuals/deprecated.md) [(jp)](doc/manuals.jp/deprecated.md)

[Top](#top)

## License

Orion Context Broker is licensed under Affero General Public License (GPL) version 3.

[Top](#top)

## Support

Ask your thorough programming questions using [stackoverflow](http://stackoverflow.com/questions/ask)
and your general questions on [FIWARE Q&A](https://ask.fiware.org). In both cases please use the tag `fiware-orion`

[Top](#top)
