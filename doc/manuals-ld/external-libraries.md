# External Libraries
The LD part of Orion-LD depends on the following external libraries:
* microhttpd
* mongo client library (C++ Legacy driver)
* libcurl
* libuuid
* kbase
* klog
* kalloc
* khash
* kjson

The "heart" of the linked-data extension of orionld is the [kjson](https://gitlab.com/kzangeli/kjson) library.
kjson takes care of the parsing of incoming JSON and transforms the textual input as a tree of KjNode structs.

A KjNode tree is basically a linked list of attributes (id and type are some kind of attributes of the entity, right?)
that can have children, the attribute metadata.
In NGSI-LD the attribute metadata takes another name, namely Property-of-Property, or Property-of-Relationhsip, or ...

Trees based on the KjNode structure isn't just for the incoming payload, but also for:
* outgoing payload
* context cache
* intermediate storage format for the DB abstaction layer
* etc.
