# Developer Guide of Orion-LD

Welcome to the Developer Guide of Orion-LD, the NGSI-LD context broker!

The Orion-LD context broker doesn't have a GUI and the only way to contact the broker is programmatically,
be it a shell-script, a javascript program, a python program or whatever.
Well, apart from using *Postman* and similar tools, of course.

This guide will *not* bother about what program language is used, as that is completely up to the programmer.
The complete requests to be sent to Orion-LD will of course be presented, but in a neutral format, like this:
```text
Content-Type: XXX
Accept: XXX
Link: XXX
GET /ngsi-ld/v1/xxx?p1&P2
```
I.e. first the HTTP headers (in this case: Content-Type, Accept, and Link), and then the Verb followed by the URL, including any URI parameters.

It is then up to the reader to "translate" this to a correct call in their respective programming language.

Strongly recommended reading before continuing with this document:
* [NGSI-LD API v1.1.1](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf)
* [Guide to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)
* [Guide to the context](doc/manuals-ld/the-context.md)
* [Quick Start Guide](doc/manuals-ld/quick-start-guide.md)

Also, to be able to play with your own Orion-LD broker while reading this document definitely helps to understand everything better.
If you don't have a running Orion-LD to play with, please follow the instructions in the [Orion-LD Installation Guide](doc/manuals-ld/installation-guide.md).

This document will go into more detail on pretty much everything about the Orion-LD context broker:
* ProblemDetails to describe errors
* Contexts
* Creation of Entities
* Modification of Entities and Attributes
* Deletion of Entities and Attributes
* Forwarding of Creation/Modification of Entities/Attributes
* System Attributes
* Pagination
* Querying for Entities
* Retrieval of a Specific Entity
* Forwarding of Query Requests
* Creation of Subscriptions
* Modification of a Subscription
* Querying for Subscriptions
* Retrieval of a Specific Subscription
* Deletion of Subscription
* Notifications
* Creation of Registrations
* Modification of Registrations
* Querying for Registrations
* Retrieval of a Specific Registration
* Deletion of Registrations
* Subscription to Registrations
* Temporal Representation
* Geolocation
* Query Filter

