# Developer Guide of Orion-LD

Welcome to the Developer Guide of Orion-LD, the NGSI-LD context broker!

The Orion-LD context broker doesn't have a GUI and the only way to contact the broker is programatically,
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

Strongly recommended reading before continuing withtis document:
* [NGSI-LD API v1.1.1](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf)
* [Guide to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)
* [Guide to the context](doc/manuals-ld/the-context.md)
* [Quick Start Guide](doc/manuals-ld/quick-start-guide.md)

Also, to be able to play with your own Orion-LD broker while reading this document definitely helps top understand everything better.
If you don't have a running Orion-LD to play with, please follow the instructions in the [Orion-LD Installation Guide](doc/manuals-ld/installation-guide.md).

This document will go into more detail on pretty much everything about the Orion-LD context broker:
* Creation of Entities
* Modification of Entities and Attributes
* Deletion of Entities and Attributes
* Forwarding of Creation/Modification of Entities/Attributes
* System Attributes
* Querying for Entities
* Retrieval of Entities
* Forwarding of Query Requests
* Creation of Subscriptions
* Modification of Subscription 
* Deletion of Subscription
* Notifications
* Creation of Registrations
* Modification of Registrations
* Deletion of Registrations
* Geolocation

## Creation of Entities
Entities can be created in three different ways:
* `POST /ngsi-ld/v1/entities` (to create a single entity)
* `POST /ngsi-ld/v1/entityOperations/create` (to create more than one entity in one go)
* `POST /ngsi-ld/v1/entityOperations/upsert` (to both create and modify entities with one single request)

### Contexts
If a context is desired for the creation, it can be put either in the payload, as part of the entity, a "special attribute",
or in the HTTP header "Link".

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

This is all explained in the [guide to the Context](doc/manuals-ld/the-context.md).

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

The syntaxis for the attributes is described in the [introduction to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)

#### Request URI Parameters
As all the information of the entity to create resides in the payload data, there are no URI parameters for this request.

#### Response HTTP Status Code
* 201 Created              - if everything works, and no payload data is present.
* 400 Bad Request          - for a payload data with an invalid JSON syntax or invalid JSON types for the fields in the payload data.
* 409 Conflict             - the entity already exists.
* 422 Unprocessable Entity - operation not available

#### Response HTTP Headers
* Location - to inform the creator of where the created entity resides.
* Link     - to echo back to the creator the context that was used during modification of the entities.

