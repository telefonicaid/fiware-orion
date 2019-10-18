# STATUS OF THE IMPLEMENTATION

## Overview
  orionld is an NGSI-LD broker, fully aware of "Linked Data".
  When specifying the ngsi-ld protocol, an attempt was made to not divert too much from the APIv2 protocol, so that a maximum of
  the implementation of the already implemented APIv2 orion could be reused.

  Whenever possible, orionld reuses old code of orion before implementing anything new, to save implementation time.
  Sometimes this is not possible, notably for context source registrations, where the data model has been altered,
  and the query language that has been much improved and incompatible with the APIv2 query language.

  Around 90% of the REST services can reuse the already existing implementation, with "minor" changes for expansion/contraction.
  For those few services that aren't compatible with already existing code, an attempt is always made to alter the existing code.
  In one single case it has turned out just TOO DARN DIFFICULT (POST /ngsi-ld/v1/entities/{entity-id}/attr without options=noOverwrite)
  and a complete new implementation for that part of the service has been implemented.

  It is of course a nice advantage to be able to use the code that is already implemented, but the most important part is that both
  notifications and forwarding comes for free when reusing the existing code.

  As of October 2019, an estimation of how much of the ngsi-ld spec is implemented in orionld would be "around 70-75 percent".
  About services implemented or not, at least 90% is done, but details are missing, like correct error handling.

  There is a test suite (github repo: FIWARE/NGSI-LD_TestSuite) for ngsi-ld brokers and currently, the result for orionld is
  130 out of 150 tests OK.

## New Way
  The implementation of orion started in 2012, implementing the APIv1 requests and creating the data model according to APIv1, of course.
  We kept working for years, adding more and more functionality.
  Around 2015 we decided that APIv1 was no good and we started implementing a much improved API, Version 2.
  As we needed to maintain APIv1, we didn't change the data model but we made APIv2 work using the APIv1 data model.
  And we kept adding functionality, year after year.

  Orion is a "beast" - a big program, with over 1000 source code files. Hundreds of thousands of code lines.
  The normal "birth" for a program of this caliber is a complete rewrite after a few years.
  A complete rewrite makes A LOT OF SENSE.
  Frankly, we didn't know very well what we were implementing back in 2012.
  After some 5 years, to take all that acquired knowledge and reimplement from scratch ... that's a good good thing.
  We never got to do it with orion, and now, with orionld, yet another protocol to be covered by the old orion ...

  After 7 years working full-time on the broker a have a few dozen ideas on how to implement a good broker.
  Some of them I have incorporated into orionld, for example my own json parser (that is now the heart of orionld)
  and the new URI parsing algorithm.
  Also, the idea of using a thread variable for all "request variables", like the method, the payload, the URI params, etc, etc.
  In orion we allocate a big struct to keep all that info, and then we pass a pointer of that struct to each and every function.
  This is not necessary. That struct contains read-only info for the request and can very well be a global variable.
  Well, not a global global variable, but a *thread variable* that is global only inside the thread.
  We save the allocation and to push it on the stack a thousand times (at least) for each request.
  orionld uses a thread variable "orionldState", global inside the request thread. No allocation necessary, no pushing to the stack.

  Imagine now that we have 2000 requests per second.
  This single little idea saved us 2000 calls to malloc every second (malloc is a very slow system call).
  It also saves us millions of push to the stack, and some stack size is saved as well.
  Of course, it makes the code easier to implement and easier to read at the same time.
  What I would call a Win-Win.

  Another *horrible* thing about orion are all the copies that are made of the incoming payload.
  * A request comes in, as JSON ASCII text
  * rapidjson is called to parse the payload and the JSON test is converted into a rapidjson tree
  * The service routines convert the rapidjson tree into C++ classes (hundreds of functions are needed for this)
  * The database layer (mongoBackend) converts the C++ classes into BSON, that mongo understands.
    To convert all different C++ classes into BSON, many thousands of lines of code have been implemented

  And after this, the response is created from the BSON output from the mongo driver, via C++ classes and then a whole tree
  of functions to build JSON from the C++ classes.

  The main idea behind the "New Way" is to simply use the tree that is output from the new JSON parser.
  It is a very simple tree with next pointers and child pointers (child pointers for arrays and objects).
  Very easy to use.
  And yes, I understand the question that comes naturally right now: "Why not use the tree that rapidjson outputs, and then the same idea?".
  Again, yes, I tried. The tree that rapidjson gives us is extremely complex and really hard to use.
  I tried and I gave up.
  For orionld, as I already had my own json parser, that by the way is at least twice as fast as rapidjson for typical orion payloads,
  it was a very easy decision to use it.
  Now, the tree that is output from my json parser is used all over the broker. The KjNode tree node is pretty much the heart of orionld.

  So, instead of copying the payload over and over, I plan to just use the KjNode tree;
  * A request comes in, as JSON ASCII text
  * kjson is called to parse the payload and the JSON test is converted into a KjNode tree
  * The service routines send this KjNode tree AS IS to the database layer
  * The database layer (not mongoBackend) simply uses this easy-to-use KjNode tree
  * The database layer calls a single function (kjTreeToBson) - not a hundred different - to finally convert the payload to BSON for mongo
    [ Also, with the new database layer, we could easily change mongo for some other database architecture]

  The response is converted into a KjNode tree by a single function - not thousands of lines of code - and the KjNode tree is rendered to JSON ASCII text
  by a single function.

  So, this is basically what the "New Way" is about.

  The idea is to always use mngoBackend when possible and if not, use the "New Way".
  In the future I'd love to merge all the ngsi-ld services to the "New Way".
  Perhaps even all the APIv2 services ...

