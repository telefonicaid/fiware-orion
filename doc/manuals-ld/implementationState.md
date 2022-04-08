# State of the Implementation



## 1. Features that may or not be implemented by the Service Routines (API endpoints)
All Services:
* Use of the **new C driver "mongoc"** vs the _deprecated MongoDB Lecacy C++ driver_

Entity Services:
* Support of **LanguageProperty**
* **Forwarding**, as of NGSI-LD API spec 1.6 (due summer 2022)
* Support for **Multi-Attributes** (datasetId)

Entity-updating services:
* **Notifications** - _native NGSI-LD_ vs _old mongoBackend_
* **TRoE** - Temporal Representation of Entities

Features that are implemented by all services, like _Concise Representation_, etc, are not mentioned in this document.
This document is a guide to what is missing, not what has been implemented.



## 2. Stuff that differ from the NGSI-LD API
### Relationships
According to the NGSI-LD API, a Relationship must be a String that is a valid URI.
However, there is a demand to have an Array of URIs, e.g.:
```
  "cousins": {
    "type": "Relationship",
    "object": [
      "urn:ngsi-ld:people:PeterX",
      "urn:ngsi-ld:people:PietroX",
      "urn:ngsi-ld:people:PedroX",
      ...
    ]
  }
```
While it is true that the same effect can be achived (well, not really the same) using the Multi-Attribute concept (datasetId),
that **is not** what our users are asking for. It's merely a _workaround_ for them to get what they need, which is an array of relationships.
Orion-LD doesn't really implement Multi-Attribute and it's going to take quite some time for that feature to be supported (if ever).
So, until the Multi-Attribute concept is fully supported, Orion-LD accepts Array as `object` for a Relationship Attribute.

_I implemented it that way by mistake as I thought it made a lot of sense to have a Relationship as an array of URIs.
Then I got users (a European project) using the feature and it was too late to remove it._



## 3. Other critical stuff that need to be implemented
* Memory management for containers
* Safe connection to MongoDB Server (-dbSSL and -dbAuthDb CLI arguments that Orion now implements)
* Subscription matching for Notification directly on DB (the matching is done on the Subscription Cache)
* Complete rewrite of the Subscription Cache and especially how brokers on the same DB communicate.
  Right now, the communication is done via the database and that is ... well, the _easiest possible way_, but also
  the **worst possible way**
* Registration Cache
* Perhaps even an Entity Cache?



## 4. Extra Stuff that Orion-LD supports but that may never enter the NGSI-LD API
### Cross Notifications
### POST /ngsi-ld/ex/v1/notify
  The ability to receive notifications from an NGSI-LD Broker and treat the notification as a BATCH Upsert.
  This feature allows a Broker to subscribe to information from other brokers and keep its own copy of that information.
  A typical use case is for treatment of the information by other GEs like Cosmos, or Perseo, that need the information in the local
  database.

### GET /ngsi-ld/ex/v1/version
  Retrieve version information specific to the NGSI-LD API.
  The old NGSI /version is of course supported as well: `GET /version`

### GET /ngsi-ld/ex/v1/ping
  Just to have the broker respond with the same - used for debugging purposes.

### GET /ngsi-ld/ex/v1/tenants
  Retrieve a list of the current tenants - used for debugging purposes.

### GET /ngsi-ld/ex/v1/dbIndexes
  Retrieve a list of the current database indices - used for debugging purposes.



## 5. Implementation state of Services (API Endpoints)


### POST /ngsi-ld/v1/entities
Creation of an entity.
If the entity in question already exists (depending on the "id" field in the payload body),
then a `409 Conflict` is returned.

There are two different implementations of this API endpoint.
- The old one that uses mongoBackend and thus the deprecated MongoDB Legacy C++ driver
- A new one that uses the new mongoc driver

The old implementation supports everything but Forwarding (well, apart from using the old Legacy driver ...)
and is used by default by Orion-LD.  
The new implementation is just that, brand new (as of April 8 2022).
To test it, Orion-LD must be started with the CLI parameter `-experimental`.

#### Done in old implementation
  * TRoE is fully working
  * LanguageProperty attributes are supported
  * Multi-attributes are supported