## ProblemDetails to describe errors
**ProblemDetails** is a standard way of specifying errors in HTTP API responses and the NGSI-LD specifies this to be used in a variety of responses.
[Here](https://lurumad.github.io/problem-details-an-standard-way-for-specifying-errors-in-http-api-responses-asp.net-core) is a tutorial on _ProblemDetails_
and [here](https://tools.ietf.org/html/rfc7807) is the RFC.

Orion-LD uses part of _ProblemDetails_, namely:
* type
* title
* detail
* status (sometimes)

## Contexts
Contexts is a way to use shortnames instead of longnames in the payload and in URI parameters, for attribute names and entity types.
It is also a way for the user to use its own aliases (shortnames) for attribute names and entity types.

The context is thoroughly explained in the [guide to the Context](doc/manuals-ld/the-context.md).

### In the Request
If a context is desired for a service, it can be put either in the payload, as part of the entity/subscription/registration, like a "special attribute",
or in the HTTP header "Link".
Especially the services that have no request payload data, like GET services will have to use the "Link" HTTP header.

If in the Link header, its syntax is a bit complex:
```
Link: <URL-to-context>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
```
You need to replace `URL-to-context` with the URL of your context.
There can only be a single context when passing it via HTTP header.
Just a string is allowed.

If instead the context is put in the payload, the context can be of three different JSON types:
* String (the value is a URL to the context)
* Array (each item in the array is either a string or an object - contexts on their own)
* Object (a key-value list - an "inline" context!)

### In the Response
The `Accept` HTTP header tells the broker where to put the context in the response.
If `Accept: application/json` then the context is put in the Link HTTP header of the response,
if `Accept: application/ld+json`, then the context is put in the payload data.

As the context in the response will *always* be the very same context of the request, be careful with services
that return arrays of entities/subscriptions/registrations. If `application/ld+json` is used for such a service, then the
context will be copied in each and every item (entity/subscription/registration) is the response payload data.
Just sayin'.

## Creation of Entities
Entities can be created in three different ways:
* `POST /ngsi-ld/v1/entities` (to create a single entity)
* `POST /ngsi-ld/v1/entityOperations/create` (to create more than one entity in one go)
* `POST /ngsi-ld/v1/entityOperations/upsert` (to both create and modify entities with one single request)

### POST /ngsi-ld/v1/entities
The service `POST /ngsi-ld/v1/entities` is used for creation of a single entity.

#### Request Payload Data
The payload data of the request `POST /ngsi-ld/v1/entities` looks like this:
```json
{
  "id": "entity-id",            # Mandatory - must be a URI
  "type": "entity-type",        # Mandatory - will be expanded if not a URI
  "location": {},               # Optional
  "observationSpace": {},       # Optional
  "operationSpace": {},         # Optional
  "property1": {},              # Optional
  "property2": {},              # Optional
  ...
  "propertyN": {},              # Optional
  "relationship1" {},           # Optional
  "relationship2" {},           # Optional
  ...
  "relationshipN" {},           # Optional
  "@context": "" | [] | {}      # Optional
}
```

The syntax for the attributes is described in the [introduction to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)

#### Request URI Parameters
As all the information of the entity to create resides in the payload data, there are no URI parameters for this request.

#### Response HTTP Status Code
* 201 Created - if everything works, and no payload data is present
* 400 Bad Request - for a payload data with an invalid JSON syntax or invalid JSON types for the fields in the payload data
* 409 Conflict - the entity already exists
* 422 Unprocessable Entity - operation not available.

#### Response HTTP Headers
* Location - to inform the creator of where the created entity resides.
* Link - to echo back to the creator the context that was used during modification of the entities.

About `Link`, the issuer of the request already knows the context that was used, as he/she provided it!
However, a gateway between the issuer and the broker may need to have this information as well, that's why the context is echoed back.

Below, a typical response to a `POST /ngsi-ld/v1/entities` request:
```
HTTP/1.1 201 Created
Content-Length: 0
Link: <https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testContext.jsonld>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
Location: /ngsi-ld/v1/entities/http://a.b.c/entity/E6
Date: xxx
```
As you can see, this response was for a successful request to create an entity with the id `http://a.b.c/entity/E6`, and the context used
when creating the entity was `https://fiware.github.io/NGSI-LD_TestSuite/ldContext/testContext.jsonld`.

#### Response Payload Data
In case of success, there is no payload data in the response.
In case of an error, a _ProblemDetails_ in the payload data describes the error. 

An example of a response payload data for an invalid JSON syntax in the request payload data:
```text
HTTP/1.1 400 Bad Request
Content-Length: 150
Content-Type: application/json
Date: REGEX(.*)

{
    "detail": "JSON Parse Error: expecting comma or end of object",
    "title": "JSON Parse Error",
    "type": "https://uri.etsi.org/ngsi-ld/errors/InvalidRequest"
}
```

#### Pointers to the ETSI NGSI-LD documentation
The latest version (1.2.1) of the NGSI-LD API definition (as of December 2019) is found [here](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.02.01_60/gs_CIM009v010201p.pdf).
Some chapters of interest in said document for `POST /ngsi-ld/v1/entities` are:
* 5.2.4 - The NGSI-LD Entity Data Type
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.6.1 - Create Entity
* 6.4.3.1 - POST /ngsi-ld/v1/entities.


### POST /ngsi-ld/v1/entityOperations/create
The service `POST /ngsi-ld/v1/entityOperations/create` is used to create more than one entity in a single request.
If any of the entities in the incoming payload data already exist, the broker will flag that entity with an error in the outgoing payload data.

#### Request Payload Data
The payload data of the request `POST /ngsi-ld/v1/entityOperations/create` is an JSON Array of entities:
```json
[
  {
    <Entity 0>
  },
  {
    <Entity 1>
  },
  ...
  {
    <Entity N>
  }
]
```
The syntax of an entity is fully described in the previous chapter (about `POST /ngsi-ld/v1/entities`).

#### Request URI Parameters
There are no URI parameters for this request. All necessary information resides in the request payload data.

#### Response HTTP Status Code
* 201 Created - all entities were successfully created. No response payload data supplied.
* 207 Multi Status - some entities were successfully created, others weren't. Details of each error in the response payload data.
* 400 Bad Request - none of the entities in the request payload data were created. Details of each error in the response payload data.

#### Response HTTP Headers
* Link - to echo back to the creator the context that was used during modification of the entities.

#### Response Payload Data
In case of All OK, a `204 No Content` is returned and no payload data.
In case of `400 Bad Request` (a JSON parse error, or similar errors), the response payload data is the typical { type, title and detail }, e.g.:
```json
{
    "detail": "JSON Parse Error: expecting comma or end of object",
    "title": "JSON Parse Error",
    "type": "https://uri.etsi.org/ngsi-ld/errors/InvalidRequest"
}
```

If there is a mix of results (some entities are OK others are not) then the response payload contains two arrays:
* success - a string array containing the Entity ID of the entities that were successfully created.
* errors - an error of objects describing the error details for each entity that weren't created.

Example response payload data for a request to create two entities, urn:ngsi-ld:Vehicle:302 (which worked), and ABC_123456 (which did not work):
```json
{
    "errors": [
        {
            "entityId": "ABC_123456",
            "error": {
                "detail": "ABC_123456",
                "status": 400,
                "title": "Not a URI",
                "type": "https://uri.etsi.org/ngsi-ld/errors/BadRequestData"
            }
        }
    ],
    "success": [
        "urn:ngsi-ld:Vehicle:302"
    ]
}
```
The fields `"errors"` and `"success"` are *always* present, even if empty arrays.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.2.16 - BatchOperationResult
* 5.2.17 - BatchEntityError
* 5.6.7 - Batch Entity Creation
* 6.14 - POST /ngsi-ld/v1/entityOperations/create

### POST /ngsi-ld/v1/entityOperations/upsert
The service `POST /ngsi-ld/v1/entityOperations/upsert` is used to both create and update entities, in a single request.
Those entities that already exist are updated. Those that don't exist are created.
In case of error, the details of the errors are specified per entity in the outgoing payload data.

#### Request Payload Data
Exactly like `POST /ngsi-ld/v1/entityOperations/create`.

#### Request URI Parameters
* options=update - existing entity content shall be updated and not replaced.
The default mode is to *replace* the entire already existing entity with the corresponding entity in the payload data.

#### Response HTTP Status Code
* 201 Created - all entities were successfully created/updated. No response payload data supplied.
* 207 Multi Status - some entities were successfully created/updated, others weren't. Details of each error in the response payload data.
* 400 Bad Request - none of the entities in the request payload data were created/updated. Details of each error in the response payload data.

#### Response HTTP Headers
* Link - to echo back to the creator the context that was used during modification of the entities.

#### Response Payload Data
See the corresponding section for `POST /ngsi-ld/v1/entityOperations/create`.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.2.16 - BatchOperationResult
* 5.2.17 - BatchEntityError
* 5.6.8 - Batch Entity Creation or Update (Upsert)
* 6.15 - POST /ngsi-ld/v1/entityOperations/upsert.


## Modification of Entities and Attributes
There are a number of services for modification of entities and attributes:
* POST /ngsi-ld/v1/entities/{entityId}/attrs
* PATCH /ngsi-ld/v1/entities/{entityId}/attrs
* PATCH /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}
* POST /ngsi-ld/v1/entityOperations/upsert
* POST /ngsi-ld/v1/entityOperations/update.

### Contexts
See 'Creation of Entities'.

### POST /ngsi-ld/v1/entities/{entityId}/attrs
The service `POST /ngsi-ld/v1/entities/{entityId}/attrs` lets you append attributes to an entity.
A URI parameter (`?options=noOverwrite`) tells the broker to *not overwrite* any already existing attributes
and instead report those already existing attributes as erroneous in the response payload data.

#### Request Payload Data
The payload data for this service is a JSON object with attributes.
Remove the "id" and "type" from the payload of a complete entity and there you have it:
```json
{
  "location": {},               # Optional
  "observationSpace": {},       # Optional
  "operationSpace": {},         # Optional
  "property1": {},              # Optional
  "property2": {},              # Optional
  ...
  "propertyN": {},              # Optional
  "relationship1" {},           # Optional
  "relationship2" {},           # Optional
  ...
  "relationshipN" {},           # Optional
  "@context": "" | [] | {}      # Optional
}
```
The syntax for attributes is described in the [introduction to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md).

#### Request URI Parameters
* options=noOverwrite - to ask the broker to *not overwrite* any already existing attribute

#### Response HTTP Status Code
* 204 No Content - if all went well
* 207 Multi-Status - if partial success
* 400 Bad Request - in case of JSON parse error or similar (invalid entity id in the URL path)
* 404 Not Found - if the entity id of the URL path doesn't get a hit for an entity in the database.

#### Response HTTP Headers
No HTTP headers relevant to NGSI-LD are present in the response.

#### Response Payload Data
If all went well, no payload data is returned, just the `204 No Content`.
On partially successful operation, a `207 Multi-Status` is returned and a payload data that contains two arrays (similar to the response payload data of the Batch operations):
```json
{
  "updated": [ "attr-name-1", "attr-name-2", ... "attr-name-N ],
  "unchanged": [
    {
      "attributeName": "attr-name-3",
      "reason": "error reason for not having changed the attribute"
    },
    {
      "attributeName": "attr-name-4",
      "reason": "error reason for not having changed the attribute"
    }
  ]
}
```

For 400 and 404 status codes, the typical { type, title, detail } error object (ProblemDetails) is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.6.3 - Append Entity Attributes
* 6.6.3.1 - POST /ngsi-ld/v1/entities/{entityId}/attrs


### PATCH /ngsi-ld/v1/entities/{entityId}/attrs
The service `PATCH /ngsi-ld/v1/entities/{entityId}/attrs` is used to modify more than one attribute in a single shot.
Only already existing attributes are modified. The modification is done by replacing the "old" attribute with the attribute from the incoming payload.
Non-existing attributes, i.e. attributes present in the payload but not in the entity to be patched, are ignored.

#### Request Payload Data
Just like POST for the same resource, the payload data is a JSON object with attributes:
```json
{
  "location": {},               # Optional
  "observationSpace": {},       # Optional
  "operationSpace": {},         # Optional
  "property1": {},              # Optional
  "property2": {},              # Optional
  ...
  "propertyN": {},              # Optional
  "relationship1" {},           # Optional
  "relationship2" {},           # Optional
  ...
  "relationshipN" {},           # Optional
  "@context": "" | [] | {}      # Optional
}
```
The syntax for attributes is described in the [introduction to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md).

#### Request URI Parameters
There are no URI parameters for this request. All necessary information resides in the request payload data.

#### Response HTTP Status Code
Just like `POST` for the same resource.

#### Response HTTP Headers
No HTTP headers relevant to NGSI-LD are present in the response.

#### Response Payload Data
Just like `POST` for the same resource.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.6.2 - Update Entity Attributes
* 6.6.3.2 - PATCH /ngsi-ld/v1/entities/{entityId}/attrs.


### PATCH /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}
This operation allows performing a partial update on an attribute of an entity.
A partial update only changes the fields provided in the payload data, leaving the rest of the attribute unaffected. 

#### Request Payload Data
The payload data for this service is a fragment of an attribute.
For example, to change the value to 45, and add a sub-property P11, the payload could look something like this:
```json
{
  "value": 45,
  "P11": {
    "type": "Property",
    "value": "p"
  }
}
```

#### Request URI Parameters
There are no URI parameters for this request. All necessary information resides in the request payload data.

#### Response HTTP Status Code
* 204 No Content - if all is good and well
* 400 Bad Request - if the request or its content is somehow incorrect
* 404 Not Found - if the entity or the attribute specified in the URL does not exist.

#### Response HTTP Headers
No HTTP headers relevant to NGSI-LD are present in the response.

#### Response Payload Data
If all went well, no payload data is returned, just the `204 No Content`.
For 400 and 404 status codes, the typical { type, title, detail } (ProblemDetails) error object is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.6.4 - Partial Attribute update 
* 6.7.3.1 - PATCH.


### POST /ngsi-ld/v1/entityOperations/upsert
Already documented under "Creation of Entities".

### POST /ngsi-ld/v1/entityOperations/update
Not implemented in Alpha Release 1.

## Deletion of Entities and Attributes
There are three services for deletion of entities and attributes:
* POST /ngsi-ld/v1/entityOperations/delete
* DELETE /ngsi-ld/v1/entities/{entityId}
* DELETE /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}.