Well, the issuer of the request already knows the context that was used, as he/she provided it!
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
In case of an error, the payload data describes the error.

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
The latest version of the NGSI-LD API definition (as of December 2019) is found [here](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf).
Some chapters of interest in said document for `POST /ngsi-ld/v1/entities` are:
* 5.2.4   - The NGSI-LD Entity Data Type
* 5.2.5   - The NGSI-LD Property Data Type
* 5.2.6   - The NGSI-LD Relationship Data Type
* 5.2.7   - The NGSI-LD GeoProperty Data Type
* 5.6.1   - Create Entity
* 6.4.3.1 - POST /ngsi-ld/v1/entities


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
  },
]
```
The syntaxis of an entity is fully described in the previous chapter (about `POST /ngsi-ld/v1/entities`).

#### Request URI Parameters
There are no URI parameters for this request. All necessary information resides in the request payload data.

#### Response HTTP Status Code
201 Created      - all entities were successfully created. No response payload data supplied.
207 Multi Status - some entities were	successfully created, others weren't. Details of each error in the response payload data.
400 Bad Request  - none of the entities in the request payload data were created. Details of each error in the response payload data.

#### Response HTTP Headers
* Link - to echo back to the creator the context that was used during modification of the entities.

#### Response Payload Data
In case of All OK, a `204 No Content` is returned and no payload data.
In case of 400 Bad Request (a JSON parse error, or similar errors), the response payload data is the typical { type, title and detail }. E.g.:
```json
{
    "detail": "JSON Parse Error: expecting comma or end of object",
    "title": "JSON Parse Error",
    "type": "https://uri.etsi.org/ngsi-ld/errors/InvalidRequest"
}
```

If there is a mix of results (some entities are OK others aren't, then the response payload contains two arrays:
* success - a string array containing  the Entity ID of the entities that were successfully created.
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
The fields `"errors"` and `"success"` are *always* present, also if empty arrays.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5  - The NGSI-LD Property Data Type
* 5.2.6  - The NGSI-LD Relationship Data Type
* 5.2.7  - The NGSI-LD GeoProperty Data Type
* 5.2.16 - BatchOperationResult
* 5.2.17 - BatchEntityError
* 5.6.7  - Batch Entity Creation
* 6.14   - POST /ngsi-ld/v1/entityOperations/create

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
201 Created      - all entities were successfully created/updated. No response payload data supplied.
207 Multi Status - some entities were successfully created/updated, others weren't. Details of each error in the response payload data.
400 Bad Request  - none of the entities in the request payload data were created/updated. Details of each error in the response payload data.

#### Response HTTP Headers
* Link - to echo back to the creator the context that was used during modification of the entities.

#### Response Payload Data
See the corresponding section for `POST /ngsi-ld/v1/entityOperations/create`.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5  - The NGSI-LD Property Data Type
* 5.2.6  - The NGSI-LD Relationship Data Type
* 5.2.7  - The NGSI-LD GeoProperty Data Type
* 5.2.16 - BatchOperationResult
* 5.2.17 - BatchEntityError
* 5.6.8  - Batch Entity Creation or Update (Upsert)
* 6.15   - POST /ngsi-ld/v1/entityOperations/upsert


## Modification of Entities and Attributes
There are a number of services for modification of entities and attributes:
* POST /ngsi-ld/v1/entities/{entityId}/attrs
* PATCH /ngsi-ld/v1/entities/{entityId}/attrs
* PATCH /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}
* POST /ngsi-ld/v1/entityOperations/upsert
* POST /ngsi-ld/v1/entityOperations/update

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
The syntaxis for attributes is described in the [introduction to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)

#### Request URI Parameters
* options=noOverwrite - to ask the broker to *not overwrite* any already existing attribute

#### Response HTTP Status Code
* 204 No Content - if all went well
* 207 Multi-Status - if partial success
* 400 Bad Request - in case of JSON parse error or similar (invalid entity id in the URL path)
* 404 Not Found - if the entity id of the URL path doesn't get a hit for an entity in the database

#### Response HTTP Headers
No HTTP headers relevant for NGSI-LD are present in the response.

#### Response Payload Data
If all went well, no payload data is returned, just the `204 No Content`.
On partially successful operation, a `207 Multi-Status` is returned and a payload data that contains two arrays (similar to the resonse payload data of the Batch operations):
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

For 400 and 404, the typical { type, title, detail } error object (ProblemDetails) is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5   - The NGSI-LD Property Data Type
* 5.2.6   - The NGSI-LD Relationship Data Type
* 5.2.7   - The NGSI-LD GeoProperty Data Type
* 5.6.3   - Append Entity Attributes
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
The syntaxis for attributes is described in the [introduction to NGSI-LD entities and attributes](doc/manuals-ld/entities-and-attributes.md)

#### Request URI Parameters
There are no URI parameters for this request. All necessary information resides in the request payload data.

#### Response HTTP Status Code
Just like `POST` for the same resource.

#### Response HTTP Headers
No HTTP headers relevant for NGSI-LD are present in the response.

#### Response Payload Data
Just like `POST` for	the same resource.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5   - The NGSI-LD Property Data Type
* 5.2.6   - The NGSI-LD Relationship Data Type
* 5.2.7   - The NGSI-LD GeoProperty Data Type
* 5.6.2   - Update Entity Attributes
* 6.6.3.2 - PATCH /ngsi-ld/v1/entities/{entityId}/attrs


### PATCH /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}
This operation allows performing a partial update on an attribute of an entity.
A partial update only changes the fields provided in the payload data, leaving the rest of the attribute unaffected. 

#### Request Payload Data
The payload data for this service is a fragment of an attribute.
E.g., to change the value to 45, and add a sub-property P11, the payload could look something like this:
```json
{
  "value": 45,
  "P11": {
    "type": "Property",
    "value": "p"
  }
}