## Contexts
  The orionld broker has a complete implementation of NGSI-LD @contexts, including creation and servicing of contexts,
  which is necessary when inline contexts are used.
  @context handling includes expanding entity types, attribute names, and attribute values (if applicable).
  The *real* name of an attribute (or entity type) is the expanded name, and that is what is stored in the database.
  Attribute values (only string values or string values inside arrays) are expanded if the @context says that they should be.
  The mechanism for attribute values expansion is that the @type of the @context object has the value "@vocab".
  Prefix-expansion is handled as well.

  What is missing is:

    1. The newest decision in ETSI, about continuing the search of hits of an expansion/contraction until the end, and use the
       very LAST hit - this is how JSON-LD works and NGSi-LD will work like this as well.
    2. Store all key-values of the contexts as hash-tables to make lookups more efficient.

  The implementation of @context handling has become a bit of a patchwork (it's very complex) and the idea is to implement the two missing features
  in a complete rewrite of the @context handling functionality.

  This rewrite will be started after the Berlin summit in October 2019.

## Notifications
  As most services reuse old orion code (mongoBackend), notifications, come for free.
  For whenever mongoBackend *cannot* be used, an alternative, more lightweight, notification mechanism has been implementated.

### New Notification mechanism
  orion's notification mechanism consists of a threadpool of notification workers and a message queue between the service threads and the notification threads.
  It is important to hand over the work to some other thread, so that the originator of the request doesn't have to wait for all the notifications before receiving
  the response of the request. Especially as the broker awaits responses for the notifications and those may timeout ...
  So, a pool of threads is one idea on how to solve this problem.

  However, I found another way of doing this, many many times simpler and thus a lot faster.

  Some background is necessary.
  Microhttpd (MHD) is the library that takes care of TCP connections, reading requests, creating threads to treat the requests, and responding to the requests.
  Whenever a request comes in, MHD calls the callback function that orion/orionld asked MHD to use during setup. MHD maintains a threadpool (if we've asked MHD to, and we have)
  and makes sure every request runs in a separate thread.
  After treating the request, orion/orionld calls an MHD function to respond to the request and when that is done, MHD prepares to reuse the thread for a nextcoming request.
  But, before that, another callback is called, one that informs the MHD user (orion/orionld in our case) that the request is DONE.
  This callback is called after responding to the request, of course.

  So, the idea I had was to not let this thread die (and instead use a different threadpool to send the notifications).
  Let's just use that thread that is already created!
  If we have 20 notifications to send, we send all 20 at once. After that, we await the response to the 20 notifications and we update subscriptions in case of errors, etc.
  It is done like this to avoid to delay the reception of the notification for all receptors.
  
  So, for notifications, no threadpool is necessary.
  No message queues are necessary.
  We only need a single thread and WE ALREADY HAVE IT.

  A LOT of overhead is saved with this idea.

  When the "New Way" is fully implemented, *all* ngsi-ld notifications will be sent this way.

### Still Missing
  * Use the new ngsi-ld Q filter and all other filtering mechanisms
  * Implement for all services

## Forwarding
  Just like notifications, many of the services will have forwarding for free.
  Notifications are implemented inside mongoBackend, whereas forwarding is split between mongoBackend and two APIv1 service routines: postUpdateContext and postQueryContext.
  Implementing a new service, one can either use the already existing stuff by calling mongoBackend directly, or, call one of those two APIv1 service routines.
  If mongoBackend is called directly, notifications come for free, while if postUpdateContext/postQueryContext is called, both notifications and forwarding comes for free.

  However, this is the old orion forwarding, not ngsi-ld forwarding.
  Forwarding for ngsi-ld is still under discussion and will probably not be functional in orionld until the end of 2020.

### Still Missing
  * All of it

## Query Filters
  The new ngsi-ld Query Filter is a lot more sophisticated than the one used in APIv2.
  So sophisticated that it cannot be done without converting the Query Filter into a tree.
  I implemented this in September 2019 and it is in use for the query entities service.
  It is NOT in use for subscriptions, still, and that is a task that needs to be implemented.
  
### Still Missing
  * Use the new Query Filter for subscriptions.

## Merging in Telefonica's Orion repo
  The idea is to implement plug-ins for Orion and add the LD part as a plug-in.
  The advantages with this idea are many and important:
  - The FIWARE Foundation keeps the IP of orionld
  - The	FIWARE Foundation has FULL control of the source code, without depending on Telefonica
  - The FIWARE Foundation can make new releases of the lug-in whenever necessary
  - etc.

### Time Estimation
  What needs to be done:

* Merge the newest orion repo into the orionld repo (2 months)
* Implement plug-ins for Orion (2 months)
* Convert the current orionld implementation into a plug-in library (2 months)
  
  
## Services
### POST /ngsi-ld/v1/entities
  The entity creation service is 100% implemented.
  MongoBackend is used, so notifications are supported, but no forwarding.

#### URI Parameters
  None

#### Still Missing
  Nothing that we know of



### GET /ngsi-ld/v1/entities
  The entity query service is 100% implemented.
  `postQueryContext()` is used, so both notifications and APIv1 forwarding is supported.
  
#### URI Parameters
  The supported URI parameters are:
  * id                  (comma spearated list of entity identifiers)
  * idPattern           (regular expression to match entity identifiers)
  * type                (comma spearated list of entity types - they are expanded)
  * q                   (ngsi-ld style query filter)
  * attrs               (comma spearated list of attribute names - they are expanded)
  * geometry            (geofencing)
  * coordinates         (geofencing)
  * georel              (geofencing)
  * maxDistance         (geofencing)
  * options=keyValues   (short form only showing attribute name and value)
  
#### Still Missing
  * URI param 'csf'
  * URI param 'geoproperty'



### GET /ngsi-ld/v1/entities/{entity-id}
  The "single entity query" service is 100% implemented.
  MongoBackend is used, so no forwarding.

#### Still Missing
  Nothing that we know of

#### URI Parameters
  The supported URI parameters are:
  * attrs               (comma spearated list of attribute names - only those in this list are included in the response)
  * options=keyValues   (short form only showing attribute name and value)
  


### DELETE /ngsi-ld/v1/entities/{entity-id}
  The "delete entity" service is 100% implemented.

#### URI Parameters
  None

#### Still Missing
  Nothing that we know of



### POST /ngsi-ld/v1/entities/{entity-id}/attrs
  The "Append Entity Attributes" service consists of two different service, selected with the URI param `options`.
  * 1. If `options` contains the key-word *noOverwrite*, then it's a pure append service mode
  * 2. If not, then it's an "upsert" service mode

#### URI Parameters
  The supported URI parameters are:
  * options=noOverwrite

#### Pure Append Mode
  Mode 1 (pure append) is fully implemented. MongoBackend is used, so, notifications are supported.
  Forwarding is not.

#### Upsert Mode
  The "upsert" mode replaces attributes that already exist - like a PUT behaviour.
  Unfortunately I implemented a PATCH behaviour instead and I need to fix this.

  This is where I started the "New Way" implementation and this part of the service is implemented without mongoBackend.
  It works just fine, and the implementation is so much cleaner and faster that if I had used mngoBackend (I tried but I wasn't able).
  I also implemented subscriptions, the "New Way" and it works just fine, except that lots of bells and whistles still need
  to be implemented for "New Way" subscriptions.

#### Still Missing
  * Rewrite the "Upsert Mode" to use a PUT behaviour and not PATCH.



### PATCH /ngsi-ld/v1/entities/{entity-id}/attrs
  The "Update Entity Attributes" service is implemented, using mongoBackend, so notifications are supproted, but not forwarding.
  
#### URI Parameters
  None

#### Still Missing
  Nothing that we know of



### PATCH /ngsi-ld/v1/entities/{entity-id}/attrs/{attr-name}
  The "Partial Attribute Update" service has NOT been implemented, just some error handling.
  Jorge is working on this as of Oct 2019

#### Still Missing
  * All of it

#### URI Parameters
  Node



### DELETE /ngsi-ld/v1/entities/{entity-id}/attrs/{attr-name}
  The "Delete Entity Attribute" service has been implemented, just without taking `datesetId` into consideration.

#### URI Parameters
  * deleteAll  (all attribute instances are deleted, not just the one matching the dataSetId)
  * datasetId  (Specifies the datasetId of the dataset to be deleted)

#### Still Missing
  * Everything about `datasetId`



### POST /ngsi-ld/v1/csourceRegistrations
  The "Register Context Source" service is 100% implemented.

#### URI Parameters
  None

#### Still Missing
  Nothing that we know of



### GET /ngsi-ld/v1/csourceRegistrations
  The "Query Context Source Registrations" service is 100% implemented.
  
#### URI Parameters
  The supported URI parameters so far are:
  * id
  * type
  * idPattern
  * attrs


#### Still Missing
  * URI param: q
  * URI param: csf
  * URI param: georel
  * URI param: geometry
  * URI param: coordinates
  * URI param: geopreperty
  * URI param: timeproperty
  * URI param: timerel
  * URI param: time
  * URI param: endTime



### GET /ngsi-ld/v1/csourceRegistrations/{registrationId}
  The "Retrieve Context Source Registration" service has been implemented, with the exception that the error responses do not follow the spec.

#### URI Parameters
  None

#### Still Missing
  Nothing that we know of



### PATCH /ngsi-ld/v1/csourceRegistrations/{registrationId}
  The "Update Context Source Registration" service has not been implemented.

#### URI Parameters
  None

#### Still Missing
  * All of it



### DELETE /ngsi-ld/v1/csourceRegistrations/{registrationId}
  The "Delete Context Source Registration" service has not been implemented.

#### URI Parameters
  None

#### Still Missing
  * All of it



### POST /ngsi-ld/v1/subscriptions
  The "Create Subscription" service is 100% implemented.

#### Still Missing
  Nothing that we know of

#### URI Parameters
  None

### GET /ngsi-ld/v1/subscriptions
  The "Query Subscriptions" service is 100% implemented.

#### Still Missing
  Nothing that we know of

#### URI Parameters
  * limit
  * offset



### GET /ngsi-ld/v1/subscriptions/{subscriptionId}
  The "Retrieve Subscription" service is 100% implemented.
  
#### Still Missing
  Nothing that we know of
  
#### URI Parameters
  None



### PATCH /ngsi-ld/v1/subscriptions/{subscriptionId}
  The "Update Subscription" service has not been implemented.
  
#### Still Missing
  * All of it
  
#### URI Parameters
  None



### DELETE /ngsi-ld/v1/subscriptions/{subscriptionId}
  The "Delete Subscription" service is 100% implemented.
  
#### Still Missing
  Nothing that we know of
  
#### URI Parameters
  None



### POST /ngsi-ld/v1//ngsi-ld/v1/entityOperations/delete
  The "Batch Delete" service is 100% implemented.

#### Still Missing
  Nothing that we know of

#### URI Parameters
  None



### POST /ngsi-ld/v1/entityOperations/upsert
  The "Batch Upsert" service has been implemented for the "update" case.
  The "replace" case is under development and assumed to be finished during October 2019.

  As mongoBackend is used, we get notifications for free.
  To get also forwarding for free, we'd need to use the APIv1 service function "postUpdateContext()", which would be easy enough to do.
  However, V1 forwarding isn't too interesting - see "Forwarding".

#### Still Missing
  * The "replace" case

#### URI Parameters
  None



### POST /ngsi-ld/v1//ngsi-ld/v1/entityOperations/create
  Gabriel will implement this service in November 2019

#### Still Missing
  * All of it

#### URI Parameters
  None
  


### POST /ngsi-ld/v1//ngsi-ld/v1/entityOperations/update
  Gabriel will implement this service in December 2019

#### Still Missing
  * All	of it
  
#### URI Parameters
  None



### POST /ngsi-ld/v1/csourceSubscriptions
  The "Create Context Source Registration Subscription" has not been implemented.
  No plans (for now, at least) to implement this in the near future (before end of 2020)

#### Still Missing
  All of it

#### URI Parameters
  None



### GET /ngsi-ld/v1/csourceSubscriptions
  The "Query Context Source Registration Subscription" has not been implemented.
  No plans (for now, at least) to implement this in the near future (before end of 2020)

#### Still Missing
  All of it

#### URI Parameters
  None



### GET /ngsi-ld/v1/csourceSubscriptions/{subscriptionId}
  The "Retrieve Context Source Registration Subscription" has not been implemented.
  No plans (for now, at least) to implement this in the near future (before end of 2020)

#### Still Missing
  All of it

#### URI Parameters
  None



### PATCH /ngsi-ld/v1/csourceSubscriptions/{subscriptionId}
  The "Update Context Source Registration Subscription" has not been implemented.
  No plans (for now, at least) to implement this in the near future (before end of 2020)

#### Still Missing
  All of it

#### URI Parameters
  None



### DELETE /ngsi-ld/v1/csourceSubscriptions/{subscriptionId}
  The "Delete Context Source Registration Subscription" has not been implemented.
  No plans (for now, at least) to implement this in the near future (before end of 2020)

#### Still Missing
  All of it

#### URI Parameters
  None


