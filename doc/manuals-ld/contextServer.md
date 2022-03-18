# The Broker as a Context Server
Starting from the Beta 3 release (v0.8), Orion-LD is able to serve as a context server.
Note that this feature has been "discussed and agreed upon in principle", by ETSI ISG CIM, that defines the NGSI-LD API.
No details have been sorted out yet and it will probably not enter the official NGSI-LD API specification until the end of 2021.
Meaning, things may change. The current Context Server implementation is *experimental*.

The Broker as a Context Server means that users can push contexts to the broker and will no longer need to host their contexts elsewhere.
One caveat: the contexts aren't persisted - they only live in RAM, meaning, if the broker is restarted the contexts are lost and must be pushed again.
The idea is to fix this problem for the next release (v0.9?).

The supported services are:

* POST /ngsi-ld/v1/jsonldContexts - to create a new context
* GET /ngsi-ld/v1/jsonldContexts - to retrieve the list of all the served contexts
* GET /ngsi-ld/v1/jsonldContexts?details=true - to retrieve the list of all the served contexts with additional information for each context
* GET /ngsi-ld/v1/jsonldContexts/{contextId} - to retrieve an individual context
* DELETE /ngsi-ld/v1/jsonldContexts/{contextId} - to delete a user created context

Note that not only user created contexts are in the list from the GET operations. Also the core context, and contexts that the broker has downloaded on demand from other requests.

## Creating a context
The "value" of the context comes in the payload body, either as a JSON object or as a JSON array:
```
POST /ngsi-ld/v1/jsonldContexts
{
  "key1": "value1",
  "key2": "value2",
  ...
  "keyN": "valueN"
}
```
OR
```
POST /ngsi-ld/v1/jsonldContexts
[
  "URL-1", 
  "URL-2", 
  ...
  "URL-N"
}
```
It is also allowed to have the "complete information" inside the payload body, not just the "value" of the @context:
```
POST /ngsi-ld/v1/jsonldContexts
{
  "@context": {
    "key1": "value1",
    "key2": "value2",
    ...
    "keyN": "valueN"
  }
}
```
OR
```
POST /ngsi-ld/v1/jsonldContexts
{
  "@context": [
    "URL-1", 
    "URL-2", 
    ...
    "URL-N"
  ]
}
```

The 'id' of the created context is returned in the Location header, e.g.:
```
HTTP/1.1 201 Created
Content-Length: 0
Location: http://host:port/ngsi-ld/v1/jsonldContexts/XXXXX
Date: REGEX(.*)
```
XXXXX would be the 'id' of the context, while the value of the Location header is the URL to be used to use (include in other requests), retrieve (GET) the context or to DELETE it from the broker.

## Retrieve the list of all the served contexts
To obtain a list of all the contexts in the brokers context cache, use the following request:
```
GET /ngsi-ld/v1/jsonldContexts
```
The payload body of the response is an array of URLs - one URL per context in the brokers context cache.
E.g.:
```
HTTP/1.1 200 OK
Content-Length: xxx
Content-Type: application/json
Date: REGEX(.*)

[
    "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
    "https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testContext.jsonld",
    "https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testFullContext.jsonld",
    "http://BROKER_IP:BROKER_PORT/ngsi-ld/v1/jsonldContexts/xxx-yyy-zzz..."
]
```
In this example, the first item in the context URL array is the URL of the core context.
As the core context is downloaded when the broker first starts up, it will always be the first in the list.
The second and the third URLs are contexts that the broker downloaded on demand, as the contexts were used as part of a request,
while the fourth URL is a user created context - you can see that as the IP and port are the brokers IP and port,
and the URL PATH starts with '/ngsi-ld/v1/jsonldContexts/'.


## Retrieve the list of all the served contexts with additional information for each context
The URI parameter `detail=true` is supported for `GET /ngsi-ld/v1/jsonldContexts` and this adds quite some detail to the response.
E.g.:
```
HTTP/1.1 200 OK
Content-Length: REGEX(.*)
Content-Type: application/json
Date: REGEX(.*)

[
    {
        "hash-table": {
            "attributes": "https://uri.etsi.org/ngsi-ld/attributes",
            "instanceId": "https://uri.etsi.org/ngsi-ld/instanceId",
            "notifiedAt": "https://uri.etsi.org/ngsi-ld/notifiedAt",
            "observedAt": "https://uri.etsi.org/ngsi-ld/observedAt",
            "properties": "https://uri.etsi.org/ngsi-ld/properties"
        },
        "id": "Core",
        "origin": "Downloaded",
        "type": "hash-table",
        "url": "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld"
    },
    {
        "hash-table": {
            "A1": "urn:ngsi-ld:attributes:A1",
            "A2": "urn:ngsi-ld:attributes:A2",
            "A3": "urn:ngsi-ld:attributes:A3"
        },
        "id": "xxx-yyy-zzz-...",
        "origin": "UserCreated",
        "type": "hash-table",
        "url": "http://BROKER_IP:BROKER_PORT/ngsi-ld/v1/jsonldContexts/xxx-yyy-zzz..."
    },
    ...
]
```
The body of the response reveals some of the intrinsics ofOrion-LD's context cache.
This request will NOT be standardized by ETSI as it is very much up to the broker implementation on how to (even IF) implement the
context caching.
In the case of Orion-LD, every context is modified into a hash table for quicker lookups.
The body also tells us the origin of the context, whether it's been downloaded on demand, of it was supplied by a user, etc.


## Retrieve an individual context
As mentioned above, when a context is created (using POST /ngsi-ld/v1/jsonldContexts), the ID of the context is returned in the Location header.
Actually, the entire URL to the context is returned in the Location header.
This value can then be used for the Link header of subsequent requests, or be part of an array with URLs for a composite context or ...
It can also be used to retrieve the context, which is what the broker will have to do whenever this context is used.
Well, the broker notices that the broker itself is the server of the context and instead of issuing a GET request,
the context is just looked up in the brokers context cache).
```
GET /ngsi-ld/v1/jsonldContexts/{contextId}
```
This request retrieves the entire context (with id {contextId}), with the "@context" key and the context itself as its value, e.g.:
```
GET /ngsi-ld/v1/jsonldContexts/{contextId}

{
  "@context": {
    "A1": "urn:ngsi-ld:attributes:A1",
    "A2": "urn:ngsi-ld:attributes:A2",
    "A3": "urn:ngsi-ld:attributes:A3"
  }
}
```

## DELETE a user created context
Lastly, if a user needs modifications for its "hosted" context, the broker also supports for deletion of user created contexts:
```
DELETE /ngsi-ld/v1/jsonldContexts/{contextId}
```
So, DELETE and then POST, to update a hosted context.
Or, just DELETE if you no longer need the context to be hosted.