#### Request URI Parameters
There are no URI parameters for this request. All necessary information resides in the request payload data.

#### Response HTTP Status Code
* 204 No Content  - if all is good and well
* 400 Bad Request - if the request or its content is somehow incorrect
* 404 Not Found   - if the entity or the attribute specified in the URL does not exist

#### Response HTTP Headers
No HTTP headers relevant for NGSI-LD are present in the response.

#### Response Payload Data
If all went well, no payload data is returned, just the `204 No Content`.
For 400 and 404, the typical { type, title, detail } (ProblemDetails) error object is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.5   - The NGSI-LD Property Data Type
* 5.2.6   - The NGSI-LD Relationship Data Type

* 5.2.7   - The NGSI-LD GeoProperty Data Type
* 5.6.4   - Partial Attribute update 
* 6.7.3.1 - PATCH


### POST /ngsi-ld/v1/entityOperations/upsert
Already documented under "Creation of Entities".

### POST /ngsi-ld/v1/entityOperations/update
NOT IMPLEMENTED IN ALPHA RELEASE 1

## Deletion of Entities and Attributes
There are three services for deletion of entities and attributes:
* POST /ngsi-ld/v1/entityOperations/delete
* DELETE /ngsi-ld/v1/entities/{entityId}
* DELETE /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}

The first service allows to delete a number of entities in one go, while the second service deltes one single entity
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
* 204 No Content  - if all is good and well
* 207 Multi-Status - if partial success
* 400 Bad Request - if the request or its content is somehow incorrect, e.g. JSON parse error

#### Response HTTP Headers
No HTTP headers relevant for NGSI-LD are present in the response.

#### Response Payload Data
See the corresponding section for `POST /ngsi-ld/v1/entityOperations/create`

#### Pointers to the ETSI NGSI-LD documentation
* 5.2.16   - BatchOperationResult
* 5.2.17   - BatchEntityError
* 5.6.10   - Batch Entity Delete
* 6.17.3.1 - POST /ngsi-ld/v1/entityOperations/delete

### DELETE /ngsi-ld/v1/entities/{entityId}
To delete a single entity, without any payload data, the service `DELETE /ngsi-ld/v1/entities/{entityId}`.
Not mush to say, if the entity with id `entityId` is found, it is deleted.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
This service has no URI Parameters.

#### Response HTTP Status Code
* 200 OK - if all OK
* 400 Bad Request - if the entity id of the URL PATH is not a valid URI
* 404 Not Found - if thje entity doe snot exist

#### Response HTTP Headers
No relevant HTTP headers are present in the response.

#### Response Payload Data
For 400 and 404, the typical { type, title, detail } error object (ProblemDetails) is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.6.6   - Delete Entity
* 6.5.3.2 - DELETE /ngsi-ld/v1/entities/{entityId}


### DELETE /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}
This service allows a user to delete a single attribute of a given entity.

#### Request Payload Data
There is no payload data for this service.

#### Request URI Parameters
This service has no URI Parameters.

#### Response HTTP Status Code
* 204 No Content - if all is OK
* 400 Bad Request - in case of invalid entity id in the URL path
* 404 Not Found - if the entity id of the URL path doesn't get a hit for an entity in the database

#### Response HTTP Headers
No relevant HTTP headers are present in the response.

#### Response Payload Data
For 400 and 404, the typical { type, title, detail } error object (ProblemDetails) is returned as payload data.

#### Pointers to the ETSI NGSI-LD documentation
* 5.6.5   - Delete Entity Attribute
* 6.7.3.2 - DELETE /ngsi-ld/v1/entities/{entityId}/attrs/{attrId}


## Forwarding of Creation/Modification of Entities/Attributes
The _Forwarding Concept_ for NGSI-LD has still to be specified. We have started to look at this a little (we == the ETSI CIM group defining the NGSI-LD API)
but we still have a long way to go before having anything decided and documented.

