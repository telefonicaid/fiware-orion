# Orion-LD Context Broker Roadmap

This product is a FIWARE Generic Enabler.
If you would like to learn about the overall Roadmap of FIWARE, please check out the section "Roadmap" on the FIWARE Catalogue.

## Introduction
This section elaborates on proposed new features or tasks which are expected to
be added to the product in the foreseeable future. There should be no assumption
of a commitment to deliver these features on specific dates or in the order
given. The development team will be doing its very best to follow the proposed
dates and priorities, but please bear in mind that plans to work on a given
feature or task may be revised. All information is provided as a general
guidelines only, and this section may be revised to provide newer information at any time.

## Short term
The following list of features are planned to be addressed in the short term,
and incorporated into the coming Alpha release 2 of Orion-LD:
* Issue #302 (PATCH /ngsi-ld/v1/entities/{entityId}/attrs)
* Check incoming payload for any null value of any field and return error if found, or, do whatever the spec states where null is allowed (to remove an attribute for example).
* Contexts to be saved in DB
* Give error for non-supported URI param?
* No @context returned if no payload (and no error)
* Add Ubuntu 19.10 to the list of supported distros
* Add Ubuntu 19.10 SERVER to the list of supported distros
* Implement PATCH /entities/* - Use what is implemented for "POST /entities/*/attrs w/o options=noOverwrite"
* Implement PATCH /entities/*/attrs
* Implement PATCH /ngsi-ld/v1/entities/{entityId}/attrs/{attrName}
* Implement POST /ngsi-ld/v1/entityOperations/create
* Implement POST /ngsi-ld/v1/entityOperations/update
* Implement PATCH /ngsi-ld/v1/subscriptions/{sub-id}
* Implement DELETE /ngsi-ld/v1/cSourceRegistrations/{reg-id}
* Implement GET /ngsi-ld/v1/types - return list of entity types (include types in registrations)
* JMeter performance tests
* options=expanded => return response in expanded form - for all requests?
* If necessary: PUT /ngsi-ld/ex/v1/mode?etsi|orionld
    * 'standard' is default - 100% etsi spec followed
    * if 'orionld', then GET /ngsi-ld/v1/entities?id=XXX is ALLOWED
* Geo-query for arbitrary property (must be named 'location' right now)
* NGSI-LD Q filter for subscriptions
* Support for Ubuntu 20.04 LTS - made new default distro
* The context list request should not return a context in the Link header (nor "the current" context in the payload)

## Medium term
The following list of features is for the first real release (1.0.0) that is planned for the end of 2020:
* Attribute Domains
* POST /ngsi-ld/v1/cSourceSubscriptions
* PATCH /ngsi-ld/v1/cSourceSubscriptions/{csub-id}
* GET /ngsi-ld/v1/cSourceSubscriptions
* GET /ngsi-ld/v1/cSourceSubscriptions/{csub-id}
* DELETE /ngsi-ld/v1/cSourceSubscriptions/{csub-id}
* NGSI-LD Geo Queries
* datasetId

## Long term
The following list of features are possible ideas for the future.
* Still not sure whether any of all this will ever be implemented.
* Update orionld branch to newest orion release branch
* Service Sets in Orion
* Plugins in Orion
  Modify the implementation to:
    * work as a plugin for Orion
    * work standalone
* All REST services that don't imply Forwards nor Notifications can safely be implemented without using mongoBackend
* Q-filter with "database variables" in the Right-Hand-Side of the expressions
* Advanced Notifications - let attributes of one entity trigger notifications for another entity (entities)
    * Seems like "Custom Notifictions" is the way to go here
* Temporal Representation of Entities
    *May need a complete change in database model, and in such case, very very costly
* Rewrite Subscription Cache (current sub-cache depends on TEF Federation Scheme)
    * No sub-cache refresh, instead, clusters that communicate in binary
    * Broker cluster - stored in mongo of each broker in cluster (move all this to separate issue):
* Entity Cache (would be great for "small" brokers)
* Registration Cache
* Registration-Subscription Cache
* Detailed Implementation doc (the source code completely explained)
* NGSI-LD Forwarding
* Implement notifications from scratch - use it everywhere
* Smart Federation (a real interconnected cluster)
* Make mongo C driver work together with the C++ legacy driver
* Implement all NGSI-LD Ops with new mongo C driver - without mongoBackend
* Stop completely using mongoBackend
* clean DB interface
* RTSP
* Websockets
* More ways to connect - direct connection with sockets?
* Rewrite all APIv2 w/o mongoBackend (24 services) - using new mongo C driver
