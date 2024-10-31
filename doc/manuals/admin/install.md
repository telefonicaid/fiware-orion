# Installing Orion

* [Introduction](#introduction)
* [Requirements](#requirements)
* [Upgrading from a previous version](#upgrading-from-a-previous-version)
    
---
> :warning: **If you plan to run orion in production, also read the information about [high availability](extra/ha.md) and [performance tuning](perf_tuning.md).**
---

## Introduction

The recommended procedure is to install using the official [Orion docker container at Dockerhub](https://hub.docker.com/repository/docker/telefonicaiot/fiware-orion). Full detail is provided in [this document](https://github.com/telefonicaid/fiware-orion/blob/master/docker/README.md).

However, if you don't have a docker-ized infrastructure, the recommended procedure is to install building from sources, check [this document](build_source.md).

## Requirements

In the case you install using the official Orion docker container at Dockerhub, you need to have the following:

* [Docker](https://docs.docker.com/engine/install/)
* [Docker compose](https://docs.docker.com/compose/install/) (not strictly needed, but strongly recommended) 

In the case you are installing Orion building from sources you need:

* Operating system: Debian. The reference operating system is Debian 12.7
  but it should work also in any later Debian 12 version.
* Database: MongoDB is required to run either in the same host where Orion Context Broker is to be installed or in a different host accessible through the network. The recommended MongoDB version
  is 6.0 (Orion may work with older versions but we don't recommend it at all!).

For system resources (CPUs, RAM, etc.) see [these recommendations](diagnosis.md#resource-availability).

## Upgrading from a previous version

From a software point of view, Orion upgrading is as easy as replace the old container or contextBroker by the new one. However, if you also want to migrate existing data you should pay attention to this section.

You only need to pay attention to this if your upgrade path crosses 0.14.1, 0.19.0, 0.21.0, 1.3.0, 1.5.0 and 2.2.0.
Otherwise, you can skip this section. You can also skip this section if your DB are not valuable (e.g. debug/testing environments) and
you can flush your DB before upgrading.

* [Upgrading to 0.14.1 and beyond from a pre-0.14.1 version](upgrading_crossing_0-14-1.md)
* [Upgrading to 0.19.0 and beyond from a pre-0.19.0 version](upgrading_crossing_0-19-0.md)
* [Upgrading to 0.21.0 and beyond from a pre-0.21.0 version](upgrading_crossing_0-21-0.md)
* [Upgrading to 1.3.0 and beyond from a pre-1.3.0 version](upgrading_crossing_1-3-0.md)
* [Upgrading to 1.5.0 and beyond from a pre-1.5.0 version](upgrading_crossing_1-5-0.md)
* [Upgrading to 2.2.0 and beyond from a pre-2.2.0 version](upgrading_crossing_2-2-0.md)

If your upgrade cover several segments (e.g. you are using 0.13.0 and
want to upgrade to 0.19.0, so both "upgrading to 0.14.1 and beyond from
a pre-0.14.1 version" and "upgrading to 0.19.0 and beyond from a
pre-0.19.0 version" applies to the case) you need to execute the
segments in sequence (the common part are done only one time, i.e. stop
CB, remove package, install package, start CB). In the case of doubt,
please [ask using StackOverflow](http://stackoverflow.com/questions/ask)
(remember to include the "fiware-orion" tag in your questions).
