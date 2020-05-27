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

For a detailed overview of what's happening in the Orion-LD repo, plese refer to the (Issue #280)[https://github.com/FIWARE/context.Orion-LD/issues/280]

## Short term
The following list of features are planned to be addressed in the short term:
* Smarter memory allocation scheme for rendering response payloads
* JMeter performance tests
* Support and Installation Guide for Ubuntu 20.04 LTS - to be made new default distro, some day
* Remove all calls to time(NULL) - use orionldState.timestamp.tv_sec instead
* Issue #463 - Multiple instances of the same entity in Batch Operations
* Make sure all Batch Ops return the correct response according to the newest NGSI-LD draft
* Make sure the 'old mongo semaphore' is correctly used in libdb functions
* Password and QoS for MQTT subscriptions
* options=expanded => return response in expanded form (for all requests?)
* NGSI-LD Q filter for subscriptions


## Medium term
The following list of features is for the first real release (1.0.0) that is planned for the end of 2020:

* Temporal Representation of Entities
* Mode support for datasetId, probably not 100%
* Attribute Domains
* Give error for non-supported URI param     - request dependent
* Give error for non-supported HTTP hraders  - request dependent
* Use python 3.8 instead of 2.7
* Notifications using WebSockets

## Long term
The following list of features are possible ideas for the future.
* Notifications using Fast RTSP
* Notifications using OPC-UA
* Support for Context Source Subscriptions
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
* Implement all NGSI-LD Ops with new mongo C driver - without the Orion library "mongoBackend" - clean DB interface
* Complete API over WebSockets
* More ways to connect - direct connection with TCP sockets?
* Rewrite all APIv2 w/o mongoBackend (24 services) - using new mongo C driver and Orion-LD architecture