The first service allows to delete a number of entities in one go, while the second service deletes one single entity
and the last service is for deletion of a single attribute of a given entity.

### POST /ngsi-ld/v1/entityOperations/delete
The "Batch Entity Delete" service allows for deletion of a number of entities in a single request.

#### Request Payload Data
The payload data of this service is a JSON Array of strings, each string being an Entity-ID:
```json
[
  "entity-id 1",
  "entity-id 2",
  ...
  "entity-id N"
]
```

#### Request URI Parameters
This service has no URI parameters.

#### Response HTTP Status Code
* 204 No Content - if all is good and well
* 207 Multi-Status - if partial success
* 400 Bad Request - if the request or its content is somehow incorrect, e.g. JSON parse error

#### Response HTTP Headers
No HTTP headers relevant to NGSI-LD are present in the response.

#### Response Payload Data
See the corresponding section for `POST /ngsi-ld/v1/entityOperations/create`

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.16 - BatchOperationResult
* 5.2.17 - BatchEntityError
* 5.6.10 - Batch Entity Delete
* 6.17.3.1 - POST /ngsi-ld/v1/entityOperations/delete.

### DELETE /ngsi-ld/v1/entities/{entityId}
To delete a single entity, without any payload data, the service `DELETE /ngsi-ld/v1/entities/{entityId}`.
Not much to say, if the entity with id `entityId` is found, it is deleted.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
This service has no URI Parameters.

