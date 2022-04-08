# Orion Context Broker (with Linked Data Extensions)

[![FIWARE Core Context Management](https://nexus.lab.fiware.org/repository/raw/public/badges/chapters/core.svg)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/FIWARE/context.Orion-LD.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion-ld.svg)](https://hub.docker.com/r/fiware/orion-ld/)
[![Docker Repository on Quay](https://img.shields.io/badge/quay.io-orionld-green "Docker Repository on Quay")](https://quay.io/repository/fiware/orion-ld?tab=tags)
[![Support badge](https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI-LD badge](https://img.shields.io/badge/NGSI-LD-red.svg)](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.04.01_60/gs_cim009v010401p.pdf)
<br>
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion-ld.readthedocs.io/en/latest/?badge=latest)
![Status](https://nexus.lab.fiware.org/static/badges/statuses/incubating.svg)
[![Coverage Status](https://coveralls.io/repos/github/FIWARE/context.Orion-LD/badge.svg?branch=develop)](https://coveralls.io/github/FIWARE/context.Orion-LD?branch=develop)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/4800/badge)](https://bestpractices.coreinfrastructure.org/projects/4800)
![Static analysis](https://softacheck.com/app/repository/FIWARE/context.Orion-LD/badge)

Orion-LD is a Context Broker and [CEF](https://ec.europa.eu/cefdigital/wiki/display/CEFDIGITAL/CEF+Digital+Home)
[building block](https://ec.europa.eu/cefdigital/wiki/display/CEFDIGITAL/What+is+a+building+Block) for context data
management which supports both the [NGSI-LD](https://en.wikipedia.org/wiki/NGSI-LD) and the
[NGSI-v2](https://fiware.github.io/specifications/OpenAPI/ngsiv2) APIs. It is currently a fork of the original
[Orion Context Broker](https://github.com/telefonicaid/fiware-orion) extending support to add **NGSI-LD** and linked
data concepts. Orion-LD follows the [ETSI](https://en.wikipedia.org/wiki/ETSI) specification for **NGSI-LD** and has
been tested to be a stable and fast **NGSI-LD** broker with close compliance to the version 1.3.1 of the NGSI-LD API
specification.

Note that the **NGSI-LD** [specification](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.04.01_60/gs_cim009v010401p.pdf)
is a living, changing document (version 1.5 as of June 2021), with features being continuously added at a pace such that it is not
possible to align a context broker to the cutting edge specification for **NGSI-LD** implementation.

> The latest release of Orion-LD is [1.0.1](https://github.com/FIWARE/context.Orion-LD/releases/tag/1.0.1) from January 2022

This project is part of [FIWARE](https://www.fiware.org/). For more information check the FIWARE Catalogue entry for
[Core Context](https://github.com/Fiware/catalogue/tree/master/core).

Issues on this projects can be reported as [github issues](https://github.com/FIWARE/context.Orion-LD/issues), while
questions are preferred on Stackoverflow, using the tag `fiware-orion`.

| :books: [Documentation](https://github.com/FIWARE/context.Orion-LD/tree/develop/doc/manuals-ld) | :mortar_board: [Academy](https://fiware-academy.readthedocs.io/en/latest/core/orion-ld) | :whale: [Docker Hub](https://hub.docker.com/r/fiware/orion-ld/) |
| ----------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------- | --------------------------------------------------------------- |

If you want to start testing **NGSI-LD**, one option is to use Docker.

There are a number of docker images to choose from.

If you (at your own risk) want to evaluate the bleeding edge development changes, you can use the latest image:
`docker run fiware/orion-ld:latest` or even better: use
[docker compose](https://github.com/FIWARE/context.Orion-LD/blob/develop/docker/docker-compose.yml) to run it

If you instead want to use a more stable image, the latest release (as of Jan 2022) is
[1.0.1](https://github.com/FIWARE/context.Orion-LD/releases/tag/1.0.1)

```console
docker run fiware/orion-ld:1.0.1
```

Please check [dockerhub](https://hub.docker.com/r/fiware/orion-ld/tags) for newer releases.

**NGSI-LD** is an an extended subset of [JSON-LD](https://en.wikipedia.org/wiki/JSON-LD) for use with context management systems.
Its payloads are encoded as [linked data](https://en.wikipedia.org/wiki/Linked_data) using JSON.
This presentation from FIWARE Summit Malaga 2018 might be of interest:
https://www.slideshare.net/FI-WARE/fiware-global-summit-ngsild-ngsi-with-linked-data

If you are not sharing your data across systems and have no need for linked data concepts, then the current stable
version of **NGSI** (**NGSI-v2**) is more than sufficient.
If so, please use the original unforked [Orion](https://github.com/telefonicaid/fiware-orion) instead.

Examples of **NGSI-LD** can be found in [ETSI](https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/tree/master/examples). See
also the
[OpenAPI Specification of NGSI-LD](https://forge.etsi.org/swagger/ui/?url=https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/spec/updated/full_api.json)

Documentation:

-   [Guide to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)
-   [Guide to the context](doc/manuals-ld/the-context.md)
-   [Installation Guide](doc/manuals-ld/installation-guide.md)
-   [Quick Start Guide](doc/manuals-ld/quick-start-guide.md)
-   [External Libraries](doc/manuals-ld/external-libraries.md)
-   [Temporal Representation](doc/manuals-ld/troe.md)
-   [The Broker as Context Server](doc/manuals-ld/contextServer.md)
-   [State of the Implementation](doc/manuals-ld/implementationState.md)

A Test Suite for NGSI-LD compliant brokers can be found [here](https://github.com/fiware/NGSI-LD_Tests).
This test suite is kind of deprecated in favor of the [ETSI NGSI-LD API Conformance Test Suite](https://forge.etsi.org/rep/cim/ngsi-ld-test-suite).
However, the ETSI test suite still needs quite some work to be usable.
A TTF (Testing Task force) is applied for (ETSI fundings) and hopefully the test suite will be improved upon during this year of 2022.

Orion-LD passes about 95% of the test cases of the older deprecated test suite, while ETSI's test suite isn't usable at the moment.
