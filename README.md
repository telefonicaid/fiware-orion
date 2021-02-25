# <a name="top"></a>Orion Context Broker (Linked Data Extensions)


[![FIWARE Core Context Management](https://nexus.lab.fiware.org/repository/raw/public/badges/chapters/core.svg)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/FIWARE/context.Orion-LD.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion-ld.svg)](https://hub.docker.com/r/fiware/orion-ld/)
[![Support badge]( https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI-LD badge](https://img.shields.io/badge/NGSI-LD-red.svg)](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.04.01_60/gs_cim009v010401p.pdf)
<br>
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion-ld.readthedocs.io/en/latest/?badge=latest)
![Status](https://nexus.lab.fiware.org/static/badges/statuses/incubating.svg)
[![Coverage Status](https://coveralls.io/repos/github/FIWARE/context.Orion-LD/badge.svg?branch=develop)](https://coveralls.io/github/FIWARE/context.Orion-LD?branch=develop)

Orion-LD is a Context Broker which supports both the [NGSI-LD](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.04.01_60/gs_cim009v010401p.pdf) and the 
[NGSI-v2](https://fiware.github.io/specifications/OpenAPI/ngsiv2) APIs.
It is a fork of the original [Orion Context Broker](https://github.com/telefonicaid/fiware-orion) which supports NGSI-v2 only.
The NGSI-LD specification is a living, changing document, and the latest Orion-LD beta release is nearly feature complete to the
1.3.1 ETSI specification. The plan is to merge Orion-LD back into the main branch of Orion at some point.

This project is part of [FIWARE](https://www.fiware.org/). For more information check the FIWARE Catalogue entry for
[Core Context](https://github.com/Fiware/catalogue/tree/master/core).

| :books: [Documentation](https://github.com/FIWARE/context.Orion-LD/tree/develop/doc/manuals-ld) | :mortar_board: [Academy](https://fiware-academy.readthedocs.io/en/latest/core/orion-ld) | :whale: [Docker Hub](https://hub.docker.com/r/fiware/orion-ld/) |
| ----------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- | ---------------------------------------------------------------- |

If you want to start testing NGSI-LD, one option is to use Docker.

There are a number of docker images to choose from.

If you (at your own risk) want to evaluate the bleeding edge development changes, you can use the latest image:
```docker run fiware/orion-ld:latest```  or even better: use [docker compose](https://github.com/FIWARE/context.Orion-LD/blob/develop/docker/docker-compose.yml) to run it

If you instead want to use a more stable image, the latest beta release (as of December 2020) is 0.6.1

```docker run fiware/orion-ld:0.6.1```

Please check [dockerhub](https://hub.docker.com/r/fiware/orion-ld/tags) for newer releases.

If you only intend to use NGSI-v2, please use instead [Orion](https://github.com/telefonicaid/fiware-orion). 

This component is currenrtly a beta release but already passing most of the [NGSI-LD test suite](https://github.com/FIWARE/NGSI-LD_TestSuite)

For a description of what NGSI-LD is please check [this](https://github.com/Fiware/NGSI-LD_Wrapper/blob/master/README.md).
This presentation from FIWARE Summit Malaga 2018 is also of interest: https://www.slideshare.net/FI-WARE/fiware-global-summit-ngsild-ngsi-with-linked-data

Examples of NGSI-LD can be found in [ETSI](https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/tree/master/examples).
See also the [OpenAPI Specification of NGSI-LD](https://forge.etsi.org/swagger/ui/?url=https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/spec/updated/full_api.json)

Documentation:
* [Guide to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)
* [Guide to the context](doc/manuals-ld/the-context.md)
* [Installation Guide](doc/manuals-ld/installation-guide.md)
* [Quick Start Guide](doc/manuals-ld/quick-start-guide.md)
* [The current state of the implementation](doc/manuals-ld/progress.md).
* [External Libraries](doc/manuals-ld/external-libraries.md)

A Test Suite for NGSI-LD can be found [here](https://github.com/fiware/NGSI-LD_Tests). 

