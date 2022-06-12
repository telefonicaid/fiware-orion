# State of the Implementation



## 1. Features that may or not be implemented by the Service Routines (API endpoints)
All Services:
* Use of the **new C driver "mongoc"** vs the _deprecated MongoDB Lecacy C++ driver_

Entity Services:
* Support of **LanguageProperty**  (supported everywhere)
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
* Once PATCH /entities/{entityId} got implemented, the brpkler can now delete individual sub-attributes.
    That was not possible before and there is no "opCode" in the TRoE table "subAttributes" to reflect this.
    We need to modify the TRoE database and add an "opCode" to the "subAttributes" table - then mark it as DELETED.
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


## 5. Important Info on the NGSi-LD concepts
### Subscriptions/Notifications
Right now, 3 operations support "Native NGSI-LD" notifications:
* POST /entities
* PUT /entities/{entityId}
* PATCH /entities/{entityId}
This is true only if Orion-LD is started with the `-experimental` CLI argument

Now, creation/updates using those three operations match subscriptions in the new way, using the more powerful NGSI-LD 'q'.
However, this is still under development and not everything is implemented.
What is missing:
* REGEX (~= and !~= operators)

Especially tricky is the balancing act between NGSIv2 subs and "Native NGSI-LD subs", or really, the dfiference in 'q'.
For example, NGSI-LD 'q' supports OR and parenthesis while NGSIv2 'q' does not.
This problem has been fixed by making the NGSIv2 part never match (q is set to P&!P) if the 'q' of the subscription is "too complex".
The NGSIv2 matach is done via Scope, while NGSI-LD match is done with qLex/qParse/qMatch, and a QNode* qP of CachedSubscription and that has
been taken advantage of to distinguish between the two.

### Support of the new mongoc driver
All generic features now use "mongoc" - the new Mongo C driver:
* Creation of database indices (only if started with '-experimental')
* Persisting Contexts
* Tenants
* Subscription Cache

The following API endpoints use "mongoc" - the new Mongo C driver:
* PATCH /entities/{entityId}
* PUT /entities/{entityId}
* POST /jsonldContexts
* GET /jsonldContexts
* GET /jsonldContexts/{contextId}
* DELETE /jsonldContexts/{contextId}

The following API endpoints use "mongoc" if Orion-LD is started with the `-experimental` CLI parameter:
* POST /entities
* GET /entities
* GET /entities/{entityId}
* POST /subscriptions
* PATCH /subscriptions/{subscriptionId}
* DELETE /subscriptions/{subscriptionId}

The following API endpoints use no database access at all, just the subscription cache:
* GET /subscriptions
* GET /subscriptions/{subscriptionId}

So, without using the old Mongo C++ Legacy driver, it is now possible to:
* Create entities
* Update entities (PATCH+PUT)
* Query entities
* Retrieve individual entity
* Create subscriptions
* Retrieve subscription (is uses the subscription cache, no DB request is performed)
* List subscriptions (is uses the subscription cache, no DB request is performed)
* Modify subscriptions
* Delete subscriptions
* Create JSONLD Contexts
* Retrieve individual JSONLD Context
* List JSONLD Contexts

## 6. Implementation state of Services (API Endpoints)

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
  * Multi-attributes are supported

#### Missing in old implementation
  * Still uses the MongoDB C++ Legacy Driver (mongoBackend)
  * Notifications are supported but, that's part of mongoBackend and needs a rewrite
  * Forwarding - the old NGSIv2 style forwarding has been disabled

#### Done in new implementation
  * Uses the new mongoc driver
  * TRoE is implemented

#### Missing in new implementation
  * Multi-attributes
  * Forwarding


### GET /ngsi-ld/v1/entities
#### Done
  * Optionally uses mongoc (CLI option -experimental)

#### Missing
  * Multi-attributes
  * Forwarding - the old NGSIv2 style forwarding has been disabled


### GET /ngsi-ld/v1/entities/*
#### Done
  * Uses the new mongoc driver
  * Multi-attributes are supported
  * Forwarding - new for NGSI-LD but not according to the version 1.6 of the NGSI-LD API spec

#### Missing
  * Forwarding 100% according to the version 1.6 of the NGSI-LD API spec


### PATCH /ngsi-ld/v1/entities/*
This service is experimental and is only in place when Orion-LD is started with the `-experimental` CLI parameter.

#### Done
  * Uses the new mongoc driver
  * TRoE is almost fully working - we need to add an opMode in the sub-attrs table to indicate "Removal of Sub-Attribute"

