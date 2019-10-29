# <a name="top"></a>Orion Context Broker (Linked Data Extensions)


[![License badge](https://img.shields.io/badge/license-AGPL-blue.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion.readthedocs.io/en/latest/?badge=latest)
[![Docker badge](https://img.shields.io/docker/pulls/fiware/orion-ld.svg)](https://hub.docker.com/r/fiware/orion-ld/)
[![Support badge]( https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI-LD badge](https://img.shields.io/badge/NGSI-LD-red.svg)](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.02.01_60/gs_CIM009v010201p.pdf)

This Generic Enabler implements the [NGSI-LD API](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.02.01_60/gs_CIM009v010201p.pdf)

If you want to start testing NGSI-LD, one option is to use Docker:

```docker run fiware/orion-ld```  or even better: use [docker compose](https://github.com/FIWARE/context.Orion-LD/blob/develop/docker/docker-compose.yml) to run it

If you only intend to use NGSIv2, please use instead [Orion](https://github.com/telefonicaid/fiware-orion). 

This component is still in Alpha state but already passing a bunch of [tests](https://github.com/FIWARE/NGSI-LD_TestSuite)

For a description of what NGSI-LD is please check [this](https://github.com/Fiware/NGSI-LD_Wrapper/blob/master/README.md).
This presentation from FIWARE Summit Malaga 2018 is also of interest: https://www.slideshare.net/FI-WARE/fiware-global-summit-ngsild-ngsi-with-linked-data

Examples of NGSI-LD can be found in [ETSI](https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/tree/master/examples).
See also [OpenAPI Specification of NGSI-LD](https://forge.etsi.org/swagger/ui/?url=https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/spec/updated/full_api.json)

A Test Suite for NGSI-LD can be found [here](https://github.com/fiware/NGSI-LD_Tests). 

For full Orion documentation, please check Orion's [README.md](https://github.com/telefonicaid/fiware-orion)

(The content of this repo may eventually be merged into the main Orion development line)

The current state of the implementation is found [here](doc/manuals/orionld-progress.md).

## External Libraries
The LD part of orionld depend on the following external libraries:
* microhttpd
* mongo client library (C++ Legacy driver)
* libcurl
* kbase
* klog
* kalloc
* kjson

The "heart" of the linked-data extension of orionld is the _kjson_ library.
kjson takes care of the parsing of incoming JSON and transforms the textual input as a tree of KjNode structs.
In the case of entities, the tree is basically a linked list of attributes (id and type are some kind of attributes of the entity, right?)
that can have children, the metadata. In NGSI-LD the metadata takes another name, namely Property-of-Property, or Property-of-Relationhsip, or ...

But, trees based on the KjNode structure isn't just for the incoming payload, but also for:
* outgoing payload
* context cache
* intermediate storage format for DB abstaction layer
* etc.

