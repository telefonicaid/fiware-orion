# Orion Context Broker (with Linked Data Extensions)

[![FIWARE Core Context Management](https://nexus.lab.fiware.org/repository/raw/public/badges/chapters/core.svg)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/FIWARE/context.Orion-LD.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion-ld.svg)](https://hub.docker.com/r/fiware/orion-ld/)
[![Support badge](https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI-LD badge](https://img.shields.io/badge/NGSI-LD-red.svg)](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.04.01_60/gs_cim009v010401p.pdf)
<br>
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion-ld.readthedocs.io/en/latest/?badge=latest)
![Status](https://nexus.lab.fiware.org/static/badges/statuses/incubating.svg)
[![Coverage Status](https://coveralls.io/repos/github/FIWARE/context.Orion-LD/badge.svg?branch=develop)](https://coveralls.io/github/FIWARE/context.Orion-LD?branch=develop)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/4800/badge)](https://bestpractices.coreinfrastructure.org/projects/4800)

Orion-LD is a Context Broker and [CEF](https://ec.europa.eu/cefdigital/wiki/display/CEFDIGITAL/CEF+Digital+Home)
[building block](https://ec.europa.eu/cefdigital/wiki/display/CEFDIGITAL/What+is+a+building+Block) for context data
management which supports both the [NGSI-LD](https://en.wikipedia.org/wiki/NGSI-LD) and the
[NGSI-v2](https://fiware.github.io/specifications/OpenAPI/ngsiv2) APIs. It is currently a fork of the original
[Orion Context Broker](https://github.com/telefonicaid/fiware-orion) extending support to add **NGSI-LD** and linked
data concepts. Orion-LD follows the [ETSI](https://en.wikipedia.org/wiki/ETSI) specification for **NGSI-LD** and has
been tested to be a stable and fast **NGSI-LD** broker with close compliance to the version 1.3.1 of the NGSI-LD API
specification.

Note that the **NGSI-LD**
[specification](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.04.01_60/gs_cim009v010401p.pdf) is a living,
changing document (version 1.5 as of June 2021), with features being continuously added at a pace such that it is not
possible to align a context broker to the cutting edge specification for **NGSI-LD** implementation.

> The latest release of Orion-LD is ([Beta 3](https://github.com/FIWARE/context.Orion-LD/releases/tag/0.8.0) of
> June 2021) and it contains the following additions with respect to
> [Beta 1 (initially alpha-6)](https://github.com/FIWARE/context.Orion-LD/releases/tag/v0.6.1-alpha):
>
> -   Query responses and notifications in GeoJSON format, if so requested (Accept: application/geo+json)
> -   Performance improvements
> -   Bug fixes
> -   Working and tested implementation of the optional interface for Temporal Representation of Entities (TRoE - feel
>     free to use, but at your own risk ;-))
>
> _The plan is to merge Orion-LD back into the main branch of the original Orion at some point._

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

If you instead want to use a more stable image, the latest beta release (as of June 2021) is
[0.8.0](https://github.com/FIWARE/context.Orion-LD/releases/tag/0.8.0)

```console
docker run fiware/orion-ld:0.8.0
```

Please check [dockerhub](https://hub.docker.com/r/fiware/orion-ld/tags) for newer releases.

**NGSI-LD** is an an extended subset of [JSON-LD](https://en.wikipedia.org/wiki/JSON-LD) for use with context management
systems, its payloads are encoded as [linked data](https://en.wikipedia.org/wiki/Linked_data) using JSON. This
presentation from FIWARE Summit Malaga 2018 is also of interest:
https://www.slideshare.net/FI-WARE/fiware-global-summit-ngsild-ngsi-with-linked-data

If you are not sharing your data across systems and have no need of linked data concepts, then the current stable
version of **NGSI** - **NGSI-v2** is sufficient please use the original unforked
[Orion](https://github.com/telefonicaid/fiware-orion) instead.

This component is currently a beta release but already passing most of the
[NGSI-LD test suite](https://github.com/FIWARE/NGSI-LD_TestSuite)

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

A Test Suite for **NGSI-LD** can be found [here](https://github.com/fiware/NGSI-LD_Tests).
