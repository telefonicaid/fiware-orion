# <a name="top"></a>Development Manual

*Note: This document describes Orion Context Broker as of release 2.0.x.*

## Intended audience
The intended audience of this manual is developers that need to understand the internals of the Orion Context Broker
because they are to modify or add features of Orion.
Or, readers that simply have an interest in how the Orion Context Broker is implemented.  

Orion is written in C/C++ and previous knowledge of these programming languages will definitely help to understand this document.  

Previous knowledge of the external libraries that Orion depends on also helps the understanding. Namely:

* Microhttpd
* Libcurl
* Rapidjson (NGSIv2 JSON parsing)
* MongoMB C++ driver
* Boost property tree (NGSIv1 JSON parsing)

In the case of MongoDB, not only knowledge of the driver is recommended, but also MongoDB technology in general.

Also, as NGSI is used for the payload of the requests, some previous knowledge of NGSI helps.

## Contents

* [High level internal architecture](architecture.md). A high level description of the Orion Context Broker internal architecture. We recommend to read this document before any other.
* [Directory structure](directoryStructure.md). A description of the directory structure used by Orion Context Broker repository.
* [Source code: main program and libraries](sourceCode.md). A description of the different libraries (and main program) in which the source code is structured.
* [Flow index](flowsIndex.md). An index for all the flow diagrams described in the development documentation. A very useful "map" to have at hand.
* [Semaphores](semaphores.md). This document provides detailed information about the different semaphores that Orion uses for internal synchronization.
* [Cookbook](cookbook.md). This document describes some useful development related recipes.

