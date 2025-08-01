# Flows index

This document provides an index of the different diagrams used in the development documentation.

Note there is some gaps in the number sequences (for instance, between MB-19 an MB-23). This is due
some diagrams were removed due to changes in Orion Context Broker functionality and it's fine.

Related to request management (RQ management):

* [RQ-01: Reception of a request](sourceCode.md#flow-rq-01)
	* Continues in RQ-02
* [RQ-02: Treatment of a request](sourceCode.md#flow-rq-02)
	* Continues from RQ-01
	* Continues in PP-*
	* Continues in MB-*

Related to parsing (PP prefix):

* [PP-02: Parsing a text payload](sourceCode.md#flow-pp-02)
	* Continues from RQ-02
* [PP-03: Parsing an NGSIv2 payload](jsonParseV2.md#flow-pp-03)
	* Continues from RQ-02

Related to forwarding (FW prefix):

* [FW-01: Forward an update to Context Providers](cprs.md#flow-fw-01)
	* Continues in MB-01 or MB-02
	* Continues in FW-02
* [FW-02: `updateForward()` function detail](cprs.md#flow-fw-02)
	* Continues from FW-01
	* Continues in PP-01
* [FW-03: Forward a query to Context Provider](cprs.md#flow-fw-03)
	* Continues in MB-07
	* Continues in FW-04
* [FW-04: `queryForward()` function detail](cprs.md#flow-fw-04)
	* Continues from FW-03
	* Continues in PP-01
	
Related to mongoBackend logic (MB and MD prefixes):

* [MB-01: mongoUpdate - update/replace - entity found](mongoBackend.md#flow-mb-01)
    * Continues from RQ-02 or FW-01
    * Continues in MD-01
    * Continues in MD-02
* [MB-02: mongoUpdate - update/replace - entity not found](mongoBackend.md#flow-mb-02)
    * Continues from RQ-02 or FW-01   
    * Continues in MD-02 
* [MB-03: mongoUpdate - append/appendStrict - existing entity](mongoBackend.md#flow-mb-03)
    * Continues from RQ-02
    * Continues in MD-01
    * Continues in MD-02 
* [MB-04: mongoUpdate - append/appendStrict - new entity](mongoBackend.md#flow-mb-04)
    * Continues from RQ-02
    * Continues in MD-01
* [MB-05: mongoUpdate - delete - not remove entity](mongoBackend.md#flow-mb-05)
    * Continues from RQ-02
    * Continues in MD-01
    * Continues in MD-02  
* [MB-06: mongoUpdate - delete - remove entity](mongoBackend.md#flow-mb-06)
	* Continues from RQ-02 
* [MB-07: mongoQueryContext](mongoBackend.md#flow-mb-07)
	* Continues from RQ-02 or FW-03
* [MB-08: mongoEntityTypes](mongoBackend.md#flow-mb-08)
	* Continues from RQ-02 
* [MB-09: mongoEntityTypesValues](mongoBackend.md#flow-mb-09)
	* Continues from RQ-02
* [MB-10: mongoAttributesForEntityType](mongoBackend.md#flow-mb-10)
	* Continues from RQ-02
* [MB-11: mongoCreateSubscription](mongoBackend.md#flow-mb-11)
	* Continues from RQ-02
* [MB-12: mongoUpdateSubscription](mongoBackend.md#flow-mb-12)
	* Continues from RQ-02
* [MB-13: mongoGetSubscription](mongoBackend.md#flow-mb-13)
	* Continues from RQ-02
* [MB-14: mongoListSubscriptions](mongoBackend.md#flow-mb-14)
	* Continues from RQ-02
* [MB-15: mongoUnsbuscribeContext](mongoBackend.md#flow-mb-15)
	* Continues from RQ-02
* [MB-23: mongoRegistrationGet](mongoBackend.md#flow-mb-23)
	* Continues from RQ-02
* [MB-24: mongoRegistrationsGet](mongoBackend.md#flow-mb-24)
	* Continues from RQ-02
* [MB-25: mongoRegistrationCreate](mongoBackend.md#flow-mb-25)
	* Continues from RQ-02
* MB-26: mongoRegistrationUpdate (*pending*)
	* Continues from RQ-02
* [MB-27: mongoRegistrationDelete](mongoBackend.md#flow-mb-27)
	* Continues from RQ-02
* [MD-01: `processSubscriptions()` function detail](mongoBackend.md#flow-md-01)
	* Continues from MB-01, MB-03, MB-04 or MB-05
	* Continues in NF-01 or NF-03  
* [MD-02: `searchContextProviders()` function detail](mongoBackend.md#flow-md-02)
	* Continues from MB-01, MB-02, MB-03 or MB-05

Related to notifications (NF prefix):

* [NF-01: HTTP Notification on entity-attribute update/creation without thread pool](sourceCode.md#flow-nf-01)
  * Continues from MD-01
* [NF-01b: MQTT Notification on entity-attribute update/creation without thread pool](sourceCode.md#flow-nf-01b)
  * Continues from MD-01
* [NF-03: HTTP Notification on entity-attribute update/creation with thread pool](sourceCode.md#flow-nf-03)
  * Continues from MD-01
* [NF-03b: MQTT Notification on entity-attribute update/creation with thread pool](sourceCode.md#flow-nf-03b)
  * Continues from MD-01

Related to the subscription cache (SC prefix):

* [SC-01: Subscription cache refresh](subscriptionCache.md#flow-sc-01)
* [SC-02: Subscription propagation in active-active configuration](subscriptionCache.md#flow-sc-02) 
