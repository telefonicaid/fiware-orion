# Design Specification of Orion Context Broker

## Directory Structure

Orion Context Broker is divided into a number of libraries, each library
containing a number of modules (with *module* we refer to a source code file
and its corresponding header file).  
Each library has its own directory under `src/lib/`.  

The main program, that basically initializes the libraries and starts the
REST interface resides in its own directory `src/app/contextBroker/`.

Unit tests and functional tests reside under the `test/` directory while
scripts used for testing and release making are found under `scripts/`.

Quick listing of source code directories:

* src/app/contextBroker/ (Main program)
* src/lib/logMsg/ (Logging)
* src/lib/parseArgs/ (CLI argument parsing)
* src/lib/common/ (Common types and functions for all the libraries)
* src/lib/orionTypes/ (Common types)
* src/lib/rest/ (REST interface, using external library microhttpd)
* src/lib/ngsi/ (Common NGSI types)
* src/lib/ngsi10/ (Common NGSI10 types)
* src/lib/ngsi9/ (Common NGSI9 types)
* src/lib/apiTypesV2/ (NGSIv2 types)
* src/lib/parse/ (common functions and types for payload parsing)
* src/lib/jsonParse/ (Parsing of JSON payload for NGSIv1 requests)
* src/lib/jsonParseV2/ (Parsing of JSON payload for NGSIv2 requests)
* src/lib/serviceRoutines/ (Service routines for NGSIv1)
* src/lib/serviceRoutinesV2/ (Service routines for NGSIv2)
* src/lib/convenience/ (Convenience operations in NGSIv1)
* src/lib/mongoBackend/ (Database interface to mongodb)
* src/lib/ngsiNotify/ (NGSIv1 notifications) 
* src/lib/alarmMgr/ (Alarm Manager implementation)
* src/lib/cache/ (Subscription cache implementation)
* src/lib/logSummary/ (Log Summary implementation)
* src/lib/metricsMgr/ (Metrics Manager implementation)

## src/app/contextBroker/
The main program is found in `contextBroker.cpp` and its purpose it to:

* Parse and treat the command line parameters.
* Initialize the libraries of the broker,
* Especially, setup the service vector (RestService restServiceV) that defines
  the REST services that the broker supports.
* Start the REST interface (that runs in a separate thread).

This is the file to go to when adding a command line parameter and when adding
a REST service for the broker.
See the [cookbook](cookbook.md) for details about these two important topics.
