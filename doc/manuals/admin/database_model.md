# <a name="top"></a>Data model

* [Introduction](#introduction)
* [entities collection](#entities-collection)
* [registrations collection](#registrations-collection)
* [csubs collection](#csubs-collection)

## Introduction

Normally you don't need to access MongoDB directly as Orion Contex
Broker uses it transparently. However, for some operations (e.g. backup,
fault recovery, etc.) it is useful to know how the database is
structured. This section provides that information.

In the case you need to access database directly, be very careful when manipulation it, as some actions could be irreversible ([doing a
backup](database_admin.md#backing-up-and-restoring-database) at the beginning
it's a good idea).

Orion Context Broker uses four collections in the database, described in
the following subsections.

[Top](#top)

## entities collection

The *entities* collection stores information about NGSI entities. Each
document in the collection corresponds to an entity.

Fields:

-   **\_id** stores the EntityID, including the ID itself and type.
    Given that we use \_id for this, we ensure that EntityIDs
    are unique. The JSON document for this field includes:
    -   **id**: entity NGSI ID
    -   **type**: entity NGSI type
    -   **servicePath**: related with [the service
        path](../orion-api.md#service-path) functionality.
-   **attrs** is an keymap of the different attributes that have been
    created for that entity. The key is generated with the attribute
    name (changing "." for "=", as "." is not a valid character in
    MongoDB document keys).
    Each element in the map has the following information:
    -   **type**: the attribute type
    -   **value**: the attribute value (for those attribute that has
        received at least one update). Up to version 0.10.1, this value
        is always a string, but in 0.11.0 this value can be also a JSON
        object or JSON vector to represent an structured value (see
        section about [attribute representation in Orion API
        specification](../orion-api.md#json-attribute-representation)).
    -   **md** (optional): custom metadata. This is a keymap of metadata
        objects. The key is generated with the metadata
        name (changing "." for "=", as "." is not a valid character in
        MongoDB document keys), e.g. a metadata with name "m.x" will
        use the key "m=x". The object value of each key has two
        fields: **type** and **value** (of the metadata).
    -   **mdNames**: an array of strings. Its elements are the names of the
        metadata of the attribute. Here the "." to "="
        replacement is not done.
    -   **creDate**: the timestamp (as a floating point number, meaning seconds with milliseconds)
        corresponding to attribute creation (as a consequence of append).
    -   **modDate**: the timestamp (as a floating point number, meaning seconds with milliseconds)
        corresponding to last attribute update. It matches creDate if the attribute has
        not been modified after creation.
-   **attrNames**: an array of strings. Its elements are the names of the
    attributes of the entity (without IDs). In this case, the "." to "="
    replacement is not done.
-   **creDate**: the timestamp (as a floating point number, meaning seconds with milliseconds)
    corresponding to entity creation date (as a consequence of append).
-   **modDate**: the timestamp (as a floating point number, meaning seconds with milliseconds)
    corresponding to last entity update. Note that it is usually the same as a
    modDate corresponding to at least one of the attributes (not always: it will
    not be the same if the last update was a DELETE operation). It matches creDate
    if the entity has not been modified after creation.
-   **location** (optional): geographic location of the entity, composed
    of the following fields:
    -   **attrName**: the attribute name that identifies the geographic
        location in the attrs array
    -   **coords**: a GeoJSON representing the location of the entity. See
        below for more details.
-   **lastCorrelator**: value of the root correlator in the last
    update request on the entity. Used by the self-notification loop protection
    logic. By *root correlator* we mean the value of the `Fiware-Correlator` request header
    in the update request without any suffix. Eg. the root correlator
    for `Fiware-Correlator: f320136c-0192-11eb-a893-000c29df7908; cbnotif=32`
    is `f320136c-0192-11eb-a893-000c29df7908`.
-   **expDate** (optional): expiration timestamp (as a Date object) for the
    entity. Have a look to the [transient entities functionality](../orion-api.md#transient-entities)
    for more detail.  

Regarding `location.coords` in can use several formats:

* Representing a point:

```
{
  "type": "Point",
  "coordinates": [ -3.691944, 40.418889 ]
}
```

* Representing a line:

```
{
  "type": "LineString",
  "coordinates": [ [ 10, 0], [0, 10] ]
}
```

* Representing a polygon:

```
{
  "type": "Polygon",
  "coordinates": [ [ [ 10, 0], [0, 10], [0, 0], [10, 0] ] ]
}
```

* Finally, `location.coords` could hold an arbitrary JSON object, representing a location
  in [GeoJSON](http://www.macwright.org/2015/03/23/geojson-second-bite.html) format. Arbitrary
  GeoJSON can be used with the geo:json attribute type and it is up to the user to introduce
  a valid object. Note that the three above cases are actually GeoJSON representation for
  "fixed" cases.

Note that coordinate pairs use the longitude-latitude order, which is opposite to the order used
in the [Geographical Queries](../orion-api.md#geographica-queries). This is due to the internal
[MongoDB geolocation implementation](http://docs.mongodb.org/manual/tutorial/query-a-2dsphere-index/),
(which is based in GeoJSON) uses longitude-latitude order. However, other systems closer
to users (e.g. GoogleMaps) use latitude-longitude format, so we have used the latter for the Geographical Queries API.

Example document:

```
 {
   "_id":
       "id": "E1",
       "type": "T1",
       "servicePath": "/"
   },
   "attrs": {
       "A1": {
           "type": "TA1",
           "value": "282",
           "creDate" : 1389376081.8471954,
           "modDate" : 1389376120.2154321,
           "md" : {
              "customMD1": {
                 "type" : "string",
                 "value" : "AKAKA"
              },
              "customMD2": {
                 "type" : "integer",
                 "value" : "23232"
              }
           },
           "mdNames": [ "customMD1", "customMD2" ]
       },
       "A2()ID101": {
           "type": "TA2",
           "value": "176",
           "creDate" : 1389376244.6651231,
           "modDate" : 1389376244.6651231
       },
       "position": {
           "type": "location",
           "value": "40.418889, -3.691944",
           "creDate" : 1389376244.6651231,
           "modDate" : 1389376244.6651231
       }
   },
   "attrNames": [ "A1", "A2", "position" ],
   "creDate": 1389376081.8471954,
   "modDate": 1389376244.6651231,
   "location": {
       "attrName": "position",
       "coords": {
           "type": "Point",
           "coordinates": [ -3.691944, 40.418889 ]
       }
   },
   "lastCorrelator" : "aa01d6c6-4f7e-11e7-8059-000c29173617"
 }
```
[Top](#top)

## registrations collection

The *registrations* collection stores information about
registrations. Each document in the collection corresponds to a
registration.

Fields:

-   **\_id** is the registration ID (the value that is provided to the
    user to update the registration). Given that we use \_id for this,
    we ensure that registration IDs are unique and that queries by
    registration IDs will be very fast (as there is an automatic default
    index in \_id).
-   **format**: the format to use to send forwarded requests.
    For NGSIv1 format, use **JSON** as value for `format`.
    For NGSIv2, as of today, only **normalized** format is supported.
-   **servicePath**: related with [the service
    path](../orion-api.md#service-path) functionality.
-   **status** (optional): either `active` (for active registrations) or `inactive` (for inactive registrations).
    The default status (i.e. if the document omits this field) is "active".
-   **description** (optional): a free text string describing the registration. Maximum length is 1024.
-   **expiration**: this is the timestamp (as integer number, meaning seconds) for which the
    registration expires.
-   **fwdMode**: the forwarding mode supported by the provider, either: `all`, `query`, `update` or `none`.
    If omitted (Orion versions previous to 2.6.0), `all` is assumed.
-   **contextRegistration**: is an array whose elements contain the
    following information:
    -   **entities**: an array containing a list of
        entities (mandatory). The JSON for each entity contains **id** (string),
        **type** (string), **isPattern** (bool) and **isTypePattern** (bool) (*).
    -   **attrs**: an array containing a list of attributes (optional).
        The JSON for each attribute contains **name** and **type**.
    -   **providingApplication**: the URL of the providing application
        for this registration (mandatory)

(*) Versions previous to Orion 4.3.0 use `isPattern` as strings (`"true"` or `"false"`). Orion suppports
reading this as strings (legacy) or as bool (current) but always stores them as bool (`true` or `false`).

Example document:

```
 {
   "_id": ObjectId("5149f60cf0075f2fabca43da"),
   "format": "JSON",
   "fwdMode": "all",
   "expiration": 1360232760,
   "contextRegistration": [
       {
           "entities": [
               {
                   "id": "E1",
                   "type": "T1",
                   "isPattern": false
               },
               {
                   "id": "E2",
                   "type": "T2",
                   "isPattern": false
               }
           ],
           "attrs": [
               {
                   "name": "A1",
                   "type": "TA1"
               },
               {
                   "name": "A2",
                   "type": "TA2"
               }
           ],
           "providingApplication": "http://foo.bar/notif"
      },
      "status": "active"
  ]
 }
```
[Top](#top)

## csubs collection

The *csubs* collection stores information about context subscriptions.
Each document in the collection corresponds to a subscription.

Fields:

-   **\_id** is the subscription ID (the value that is provided to the
    user to update and cancel the subscription). Given that we use \_id
    for this, we ensure that subscription IDs are unique and that
    queries by subscription IDs are very fast (as there is an automatic
    default index in \_id).
-   **servicePath**: related with [the service
    path](../orion-api.md#service-path) functionality. This is the service path
    associated to the query "encapsulated" by the subscription. Default
    is `/#`.
-   **expiration**: this is the timestamp (as integer number, meaning seconds) on which the
    subscription expires. For permanent subscriptions
    an absurdly high value is used (see PERMANENT_SUBS_DATETIME in the source code).
-   **lastNotification**: the time (as integer number, meaning seconds) when last notification was sent. This
    is updated each time a notification is sent, to avoid violating throttling.
-   **throttling**: minimum interval between notifications. 0 or -1 means no throttling.
-   **reference**: the URL for notifications, either HTTP or MQTT
-   **topic**: MQTT topic (only in MQTT notifications)
-   **qos**: MQTT QoS value (only in MQTT notifications)
-   **retain**: MQTT retain value (only in MQTT notifications)
-   **entities**: an array of entities (mandatory). The JSON for each
    entity contains **id** (string), **type** (string), **isPattern** (bool)  **isTypePattern** (bool) (*).
-   **attrs**: an array of attribute names (strings) (optional).
-   **blacklist**: a boolean field that specifies if `attrs` has to be interpreted
    as a whitelist (if `blacklist` is equal to `false` or doesn't exist) or a
    blacklist (if `blacklist` is equal to `true`).
-   **onlyChanged**: a boolean field that specifies if only attributes that change has 
    to be included in notifications (if onlyChanged is equal to true) or not (if 
    onlyChanged is equal to false or doesn't exist).
-   **metadata**: an array of metadata names (strings) (optional).
-   **conditions**: a list of attributes that trigger notifications.
-   **expression**: an expression used to evaluate if notifications has
    to be sent or not when updates come. It may be composed of the following
    fields: q, mq, georel, geometry and/or coords (optional)
-   **count**: the number of notifications sent associated to
    the subscription.   
-   **format**: the format to use to send notification, possible values are **normalized**, **keyValues**, **simplifiedNormalized**, **simplifiedKeyValues** and **values**.
-   **status**: either `active` (for active subscriptions), `inactive` (for inactive subscriptions) or
    `oneshot` (for [oneshot subscriptions](../orion-api.md#oneshot-subscriptions)). Note that Orion API consider additional states (e.g. `expired`)
    but they never hit the DB (they are managed by Orion).
-   **statusLastChange**: last time status was updated (as decimal number, meaning seconds and fractions of seconds).
    This is mainly to be used by the subscription cache update logic (so status in updated in DB from cache only if it is newer).
-   **description** (optional field): a free text string describing the subscription. Maximum length is 1024.
-   **timeout** this field configures the maximum time the subscription waits for the response for http 
notifications. It is a number between 0 and 1800000. If defined to 0 or omitted, the default timeout will be used.
-   **custom**: a boolean field to specify if this subscription uses customized notifications (a functionality in the Orion API).
    If this field exist and its value is "true" then customized notifications are used and the `headers`, `qs`, `method` and
    `payload` fields are taken into account.
-   **headers**: optional field to store the HTTP headers keymap for notification customization functionality.
-   **qs**: optional field to store the query parameters keymap for notification customization functionality.
-   **method**: optional field to store the HTTP method for notification customization functionality.
-   **payload**: optional field to store the payload for notification customization functionality. If
    its value is `null` means that no payload has to be included in the notification. If its value is `""` or if
    the field is omitted, then the NGSIv2 normalized format is used.
-   **json**: optional field to store a JSON object or array to generated JSON-based payload for
    notification customization functionality in the Orion API. More detail of this functionality [here](../orion-api.md#json-payloads)
-   **ngsi**: optional field to store a NGSI patching object for notification customization functionality
    in the Orion API. More detail of this functionality [here](../orion-api.md#ngsi-payload-patching).
    The value of this field is an object with a `attrs` key which value is a simplified version of
    `attrs` in [the entities collection](#entities-collection).
-   **lastFailure**: the time (as integer number, meaning seconds) when last notification failure occurred.
    Not present if the subscription has never failed.
-   **lastFailureReason**: text describing the cause of the last failure.
    Not present if the subscription has never failed.
-   **lastSuccess**: the (as integer number, meaning seconds) time when last successful notification occurred.
    Not present if the subscription has never provoked a successful notification.
-   **lastSuccessCode**: HTTP code (200, 400, 404, 500, etc.) returned by receiving endpoint last
    time a successful notification was sent.
    Not present if the subscription has never provoked a successful notification.
-   **maxFailsLimit**: An optional field used to specify the maximum limit of connection attempts, so when that number of failing notifications is reached, then the subscription passes automatically to inactive state.
-   **failsCounter**: the number of consecutive failing notifications associated to the subscription. This is increased by one each time a notification attempt fails. It is reset to 0 if a notification attempt successes.
-   **altTypes**: array with a list of alteration types associated to the subscription. If the field is not included, a default is assumed (check [this document](../orion-api.md#subscriptions-based-in-alteration-type)).
-   **covered**: a boolean field that specifies if all `attrs` have to be included in notifications (if value is true)
    or only the ones existing in the triggering entity (if value is false or field is omitted).
    More information in [covered subscription section in Orion API specification](../orion-api.md#covered-subscriptions).
-   **notifyOnMetadataChange**: if `true` metadata is considered part of the value of the attribute regarding subscription triggering. If `false` metadata is not considered part of the value of the attribute regarding subscription triggering. Default behaviour (if omitted) is the one for `true`.

(*) Versions previous to Orion 4.3.0 use `isPattern` as strings (`"true"` or `"false"`). Orion suppports
reading this as strings (legacy) or as bool (current) but always stores them as bool (`true` or `false`).

Example document:

```
{
        "_id" : ObjectId("5697d4d123acbf5e794ab031"),
        "expiration" : NumberLong(1459864800),
        "reference" : "http://localhost:1234",
        "servicePath" : "/",
        "entities" : [
                {
                        "id" : ".*",
                        "type" : "Room",
                        "isPattern" : true,
                        "isTypePattern": false
                }
        ],
        "attrs" : [
                "humidity",
                "temperature"
        ],
        "conditions" : [ "temperature" ],
        "expression" : {
                "q" : "temperature>40",
                "mq" : "temperature.accuracy<1",
                "geometry" : "",
                "coords" : "",
                "georel" : ""
        },
        "format" : "JSON",
        "status" : "active",
        "statusLastChange" : 1637226173.6940024
}
```
[Top](#top)