#### Response HTTP Status Code
* 200 OK - if all OK
* 400 Bad Request - if the entity id of the URL PATH is not a valid URI
* 404 Not Found - if thje entity does not exist.

#### Response HTTP Headers
No relevant HTTP headers are present in the response.

#### Response Payload Data
For 400 and 404 status codes, the typical { type, title, detail } error object (ProblemDetails) is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.6.6 - Delete Entity
* 6.5.3.2 - DELETE /ngsi-ld/v1/entities/{entityId}.


### DELETE /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}
This service allows a user to delete a single attribute of a given entity.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
This service has no URI Parameters.

#### Response HTTP Status Code
* 204 No Content - if all is OK
* 400 Bad Request - in case of invalid entity id in the URL path
* 404 Not Found - if the entity id of the URL path does not get a hit for an entity in the database.

#### Response HTTP Headers
No relevant HTTP headers are present in the response.

#### Response Payload Data
For 400 and 404 status codes, the typical { type, title, detail } error object (ProblemDetails) is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.6.5 - Delete Entity Attribute
* 6.7.3.2 - DELETE /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}.


## Forwarding of Creation/Modification of Entities/Attributes
The _Forwarding Concept_ for NGSI-LD has still to be specified. We have started to look at this a little (we == the ETSI CIM group defining the NGSI-LD API),
but we still have a long way to go before having anything decided and documented.

As Orion-LD builds on Orion and reuses (with modification for expansion and compaction of items) whatever Orion has implemented, Orion-LD actually supports
forwarding already, but, this will have to change to the NGSI-LD way, once that way is defined.

Forwarding is a mechanism to include context providers as "part of the broker". An example always makes it easier to explain/understand:
1. Context Provider CP-1 informs the broker that it knows about the entity XXX, with attributes A1, A2, ...
   This is done by sending a Registration request to the broker.
   The registration request contains information about the entities and attributes, and, importantly, the IP, port and URL-PATH to use to contact the context provider.
2. A query for entity E13, attributes A1-A12 enters the broker.
   The broker looks at its current registrations, and sees that the registration for CP-1 matches what the query is asking for, so:
3. The broker forwards the initial request to CP-1 and waits for CP-1 to answer.
4. Once the answer from CP-1 has arrived, the broker merges the response from CP-1 with what the broker found in its local database and:
5. The broker responds to the initial request with the merged results from the broker itself and the response from CP-1.