As Orion-LD builds on Orion and reuses (with modification for expansion and compaction of items) whatever Orion has implemented, Orion-LD actually supports
forwarding already, but, this will have to change to the NGSI-LD way, once that way is defined.

Forwarding is a mechanism to include context providers as "part of the broker". An example always makes it easier to explain/understand:
1. Context Provider CP-1 informs the broker that it knows about the entity XXX, with attributes A1, A2, ...
   This is done by sending a Registration request to the broker.
   The registration request contains infoirmation about the entities and attributes, and, importantly, the IP, port and URL-PATH to use to contact the context provider.
2. A query for entity E13, attributes A1-A12 enters the broker.
   The broker looks at its current registrations, and sees that the registration for CP-1 matches what the query is asking for, so:
3. The broker forwards the initial request to CP-1 and waits for CP-1 to answer.
4. Once the answer from CP-1 has arrived, the broker merges the response from CP-1 with what the broker found in its local database and:
5. The broker respons to the initial request with the merged results from the broker itself and the response from CP-1

That was for retrieval pf entities.
Updating of entities works pretty much the same way.

Now, for forwarding NGSi-LD requests in Orion-LD. please see the proper documentation of Orion and imagine the attribute names and entity types to be expanded/compacted according to the context.
The context used for forwarding must of course be the context specified in the registration.
This hasn't even been tested, but still, it should "work a little" ...

## System Attributes
Each entity and attribute has two timestamps, namely "createdAt", and "modifiedAt".
The broker makes sure these two "builtin special attributes" are created and updated accordingly during the lifetime of the entity/attribute.
The values *can not* be altered from outside the broker, i.e., there is no service that let's a user set these timestamps to aleatory values.
But of course, when modifying an entity, its "modifiedAt" is updated, same with attributes.

A user can ask the broker to include these special attributes in queries though, by specifying the value "sysAttrs" to the URI parameter "options":
```bash
GET /ngsi-ld/v1/entities?options=sysAttrs&type=T
```

## Querying for Entities

### GET /ngsi-ld/v1/entities

## Retrieval of Entities
### GET /ngsi-ld/v1/entities/{entityId}

## Forwarding of Query Requests
NGSI-LD forwarding is still to be specified.
Please see chapter `Forwarding of Creation/Modification of Entities/Attributes` for more information.

## Creation of Subscriptions
### POST /ngsi-ld/v1/subscriptions

## Modification of Subscription
### PATCH /ngsi-ld/v1/subscriptions/{subscriptionId}

## Retrieval of Subscription
### GET /ngsi-ld/v1/subscriptions
### GET /ngsi-ld/v1/subscriptions/{subscriptionId}

## Deletion of Subscription
### DELETE /ngsi-ld/v1/subscriptions/{subscriptionId}

## Notifications
When an update/creation of an entity/attribute gets a hit in the list of subscriptions, a *notification* request is sent to the
endpoint that is stated as the receptor of notifications for that subscription.
There can of course be more than *one* hit is the list of subscriptions and thus, more than one notification may be sent as a result of the entity modification.

### Notification Context
The context of the notification is the context that was used when creating the subscription.
If the subscription that triggered the notification was created with `endpoint::accept` equal to *application/ld+json*, then the context is sent as part of
the payload data. If instead `endpoint::accept`	is equal to *application/json*, then the context is sent as a Link HTTP header.

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
    { Entity N}
  ]
}
```

Note that the number of attributes of the entities can be limited by specifying a list of "interesting" attributes in the field `notification::attributes` when
creating the subscription.

## Creation of Registrations
### POST /ngsi-ld/v1/registrations

## Modification of Registrations
### PATCH /ngsi-ld/v1/registrations/{registrationId}
NOT IMPLEMENTED IN ALPHA RELEASE 1

## Retrieval of Registrations
### GET /ngsi-ld/v1/registrations
### GET /ngsi-ld/v1/registrations/{registrationId}

## Deletion of Registrations
### DELETE /ngsi-ld/v1/registrations/{registrationId}
NOT IMPLEMENTED IN ALPHA RELEASE 1

## Geolocation
TBD
