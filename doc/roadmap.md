# Orion-LD Context Broker Roadmap

This product is a FIWARE Generic Enabler.
If you would like to learn about the overall Roadmap of FIWARE, please check out the section "Roadmap" on the FIWARE Catalogue.

## Short Term
The following list of features are planned to be addressed in the short term (~6 months):
* Complete support for LanguageProperty attributes, including in notifications
* "NGSI-LD native" notifications - not using mongoBackend, for API endpoints that use the new mongo driver (mongoc)
  - PATCH /entities/{entityId}
  - PUT /entities/{entityId}
  What's missing is the treatment of 'q' and 'geoQ' for the notifications provoked by those two endpoints
* Memory management for containers - for a better experience with Orion-LD inside containers
* Safe connection to MongoDB Server (-dbSSL and -dbAuthDb CLI arguments that Orion now implements)
* Reimplement as many API endpoints as possible to use the new mongoc driver
* NGSI-LD Forwarding - this is big - won't be complete in only 6 months

For Medium/Long term planning, please refer to the (Issue #280)[https://github.com/FIWARE/context.Orion-LD/issues/280]