That was for retrieval of entities.
Updating of entities works pretty much the same way.

Now, for forwarding NGSI-LD requests in Orion-LD. please see the proper documentation of Orion and imagine the attribute names and entity types to be expanded/compacted according to the context.
The context used for forwarding must of course be the context specified in the registration.
This hasn't even been tested, but still, it should "work a little" ...

## System Attributes
Each entity and attribute have two timestamps, namely "createdAt", and "modifiedAt".
The broker makes sure these two "built-in special attributes" are created and updated accordingly during the lifetime of the entity/attribute.
The values *cannot* be altered from outside the broker, i.e., there is no service that let's a user set these timestamps to aleatory values.
But of course, when modifying an entity, its "modifiedAt" is updated, same with attributes.

A user can ask the broker to include these special attributes in queries though, by specifying the value "sysAttrs" to the URI parameter "options":
```bash
GET /ngsi-ld/v1/entities?options=sysAttrs&type=T
```

## Pagination
To avoid returning thousands of items (entities, subscriptions, registrations), Orion-LD establishes a maximum number of items to return.
If there are more items to be returned, then the client will have to query again, and again until the client has retrieved all of the items.
This concept is called _Pagination_ and it's a mechanism for Orion-LD to protect itself against flooding.

The services that use pagination are:
* GET /ngsi-ld/v1/entities
* GET /ngsi-ld/v1/subscriptions
* GET /ngsi-ld/v1/csourceRegistrations
* GET /ngsi-ld/v1/csourceSubscriptions  # Not implemented in Alpha Release 1
* GET /ngsi-ld/v1/temporal/entities     # Not implemented in Alpha Release 1

The number of items and the index of the first item for pagination are defined by two URI parameters:
* limit=X   # X is the number of items
* offset=Y  # Y is the offset of the first item

The default values for these two are:
* limit:  20
* offset: 0