#### Missing in old implementation
  * Still uses the MongoDB C++ Legacy Driver (mongoBackend)
  * Notifications are supported but, that's part of mongoBackend and needs a rewrite
  * Forwarding - the old NGSIv2 style forwarding has been disabled

#### Done in new implementation
  * Uses the new mongoc driver

#### Missing in new implementation
  * TRoE is implemented just not tested
  * LanguageProperty
  * Multi-attributes
  * Notifications
  * Forwarding


### GET /ngsi-ld/v1/entities
#### Done
  * LanguageProperty attributes are supported

#### Missing
  * Still uses the MongoDB C++ Legacy Driver (mongoBackend)
  * Multi-attributes
  * Forwarding - the old NGSIv2 style forwarding has been disabled

###


### GET /ngsi-ld/v1/entities/*
#### Done
  * Uses the new mongoc driver
  * LanguageProperty attributes are supported
  * Multi-attributes are supported

#### Missing
  * Forwarding


### PATCH /ngsi-ld/v1/entities/*
This service is experimental and is only in place when Orion-LD is started with the `-experimental` CLI parameter.

#### Done
  * Uses the new mongoc driver
  * LanguageProperty attributes are supported
  * TRoE is almost fully working - we need to add an opMode in the sub-attrs table to indicate "Removal of Sub-Attribute"

## Missing
  * Multi-attributes
  * New "native NGSI-LD" Notifications are supported but some parts are missing: { "q", "geoQ" }. It is also very green.
  * Forwarding


### PUT /ngsi-ld/v1/entities/*
This service is experimental and is only in place when Orion-LD is started with the `-experimental` CLI parameter.

#### Done
  * Uses the new mongoc driver

## Missing
  * Multi-attributes
  * New "native NGSI-LD" Notifications are supported but some parts are missing: { "q", "geoQ" }. It is also very green.
  * Forwarding
  * LanguageProperty attributes
  * TRoE

------ TBI --------------------------------------------------------

### DELETE /ngsi-ld/v1/entities/*

### POST /ngsi-ld/v1/entities/*/attrs
### PATCH /ngsi-ld/v1/entities/*/attrs

### PATCH /ngsi-ld/v1/entities/*/attrs/*
### DELETE /ngsi-ld/v1/entities/*/attrs/*

### POST /ngsi-ld/v1/entityOperations/create
### POST /ngsi-ld/v1/entityOperations/upsert
### POST /ngsi-ld/v1/entityOperations/update
### POST /ngsi-ld/v1/entityOperations/delete
### POST /ngsi-ld/v1/entityOperations/query

### GET /ngsi-ld/v1/types
### GET /ngsi-ld/v1/types/*

### GET /ngsi-ld/v1/attributes
### GET /ngsi-ld/v1/attributes/*

### POST /ngsi-ld/v1/subscriptions
### GET /ngsi-ld/v1/subscriptions

### GET /ngsi-ld/v1/subscriptions/*
### PATCH /ngsi-ld/v1/subscriptions/*
### DELETE /ngsi-ld/v1/subscriptions/*

### POST /ngsi-ld/v1/csourceRegistrations
### GET /ngsi-ld/v1/csourceRegistrations

### PATCH /ngsi-ld/v1/csourceRegistrations/*
### GET /ngsi-ld/v1/csourceRegistrations/*
### DELETE /ngsi-ld/v1/csourceRegistrations/*

### POST /ngsi-ld/v1/jsonldContexts
### GET /ngsi-ld/v1/jsonldContexts

### GET /ngsi-ld/v1/jsonldContexts/*
### DELETE /ngsi-ld/v1/jsonldContexts/*

### GET /ngsi-ld/v1/temporal/entities
### GET /ngsi-ld/v1/temporal/entities/*

### POST   /ngsi-ld/v1/temporal/entityOperations/query
### POST   /ngsi-ld/v1/temporal/entities
### DELETE /ngsi-ld/v1/temporal/entities/*
### POST   /ngsi-ld/v1/temporal/entities/*/attrs
### DELETE /ngsi-ld/v1/temporal/entities/*/attrs/*
### PATCH  /ngsi-ld/v1/temporal/entities/*/attrs/*/*
