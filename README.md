# <a name="top"></a>Orion Context Broker (Linked Data Extensions)


[![FIWARE Core Context Management](https://nexus.lab.fiware.org/repository/raw/public/badges/chapters/core.svg)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/telefonicaid/fiware-orion.svg)](https://opensource.org/licenses/AGPL-3.0)
[![License badge](https://img.shields.io/badge/license-AGPL-blue.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion.readthedocs.io/en/latest/?badge=latest)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion-ld.svg)](https://hub.docker.com/r/fiware/orion-ld/)
[![Support badge]( https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI-LD badge](https://img.shields.io/badge/NGSI-LD-red.svg)](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.02.01_60/gs_CIM009v010201p.pdf)

This Generic Enabler implements the [NGSI-LD API](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.02.01_60/gs_CIM009v010201p.pdf)
Orion-LD extends [the Orion Context Broker](https://github.com/telefonicaid/fiware-orion)

If you want to start testing NGSI-LD, one option is to use Docker:

```docker run fiware/orion-ld```  or even better: use [docker compose](https://github.com/FIWARE/context.Orion-LD/blob/develop/docker/docker-compose.yml) to run it

If you only intend to use NGSIv2, please use instead [Orion](https://github.com/telefonicaid/fiware-orion). 

This component is still in Alpha state but already passing a bunch of [tests](https://github.com/FIWARE/NGSI-LD_TestSuite)

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