#### Missing
  * Multi-attributes
  * New "native NGSI-LD" Notifications are supported but "geoQ" is still missing. It is also quite green.
  * Forwarding


### PUT /ngsi-ld/v1/entities/*
This service is experimental and is only in place when Orion-LD is started with the `-experimental` CLI parameter.

#### Done
  * Uses the new mongoc driver

#### Missing
  * Multi-attributes
  * New "native NGSI-LD" Notifications are supported but "geoQ" is still missing. It is also quite green.
  * Forwarding
  * TRoE is almost fully working - we need to add an opMode in the sub-attrs table to indicate "Removal of Sub-Attribute"


### PATCH /ngsi-ld/v1/entities/*/attrs/*

#### Done
  * Forwarding - new for NGSI-LD but not according to the version 1.6 of the NGSI-LD API spec
  * TRoE
  * Notifications - using the Legacy driver
  * datasetId (using the Legacy driver)

#### Missing
  * Still uses the MongoDB C++ Legacy Driver (mongoBackend)
  * Multi-attributes

### POST /ngsi-ld/v1/subscriptions

#### Done
  * Uses the new mongoc driver (only if -experimental is set)

### GET /ngsi-ld/v1/subscriptions

#### Done
  * Uses the subscription cache - no need for the legacy driver (only if -experimental is set)

#### Missing
  * Must implement also GET from database with mopngoc - if not, the broker can't run without sub-cache

### GET /ngsi-ld/v1/subscriptions/*

#### Done
  * Uses the subscription cache - no need for the legacy driver (only if -experimental is set)

#### Missing
  * Must implement also GET from database with mopngoc - if not, the broker can't run without sub-cache

### DELETE /ngsi-ld/v1/subscriptions/*

#### Done
  * Uses the new mongoc driver (only if -experimental is set)
  * Disconnects from MQTT broker if need be


### PATCH /ngsi-ld/v1/subscriptions/*

#### Done
  * Uses the new mongoc driver (-experimental not needed)
  * Disconnects/Reconnects from/to MQTT broker if needed

### DELETE /ngsi-ld/v1/entities/*

#### Done
  * Uses the new mongoc driver (-experimental not needed)

#### Missing
  * Notifications

------ TBI --------------------------------------------------------

### POST /ngsi-ld/v1/entities/*/attrs
### PATCH /ngsi-ld/v1/entities/*/attrs

### DELETE /ngsi-ld/v1/entities/*/attrs/*
* Supports datasetId
* No Notifications (notifications on DELETE is not yet part of the NGSI-LD API - hopefully for 1.6.1)
* Not using mongoBackend, but using the the mongo C++ legacy driver 
  * dbEntityAttributeInstanceLookup
  * dbEntityAttributeLookup
  * dbEntityAttributeWithDatasetsLookup
  * dbEntityAttributesDelete
  * dbEntityFieldDelete
  * dbEntityFieldReplace
  
### POST /ngsi-ld/v1/entityOperations/create
### POST /ngsi-ld/v1/entityOperations/upsert
### POST /ngsi-ld/v1/entityOperations/update
### POST /ngsi-ld/v1/entityOperations/delete
* Not using mongoBackend, but using the the mongo C++ legacy driver:
  * dbEntityListLookupWithIdTypeCreDate
  * dbEntitiesDelete

### POST /ngsi-ld/v1/entityOperations/query
* Not using mongoBackend, but using the the mongo C++ legacy driver:
  * dbEntitiesQuery

### GET /ngsi-ld/v1/types
* Not using mongoBackend, but using the the mongo C++ legacy driver 
  * dbEntityTypesGet

### GET /ngsi-ld/v1/types/*
* Not using mongoBackend, but using the the mongo C++ legacy driver 
  * dbEntityTypeGet

### GET /ngsi-ld/v1/attributes
### GET /ngsi-ld/v1/attributes/*

### POST /ngsi-ld/v1/csourceRegistrations
### GET /ngsi-ld/v1/csourceRegistrations

### PATCH /ngsi-ld/v1/csourceRegistrations/*
* Not using mongoBackend, but using the the mongo C++ legacy driver:
  * dbRegistrationGet
  * dbRegistrationReplace

### GET /ngsi-ld/v1/csourceRegistrations/*
### DELETE /ngsi-ld/v1/csourceRegistrations/*
* Not using mongoBackend, but using the the mongo C++ legacy driver:
  * dbRegistrationExists
  * dbRegistrationDelete

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
