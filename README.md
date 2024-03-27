# Orion Context Broker (with Linked Data Extensions)

[![FIWARE Core Context Management](https://nexus.lab.fiware.org/repository/raw/public/badges/chapters/core.svg)](https://www.fiware.org/developers/catalogue/)
[![License badge](https://img.shields.io/github/license/FIWARE/context.Orion-LD.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Docker badge](https://img.shields.io/badge/quay.io-fiware%2Forion--ld-grey?logo=red%20hat&labelColor=EE0000)](https://quay.io/repository/fiware/orion-ld)
[![Support badge](https://img.shields.io/badge/support-sof-yellowgreen.svg)](http://stackoverflow.com/questions/tagged/fiware-orion)
[![NGSI-LD badge](https://img.shields.io/badge/NGSI-LD-red.svg)](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.08.01_60/gs_cim009v010801p.pdf)
<br>
[![Documentation badge](https://readthedocs.org/projects/fiware-orion/badge/?version=latest)](http://fiware-orion-ld.readthedocs.io/en/latest/?badge=latest)
![Status](https://nexus.lab.fiware.org/static/badges/statuses/incubating.svg)
[![Coverage Status](https://coveralls.io/repos/github/FIWARE/context.Orion-LD/badge.svg?branch=develop)](https://coveralls.io/github/FIWARE/context.Orion-LD?branch=develop)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/4800/badge)](https://bestpractices.coreinfrastructure.org/projects/4800)

This project is part of [FIWARE](https://www.fiware.org/). For more information check the FIWARE Catalogue entry for
[Core Context](https://github.com/Fiware/catalogue/tree/master/core).

Issues on this projects can be reported as [github issues](https://github.com/FIWARE/context.Orion-LD/issues),
while questions are preferred on Stackoverflow, using the tag `fiware-orion`.

> The latest release of Orion-LD is [1.5.1](https://github.com/FIWARE/context.Orion-LD/releases/tag/1.5.1) from January 2024

Orion-LD is a Context Broker and [CEF](https://ec.europa.eu/digital-building-blocks/sites/display/DIGITAL/About+us)
[building block](https://joinup.ec.europa.eu/collection/egovernment/solution/cef-context-broker) for context data
management which supports both the [NGSI-LD](https://en.wikipedia.org/wiki/NGSI-LD) and the
[NGSI-v2](https://fiware.github.io/specifications/OpenAPI/ngsiv2) APIs. It is currently a fork of the original
[Orion Context Broker](https://github.com/telefonicaid/fiware-orion) extending support to add **NGSI-LD** and linked
data concepts. Orion-LD follows the [ETSI](https://en.wikipedia.org/wiki/ETSI) specification for **NGSI-LD** and has
been tested to be a stable and fast **NGSI-LD** broker with near compliance to the version 1.6.1 of the NGSI-LD API
specification.


## License
Orion-LD is licensed under [Affero General Public License (GPL) version 3](./LICENSE).

<details>
<summary><strong>Further information on the use of the AGPL open source license</strong></summary>
  
### Are there any legal issues with AGPL 3.0? Is it safe for me to use?
There is absolutely no problem in using a product licensed under AGPL 3.0. Issues with GPL
(or AGPL) licenses are mostly related with the fact that different people assign different
interpretations on the meaning of the term “derivate work” used in these licenses. Due to this,
some people believe that there is a risk in just _using_ software under GPL or AGPL licenses
(even without _modifying_ it).

For the avoidance of doubt, the owners of this software licensed under an AGPL-3.0 license
wish to make a clarifying public statement as follows:

> Please note that software derived as a result of modifying the source code of the
> software in order to fix a bug or incorporate enhancements IS considered a derivative
> work of the product. Software that merely uses or aggregates (i.e. links to) an
> otherwise unmodified version of existing software IS NOT considered a derivative work.

</details>

## Contribution to Orion-LD
Anyone wishing to contribute to Orion-LD, be it fixing/adding documentation, tests, source code, all types of contributions are welcome.
For source code contributions, please see the [Contribution guidelines](doc/manuals/devel/contribution_guidelines.md).


## General Information on NGSI-LD
**NGSI-LD** is an an extended subset of [JSON-LD](https://en.wikipedia.org/wiki/JSON-LD) for use with context management systems.
Its payloads are encoded as [linked data](https://en.wikipedia.org/wiki/Linked_data) using JSON.

The NGSI-LD Specification is regularly updated and published by ETSI.
The latest specification is [version 1.8.1](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.08.01_60/gs_cim009v010801p.pdf)
and it was published in March 2024.

This presentation from the FIWARE Summit in Malaga 2018 might be of interest:
https://www.slideshare.net/FI-WARE/fiware-global-summit-ngsild-ngsi-with-linked-data

Examples of **NGSI-LD** can be found in [ETSI](https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/tree/master/examples).
See also the [OpenAPI Specification of NGSI-LD](https://forge.etsi.org/swagger/ui/?url=https://forge.etsi.org/rep/NGSI-LD/NGSI-LD/-/raw/master/spec/updated/generated/full_api.json)

If you are not sharing your data across systems and have no need for linked data concepts, then the current stable
version of **NGSI** (**NGSI-v2**) is more than sufficient.
If so, please use the original unforked [Orion](https://github.com/telefonicaid/fiware-orion) instead of Orion-LD (note that Orion-LD is **not** up-to-date with Orion in terms of NGSI v2).


## NGSI-LD Context Broker Feature Comparison
An Excel file detailing the current compatibility of the development version of the Orion-LD Context Broker against the features of v1.6.1 of the API specification can be downloaded [here](https://docs.google.com/spreadsheets/d/18tq0_PZFl5WCfYUElcdI6M3Vlin4hP-M).

| :books: [Documentation](https://github.com/FIWARE/context.Orion-LD/tree/develop/doc/manuals-ld) | :mortar_board: [Academy](https://fiware-academy.readthedocs.io/en/latest/core/orion-ld) | <img style="height:1em" src="https://quay.io/static/img/quay_favicon.png"/> [quay.io](https://quay.io/repository/fiware/orion-ld) |
| ----------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------- | --------------------------------------------------------------- |


## Test and Deployment of Orion-LD
If you want to start testing Orion-LD, the most common option is to use Docker.
There are a number of docker images to choose from.

If you (at your own risk) want to evaluate the bleeding edge development changes, you can use the latest image:
`docker run fiware/orion-ld:latest` or even better: use
[docker compose](https://github.com/FIWARE/context.Orion-LD/blob/develop/docker/docker-compose.yml) to run it.

The use of the "latest" tag is not recommended, as it keeps changing.
Much better would be to use the newest non-changing tag and stick to it until you have need for some newer feature/fix and then change to a newer non-changing tag.
Every merged pull request results in a new non-changing tag in dockerhub.

Please note that for production and/or performance implementations, there is a thorough guide for that right [here](https://github.com/FIWARE/load-tests)

If you instead want to use a more stable image, the latest release (as of January 2024) is
[1.5.1](https://github.com/FIWARE/context.Orion-LD/releases/tag/1.5.1)

```console
docker run quay.io/fiware/orion-ld:1.5.1
```

Please check [quay.io](https://quay.io/repository/fiware/orion-ld?tab=tags) for other releases.

## Documentation:
-   [Guide to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)
-   [Guide to the context](doc/manuals-ld/the-context.md)
-   [Installation Guide](doc/manuals-ld/installation-guide.md)
-   [Quick Start Guide](doc/manuals-ld/quick-start-guide.md)
-   [External Libraries](doc/manuals-ld/external-libraries.md)
-   [Temporal Representation](doc/manuals-ld/troe.md)
-   [The Broker as Context Server](doc/manuals-ld/contextServer.md)
-   [State of the Implementation](doc/manuals-ld/implementationState.md)
-   [Roadmap](roadmap.md)
-   [Planning - Issue #280](https://github.com/FIWARE/context.Orion-LD/issues/280)

A Test Suite for NGSI-LD compliant brokers can be found [here](https://github.com/fiware/NGSI-LD_Tests).
This test suite is kind of deprecated in favor of the [ETSI NGSI-LD API Conformance Test Suite](https://forge.etsi.org/rep/cim/ngsi-ld-test-suite).
However, the ETSI test suite still needs quite some work to be usable.
A TTF (Testing Task force) is applied for (ETSI fundings) and hopefully the test suite will be improved upon during 2023.

Orion-LD passes about 95% of the test cases of the older deprecated test suite, while ETSI's test suite isn't really usable at the moment.