Orion-LD implements a maximum limit of 1000. If a request tries to set `limit` above 1000, an error is returned.
In Alpha Release 1, Orion-LD reuses the pagination of Orion. 
Please see the documentation of pagination of [Orion](https://github.com/telefonicaid/fiware-orion) for more info.

## Querying for Entities

### GET /ngsi-ld/v1/entities
The `GET /ngsi-ld/v1/entities` service returns an array of entities matching the characteristics specified as URI parameters.
Pagination is used to limit the number of entities returned.
Also, one of the following URI paramaters *must* be present, to narrow down the number of matching entities:
* type
* attrs
* q.
This is what the ETSI NGSI-LD specification version 1.2.1 says, at least.
Hopefully, this restriction will be removed in version 1.3.1.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
* id          - list of entity ids to be retrieved (comma-separated list of URIs)
* type        - list of entity types to be retrieved (comma-separated list of types)
* idPattern   - regular expression to be matched by entity ids
* attrs       - list of attributes (only those attributes are returned and only entities with any of the attrs match)
* q           - query string - see separate chapter on 'q'
* csf         - not implemented in Alpha Release 1
* georel      - geo relationship (near, within, etc.)
* geometry    - geometry (point, circle, polygon, ...)
* coordinates - coordinates array, serialized as a string
* geoproperty - not implemented in Alpha Release 1 - only "location" can be used as geo attribute
* limit       - maximum number of entities to be returned
* offset      - the index of the first entity

For all geofencing URI parameters (last four), please refer to the separate chapter on Geolocation.

#### Response HTTP Status Code
* 200 OK - the payload body is a JSON array that contains the matching entities
* 400 Bad Request - something is off with the request and the payload body contains information about the error.

#### Response HTTP Headers
* Link - to echo back to the context (if `Accept: application/json`)

#### Response Payload Data
The response payload data is a JSON array of the matching entities:
```json
[
  { Entity 1 },
  { Entity 2 },
  ...
  { Entity N }
]
```

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.4 - The NGSI-LD Entity Data Type
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.7.2 - Query Entities 
* 6.6.4.3.2 - GET /ngsi-ld/v1/entities

## Retrieval of a Specific Entity
To retrieve a specific entity, one needs to know its _Entity ID_.

### GET /ngsi-ld/v1/entities/{entityId}
The `GET /ngsi-ld/v1/entities/{entityId}` service returns the entity with ID `entityId`, possibly filtered by the URI parameter `attrs`, to only return a specific set of attributes.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
* attrs - list of attributes (only those attributes are returned)

#### Response HTTP Status Code
* 200 OK - the payload body is a JSON object containing the requested information of the entity
* 400 Bad Request - if the entity ID of the URL PATH is not a valid URI
* 404 Not Found - if the entity ID of the URL PATH does not specify an existing entity

#### Response HTTP Headers
* Link - to echo back to the context (if Accept: application/json)

#### Response Payload Data
The response payload data is a JSON object describing the entity in question:
```json
{
  <Entity>
}
```

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.4 - The NGSI-LD Entity Data Type
* 5.2.5 - The NGSI-LD Property Data Type
* 5.2.6 - The NGSI-LD Relationship Data Type
* 5.2.7 - The NGSI-LD GeoProperty Data Type
* 5.7.1 - Retrieve Entity
* 6.5.3.1 - GET /ngsi-ld/v1/entities/{entityId}.

## Forwarding of Query Requests
NGSI-LD forwarding is still to be specified.
Please see chapter `Forwarding of Creation/Modification of Entities/Attributes` for more information.

## Subscriptions
Subscriptions are used to obtain notifications whenever entities/attributes are updated/created, instead of using polling.
It's much more efficient to subscribe than to continuously query and investigate the response. Like interrupts vs polling.
Subscriptions are like interrupts. Whenever some criteria (defined by the subscription) is fulfilled, a notification is launched (much like an interrupt).

## Creation of Subscriptions
### POST /ngsi-ld/v1/subscriptions
Subscriptions are created using the service `POST /ngsi-ld/v1/subscriptions`.

#### Request Payload Data
A subscription with ALL the field should look like this:
```json
{
  "id": "URI",  # if not given,  it will be assigned during subscription process and returned to client
  "type": "Subscription",
  "name": "Name of the subscription",
  "description": "Description of the subscription",
  "entities": [
    {
      "id": "entity id",     # Optional, takes precedence over idPattern
      "idPattern": "REGEX",  # Optional
      "type": "entity type"  # Mandatory
    }
    {
      "id": "entity id",     # Optional, takes precedence over idPattern
      "idPattern": "REGEX",  # Optional
      "type": "entity type"  # Mandatory
    },
    ...
  ],
  "watchedAttributes": [ "attr1", "attr2", ..., "attrN" ],
  "timeInterval": Number,  # Not Implemented in Alpha Release 1
  "q": "Query Filter",
  "geoQ": {},  # Not Implemented in Alpha Release 1
  "csf": "",   # Not Implemented in Alpha Release 1
  "isActive": true/false,
  "notification": {
    "attributes": [ "attr1", "attr2", ..., "attrN" ],
    "format": "keyValues" / "normalized",
    "endpoint": {
      "uri": "URI which conveys the endpoint which will receive the notification",
      "accept": "application/json" / "application/ld+json"
    },
    "status": "ok" / "failed"
  },
  "expires": "ISO 8601 String",
  "throttling": Number, # Minimal period of time in seconds which shall elapse between two consecutive notifications
  "temporalQ": # Not Implemented in Alpha Release 1,
  "status": "active"/"paused"/"expired"  # Read-only - not to be given at creation time
}

```
Lots of things to say about this structure. However, everything is already said in the ETSI specification. No need to repeat it here.
Please refer to page 46, Table 5.2.12-1 of the ETSI spec version 1.2.1.

The only mandatory fields are: "type" and "notification".
Also, either "entities" or "watchedAttributes" *must* be present (both of them at the same time is OK too).

NOTE that the `q` query filter for subscriptions uses the Orion NGSIv2 `q` for subscriptions in Alpha Release 1.
The new NGSI-LD `q` Query Filter has been implemented, but it is in use only for Queries of Entities in Alpha Release 1.

##### MQTT Notifications
The notifications of a subscription can be sent either as REST requests or as MQTT publish messages.
In the case of MQTT, the `endpoint::uri` would look as follows:
```json
  "endpoint": "mqtt://<server-ip>:<port>/<topic>"
```
The normal way for MQTT notifications would be to start an MQTT broker beside the NGSI-LD Broker and make the notifications
go to the MQTT broker, while the client subscribes to the `topic` in the MQTT broker apart from creating the subscription in the NGSI-LD broker.

#### Request URI Parameters
This service has no URI Parameters.

#### Response HTTP Status Code
* 201 Created - no payload data for a successful creation of a subscription
* 400 Bad Request - in case the request or its content is incorrect
* 409 Already Exists - if the provided subscription id is the id of an already existing subscription

#### Response HTTP Headers
No HTTP headers relevant to NGSI-LD are present in the response.

#### Response Payload Data
If all is OK, a `201 Created` is returned and no payload data is present in the response.
On error, the typical `ProblemDetails` structure is returned.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.8 - EntityInfo
* 5.2.12 - Subscription
* 5.2.14 - NotificationParams
* 5.8.1 - Create Subscription 
* 6.10.3.1 - POST /ngsi-ld/v1/subscriptions.

## Modification of a Subscription
### PATCH /ngsi-ld/v1/subscriptions/{subscriptionId}
Not Implemented in Alpha Release 1.

## Querying for Subscriptions
### GET /ngsi-ld/v1/subscriptions
The service `GET /ngsi-ld/v1/subscriptions` lets a user query subscriptions.
The response is an array of subscriptions that match the query criteria.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
* limit - maximum number of subscriptions to be returned
* offset - the index of the first subscription.

#### Response HTTP Status Code
* 200 OK - the payload body is a JSON array that contains the matching subscriptions
* 400 Bad Request - only way to get here is by setting the `limit` too high.

#### Response HTTP Headers
* Link
#### Response Payload Data
In case of `200 OK` status code, the response payload data is an array of subscriptions:
```json
[
  { Subscription 1 },
  { Subscription 2 },
  ...
  { Subscription N }
]
```

In case of `400 Bad Request`, the typical _ProblemDetails_ structure is returned.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.8 - EntityInfo
* 5.2.12 - Subscription
* 5.2.14 - NotificationParams
* 5.8.4 - Query Subscriptions 
* 6.10.3.2 - GET /ngsi-ld/v1/subscriptions.

## Retrieval of a Specific Subscription
### GET /ngsi-ld/v1/subscriptions/{subscriptionId}
This service returns the full information of the subscription whose _Subscription ID_ is exactly the same as the last item of the URL PATH.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
There are no URI Parameters for this service.

#### Response HTTP Status Code
* 200 OK - the payload body is a JSON object containing the requested subscription
* 400 Bad Request - the _subscriptionId_ in the URL PATH is not a valid URL
* 404 Not Found - the subscription specified in the URL PATH does not exist.

#### Response HTTP Headers
* Link

#### Response Payload Data
In case of `200 OK`, the response payload data is the entire subscription, for example:
```json
{
  "id": "http://a.b.c/subs/sub01",
  "type": "Subscription",
  "name": "Test subscription 01",
  "description": "Description of Test subscription 01",
  "entities": [
    {
      "type": "T1"
    },
    {
      "id": "http://a.b.c/E02",
      "type": "T2"
    },
    {
      "idPattern": ".*E03.*",
      "type": "T3"
    }
  ],
  "watchedAttributes": [
    "P2"
  ],
  "q": "P2>10",
  "geoQ": {
    "geometry": "circle",
    "coordinates": "1,2",
    "georel": "near"
  },
  "isActive": false,
  "notification": {
    "attributes": [
      "P1",
      "P2",
      "A3"
    ],
    "format": "keyValues",
    "endpoint": {
      "uri": "http://valid.url/url",
      "accept": "application/ld+json"
    }
  },
  "expires": "2028-12-31T10:00:00Z",
  "throttling": 5,
  "status": "paused"
}
```

On error, the typical _ProblemDetails_ is returned.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.8 - EntityInfo
* 5.2.12 - Subscription
* 5.2.14 - NotificationParams
* 5.8.3 - Retrieve Subscription
* 6.11.3.1 - GET /ngsi-ld/v1/subscriptions/{subscriptionId}.

## Deletion of a Specific Subscription
### DELETE /ngsi-ld/v1/subscriptions/{subscriptionId}
To delete a specific subscription, use the service `DELETE /ngsi-ld/v1/subscriptions/{subscriptionId}`.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
There are no URI Parameters for this service.

#### Response HTTP Status Code
* 204 No Content - no payload data for a successful deletion of a subscription
* 400 Bad Request - the subscription ID of the URI PATH is not a valid ID (not a URI)
* 404 Not Found - the subscription ID of the URI PATH is not found among the subscriptions.

#### Response HTTP Headers
No HTTP headers relevant to NGSI-LD are present in the response.

#### Response Payload Data
No payload data if all OK. If not OK, _ProblemDetails_.

#### Pointers to the ETSI NGSI-LD documentation
* 5.8.5 - Delete Subscription
* 6.11.3.3 - DELETE /ngsi-ld/v1/subscriptions/{subscriptionId}.


## Notifications
When an update/creation of an entity/attribute gets a hit in the list of subscriptions, a *notification* request is sent to the
endpoint that is stated as the receptor of notifications for that subscription.
There can of course be more than *one* hit is the list of subscriptions and thus, more than one notification may be sent as a result of the entity modification.

### Notification Context
The context of the notification is the context that was used when creating the subscription.
If the subscription that triggered the notification was created with `endpoint::accept` equal to *application/ld+json*, then the context is sent as part of
the payload data. If instead `endpoint::accept` is equal to *application/json*, then the context is sent as a Link HTTP header.

### Notification HTTP Headers
* Link (in case endpoint::accept == *application/json*)

### Notification Payload Data
The payload data of a notification contains information of the triggering subscription and the entities that provoked the notification:
```json
{
  "id": "notification identifier",
  "type": "Notification",
  "subscriptionId": "subscription identifier",
  "notifiedAt": "DateTime Timestamp corresponding to the instant when the notification was generated",
  "data": [
    { Entity 1 },
    { Entity 2 },
    ...
    { Entity N }
  ]
}
```

Note that the number of attributes of the entities can be limited by specifying a list of "interesting" attributes in the field `notification::attributes` when
creating the subscription.


## Registrations
Context Source Providers can be "mini brokers" that implement only a small part of the NGSI-LD API.
Typically, a Context Provider isn't contacted directly by clients but instead it registers its entities in a broker and
the broker will later contact the Context Provider when necessary.
The registration is the way for a Context Provider to inform the broker of what entities it has knowledge.

## Creation of Registrations
### POST /ngsi-ld/v1/csourceRegistrations

#### Request Payload Data
The payload data at creating a registration looks like this:
```json
{
  "id": "URI",  # if not given the system assigns an ID for the registration
  "type": "ContextSourceRegistration",  # MANDATORY
  "name": "Name of the registration",
  "description": "Description of the registration",
  "information": [
    {
      "entities": [
        {
          "id": "URI",         // Optional
          "idPattern": "REGEX" // Optional
          "type": "TYPE"       // MANDATORY
        },
        {
          ...
        }
      ],
      "properties": [
        "Property 1",
        "Property 2",
        ...
        "Property N"
      ]
      "relationships": [
        "Relationship 1",
        "Relationship 2",
        ...
        "Relationship N"
      ]
    },
    {
      ...
    }
  ],
  "observationInterval": {
    "start": "ISO 8601 DateTime",
    "end": "ISO 8601 DateTime"
  },
  "managementInterval": {
    "start": "ISO 8601 DateTime",
    "end": "ISO 8601 DateTime"
  },
  "location": { GeoLocation },
  "observationSpace": { GeoLocation },
  "operationSpace": { GeoLocation },
  "expires": "ISO 8601 DateTime",
  "endpoint": "URI",
  "Property 1": <JSON Value>,
  "Property 2": <JSON Value>,
  ...
  "Property N": <JSON Value>
}
```

NOTE: In Alpha Release 1, the `information` array must have only one item.
      This is due to the data model of Orion, which Orion-LD follows.

Only three fields are mandatory:
* type
* information
* endpoint.

#### Request URI Parameters
There are no URI Parameters for this service.

#### Response HTTP Status Code
* 201 Created - no payload data for successful creation of a registration
* 400 Bad Request - something is off with the request and the payload body contains information about the error 
* 409 Already Exists - the registration already exists
* 422 Unprocessable Entity - Unprocessable Context Source Registration.

#### Response HTTP Headers
* Location (if the creation went well)

#### Response Payload Data
No payload data if all OK. If not OK, _ProblemDetails_.

#### Pointers to the ETSI NGSI-LD documentation
* 4.7 Geospatial Properties 
* 5.2.9 CsourceRegistration 
* 5.2.10 RegistrationInfo 
* 5.2.11 TimeInterval
* 5.9.2 Register Context Source
* 6.8.3.1 POST /ngsi-ld/v1/csourceRegistrations.

## Modification of Registrations
### PATCH /ngsi-ld/v1/csourceRegistrations/{registrationId}
Not Implemented in Alpha Release 1.

## Querying for Registrations

### GET /ngsi-ld/v1/csourceRegistrations

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
* limit        - Maximum number of subscriptions to be returned
* offset       - The index of the first subscription
* id           - Comma separated list of entity identificators
* type         - Comma separated list of entity types
* idPattern    - REGEX to match entity id
* attrs        - Comma separated list of attribute names
* q            - Query Filter (Not Implemented in Alpha Release 1)
* csf          - Context Source Filter (Not Implemented in Alpha Release 1)
* georel       - Geo relationship (Not Implemented in Alpha Release 1)
* geometry     - Geometry (point, circle, polygon, ...) (Not Implemented in Alpha Release 1)
* coordinates  - Coordinates array, serialized as a string (Not Implemented in Alpha Release 1)
* geoproperty  - Which attribute to use as Geo-Attribute (Not implemented in Alpha Release 1)
* timeproperty - Not implemented in Alpha Release 1
* timerel      - Not implemented in Alpha Release 1
* time         - Not implemented in Alpha Release 1
* endTime      - Not implemented in Alpha Release 1

#### Response HTTP Status Code
* 200 OK - the payload body is a JSON array that contains the matching registrations
* 400 Bad Request - something is off with the request and the payload body contains information about the error.

#### Response HTTP Headers
* Link

#### Response Payload Data
The response payload data is an array of Context Source Registrations:
```json
{
  < Registration 1>,
  < Registration 2>,
  ...
  < Registration N>
}
```

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.9 - CsourceRegistration 
* 5.2.10 - RegistrationInfo 
* 5.2.11 - TimeInterval
* 5.9.2 - Register Context Source
* 6.8.3.2 - GET /ngsi-ld/v1/csourceRegistrations.

## Retrieval of a Specific Registration
### GET /ngsi-ld/v1/csourceRegistrations/{registrationId}
This service lets the user to retrieve a specific context source registration.
The Registration Identifier must be known though.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
There are no URI Parameters for this service.

#### Response HTTP Status Code
* 200 OK - the payload body is a JSON object that contains the requested registration
* 400 Bad Request - the registration id in the URL PATH is not a valid URI
* 404 Not Found - there is no reguistration with an ID as the one specified in the URL PATH.

#### Response HTTP Headers
* Link

#### Response Payload Data
The entire registration as a JSON object, e.g.:
```json
```

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.9 - CsourceRegistration 
* 5.2.10 - RegistrationInfo 
* 5.2.11 - TimeInterval
* 5.10.1 - Retrieve Context Source Registration
* 6.9.3.1 - GET /ngsi-ld/v1/csourceRegistrations/{registrationId}

## Deletion of Registrations
### DELETE /ngsi-ld/v1/csourceRegistrations/{registrationId}
Not Implemented in Alpha Release 1.

## Subscription to Registrations
Not Implemented in Alpha Release 1.

## Temporal Representation
Temporal Representation in FIWARE is taken care of by other GEs, such as Cygnus, and will not be implemented in Orion-LD.

## Security
Apart from forbidden characters, all security concerns are taken care of by other GEs, such as PEP, KeyRock, etc. and will not be implemented in Orion-LD.

## Geolocation
TBD.

## Query Filter
TBD.
