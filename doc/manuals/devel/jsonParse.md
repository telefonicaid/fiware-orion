# <a name="top"></a>JSON Parse NGSIv1

* [Introduction](#introduction)
* [Parsing process](#parsing-process)
	* [Implementation details](#implementation-details)
* [Top-level `jsonParse()`](#top-level-jsonparse)
* [Low-level `jsonParse()`](#low-level-jsonparse)

## Introduction

Orion Context Broker contains not one but **two** libraries for JSON parsing. The reason for this is that the external library that was originally selected for parsing of NGSIv1 JSON cannot distinguish between JSON value types such as String, Number, Boolean, Null but **treats all values as strings**. This was unacceptable for NGSIv2 and so, another external JSON library ([rapidjson](http://rapidjson.org/)) was chosen. The two JSON libraries of Orion implement the necessary adaption of the external libraries to be usable by Orion.

This document describes NGSIv1 parsing details. NGSIv2 parsing details are described in a [separate document](jsonParseV2.md).

In general, the NGSIv1 parsing logic is more complex than NGSIv2 logic. The good news is that you will probably not need to change anything in NGSIv1 parsing as this is the old version of the Orion API and the work should now concentrate in evolving NGSIv2, not NGSIv1.

The purpose of the parse step is to transform a text buffer, JSON in this case, to an instance of a class/struct in C++. The external library takes care of parsing the JSON string while the Orion library extracts the information and populates the approipriate instance of the class/struct in C++.  

A very simple but illustrative example:

The following payload (for `POST /v1/queryContext`) ... :
```
{
  "entities": [
    {
      "type": "Room",
      "id": "ConferenceRoom"
    }
  ],
  "attributes": [ "temperature" ]
}
```

... would be transformed into a C++ instance of the class `QueryContextRequest` something like this:
```
QueryContextRequest* qprP = new QueryContextRequest();
EntityId*            eP   = new EntityId();

eP->id   = "ConferenceRoom";
eP->type = "Room";

qprP->entityIdVector.push_back(eP);
qprP->attributeList.push_back("temperature");
```

This instance of `QueryContextRequest` is created by the **jsonParse** library and the service routine `postQueryContext()` passes it to the [**mongoBackend**](sourceCode.md#srclibmongobackend) function `mongoQueryContext()`.

[Top](#top)

## Parsing process
The library **jsonParse** contains two overloaded functions with the name `jsonParse()`:

* The first one is the toplevel function that is called only once per request. See [dedicated section on top-level jsonParse()](#top-level-jsonparse).
* The second `jsonParse()` is invoked by the **toplevel** `jsonParse()` once per node in the parsed tree that is output from [boost property_tree](https://theboostcpplibraries.com/boost.propertytree) and it *calls itself recursively* following the output tree (we will use `jsonParse()*` from now on to distinguish this second, lower level `jsonParse()` from the top level `jsonParse()` function). See its full explanation in the [dedicated section on low level jsonParse()](#low-level-jsonparse).

The concrete example used for the following image is the parsing of payload for `POST /v1/updateContextRequest`.

<a name="flow-pp-01"></a>
![Parsing an NGSIv1 payload](images/Flow-PP-01.png)

_PP-01: Parsing an NGSIv1 payload_  

* `payloadParse()` calls the NGSIv1 parse function for JSON payloads (which is one of three possible parse functions to call: parsing of NGSIv1 JSON, NGSIv2 JSON and text) (step 1).
* `jsonTreat()` looks up the type of the request by calling `jsonRequestGet()` (step 2), which returns a pointer to a `JsonRequest` struct that is needed to parse the payload.
    * Each type of payload needs different input to the common parsing routines. A vector of `JsonRequest` structs contains this information and `jsonRequestGet()` looks up the corresponding `JsonRequest` struct in the vector and returns it. More on the `JsonRequest` struct later.
* Knowing the specific information for the request type, `jsonTreat()` calls the toplevel `jsonParse()`, whose responsibility is to start the parsing of the payload  (step 3). `jsonParse()` reads in the payload into a `stringstream` and calls the function `read_json()` that takes care of the parsing of the payload and converts the payload into a property tree.
* After that, `jsonParse()*` (the lower level) is invoked on the resulting tree to convert the boost property tree into an Orion structure (step 4). In fact, `jsonParse()*` is invoked at this point as many times as there are top level keys in the JSON to process.
	* Example: for `{ "a": ..., "b": ..., "c:"... }`, `jsonParse()*` will be invoked three times (once for `"a"`, once for `"b"` and once for `"c"`).
* `jsonParse()*` calls the `treat()` function on each node (step 5) and if the node is not a leaf, it does an recursive call to itself for each child of the node.
* The `treat()` function checks for forbidden characters in the payload and then calls the specific Parse-Function for the node in question  (step 6). A pointer to this specific Parse-Function is found in the struct `JsonRequest`, as well as the path to each node, which is how the struct is found. 
* The Parse-Function simply extracts the information from the tree node and adds it to the resulting Orion struct that is the result of the entire parse. Note that each node in the tree has its own Parse-Function and that in this image just a few selected Parse-Functions are shown. In fact, to parse this `UpdateContextRequest` payload, there are no less than 19 Parse-Functions (see `jsonParse/jsonUpdateContextRequest.cpp`).     

[Top](#top)

### Implementation Details
As earlier stated, `jsonTreat()` in `src/lib/jsonParse/jsonRequest.cpp` is invoked by `payloadParse()` in `src/lib/rest/RestService.cpp`. Before diving into `jsonTreat()`, let's take a look at the struct `JsonRequest` that has a very important role in the function:

```
typedef struct JsonRequest
{
  RequestType      type;          // Type of request (URI PATH translated to enum)
  std::string      method;        // HTTP Method (POST/PUT/PATCH ...)
  std::string      keyword;       // Old reminiscent from XML parsing
  JsonNode*        parseVector;   // Path and pointer to entry function for parsing
  RequestInit      init;          // pointer to function for parse initialization
  RequestCheck     check;         // pointer to	function for checking of the parse result
  RequestPresent   present;       // pointer to	function for presenting the parse result in log file
  RequestRelease   release;       // pointer to	function that frees up memory after the parse result has been used
} JsonRequest;
```

See also the variable `jsonRequest`, which is a vector of `JsonRequest`, in `src/lib/jsonParse/jsonRequest.cpp` for a full list of the supported requests with payload.

The first thing that `jsonTreat()` does is to call `jsonRequestGet()` to look up an item in the vector `jsonRequest`. The search criteria to look up the vector item is the **RequestType** (which depends on the URL PATH of the request) and the **HTTP Method** used.

If the combination of URL PATH and HTTP Method is not found in the `JsonRequest` vector (`jsonRequest`), then the request is not valid and an error is returned. If found, then the vector item contains all the information needed to parse the payload and build the corresponding raw structure.

#### Adding a new request with payload 

To add a request in NGSIv1, with payload to parse, an item **must be added** to the vector `jsonRequest`.  

Now, after finding the vector item for the request, `jsonTreat()` does the following:

* `init()`
* `parse()`
* `check()`

`release()` cannot be called until the result of the parse has been used. that is, if no error has been detected. In case of errors, the `release()` function is called right after the call to payloadParse, as the result is garbage and cannot be used. Normally the parse works just fine and the resulting instance from the parse step is passed to mongoBackend for processing and the release function cannot be called until mongoBackend is done with its processing. The corresponding service routine calls mongoBackend and `restService()` calls the service routine:

```
std::string response = serviceV[ix].treat(ciP, components, compV, &parseData);
```

After returning from the service routine, the result of the parse can be released without risk.

[Top](#top)

## Top-level `jsonParse()`
As mentioned, there are two different functions called `jsonParse()` in `src/lib/jsonParse/jsonParse.cpp`. One top level and one lower level. The top level `jsonParse()` is the entry function and it is visible from outside of `src/lib/jsonParse/jsonParse.cpp`:

```
std::string jsonParse
(
  ConnectionInfo*     ciP,          // Connection Info valid for the life span of the request
  const char*         content,      // Payload as a string
  const std::string&  requestType,  // The type of request (URL PATH)
  JsonNode*           parseVector,  // Function pointers etc for treatment of the nodes
  ParseData*          parseDataP    // Output pointer to C++ classes for the result of the the parse
)
```

This function is called by `jsonTreat()` in `src/lib/jsonParse/jsonRequest.cpp`, which in its turn in called by `payloadParse()` in `src/lib/rest/RestService.cpp`.

The purpose of the function is to initiate the parsing of the content (JSON string in the parameter `content`) with the help of `boost::property_tree::ptree`, by:

* Get start-time for timing statistics, if requested
* Fix *escaped chars*, i.e remove backslash preceding a slash: `"\/"` => `"/"` 
* Load the `content` in the `ptree` variable `tree`
* Call the low-level `jsonParse()` for each first level node of the tree. The low-level `jsonTreat()` dives deeper.
* Return **Error** if low-level `jsonTreat()` fails
* Get end-time for timing statistics,	if requested, and save diff-time for later use

[Top](#top)

## Low-level `jsonParse()`
The low-level `jsonParse` is static in `src/lib/jsonParse/jsonParse.cpp` and **only** called by the high-level `jsonParse()` (except for the recursive calls it makes itself).

Its signature:

```
static std::string jsonParse
(
  ConnectionInfo*                           ciP,          // "Global" info about the current request
  boost::property_tree::ptree::value_type&  v,            // The node-in-the-tree
  const std::string&                        _path,        // The path to the node-in-the-tree 
  JsonNode*                                 parseVector,  // Function pointers etc for treatment of the nodes
  ParseData*                                parseDataP    // Output pointer to C++ classes for the result of the the parse
)
```

Let's describe the different parameters one by one.

### `ConnectionInfo* ciP`
This pointer to `ConnectionInfo` is created by the function that MHD (libmicrohttpd) uses for the callbacks while reading the request (`connectionTreat` in `src/lib/rest/rest.cpp`). `ciP` contains information about the request such as:

* HTTP Method/Verb
* HTTP Headers
* URI Parameters (E.g. `?a=1&b=2`)
* URI Path (E.g. `/v1/queryRequest`) ...
* ... and much more. See `src/lib/rest/ConnectionInfo.h`

The pointer to `ConnectionInfo` is passed to many functions in the libraries **jsonParse**, **jsonParseV2**, **rest**, **serviceRoutines** and **serviceRoutinesV2**.

### `boost::property_tree::ptree::value_type& v`

This is a reference to the currently treated node in the tree. Not much more to say about it.  See the [boost property_tree documentation](https://theboostcpplibraries.com/boost.propertytree) for more information on this.

### `const std::string& _path`

`jsonParse()` keeps the path to the node as a string, to know exactly which node in the tree is treated. E.g.:

```
{
  "entities": [
    {
      "type": "Room",
      "id": "ConferenceRoom"
    }
  ],
  "attributes": [ "temperature" ]
}
```

The node `type` would have the path `/entities/entity/type`. The middle name `entity` is because `entities` is a vector. More on this later.

### `JsonNode* parseVector`
`JsonNode` is a struct defined in `src/lib/jsonParse/JsonNode.h`:

```
typedef std::string (*JsonNodeTreat)(const std::string& path, const std::string& value, ParseData* reqDataP);

typedef struct JsonNode
{
  std::string    path;
  JsonNodeTreat  treat;
} JsonNode;
```

Instances of `JsonNode` contain the path of a node (e.g. `/entities/entity/type`) and a reference to the corresponding treat-function for a node with that very path. This is how `jsonTreat` knows which treat-function to call for each node in the tree. As illustration, see `src/lib/jsonParse/jsonQueryContextRequest.cpp`, variable `jsonQcrParseVector`:

```
JsonNode jsonQcrParseVector[] =
{
  { "/entities",                                                           jsonNullTreat           },
  { "/entities/entity",                                                    entityId                },
  { "/entities/entity/id",                                                 entityIdId              },
  { "/entities/entity/type",                                               entityIdType            },
  { "/entities/entity/isPattern",                                          entityIdIsPattern       },
  ...
```

As explained, this is a vector of **path-in-the-tree** and corresponding **treat-function** and this is how the low-level `jsonParse()` knows which treat-function to call for each node in the tree.

This vector and the other vectors (one per type of payload) is used by the variable `jsonRequest` in `src/lib/jsonParse/jsonRequest.cpp` of type `JsonRequest`:
```
typedef struct JsonRequest
{
  RequestType      type;
  std::string      method;
  std::string      keyword;
  JsonNode*        parseVector;
  RequestInit      init;
  RequestCheck     check;
  RequestPresent   present;
  RequestRelease   release;
} JsonRequest;
```

`jsonRequest` is a vector of `JsonRequest` and it defines all payloads that are to be parsed by `jsonParse`, see [Implementation Details](#implementation-details) for more info on `jsonRequest`.  

The JsonRequest vector is declared in `src/lib/jsonParse/jsonRequest.cpp`:

```
static JsonRequest jsonRequest[] =
{
  // NGSI9
  { RegisterContext,                       "POST", "registerContextRequest",                        FUNCS(Rcr)   },
  { DiscoverContextAvailability,           "POST", "discoverContextAvailabilityRequest",            FUNCS(Dcar)  },
  { SubscribeContextAvailability,          "POST", "subscribeContextAvailabilityRequest",           FUNCS(Scar)  },
  { UnsubscribeContextAvailability,        "POST", "unsubscribeContextAvailabilityRequest",         FUNCS(Ucar)  },
  { NotifyContextAvailability,             "POST", "notifyContextRequestAvailability",              FUNCS(Ncar)  },
  { UpdateContextAvailabilitySubscription, "POST", "updateContextAvailabilitySubscriptionRequest",  FUNCS(Ucas)  },

  // NGSI10
  { QueryContext,                          "POST", "queryContextRequest",                           FUNCS(Qcr)   },
  { UpdateContext,                         "POST", "updateContextRequest",                          FUNCS(Upcr)  },
  ...
};
```

The macro `FUNCS()` is to make the lines a bit shorter and it looks like this:

```
#define FUNCS(prefix) json##prefix##ParseVector, json##prefix##Init,    \
                      json##prefix##Check,       json##prefix##Present, \
                      json##prefix##Release
```

I.e., all the "methods" for the parsing of a type of payload. These functions reside in one module per type of payload, such as:

* `src/lib/jsonParse/jsonQueryContextRequest.cpp`
* `src/lib/jsonParse/jsonUpdateContextRequest.cpp`
* etc

The "methods" are for:

* `init()`,
* `check()`,
* `present()`, and
* `release()`

and there is one set of "methods" for each type of payload.  

So, the modules called `jsonXxxRequest` or `jsonXxxResponse` (`Xxx` being the name of the payload, e.g. `QueryContent` or `UpdateContent`) all contain:

* this set of methods (`init()`, `check()`, `present()` and `release()`),
* the *Parse-Vector* that contains the path and treat methods for each node,
* all treat methods for the nodes

which is the entire set of functions and variables that `jsonParse()` needs to convert the JSON input payload string into a C++ class instance.

Now some example of treat-methods, they are all pretty simple:

```
/* ****************************************************************************
*
* entityId -
*/
static std::string entityId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.entityIdP = new EntityId();

  reqDataP->qcr.entityIdP->id        = "";
  reqDataP->qcr.entityIdP->type      = "";
  reqDataP->qcr.entityIdP->isPattern = "false";

  reqDataP->qcr.res.entityIdVector.push_back(reqDataP->qcr.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId -
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.entityIdP->id = value;

  return "OK";
}
```

Also, lets have a look at the Parse-Vector where these two treat-functions reside:

```
/* ****************************************************************************
*
* qcrParseVector -
*/
JsonNode jsonQcrParseVector[] =
{
  { "/entities",                                                           jsonNullTreat           },
  { "/entities/entity",                                                    entityId                },
  { "/entities/entity/id",                                                 entityIdId              },
  ...
```

The treat-function `entityId` is called when a node `/entities/entity` is found. `entities` is a vector inside the payload for `QueryContextRequest`:
```
{
  "entities": [
    {
      "id": "",
      "type": "",
      ...
    }
}
```

As vectors items have no key-name in JSON, we decided to use the singular word of the name of the vector (which is always in plural), i.e. an instance of **entities** is called **entity**. So, when the node `/entities/entity` is found (an item of the entities vector), the treat-function `entityId()` is called and it allocates room for an `EntityId` (`class EntityId` resides in the module `src/lib/ngsi/EntityId.h/cpp`) and pushes the `EntityId` pointer to the vector `reqDataP->qcr.res.entityIdVector`.

The treat-function `entityId()` also sets `reqDataP->qcr.entityIdP` to reference this latest instance of `EntityId` so that consequent treat-functions can reach it. For example, `entityIdId()` needs it, to set the `id` field of the entity, which is all `entityIdId()` does. Pointers of this type are needed during the parse/extraction and these pointers are the reason for the ParseData structs (see `src/lib/ngsi/ParseData.h` - one "XxxData" struct per payload type, with an output class instance and these help pointers).

### `ParseData* parseDataP`

As the parsing of NGSI v1 payload is strongly centralized, and function pointers are needed, there is a need for a unique type for **all** types of payload. The types for storing the result of the parse (the C++ class instances are different for each type of payload) are all collected into a big struct contaning all types of payload. Then each treat-function picks which field to operate on.

A pointer to this structure is passed as parameter to low-level `jsonParse()`.

`ParseData` is found in `src/lib/ngsi/ParseData.h`:

```
typedef struct ParseData
{
  std::string                                 errorString;
  ContextAttribute*                           lastContextAttribute;

  RegisterContextData                         rcr;
  DiscoverContextAvailabilityData             dcar;
  SubscribeContextAvailabilityData            scar;
  UnsubscribeContextAvailabilityData          ucar;
  UpdateContextAvailabilitySubscriptionData   ucas;
  ...
} ParseData;
```

For example, parsing of an NGSI10 query operates on `ParseData::qcr` which is of the type `QueryContextData`. `QueryContextData` in its turn contains an instance of `QueryContextRequest` which is where the result of the parse is stored. But, as help variables are needed during the parse, these "XxxData structs" are used and they contain the output instance (`QueryContextRequest` in the case of `QueryContextData`) **and** the help variables needed for the parsing of a `QueryContextRequest`:

```
struct QueryContextData
{
  QueryContextRequest  res;           // Output/Result of the parse
  EntityId*            entityIdP;     // Pointer to the current EntityId
  Scope*               scopeP;        // Pointer to the	current Scope
  orion::Point*        vertexP;       // Pointer to the current Point
  int                  pointNo;       // Index of the current Point
  int                  coords;        // Number of coordinates
};
```

Each XxxData struct has a different set of help variables.  

[Top](#top)
