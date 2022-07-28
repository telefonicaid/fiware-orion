# FIWARE-NGSI v2 (release 2.1) Specification
 
<!-- TOC -->

- [Preface](#preface)
- [Specification](#specification)
    - [Introduction](#introduction)
    - [Terminology](#terminology)
        - [Context data modelling and exchange](#context-data-modelling-and-exchange)
            - [Context Entities](#context-entities)
            - [Context Attributes](#context-attributes)
            - [Context Metadata](#context-metadata)
    - [MIME Types](#mime-types)
    - [JSON Entity Representation](#json-entity-representation)
    - [JSON Attribute Representation](#json-attribute-representation)
    - [Simplified Entity Representation](#simplified-entity-representation)
    - [Partial Representations](#partial-representations)
    - [Datetime support](#datetime-support)
    - [Special Attribute Types](#special-attribute-types)
    - [Builtin Attributes](#builtin-attributes)
    - [Special Metadata Types](#special-metadata-types)
    - [Builtin Metadata](#builtin-metadata)
    - [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name)
    - [Field syntax restrictions](#field-syntax-restrictions)
    - [Attribute names restrictions](#attribute-names-restrictions)
    - [Metadata names restrictions](#metadata-names-restrictions)
    - [Ordering Results](#ordering-results)
    - [Error Responses](#error-responses)
    - [Geospatial properties of entities](#geospatial-properties-of-entities)
        - [Simple Location Format](#simple-location-format)
        - [GeoJSON](#geojson)
    - [Simple Query Language](#simple-query-language)
    - [Geographical Queries](#geographical-queries)
        - [Query Resolution](#query-resolution)
    - [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata)
    - [Oneshot Subscription](#oneshot-subscription)
    - [Notification Messages](#notification-messages)
    - [Custom Notifications](#custom-notifications)
    - [Subscriptions based in alteration type](#subscriptions-based-in-alteration-type)
- [API Routes](#api-routes)
    - [Group API Entry Point](#group-api-entry-point)
        - [Retrieve API Resources [GET /v2]](#retrieve-api-resources-get-v2)
    - [Entities Operations](#entities-operations)
        - [Entities List](#entities-list)
            - [List Entities [GET /v2/entities]](#list-entities-get-v2entities)
            - [Create Entity [POST /v2/entities]](#create-entity-post-v2entities)
        - [Entity by ID](#entity-by-id)
            - [Retrieve Entity [GET /v2/entities/{entityId}]](#retrieve-entity-get-v2entitiesentityid)
            - [Retrieve Entity Attributes [GET /v2/entities/{entityId}/attrs]](#retrieve-entity-attributes-get-v2entitiesentityidattrs)
            - [Update or Append Entity Attributes [POST /v2/entities/{entityId}/attrs]](#update-or-append-entity-attributes-post-v2entitiesentityidattrs)
            - [Update Existing Entity Attributes [PATCH /v2/entities/{entityId}/attrs]](#update-existing-entity-attributes-patch-v2entitiesentityidattrs)
            - [Replace all entity attributes [PUT /v2/entities/{entityId}/attrs]](#replace-all-entity-attributes-put-v2entitiesentityidattrs)
            - [Remove Entity [DELETE /v2/entities/{entityId}]](#remove-entity-delete-v2entitiesentityid)
        - [Attributes](#attributes)
            - [Get attribute data [GET /v2/entities/{entityId}/attrs/{attrName}]](#get-attribute-data-get-v2entitiesentityidattrsattrname)
            - [Update Attribute Data [PUT /v2/entities/{entityId}/attrs/{attrName}]](#update-attribute-data-put-v2entitiesentityidattrsattrname)
            - [Remove a Single Attribute [DELETE /v2/entities/{entityId}/attrs/{attrName}]](#remove-a-single-attribute-delete-v2entitiesentityidattrsattrname)
        - [Attribute Value](#attribute-value)
            - [Get Attribute Value [GET /v2/entities/{entityId}/attrs/{attrName}/value]](#get-attribute-value-get-v2entitiesentityidattrsattrnamevalue)
            - [Update Attribute Value [PUT /v2/entities/{entityId}/attrs/{attrName}/value]](#update-attribute-value-put-v2entitiesentityidattrsattrnamevalue)
        - [Types](#types)
            - [List Entity Types [GET /v2/type]](#list-entity-types-get-v2type)
            - [Retrieve entity information for a given type [GET /v2/types]](#retrieve-entity-information-for-a-given-type-get-v2types)
    - [Subscriptions Operations](#subscriptions-operations)
        - [Subscription payload datamodel](#subscription-payload-datamodel)
            - [`subscription.subject`](#subscriptionsubject)
            - [`subscription.subject.condition`](#subscriptionsubjectcondition)
            - [`subscription.notification`](#subscriptionnotification)
            - [`subscription.notification.http`](#subscriptionnotificationhttp)
            - [`subscription.notification.mqtt`](#subscriptionnotificationmqtt)
            - [`subscription.notification.httpCustom`](#subscriptionnotificationhttpcustom)
            - [`subscription.notification.mqttCustom`](#subscriptionnotificationmqttcustom)
        - [Subscription List](#subscription-list)
            - [List Subscriptions [GET /v2/subscriptions]](#list-subscriptions-get-v2subscriptions)
            - [Create Subscription [POST /v2/subscriptions]](#create-subscription-post-v2subscriptions)
        - [Subscription By ID](#subscription-by-id)
            - [Retrieve Subscription [GET /v2/subscriptions/{subscriptionId}]](#retrieve-subscription-get-v2subscriptionssubscriptionid)
            - [Update Subscription [PATCH /v2/subscriptions/{subscriptionId}]](#update-subscription-patch-v2subscriptionssubscriptionid)
            - [Delete subscription [DELETE /v2/subscriptions/{subscriptionId}]](#delete-subscription-delete-v2subscriptionssubscriptionid)
    - [Registration Operations](#registration-operations)
        - [Registration payload datamodel](#registration-payload-datamodel)
            - [`registration`](#registration)
            - [`registration.provider`](#registrationprovider)
            - [`registration.dataProvided`](#registrationdataprovided)
            - [`registration.forwardingInformation`](#registrationforwardinginformation)
        - [Registration list](#registration-list)
            - [List Registrations [GET /v2/registrations]](#list-registrations-get-v2registrations)
            - [Create Registration [POST /v2/registrations]](#create-registration-post-v2registrations)
        - [Registration By ID](#registration-by-id)
            - [Retrieve Registration [GET /v2/registrations/{registrationId}]](#retrieve-registration-get-v2registrationsregistrationid)
            - [Update Registration [PATCH /v2/registrations/{registrationId}]](#update-registration-patch-v2registrationsregistrationid)
            - [Delete Registration [DELETE /v2/registrations/{registrationId}]](#delete-registration-delete-v2registrationsregistrationid)
    - [Batch Operations](#batch-operations)
        - [Update operation](#update-operation)
            - [Update [POST /v2/op/update]](#update-post-v2opupdate)
        - [Query operation](#query-operation)
            - [Query [POST /v2/op/query]](#query-post-v2opquery)
        - [Notify operation](#notify-operation)
            - [Notify [POST /v2/op/notify]](#notify-post-v2opnotify)

<!-- /TOC -->

# Preface
 
This is the release 2.1 of the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable),
fully backward compatible with the original NGSIv2 released at September 15th, 2018.

# Specification

## Introduction

The FIWARE NGSI (Next Generation Service Interface) API defines 

* a **data model** for context information, based on a simple information model using the notion of
  *context entities*
* a **context data interface** for exchanging information by means of query, subscription, and
  update operations
* a **context availability interface** for exchanging information on how to obtain context
  information (whether to separate the two interfaces is currently under discussion).

## Terminology

### Context data modelling and exchange

The main elements in the NGSI data model are context entities, attributes and metadata,
as shown in the figure below.

![NGSI data model](https://raw.githubusercontent.com/telefonicaid/fiware-orion/master/doc/apiary/v2/Ngsi-data-model.png)

#### Context Entities

Context entities, or simply entities, are the center of gravity in the FIWARE NGSI information 
model. An entity represents a thing, i.e., any physical or logical object (e.g., a sensor, a person,
a room, an issue in a ticketing system, etc.). Each entity has an **entity id**.

Furthermore, the type system of FIWARE NGSI enables entities to have 
an **entity type**. Entity types are semantic types; they are intended
to describe the type of thing represented by the entity.
For example, a context entity with id *sensor-365* could have the
type *temperatureSensor*.

Each entity is uniquely identified by the combination of its id and type.

#### Context Attributes 

Context attributes are properties of context entities.
For example, the current speed of a car could be modeled as
attribute *current_speed* of entity *car-104*.

In the NGSI data model, attributes have an *attribute name*,
an *attribute type*, an *attribute value* and *metadata*. 
* The attribute name describes what kind of property the attribute value represents of the entity,
for example *current_speed*.
* The attribute type represents the NGSI value type of the attribute value.
Note that FIWARE NGSI has its own type system for attribute values, so NGSI value types are not
the same as JSON types.
* The attribute value finally contains
  * the actual data
  * optional **metadata** describing properties of the attribute value like e.g. accuracy, provider,
    or a timestamp
  
#### Context Metadata

Context metadata is used in FIWARE NGSI in several places, one of
them being an optional part of the attribute value as described
above. Similar to attributes, each piece of metadata has:
 * **a metadata name**, describing the role of the metadata in the
 place where it occurs; for example, the metadata name *accuracy* 
 indicates that the metadata value describes how accurate a given 
 attribute value is
 * a **metadata type**, describing the NGSI value type of the metadata value
 * a **metadata value** containing the actual metadata

Note that in NGSI it is not foreseen that metadata may contain nested metadata.

## MIME Types

The API response payloads in this specification are based on `application/json` and (for attribute value 
type operation) `text/plain` MIME types. Clients issuing HTTP requests with accept types different 
than those will get a `406 Not Acceptable` error.

## JSON Entity Representation

An entity is represented by a JSON object with the following syntax:

* The entity id is specified by the object's `id` property, whose value is a string containing the
  entity id.

* The entity type is specified by the object's `type` property, whose value is a string containing
  the entity's type name.

* Entity attributes are specified by additional properties, whose names are the `name` of the 
  attribute and whose representation is described in the [JSON Attribute Representation](#json-attribute-representation) section
  below. Obviously, `id` and `type` are not allowed to be used as attribute names.

An example of this syntax in shown below:

```
{
  "id": "entityID",
  "type": "entityType",
  "attr_1": <val_1>,
  "attr_2": <val_2>,
  ...
  "attr_N": <val_N>
}
```

The normalized representation of entities always include `id`, `type` and the properties that
represent attributes. However, simplified or partial representations
(see the [Partial Representations](#partial-representations) section below) may leave some of them out.
The specification of each operation includes details about what representation is expected as input
or what representation will be provided (rendered) as output.

## JSON Attribute Representation

An attribute is represented by a JSON object with the following syntax:

* The attribute value is specified by the `value` property, whose value may be any JSON datatype.

* The attribute NGSI type is specified by the `type` property, whose value is a string containing
  the NGSI type.

* The attribute metadata is specified by the `metadata` property. Its value is another JSON object
  which contains a property per metadata element defined (the name of the property is the `name` of
  the metadata element). Each metadata element, in turn, is represented by a JSON object containing
  the following properties:

  * `value`: Its value contains the metadata value, which may correspond to any JSON datatype.

  * `type`: Its value contains a string representation of the metadata NGSI type.

An example of this syntax in shown below:

```
{
  "value": <...>,
  "type": <...>,
  "metadata": <...>
}
```

## Simplified Entity Representation

There are two representation modes that must be supported by implementations. These representation
modes allow to generate simplified representations of entities.

* *keyValues* mode. This mode represents the entity attributes by their values only, leaving out the
  information about type and metadata.
  See example below.

```
{
  "id": "R12345",
  "type": "Room",
  "temperature": 22
}
```

* *values mode*. This mode represents the entity as an array of attribute values.
  Information about id and type is left out.
  See example below.
  The order of the attributes in the array is specified by the `attrs` URI param
  (e.g. `attrs=branch,colour,engine`). If `attrs` is not used, the order is arbitrary.

```
[ 'Ford', 'black', 78.3 ]
```

* *unique mode*. This mode is just like *values mode*, except that values are not repeated.

## Partial Representations

Some operations use partial representation of entities:

* `id` and `type` are not allowed in update operations, as they are immutable properties.

* In requests where entity `type` is allowed, it may be omitted. When omitted in entity
  creation operations, the default string value `Thing` is used for the type.

* In some cases, not all the attributes of the entity are shown, e.g. a query selecting a subset
  of the entity attributes.

* Attribute/metadata `value` may be omitted in requests, meaning that the attribute/metadata has
  `null` value. In responses, the value is always present.

* Attribute/metadata `type` may be omitted in requests. When omitted in attribute/metadata creation
  or in update operations, a default is used for the type depending on the value:
  * If value is a string, then type `Text` is used
  * If value is a number, then type `Number` is used.
  * If value is a boolean, then type `Boolean` is used.
  * If value is an object or array, then `StructuredValue` is used.
  * If value is null, then `None` is used.

* Attribute `metadata` may be omitted in requests, meaning that there are no metadata elements
  associated to the attribute. In responses, this property is set to `{}` if the attribute 
  doesn't have any metadata. Depending if `overrideMetadata` is used or not, this sentence has
  two interpretations:
  * If `overrideMetadata` is not used (default behaviour) it is interpreted as
  "... meaning that there are no metadata elements associated to the attribute,
  **which need to be updated**"
  * If `overrideMetadata` is used it is interpreted as
  "... meaning that there are no metadata elements associated to the attribute,
  **as a result of the the attribute update**"

The metadata update semantics used by Orion Context Broker (and the related `overrideMetadata` 
option are detailed in [this section of the documentation](metadata.md#updating-metadata).

## Datetime support

Orion support DateTime in ISO8601 by using attribute or metadata type `Datetime`. These attributes or metadata can be used with the query operators 
greater-than, less-than, greater-or-equal, less-or-equal and range.  A `DateTime` attribute with `null` value will not be taken into account in filters, 
i.e. `GET /v2/entities?q=T>2021-04-21`. 

`DateTime` attribute example (only the referred entity attribute is shown):

```
{
  "timestamp": {
    "value": "2017-06-17T07:21:24.238Z",
    "type: "DateTime"
  }
}
```

`DateTime` metadata example For instance (only the referred attribute metadata is shown):

```
"metadata": {
      "dateCreated": {
        "value": "2019-09-23T03:12:47.213Z",
        "type": "DateTime"
      }
}
```

The following considerations have to be taken into account at attribute creation/update time or when used in `q` and `mq` filters:

* Datetimes are composed of date, time and timezone designator, in one of the following patterns:
    * `<date>`
    * `<date>T<time>`
    * `<date>T<time><timezone>`
    * Note that the format `<date><timezone>` is not allowed. According to ISO8601: *"If a time zone designator is required,
      it follows the combined date and time".*
* Regarding `<date>` it must follow the pattern: `YYYY-MM-DD`
    * `YYYY`: year (four digits)
    * `MM`: month (two digits)
    * `DD`: day (two digits)
* Regarding `<time>` it must follow any of the patterns described in [the ISO8601 specification](https://en.wikipedia.org/wiki/ISO_8601#Times):
    * `hh:mm:ss.sss` or `hhmmss.sss`.
    * `hh:mm:ss` or `hhmmss`. Milliseconds are set to `000` in this case.
    * `hh:mm` or `hhmm`. Seconds are set to `00` in this case.
    * `hh`. Minutes and seconds are set to `00` in this case.
    * If `<time>` is omitted, then hours, minutes and seconds are set to `00`.
* Regarding `<timezones>` it must follow any of the patterns described in [the ISO8601 specification](https://en.wikipedia.org/wiki/ISO_8601#Time_zone_designators):
    * `Z`
    * `±hh:mm`
    * `±hhmm`
    * `±hh`
* ISO8601 specifies that *"if no UTC relation information is given with a time representation, the time is assumed to be in local time"*.
  However, this is ambiguous when client and server are in different zones. Thus, in order to solve this ambiguity, Orion will always
  assume timezone `Z` when timezone designator is omitted.

Orion always provides datetime attributes/metadata using the format `YYYY-MM-DDThh:mm:ss.sssZ`. However, note that
Orion provides other timestamps (registration/subscription expiration date, last notification/failure/sucess in notifications,
etc.) using `YYYY-MM-DDThh:mm:ss.ssZ` format (see [related issue](https://github.com/telefonicaid/fiware-orion/issues/3671)
about this)).

In addition, note Orion uses always UTC/Zulu timezone when provides datetime (which is the best default option, as
clients/receivers may be running in any timezone). This may change in the future (see [related issue](https://github.com/telefonicaid/fiware-orion/issues/2663)).

The string `ISO8601` as type for attributes and metadata is also supported. The effect is the same as when using `DateTime`.

## Special Attribute Types

Generally speaking, user-defined attribute types are informative; they are processed by the NGSIv2
server in an opaque way. Nonetheless, the types described below are used to convey a special
meaning:

* `DateTime`:  identifies dates, in ISO8601 format. These attributes can be used with the query
  operators greater-than, less-than, greater-or-equal, less-or-equal and range. For further information
  check the section [Datetime support](#datetime-support) of this documentation.

* `geo:point`, `geo:line`, `geo:box`, `geo:polygon` and `geo:json`. They have special semantics
  related with entity location. Attributes with `null` value will not be taken into account in 
  geo-queries and they doesn't count towards the limit of one geospatial attribute per entity. 
  See [Geospatial properties of entities](#geospatial-properties-of-entities) section.

## Builtin Attributes

There are entity properties that are not directly modifiable by NGSIv2 clients, but that can be
rendered by NGSIv2 servers to provide extra information. From a representation point of view, they
are just like regular attributes, with name, value and type.

Builtin attributes are not rendered by default. In order to render a specific attribute, add its
name to the `attrs` parameter in URLs (or payload field in POST /v2/op/query operation) or
subscription (`attrs` sub-field within `notification`).

The list of builtin attributes is as follows:

* `dateCreated` (type: `DateTime`): entity creation date as an ISO 8601 string.

* `dateModified` (type: `DateTime`): entity modification date as an ISO 8601 string.

* `dateExpires` (type: `DateTime`): entity expiration date as an ISO 8601 string. How the server
  controls entity expiration is an implementation specific aspect.

* `alterationType` (type: `Text`): specifies the change that triggers the notification. It is related with 
the [subscriptions based in alteration type](#subscriptions-based-in-alteration-type) feature. This attribute
  can be used only in notifications, it does not appear when querying it (`GET /v2/entities?attrs=alterationType`) and can take the following values:
   * `entityCreate` if the update that triggers the notification is a entity creation operation 
   * `entityUpdate` if the update that triggers the notification was an update but it wasn't an actual change
   * `entityChange` if the update that triggers the notification was an update with an actual change or not an actual change but with `forcedUpdate` in use
   * `entityDelete` if the update that triggers the notification was a entity delete operation

Like regular attributes, they can be used in `q` filters and in `orderBy` (`alterationType` is not included).
However, they cannot be used in resource URLs.

## Special Metadata Types

Generally speaking, user-defined metadata types are informative; they are processed by the NGSIv2
server in an opaque way. Nonetheless, the types described below are used to convey a special
meaning:

* `DateTime`:  identifies dates, in ISO8601 format. This metadata can be used with the query
  operators greater-than, less-than, greater-or-equal, less-or-equal and range. For further information
  check the section [Datetime support](#datetime-support) of this documentation.

* `ignoreType`: When `ignoreType` with value `true` is added to an attribute, Orion will ignore the
semantics associated to the attribute type. Note that Orion ignored attribute type in general so
this metadata is not needed most of the cases, but there are two cases in which attribute
type has an special semantic for Orion:
   * `DateTime`
   * Geo-location types (`geo:point`, `geo:line`, `geo:box`, `geo:polygon` and `geo:json`)

At the present moment `ignoreType` is supported only for geo-location types, this way allowing a
mechanism to overcome the limit of only one geo-location per entity (more details
in [Geospatial properties of entities](##geospatial-properties-of-entities) section). Support
for `ignoreType` in `DateTime` may come in the future.

## Builtin Metadata

Some attribute properties are not directly modifiable by NGSIv2 clients, but they can be
rendered by NGSIv2 servers to provide extra information. From a representational point of view, they
are just like regular metadata, with name, value, and type.

Builtin metadata are not rendered by default. In order to render a specific metadata, add its
name to the `metadata` URL parameter (or payload field in POST /v2/op/query operation) or
subscription (`metadata` sub-field within `notification`).

The list of builtin metadata is as follows:

* `dateCreated` (type: `DateTime`): attribute creation date as an ISO 8601 string.

* `dateModified` (type: `DateTime`): attribute modification date as an ISO 8601 string.

* `previousValue` (type: any): only in notifications. The value of this metadata is the previous
  value (to the request triggering the notification) of the associated attribute. The type of this metadata
  must be the previous type of the associated attribute. If the type/value of `previousValue` is the same
  type/value as in the associated attribute, then the attribute has not actually changed its value.

* `actionType` (type: `Text`): only in notifications.  It is included if the attribute to which it is attached
  was included in the request that triggered the notification. Its value depends on the request operation
  type: `update` for updates, `append` for creation and `delete` for deletion. Its type is always `Text`.

Like regular metadata, they can be used in `mq` filters. However, they cannot be used in resource URLs.

## User attributes or metadata matching builtin name

(The content of this section applies to all builtins except `dateExpires` attribute. Check the document
[on transient entities](user/transient_entities.md) for specific information about `dateExpires`).

First of all: **you are strongly encouraged to not use attributes or metadata with the same name as an 
NGSIv2 builtin**. In fact, the NGSIv2 specification forbids that (check "Attribute names restrictions" and
"Metadata names restrictions" sections in the specification).

However, if you are forced to have such attributes or metadata (maybe due to legacy reasons) take into
account the following considerations:

* You can create/update attributes and/or metadata which name is the same of a NGSIv2 builtin.
  Orion will let you do so.
* User defined attributes and/or metadata are shown without need to explicit declare it in the GET request
  or subscription. For instance, if you created a `dateModified` attribute with value
  "2050-01-01" in entity E1, then `GET /v2/entities/E1` will retrieve it. You don't need to use
  `?attrs=dateModified`.
* When rendered (in response to GET operations or in notifications) the user defined attribute/metadata
  will take preference over the builtin even when declared explicitly. For instance, if you created
  a `dateModified` attribute with value "2050-01-01" in entity E1 and you request
  `GET /v2/entities?attrs=dateModified` you will get "2050-01-01".
* However, filtering (i.e. `q` or `mq`) is based on the value of the builtin. For instance, if you created
  a `dateModified` attribute with value "2050-01-01" in entity E1 and you request
  `GET /v2/entities?q=dateModified>2049-12-31` you will get no entity. It happens that "2050-01-01" is
  greater than "2049-12-31" but the date you modified the entity (some date in 2018 or 2019 maybe) will
  not be greater than "2049-12-31". Note this is somehow inconsistent (i.e. user defined takes preference
  in rendering but not in filtering) and may change in the future.

For further information about builtin attribute and metadata names you can check the respective sections 
[Builtin Attributes](#builtin-attributes) and [Builtin Metadata](#builtin-metadata).

## Field syntax restrictions

Fields used as identifiers in the NGSIv2 API follow special rules regarding allowed syntax.
These rules apply to:

* Entity id
* Entity type
* Attribute name
* Attribute type
* Metadata name
* Metadata type

The rules are:

* Allowed characters are the ones in the plain ASCII set, except the following ones:
  control characters, whitespace, `&`, `?`, `/` and `#`.
* Maximum field length is 256 characters.
* Minimum field length is 1 character.

In addition to the above rules, given NGSIv2 server implementations could add additional syntactical
restrictions in those or other fields, e.g., to avoid cross script injection attacks.

The additional restrictions that apply to Orion are the ones describe in the
[forbidden characters](forbidden_characters.md) section of the manual.

Note that you can use `TextUnrestricted` attribute type (and special attribute type beyond
the ones defined in the NGSIv2 Specification) in order to skip forbidden characters checkings
in the attribute value. However, it could have security implications (possible script
injections attacks) so use it at your own risk!

In case a client attempts to use a field that is invalid from a syntax point of view, the client
gets a "Bad Request" error response, explaining the cause.

## Attribute names restrictions

The following strings must not be used as attribute names:

* `id`, as it would conflict with the field used to represent entity id.

* `type`, as it would conflict with the field used to represent entity type.

* `geo:distance`, as it would conflict with the string used in `orderBy` for proximity to
  center point.

* Builtin attribute names. It is possible to use the same attribute names but it is totally discouraged. 
Check [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name) 
section of this documentation.

* `*`, as it has a special meaning as "all the custom/user attributes" (see section on
  [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata)).

## Metadata names restrictions

The following strings must not be used as metadata names:

* Builtin metadata names. It is possible to use the same metadata names but it is totally discouraged. 
Check [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name) 
section of this documentation.

* `*`, as it has a special meaning as "all the custom/user metadata" (see section on
  [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata)).

## Ordering Results

Operations that retrieve lists of entities permit the `orderBy` URI parameter to specify the
attributes or properties to be used as criteria when ordering results.
The value of `orderBy` can be:

* The keyword `geo:distance` to order results by distance to a reference geometry when a "near"
  (`georel=near`) spatial relationship is used. 

* A comma-separated list of attributes (including builtin attributes), `id` (for entity
  ID), and `type` (for entity type), e.g. `temperature,!humidity`. Results are ordered by the first
  field. On ties, the results are ordered by the second field and so on. A "!" before
  the field name means that the order is reversed.

How each type is ordered with regard to other types is an implementation aspect. In the case of Orion,
we use the same criteria as the one used by the underlying implementation (MongoDB). See
[the following link](https://docs.mongodb.com/manual/reference/method/cursor.sort/#ascending-descending-sort) 
for details.

From lowest to highest:

1. Null
2. Number
3. String
4. Object
5. Array
6. Boolean


## Error Responses

If present, the error payload is a JSON object including the following fields:

+ `error` (required, string): a textual description of the error.
+ `description` (optional, string): additional information about the error.

All NGSIv2 server implementations must use the following HTTP status codes and `error` texts
described in this section. However, the particular text used for `description` field is
an implementation specific aspect.

NGSIv2 `error` reporting is as follows:

+ If the incoming JSON payload cannot be parsed then `ParseError` (`400`) is returned.
+ Errors which are only caused by request itself (i.e. they do not depend on the NGSIv2 server status),
  either in the URL parameters or in the payload, results in `BadRequest`(`400`).
  + Exception: incoming JSON payload errors, which have another `error` message (see previous bullet).
+ Attempt to exceed spatial index limit results in `NoResourceAvailable` (`413`). See [Geospatial properties of entities](#geospatial-properties-of-entities)
  section for details.
+ Ambiguity due to the request may refer to several resources, e.g. attempt to update an entity providing only its ID
  and several entities with that ID exist, results in `TooManyResults` (`409`).
+ If the resource identified by the request is not found then `NotFound` (`404`) is returned.
+ Errors due to the request plus state combination but not exclusively from the request
  (e.g. POST with `options=append` on an existing attribute) results in `Unprocessable` (`422`).
  + Exception: the request plus state conditions that lead to 404, 409 or 413 errors, as described in previous bullets.
+ HTTP layer errors use the following:
  + HTTP 405 Method Not Allowed corresponds to `MethodNotAlowed` (`405`)
  + HTTP 411 Length Required corresponds to `ContentLengthRequired` (`411`)
  + HTTP 413 Request Entity Too Large corresponds to `RequestEntityTooLarge` (`413`)
  + HTTP 415 Unsupported Media Type corresponds to `UnsupportedMediaType` (`415`)

## Geospatial properties of entities

The geospatial properties of a context entity can be represented by means of regular
context attributes.
The provision of geospatial properties enables the resolution of geographical queries.

Two different syntaxes must be supported by compliant implementations: 

* *Simple Location Format*. It is meant as a very lightweight format for developers and users to
  quickly and easily add to their existing entities.

* *GeoJSON*.  [GeoJSON](https://tools.ietf.org/html/draft-butler-geojson-06) is a geospatial data
  interchange format based on the JavaScript Object Notation (JSON).
  GeoJSON provides greater flexibility allowing the representation of point altitudes or even more
  complex geospatial shapes, for instance
  [multi geometries](http://www.macwright.org/2015/03/23/geojson-second-bite.html#multi-geometries).

Current implementation, (based in the [MongoDB capabilities](https://www.mongodb.com/docs/manual/reference/geojson/)) introduces some limitations in the usage of `GeoJSON` representations, supporting only the following types:

* Point
* MultiPoint
* LineString
* MultiLineString
* Polygon
* MultiPolygon

More information on the tests conducted can be found [here](https://github.com/telefonicaid/fiware-orion/issues/3586).

The types `Feature` and `FeatureCollection` are also supported, but in a special way. You can
use `Feature` or `FeatureCollection` to create/update `geo:json` attributes. However, when
the attribute value is retrieved (GET resposes or notifictaions) you will get only the content of:

* the `geometry` field, in the case of `Feature`
* the `geometry` field of the first item of the `features` array, in the case of `FeatureCollection`

Note that actually Orion stores the full value used at `Feature` or `FeatureCollection`
creation/updating time. However, from the point of view of normalization with other `geo:json` types,
it has been decided to return only the `geometry` part. In the future, maybe a flag to return
the full content would be implemented (more detail [in this issue](https://github.com/telefonicaid/fiware-orion/issues/4125)).
Another alternative to disable the special processing of `Feature` or `FeatureCollection` is to use
[`ignoreType` metadata](#ignoretype-metadata) but in that case also entity location will be ignored.

With regards to `FeatureCollection`, it is only accepted at creation/update time only if it contains a single 
`Feature` (i.e. the `features` field has only one element). Otherwise , Orion would return an `BadRequest`error.

The only GeoJSON type not supported at all is `GeometryCollection`. You will get a "Database Error"
if you try to use them.

Client applications are responsible for defining which entity attributes convey geospatial
properties (by providing an appropriate NGSI attribute type). Typically this is an entity attribute
named `location`, but nothing prevents use another different name for the geospatial attribute. 
In the case of Orion, the number of geospatial attributes is limited to one (1) attribute due to
resource constraints imposed by backend databases.

When spatial index limits are exceeded, Orion rises an error `413`, *Request entity too large*, and
the reported error on the response payload is `NoResourcesAvailable`.

However, you can set `ignoreType` metadata to `true` to mean that a given attribute contains an extra informative
location (more detail in [this section of the documentation](#special-metadata-types)). This disables Orion
interpretation of that attribute as a location, so it doesn't count towards the limit.

For instance:

```
{
  "id": "Hospital1",
  "type": "Hospital",
  ...
  "location": {
    "value": {
      "type": "Point",
      "coordinates": [ -3.68666, 40.48108 ]
    },
    "type": "geo:json"
  },
  "serviceArea": {
    "value": {
      "type": "Polygon",
      "coordinates": [ [ [-3.69807, 40.49029 ], [ -3.68640, 40.49100], [-3.68602, 40.50456], [-3.71192, 40.50420], [-3.69807, 40.49029 ] ] ]
    },
    "type": "geo:json",
    "metadata": {
      "ignoreType":{
        "value": true,
        "type": "Boolean"
      }
    }
  }
}
```

Both attributes are of type `geo:json`, but `serviceArea` uses `ignoreType` metadata to `true` so the limit 
of one non-informative location is not overpassed.

If extra locations are defined in this way take, into account that the location that is used to solve geo-queries
is the one without `ignoreType` set to `true` metadata (`location` attribute in the example above). All
the locations defined with `ignoreType` set to `true` are ignored by Orion and, in this sense, doesn't take
part in geo-queries.

### Simple Location Format

Simple Location Format supports basic geometries ( *point*, *line*, *box*, *polygon* ) and covers
the typical use cases when encoding geographical locations. It has been inspired by
[GeoRSS Simple](http://www.georss.org/simple.html).

It is noteworthy that the Simple Location Format is not intended to represent complex positions on
Earth surface.
For instance, applications that require to capture altitude coordinates will have to use GeoJSON as
representation format for the geospatial properties of their entities. 

A context attribute representing a location encoded with the Simple Location Format
must conform to the following syntax:

* The attribute type must be one of the following values: (`geo:point`, `geo:line`, `geo:box` or 
  `geo:polygon`).
* The attribute value must be a list of coordinates. By default, coordinates are defined
  using the [WGS84 Lat Long](https://en.wikipedia.org/wiki/World_Geodetic_System#WGS84),
  [EPSG::4326](http://www.opengis.net/def/crs/EPSG/0/4326) coordinate reference system (CRS),
  with latitude and longitude units of decimal degrees. Such coordinate list allow to encode
  the geometry specified by the `type` attribute and are encoded according to the specific
  rules defined below:

  * Type `geo:point`:   the attribute value must contain a string containing a
    valid latitude-longitude pair, separated by comma.
  * Type `geo:line`:    the attribute value must contain a string array of
    valid latitude-longitude pairs. There must be at least two pairs.
  * Type `geo:polygon`: the attribute value must contain a string array
    of valid latitude-longitude pairs.
    There must be at least four pairs, with the last being identical to the first
    (so a polygon has a minimum of three actual points).
    Coordinate pairs should be properly ordered so that the line segments
    that compose the polygon remain on the outer edge of the defined area.
    For instance, the following path, ```[0,0], [0,2], [2,0], [2, 2]```, is an example of an invalid
    polygon definition. 
    Implementations should raise an error when none of the former conditions are met by input data. 
  * Type `geo:box`:     A bounding box is a rectangular region, often used to define the extents of
    a map or a rough area of interest. A box is represented by a two-length string array of
    latitude-longitude pairs.
    The first pair is the lower corner, the second is the upper corner.

Note: Circle geometries are not supported, as the [literature](https://github.com/geojson/geojson-spec/wiki/Proposal---Circles-and-Ellipses-Geoms#discussion-notes)
describes different shortcomings for implementations. 

The examples below illustrate the referred syntax:

```
{
  "location": {
    "value": "41.3763726, 2.186447514",
    "type": "geo:point"
  }
}
```

```
{
  "location": {
    "value": [
      "40.63913831188419, -8.653321266174316",
      "40.63881265804603, -8.653149604797363"
    ],
    "type": "geo:box"
  }
}
```

### GeoJSON

A context attribute representing a location encoded using GeoJSON must conform to the following
syntax:

* The NGSI type of the attribute must be `geo:json`.
* The attribute value must be a valid GeoJSON object. It is noteworthy that longitude comes before
  latitude in GeoJSON coordinates.

The example below illustrates the usage of GeoJSON.
More GeoJSON examples can be found in [GeoJSON IETF Spec](https://tools.ietf.org/html/draft-butler-geojson-06#page-14).
Additionally, the following
[GeoJSON Tutorial](http://www.macwright.org/2015/03/23/geojson-second-bite.html)
might be useful in understanding the format. 

```
{
  "location": {
    "value": {
      "type": "Point",
      "coordinates": [2.186447514, 41.3763726]
    },
    "type": "geo:json"
  }
}
```

## Simple Query Language

The Simple Query Language provides a simplified syntax to retrieve entities which match a set of
conditions.
A query is composed by a list of statements separated by the ';' character.
Each statement expresses a matching condition.
The query returns all the entities that match all the matching conditions (AND logical operator). 

There are two kinds of statements: *unary statements* and *binary statements*.

Binary statements are composed by an attribute path (e.g. `temperature` or `brand.name`), an operator
and a value (whose format depends on the operator), e.g.:

```
temperature==50
temperature<=20
```

The syntax of an attribute path consists of a list of tokens separated by the `.` character. This list of tokens
addresses a JSON property name, in accordance with the following rules:

* The first token is the name of an NGSI attribute (*target NGSI attribute*) of an entity.
* If filtering by attribute value (i.e. the expression is used in a `q` query), the rest of tokens (if present)
  represent the path to a sub-property of the *target NGSI attribute value* (which should be a JSON object).
  Such sub-property is defined as the *target property*.
* If filtering by metadata (i.e. the expression is used in a `mq` query), the second token represents a metadata
  name associated to the target NGSI attribute, *target metadata*, and the rest of tokens
  (if present) represent the path to a sub-property of the *target metadata value* (which should be a
  JSON object). Such sub-property is defined as the *target property*.

The *target property value* is defined as the value of the JSON property addressed by the list of tokens described
above i.e. the value of the *target property*.

In case only one token is provided (two in case of filtering by metadata), then the *target property* will
be the *target NGSI attribute* itself (or the *target metadata* in case of filtering by metadata) and the
*target property value* will be the *target NGSI attribute* value (or the *target metadata* value in case
of filtering by metadata). The value of the *target NGSI attribute* (or the *target metadata*
in case of filtering by metadata) should not be a JSON object in this case.

In case some of the tokens include `.`, you can use single quote (`'`) as separator. For example, the following
attribute path `'a.b'.w.'x.y'` is composed by three tokens: the first token is `a.b`, the second token is `w` and
the third token is `x.y`.

The list of operators (and the format of the values they use) is as follows:

+ **Equal**: `==`. This operator accepts the following types of right-hand side:
    + Single element, e.g. `temperature==40`. For an entity to match, it must contain the *target
      property* (temperature) and the *target property value* must be the query value (40)
      (or include the value, in case the *target property value* is an array).
    + A list of comma-separated values, e.g. `color==black,red`. For an entity to match, it must
      contain the *target property* and the *target property value* must be **any** of the values
      in the list (OR clause) (or include **any** of the values in the list in case the *target
      property value* is an array).
      E.g. entities with an attribute named `color`, whose value is `black` are a match, while
      entities with an attribute named `color` but whose value is `white` do not match.
    + A range, specified as a minimum and a maximum, separated by `..`, e.g. `temperature==10..20`.
      For an entity to match, it must contain the *target property* (temperature),
      and the *target property value* must be between the upper and lower limits
      of the range (both included). Ranges can only be used with *target properties* that represent
      dates (in ISO8601 format), numbers or strings.
+ **Unequal**: `!=`. This operator accepts the following types of right-hand side:
    + Single element, e.g. `temperature!=41`. For an entity to match, it must contain the *target
      property* (temperature) and the *target property value* must **not** be the query value (41).
    + A list of comma-separated values, e.g. `color!=black,red`. For an entity to match, it must
      contain the *target property* and the *target property value* must **not** be any of the values
      in the list (AND clause) (or not include **any** of the values in the list in case the *target
      property value* is an array).
      E.g. entities whose attribute `color` is set to `black` will not match, while entities whose
      attribute `color` is set to `white` will match.
    + A range, specified as a minimum and maximum separated by `..`, e.g. `temperature!=10..20`.
      For an entity to match, it must contain the *target property* (temperature) and the
      *target property value* must **not** be between the upper and lower limits
      (both included). Ranges can only be used with elements *target properties* that represent dates
      (in ISO8601 format), numbers or strings.
+ **Greater than**: `>`. The right-hand side must be a single element, e.g. `temperature>42`.
    For an entity to match, it must contain the *target property* (temperature)
    and the *target property value* must be strictly greater than the query value (42).
    This operation is only valid for *target properties* of type date, number or string (used with
    *target properties* of other types may lead to unpredictable results).
+ **Less than**: `<`. The right-hand side must be a single element, e.g. `temperature<43`.
    For an entity to match, it must contain the *target property* (temperature)
    and the *target property value* must be strictly less than the value (43).
    This operation is only valid for *target properties* of type date, number or string (used with
    *target properties* of other types may lead to unpredictable results).
+ **Greater or equal than**: `>=`. The right-hand side must be a single element, e.g. `temperature>=44`.
    For an entity to match, it must contain the *target property* (temperature)
    and the *target property value* must be greater than or equal to that value (44).
    This operation is only valid for *target properties* of type date, number or string (used with
    *target properties* of other types may lead to unpredictable results).
+ **Less or equal than**: `<=`. The right-hand side must be a single element, e.g. `temperature<=45`.
    For an entity to match, it must contain the *target property* (temperature)
    and the *target property value* must be less than or equal to that value (45).
    This operation is only valid for *target properties* of type date, number or string (used with
    *target properties* of other types may lead to unpredictable results).
+ **Match pattern**: `~=`. The value matches a given pattern, expressed as a regular expression, e.g.
    `color~=ow`. For an entity to match, it must contain the *target property* (color)
    and the *target property value* must match the string in the right-hand side,
    'ow' in this example (`brown` and `yellow` would match, `black` and `white` would not).
    This operation is only valid for *target properties* of type string.

The symbol `:` can be used instead of `==`.

In case of equal or unequal, if the string to match includes a `,`, you can use single quote
(`'`) to disable the special meaning of the comma, e.g.: `color=='light,green','deep,blue'`.
The first example would match a color with the exact value 'light,green' OR 'deep,blue'. The
simple quote syntax can be also used to force string interpretation in filters, e.g.
`q=title=='20'` will match string "20" but not number 20.

Unary negatory statements use the unary operator `!`, while affirmative unary statements use no
operator at all.
The unary statements are used to check for the existence of the *target property*.
E.g. `temperature` matches entities that have an attribute called 'temperature' (no matter its
value), while `!temperature` matches entities that do not have an attribute called 'temperature'.
 
## Geographical Queries

Geographical queries are specified using the following parameters:

``georel`` is intended to specify a spatial relationship (a predicate)
between matching entities and a reference shape (`geometry`).
It is composed of a token list separated by ';'.
The first token is the relationship name, the rest of the tokens (if any) are modifiers which
provide more information about the relationship. The following values are recognized:

+ `georel=near`. The ``near`` relationship means that matching entities must be located at a certain
  threshold distance to the reference geometry. It supports the following modifiers:
  + `maxDistance`.  Expresses, in meters, the maximum distance at which matching entities must be
    located.
  + `minDistance`.  Expresses, in meters, the minimum distance at which matching entities must be
    located.
+ `georel=coveredBy`. Denotes that matching entities are those that exist entirely within the
  reference geometry.
  When resolving a query of this type, the border of the shape must be considered to be part of the
  shape. 
+ `georel=intersects`. Denotes that matching entities are those intersecting with the reference
  geometry.
+ `georel=equals`. The geometry associated to the position of matching entities and the reference
  geometry must be exactly the same.
+ `georel=disjoint`. Denotes that matching entities are those **not** intersecting with the
  reference geometry. 

`geometry` allows to define the reference shape to be used when resolving the query.
 The following geometries (see [Simple Location Format](#simple-location-format)) must be supported:

+ `geometry=point`, defines a point on the Earth surface.
+ `geometry=line`, defines a polygonal line.
+ `geometry=polygon`, defines a polygon.
+ `geometry=box`, defines a bounding box.

**coords** must be a string containing a semicolon-separated list of pairs of geographical
coordinates in accordance with the geometry specified and the rules mandated by the Simple Location
Format:

* `geometry=point`.   `coords` contains a pair of WGS-84 geo-coordinates.
* `geometry=line`.    `coords` contains a list of pairs of WGS-84 geo-coordinates.
* `geometry=polygon`. `coords` is composed by at least four pairs of WGS-84 geo-coordinates.
* `geometry=box`.     `coords` is composed by two pairs of WGS-84 geo-coordinates.

Examples:

`georel=near;maxDistance:1000&geometry=point&coords=-40.4,-3.5`.
Matching entities must be located (at most) 1000 meters from the reference point.

`georel=near;minDistance:5000&geometry=point&coords=-40.4,-3.5`.
Matching entities must be (at least) 5000 meters from the reference point. 

`georel=coveredBy&geometry=polygon&coords=25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190`
Matching entities are those located within the referred polygon.

### Query Resolution

If an implementation is not able to resolve a geographical query, the HTTP Status code of the
response must be ```422```, *Unprocessable Entity*. The error name, present in the error payload,
must be ``NotSupportedQuery``. 

When resolving geographical queries, through the Simple Query Language,
the API implementation is responsible for determining which entity attribute
contains the geographical location to be used for matching purposes.
To this aim, the following rules must be followed:

* If an entity has no attribute corresponding to a location (encoded as GeoJSON or the
  Simple Location Format), then such an entity has not declared any geospatial property and will not
  match any geographical query.

* If an entity only exposes one attribute corresponding to a location, then such an attribute will
  be used when resolving geographical queries.

* If an entity exposes more than one location, then the attribute containing a metadata property
  named ``defaultLocation``, with boolean value ``true`` will be taken as the reference location
  used for resolving geographical queries. 

* If there is more than one attribute exposing location but none of them is labeled as default
location, then the query will be declared ambiguous and an HTTP error response with a ``409`` code
must be sent.

* If there is more than one attribute exposing location labeled as *default location*, then the
  query is declared ambiguous and an HTTP error response with a ``409`` code must be sent. 

## Filtering out attributes and metadata

The `attrs` URL parameter (or field in POST /v2/op/query) can be used in retrieve operations
to specify the list of attributes that must be included in the response. In a similar way, the
`metadata` URL parameter (or field in POST /v2/op/query) can be used to specify the list of metadata
that must be included in the response.

By default, if `attrs` is omitted (or `metadata` is omitted) then all the attributes (all the
metadata) are included, except builtin attributes (metadata). In order to include
builtin attributes (metadata) they have to be explicitly included in `attrs` (`metadata`).

E.g. to include only attributes A and B:

`attrs=A,B`

Note that including *only* builtin attributes (metadata) will avoid any user-defined
attribute (metadata). If you want to include builtin attributes (metadata) *and* user-defined
attributes (metadata) at the same time then

* The user-defined attributes (metadata) have to be explicitly included, e.g. to include the user-defined
  attributes A and B along with the builtin attribute `dateModified`, use: `attrs=dateModified,A,B`.
* The special value `*` can be used as an alias meaning "all user-defined attributes (metadata)", e.g.,
  to include all the user-defined attributes along with the builtin attribute `dateModified`
  use: `attrs=dateModified,*`.

Note that the `attrs` and `metadata` fields can be used also in subscriptions (as sub-fields of `notification`)
with the same meaning to specify which attributes (metadata) to include in notifications associated
to that subscription.

## Oneshot Subscription

Oneshot subscription provides an option to subscribe an entity only for one time notification. When consumer creates a subscription 
with status `oneshot`, a subscription is created as similar to the [normal subscription](user/walkthrough_apiv2.md#subscriptions) request with a slight difference.

In the normal case, the consumer gets initial and continuous notifications whenever the entity is updated until subscription is removed or its status passes to inactive after a subscription update.

While, in the case of oneshot subscription, the consumer gets notified only one time whenever the entity is updated after creating 
the subscription. Once a notification is triggered, the subscription transitions to `status`: `inactive`. Once in this status, 
the consumer may update it with `oneshot`∫ to repeat the same behavior (i.e. to get the one time notification again). 

![](user/oneshot_subscription.png "oneshot_subscription.png")

* Assuming an entity with id Room1 and type Room already exists in the database. 

Context Consumer can create a subscription for that entity with status “oneshot” as below:

```
curl -v localhost:1026/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "A subscription to get info about Room1",
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "pressure"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/accumulate"
    },
    "attrs": [
      "temperature"
    ]
  },
  "status" : "oneshot"
}
EOF
```

As the value of pressure attribute is updated, context consumer will get the notification for temperature attribute and status 
of this subscription will automatically be turned to inactive and no further notification will be triggered until the consumer 
updates it again to "oneshot" in below manner:

```
curl localhost:1026/v2/subscriptions/<subscription_id> -s -S \
    -X PATCH -H 'Content-Type: application/json' -d @- <<EOF
{
  "status": "oneshot"
}
EOF
```
Once the status is updated to "oneshot" again, the consumer will again get the notification one time whenever the entity will be updated and the subscription status will again be changed to `inactive` automatically.

## Covered subscriptions

The `attrs` field within `notification` specifies the sub-set of entity attributes to be included in the
notification when subscription is triggered. By default Orion only notifies attributes that exist
in the entity. For instance, if subscription is this way:

```
"notification": {
  ...
  "attrs": [
    "temperature",
    "humidity",
    "brightness"
  ]
}
```

but the entity only has `temperature` and `humidity` attributes, then `brightness` attribute is not included
in notifications.

This default behaviour can be changed using the `covered` field set to `true` this way:

```
"notification": {
  ...
  "attrs": [
    "temperature",
    "humidity",
    "brightness"
  ],
  "covered": true
}
```

in which case all attributes are included in the notification, no matter if they exist or not in the
entity. For these attributes that don't exist (`brightness` in this example) the `null`
value (of type `"None"`) is used.

In the case of custom notifications, if `covered` is set to `true` then `null` will be use to replace `${...}`
for non existing attributes (the default behavior when `covered` is not set to `true` is to replace by the
empty string the non existing attributes).

We use the term "covered" in the sense the notification "covers" completely all the attributes
in the `notification.attrs` field. It can be useful for those notification endpoints that are
not flexible enough for a variable set of attributes and needs always the same set of incoming attributes
in every received notification.

Note that covered subscriptions need an explicit list of `attrs` in `notification`. Thus, the following
case is not valid:

```
"notification": {
  ...
  "attrs": [],
  "covered": true
}
```

And if you try to create/update a subscription with that you will get a 400 Bad Request error like this:

```
{
    "description": "covered true cannot be used if notification attributes list is empty",
    "error": "BadRequest"
}
```

## Notification Messages

Notifications include two fields:

* `subscriptionId` represents the concerned subscription that originates the notification
* `data` is an array with the notification data itself which includes the entity and all concerned
  attributes. Each element in the array corresponds to a different entity. By default, the entities
  are represented in `normalized` mode. However, using the `attrsFormat` modifier, a simplified
  representation mode can be requested.

If `attrsFormat` is `normalized` (or if `attrsFormat` is omitted) then default entity representation
is used:

```
{
  "subscriptionId": "12345",
  "data": [
    {
      "id": "Room1",
      "type": "Room",
      "temperature": {
        "value": 23,
        "type": "Number",
        "metadata": {}
      },
      "humidity": {
        "value": 70,
        "type": "percentage",
        "metadata": {}
      }
    },
    {
      "id": "Room2",
      "type": "Room",
      "temperature": {
        "value": 24,
        "type": "Number",
        "metadata": {}
      }
    }
  ]
}
```

If `attrsFormat` is `keyValues` then keyValues partial entity representation mode is used:

```json
{
  "subscriptionId": "12345",
  "data": [
    {
      "id": "Room1",
      "type": "Room",
      "temperature": 23,
      "humidity": 70
    },
    {
      "id": "Room2",
      "type": "Room",
      "temperature": 24
    }
  ]
}
```


If `attrsFormat` is `values` then values partial entity representation mode is used:

```json
{
  "subscriptionId": "12345",
  "data": [ [23, 70], [24] ]
}
```

If `attrsFormat` is `legacy` then subscription representation follows  NGSIv1 format. This way, users 
can benefit from the enhancements of NGSIv2 subscriptions (e.g. filtering) with NGSIv1 legacy notification receivers.

Note that NGSIv1 is deprecated. Thus, we don't recommend to use `legacy` notification format any longer.

```json
{
	"subscriptionId": "56e2ad4e8001ff5e0a5260ec",
	"originator": "localhost",
	"contextResponses": [{
		"contextElement": {
			"type": "Car",
			"isPattern": "false",
			"id": "Car1",
			"attributes": [{
				"name": "temperature",
				"type": "centigrade",
				"value": "26.5",
				"metadatas": [{
					"name": "TimeInstant",
					"type": "recvTime",
					"value": "2015-12-12 11:11:11.123"
				}]
			}]
		},
		"statusCode": {
			"code": "200",
			"reasonPhrase": "OK"
		}
	}]
}
```

Notifications must include the `Ngsiv2-AttrsFormat` (expect when `attrsFormat` is `legacy`) 
HTTP header with the value of the format of the associated subscription, so that notification receivers 
are aware of the format without needing to process the notification payload.

## Custom Notifications

NGSIv2 clients can customize notification messages using a simple template mechanism. The
`notification.httpCustom` property of a subscription allows to specify the following fields
to be templatized when using HTTP notifications:

* `url`
* `headers` (both header name and value can be templatized). Note that `Fiware-Correlator` and
  `Ngsiv2-AttrsFormat` headers cannot be overwritten in custom notifications. Any attempt of 
  doing so (e.g. `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` will be ignored.
* `qs` (both parameter name and value can be templatized)
* `payload`
* `method`, lets the NGSIv2 clients select the HTTP method to be used for delivering
the notification, but note that only valid HTTP verbs can be used: GET, PUT, POST, DELETE, PATCH,
HEAD, OPTIONS, TRACE, and CONNECT.

Regarding the MQTT notifications, the `mqttCustom` is used instead of `httpCustom`. This
topic is described with more detail [in this specific document](user/mqtt_notifications.md).

Macro substitution for templates is based on the syntax `${..}`. In particular:

* `${id}` is replaced by the `id` of the entity
* `${type}` is replaced by the `type` of the entity 
* `${service}` is replaced by the service (i.e. `fiware-service` header value) in the
  update request triggering the subscription.
* `${servicePath}` is replaced by the service path (i.e. `fiware-servicepath` header value) in the
  update request triggering the subscription.
* `${authToken}` is replaced by the authorization token (i.e. `x-auth-token` header value) in the
  update request triggering the subscription.
* Any other `${token}` is replaced by the value of the attribute whose name is `token` or with
  an empty string if the attribute is not included in the notification. If the value is a number,
  a bool or null then its string representation is used. If the value is a JSON array or object
  then its JSON representation as string is used.

In the rare case an attribute was named in the same way of the `${service}`, `${servicePath}` or 
`${authToken}`  (e.g. an attribute which name is `service`) then the attribute value takes precedence.

Example:

Let's consider the following `notification.httpCustom` object in a given subscription.

```
"httpCustom": {
  "url": "http://foo.com/entity/${id}",
  "headers": {
    "Content-Type": "text/plain"
  },
  "method": "PUT",
  "qs": {
    "type": "${type}"
  },
  "payload": "The temperature is ${temperature} degrees"
}
```

Now let's assume that a notification associated to this subscription is triggered, and that the
notification data is for an entity with id "DC_S1-D41" and type "Room", and including an attribute
named "temperature" with the value 23.4.
The resulting notification after applying the template would be:

```
PUT http://foo.com/entity/DC_S1-D41?type=Room
Content-Type: text/plain
Content-Length: 31

The temperature is 23.4 degrees
```

If `payload` is set to `null`, then the notifications associated to that subscription will not 
include any payload (i.e. content-length 0 notifications). Note this is not the same than using 
`payload` set to `""` or omitting the field. In that case, the notification will be sent using 
the NGSIv2 normalized format.

As alternative to `payload` field in `httpCustom` or `mqttCustom`, the `json` field can be
used to generate JSON-based payloads. For instance:

```
"httpCustom": {
   ...
   "json": {
     "t": "${temperature}",
     "h": [ "${humidityMin}", "${humidityMax}" ],
     "v": 4
   }
}
```

Some notes to take into account when using `json` instead of `payload`:

* The value of the `json` field must be an array or object. Although a simple string or number is
  also a valid JSON, these cases are not supported.
* The macro replacement logic works the same way than in `payload` case, with the following
  considerations:
  * It cannot be used in the key part of JSON objects, i.e. `"${key}": 10` will not work
  * The value of the JSON object or JSON array item in which the macro is used has to match
    exactly with the macro expression. Thus, `"t": "${temperature}"` works, but
    `"t": "the temperature is ${temperature}"` or `"h": "humidity ranges from ${humidityMin} to ${humidityMax}"`
    will not work
  * It takes into account the nature of the attribute value to be replaced. For instance,
    `"t": "${temperature}"` resolves to `"t": 10` if temperature attribute is a number or to
    `"t": "10"` if `temperature` attribute is a string.
  * If the attribute doesn't exist in the entity, then `null` value is used
* URL automatic decoding applied to `payload` and `headers` fields (described
  [here](forbidden_characters.md#custom-payload-and-headers-special-treatment)) is not applied
  to `json` field.
* `payload` and `json` cannot be used at the same time
* `Content-Type` header is set to `application/json`, except if overwritten by `headers` field

Some considerations to take into account when using custom notifications:

* It is the NGSIv2 client's responsibility to ensure that after substitution, the notification is a
  correct HTTP message (e.g. if the Content-Type header is application/xml, then the payload must
  correspond to a well-formed XML document). Specifically, if the resulting URL after applying the
  template is malformed, then no notification is sent.
* In case the data to notify contains more than one entity, a separate notification (HTTP message)
  is sent for each of the entities (contrary to default behaviour, which is to send all entities in
  the same HTTP message).
* Due to forbidden characters restriction, Orion applies an extra decoding step to outgoing
  custom notifications. This is described in detail in [this section](user/forbidden_characters.md#custom-payload-special-treatment) of the manual.
* Orion can be configured to disable custom notifications, using the `-disableCustomNotifications`
  [CLI parameter](../admin/cli.md). In this case:
  * `httpCustom` is interpreted as `http`, i.e. all sub-fields except `url` are ignored
  * No `${...}` macro substitution is performed.

Note that if a custom payload is used for the notification (the field `payload` is given in the
corresponding subscription), then a value of `custom` is used for the `Ngsiv2-AttrsFormat` header
in the notification.

An empty string value for a header key in the `headers` object will remove that header from 
notifications. For instance the following configuration:

```
"httpCustom": { 
   ...
   "headers": {"x-auth-token": ""}
}
```

will remove the `x-auth-token` header in notifications associated to the subscription.

This can be useful to remove headers that Orion will include automatically in notifications.
For instance:

* To avoid headers included by default in notifications (e.g. `Accept`)
* To cut the propagation of headers (from updates to notifications), such the
  aforementioned `x-auth-token`

## Subscriptions based in alteration type

By default, a subscription is triggered (i.e. the notification associated to it is sent) when
the triggered condition (expressed in the `subject` and `conditions` fields of the subscription, e.g.
covered entities, list of attributes to check, filter expression, etc.) during a create or actual
update entity operation.

However, this default behavior can be changed so a notification can be sent, for instance,
only when an entity is created or only when an entity is deleted, but not when the entity is
updated.

In particular, the `alterationTypes` field is used, as sub-field of `conditions`. The value
of this field is an array which elements specify a list of alteration types upon which the
subscription is triggered. At the present moment, the following alteration types are supported:

* `entityUpdate`: notification is sent whenever a entity covered by the subscription is updated
  (no matter if the entity actually changed or not)
* `entityChange`: notification is sent whenever a entity covered by the subscription is updated
  and it actually changes (or if it is not an actual update, but `forcedUpdate` option is used 
  in the update request)
* `entityCreate`: notification is sent whenever a entity covered by the subscription is created
* `entityDelete`: notification is sent whenever a entity covered by the subscription is deleted

For instance:

```
  "conditions": {
    "alterationTypes": [ "entityCreate", "entityDelete" ],
    ...
  }
```

will trigger subscription when an entity creation or deletion takes place, but not when an
update takes place. The elements in the `alterationTypes` array are interpreted in OR sense.

Default `alterationTypes` (i.e. the one for subscription not explicitly specifying it)
is `["entityCreate", "entityChange"]`.

The particular alteration type can be got in notifications using the
[`alterationType` builtin attribute](#builtin-attributes).

# API Routes

## Group API Entry Point

### Retrieve API Resources [GET /v2]

This resource does not have any attributes. Instead it offers the initial
API affordances in the form of the links in the JSON body.

It is recommended to follow the “url” link values,
[Link](https://tools.ietf.org/html/rfc5988) or Location headers where
applicable to retrieve resources. Instead of constructing your own URLs,
to keep your client decoupled from implementation details.


_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

This request returns a JSON object with the following elements:
+ entities_url: /v2/entities (required, string) - URL which points to the entities resource
+ types_url: /v2/types (required, string) - URL which points to the types resource
+ subscriptions_url: /v2/subscriptions (required, string) - URL which points to the
  subscriptions resource
+ registrations_url: /v2/registrations (required, string) - URL which points to the
  registrations resource

## Entities Operations

### Entities List

#### List Entities [GET /v2/entities]

Retrieves an array of entities objects following the [JSON Entity Representation](#json-entity-representation), 
that match different criteria by id, type, pattern matching (either id or type)
and/or those which match a query or geographical query (see [Simple Query Language](#simple-query-language) and 
[Geographical Queries](#geographical-queries)). A given entity has to match all the criteria to be retrieved
(i.e., the criteria is combined in a logical AND way). Note that pattern matching query parameters are incompatible
(i.e. mutually exclusive) with their corresponding exact matching parameters, i.e. `idPattern` with `id` and
`typePattern` with `type`.

_**Request query parameters**_

This requests accepts the following URL parameters to customize the request response.

<!-- Use this tool to prettify the table: http://markdowntable.com/ -->
| Parameter     | Optional | Type   | Description                                                                                                                                                                                                            | Example                           |
|---------------|----------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| `id`          | ✓        | string | A comma-separated list of elements. Retrieve entities whose ID matches one of the elements in the list. Incompatible with `idPattern`.                                                                                 | Boe_Idearium                      |
| `type`        | ✓        | string | A comma-separated list of elements. Retrieve entities whose type matches one of the elements in the list. Incompatible with `typePattern`.                                                                             | Room                              |
| `idPattern`   | ✓        | string | A correctly formatted regular expression. Retrieve entities whose ID matches the regular expression. Incompatible with `id`.                                                                                           | Bode_.*                           |
| `typePattern` | ✓        | string | A correctly formatted regular expression. Retrieve entities whose type matches the regular expression. Incompatible with `type`.                                                                                       | Room_.*                           |
| `q`           | ✓        | string | temperature>40 (optional, string) - A query expression, composed of a list of statements separated by `;`, i.e., q=statement1;statement2;statement3. See [Simple Query Language specification](#simple-query-language) | temperature>40                    |
| `mq`          | ✓        | string | A query expression for attribute metadata, composed of a list of statements separated by `;`, i.e., mq=statement1;statement2;statement3. See [Simple Query Language specification](#simple-query-language)             | temperature.accuracy<0.9          |
| `georel`      | ✓        | string | Spatial relationship between matching entities and a reference shape. See [Geographical Queries](#geographical-queries).                                                                                               | near                              |
| `geometry`    | ✓        | string | Geographical area to which the query is restricted.See [Geographical Queries](#geographical-queries).                                                                                                                  | point                             |
| `limit`       | ✓        | number | Limits the number of entities to be retrieved                                                                                                                                                                          | 20                                |
| `offset`      | ✓        | number | Establishes the offset from where entities are retrieved                                                                                                                                                               | 20                                |
| `coords`      | ✓        | string | List of latitude-longitude pairs of coordinates separated by ';'. See [Geographical Queries](#geographical-queries)                                                                                                    | 41.390205,2.154007;48.8566,2.3522 |
| `attrs`       | ✓        | string | Comma-separated list of attribute names whose data are to be included in the response. The attributes are retrieved in the order specified by this parameter. If this parameter is not included, the attributes are retrieved in arbitrary order. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.                                                                                                                                                                                                                                                      | seatNumber                        |
| `metadata`    | ✓        | string | A list of metadata names to include in the response. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.                                                                                              | accuracy                          |
| `orderBy`     | ✓        | string | Criteria for ordering results. See [Ordering Results](#ordering-results) section for details.                                                                                                                                             | temperature,!speed                |
| `options`     | ✓        | string |  A comma-separated list of options for the query. See the following table                                                                                                                                                              | count                     |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                                                    |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `count`     | when used, the total number of entities is returned in the response as an HTTP header named `Fiware-Total-Count`.                                                              |
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details.                             |
| `values`    | when used, the response payload uses the `values` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details.                                |
| `unique`    | when used, the response payload uses the `values` simplified entity representation. Recurring values are left out. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `skipForwarding` | when used, CB skips forwarding to CPrs. The query is evaluated using exclusively CB local context information. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example   |
|----------------------|----------|------------------------------------------------------------------------------------------------|-----------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`    |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`|

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The response payload is an array containing one object per matching entity. Each entity follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections).

Example:

```json
[
  {
    "type": "Room",
    "id": "DC_S1-D41",
    "temperature": {
      "value": 35.6,
      "type": "Number",
      "metadata": {}
    }
  },
  {
    "type": "Room",
    "id": "Boe-Idearium",
    "temperature": {
      "value": 22.5,
      "type": "Number",
      "metadata": {}
    }
  },
  {
    "type": "Car",
    "id": "P-9873-K",
    "speed": {
      "value": 100,
      "type": "number",
      "metadata": {
        "accuracy": {
          "value": 2,
          "type": "Number"
        },
        "timestamp": {
          "value": "2015-06-04T07:20:27.378Z",
          "type": "DateTime"
        }
      }
    }
  }
]
```

#### Create Entity [POST /v2/entities]

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                                              | Example |
|-----------|----------|--------|--------------------------------------------------------------------------|---------|
| `options` | ✓        | string | A comma-separated list of options for the query. See the following table | upsert  |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                        |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `upsert`    | when used, entity is updated if already exits. If upsert is not used and the entity already exist a `422 Unprocessable Entity` error is returned.  |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is an object representing the entity to be created. The object follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections).

Example:

```json
{
  "type": "Room",
  "id": "Bcn-Welt",
  "temperature": {
    "value": 21.7
  },
  "humidity": {
    "value": 60
  },
  "location": {
    "value": "41.3763726, 2.1864475",
    "type": "geo:point",
    "metadata": {
      "crs": {
        "value": "WGS84"
      }
    }
  }
}
```

_**Response code**_

* Successful operation uses 201 Created (if upsert option is not used) or 204 No Content (if
  upsert option is used).
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Response includes a `Location` header with the URL of the created entity.

* Location: /v2/entities/Bcn-Welt?type=Room


### Entity by ID

#### Retrieve Entity [GET /v2/entities/{entityId}]

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                      | Example |
|------------|--------|----------------------------------|---------|
| `entityId` | string | Id of the entity to be retrieved | `Room`  |


_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                                                                                                                                                                                                                                                                                                             | Example      |
|------------|----------|--------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                                                                                                                                                                                                                                                                                              | `Room`       |
| `attrs`    | ✓        | string | Comma-separated list of attribute names whose data must be included in the response. The attributes are retrieved in the order specified by this parameter. If this parameter is not included, the attributes are retrieved in arbitrary order, and all the attributes of the entity are included in the response. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail. | seatNumber   |
| `metadata` | ✓        | string | A list of metadata names to include in the response. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.                                                                                                                                                                                                                                                               | accuracy     |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                                                                                                                                                                                                                                                                                                                | count        |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                                                    |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details.                             |
| `values`    | when used, the response payload uses the `values` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details.                                |
| `unique`    | when used, the response payload uses the `values` simplified entity representation. Recurring values are left out. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `skipForwarding` | when used, CB skips forwarding to CPrs. The query is evaluated using exclusively CB local context information. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The response is an object representing the entity identified by the ID. The object follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections).

Example:

```json
{
  "type": "Room",
  "id": "Bcn_Welt",
  "temperature": {
    "value": 21.7,
    "type": "Number"
  },
  "humidity": {
    "value": 60,
    "type": "Number"
  },
  "location": {
    "value": "41.3763726, 2.1864475",
    "type": "geo:point",
    "metadata": {
      "crs": {
        "value": "WGS84",
        "type": "Text"
      }
    }
  }
}
```

#### Retrieve Entity Attributes [GET /v2/entities/{entityId}/attrs]

This request is similar to retrieving the whole entity, however this one omits the `id` and `type`
fields.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                      | Example |
|------------|--------|----------------------------------|---------|
| `entityId` | string | Id of the entity to be retrieved | `Room`  |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                                                                                                                                                                                                                                                                                                             | Example      |
|------------|----------|--------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                                                                                                                                                                                                                                                                                              | `Room`       |
| `attrs`    | ✓        | string | Comma-separated list of attribute names whose data must be included in the response. The attributes are retrieved in the order specified by this parameter. If this parameter is not included, the attributes are retrieved in arbitrary order, and all the attributes of the entity are included in the response. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail. | seatNumber   |
| `metadata` | ✓        | string | A list of metadata names to include in the response. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.                                                                                                                                                                                                                                                               | accuracy     |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                                                                                                                                                                                                                                                                                                                | count        |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                                                    |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details.                             |
| `values`    | when used, the response payload uses the `values` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details.                                |
| `unique`    | when used, the response payload uses the `values` simplified entity representation. Recurring values are left out. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `skipForwarding` | when used, CB skips forwarding to CPrs. The query is evaluated using exclusively CB local context information. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The payload is an object representing the entity identified by the ID in the URL parameter. The object follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections),
but omitting `id` and `type` fields.

Example:

```json
{
  "temperature": {
    "value": 21.7,
    "type": "Number"
  },
  "humidity": {
    "value": 60,
    "type": "Number"
  },
  "location": {
    "value": "41.3763726, 2.1864475",
    "type": "geo:point",
    "metadata": {
      "crs": {
        "value": "WGS84",
        "type": "Text"
      }
    }
  }
}
```

#### Update or Append Entity Attributes [POST /v2/entities/{entityId}/attrs]

The entity attributes are updated with the ones in the payload, depending on
whether the `append` operation option is used or not.

* If `append` is not used: the entity attributes are updated (if they previously exist) or appended
  (if they don't previously exist) with the ones in the payload.
* If `append` is used (i.e. strict append semantics): all the attributes in the payload not
  previously existing in the entity are appended. In addition to that, in case some of the
  attributes in the payload already exist in the entity, an error is returned.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                    | Example |
|------------|--------|--------------------------------|---------|
| `entityId` | string | Id of the entity to be updated | `Room`  |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                 | Example      |
|------------|----------|--------|---------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`       |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                    | append       |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                        |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `append`    | Force an append operation.                                                                                                                         |
| `overrideMetadata` | Replace the existing metadata with the one provided in the request. See [Metadata update semantics](#metadata-update-semantics) section for details. |
| `forcedUpdate` | Update operation have to trigger any matching subscription, no matter if there is an actual attribute update or no instead of the default behavior, which is to updated only if attribute is effectively updated. Check also the `entityChange` [alteration type](user/subscriptions_alttype.md) for the same effect. |
| `flowControl`  | Enable flow control mechanism, to avoid saturation under high-load scenarios. It is explained in [this section in the documentation](admin/perf_tuning.md#updates-flow-control-mechanism). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is an object with the attributes to append or update in the entity identified by the ID in the URL parameter. The object follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections),
but omitting `id` and `type` fields.

Example:

```json
{
   "ambientNoise": {
     "value": 31.5
   }
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

#### Update Existing Entity Attributes [PATCH /v2/entities/{entityId}/attrs]

The entity attributes are updated with the ones in the payload. In addition to that, if one or more
attributes in the payload doesn't exist in the entity, an error is returned.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                    | Example |
|------------|--------|--------------------------------|---------|
| `entityId` | string | Id of the entity to be updated | `Room`  |

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                                                                 | Example   |
|-----------|----------|--------|---------------------------------------------------------------------------------------------|-----------|
| `type`    | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`    |
| `options` | ✓        | string | A comma-separated list of options for the query. See the following table                    | keyValues |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                        |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `overrideMetadata` | Replace the existing metadata with the one provided in the request. See [Metadata update semantics](#metadata-update-semantics) section for details. |
| `forcedUpdate` | Update operation have to trigger any matching subscription, no matter if there is an actual attribute update or no instead of the default behavior, which is to updated only if attribute is effectively updated. Check also the `entityChange` [alteration type](user/subscriptions_alttype.md) for the same effect. |
| `flowControl`  | Enable flow control mechanism, to avoid saturation under high-load scenarios. It is explained in [this section in the documentation](admin/perf_tuning.md#updates-flow-control-mechanism). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is an object representing the attributes to update in entity identified by the ID in the URL parameter. The object follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections),
but omitting `id` and `type` fields.

Example:

```json
{
  "temperature": {
    "value": 25.5
  },
  "seatNumber": {
    "value": 6
  }
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

#### Replace all entity attributes [PUT /v2/entities/{entityId}/attrs]

New entity attributes in the payload are added to the entity.The attributes previously existing in the entity are removed and replaced by the ones in the
request.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                      | Example |
|------------|--------|----------------------------------|---------|
| `entityId` | string | Id of the entity to be modified. | `Room`  |

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                                                                 | Example   |
|-----------|----------|--------|---------------------------------------------------------------------------------------------|-----------|
| `type`    | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`    |
| `options` | ✓        | string | A comma-separated list of options for the query. See the following table                    | keyValues |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                        |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `forcedUpdate` | Update operation have to trigger any matching subscription, no matter if there is an actual attribute update or no instead of the default behavior, which is to updated only if attribute is effectively updated. Check also the `entityChange` [alteration type](user/subscriptions_alttype.md) for the same effect. |
| `flowControl`  | Enable flow control mechanism, to avoid saturation under high-load scenarios. It is explained in [this section in the documentation](admin/perf_tuning.md#updates-flow-control-mechanism). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is an object representing the new entity attributes added or replaced in the entity identified by the ID in the URL parameter. The object follows
the JSON entity representation format (described in [JSON Entity Representation](#json-entity-representation) section and
side [Simplified Entity Representation](#simplified-entity-representation) and [Partial Representations](#partial-representations) sections),
but omitting `id` and `type` fields.

Example:

```json
{
  "temperature": {
    "value": 25.5
  },
  "seatNumber": {
    "value": 6
  }
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

#### Remove Entity [DELETE /v2/entities/{entityId}]

Delete the entity.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                     | Example |
|------------|--------|---------------------------------|---------|
| `entityId` | string | Id of the entity to be deleted. | `Room`  |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                 | Example      |
|------------|----------|--------|---------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`       |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

### Attributes

#### Get attribute data [GET /v2/entities/{entityId}/attrs/{attrName}]

Returns a JSON object with the attribute data of the attribute. The object follows the JSON
representation for attributes (described in [JSON Attribute Representation](#json-attribute-representation) section).

_**Request URL parameters**_

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                           | Example       |
|------------|--------|---------------------------------------|---------------|
| `entityId` | string | Id of the entity to be retrieved      | `Room`        |
| `attrName` | string | Name of the attribute to be retrieved | `temperature` |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                                               | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                               | `Room`        |
| `metadata` | ✓        | string | A list of metadata names to include in the response. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail. | `accuracy`    |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                                                  | `skipForwarding` |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                         |
|-------------|---------------------------------------------------------------------------------------------------------------------|
| `skipForwarding` | when used, CB skips forwarding to CPrs. The query is evaluated using exclusively CB local context information. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK.
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The response is an object representing the attribute identified by the attribute name given in the URL contained in the
entity identified by the ID. The object follow structure described in the 
[JSON Attribute Representation](#json-attribute-representation) (and side [Partial Representations](#partial-representations) section)

Example:

```json
{
  "value": 21.7,
  "type": "Number",
  "metadata": {}
}
```

#### Update Attribute Data [PUT /v2/entities/{entityId}/attrs/{attrName}]

The request payload is an object representing the new attribute data. Previous attribute data
is replaced by the one in the request. The object follows the JSON representation for attributes
(described in [JSON Attribute Representation](#json-attribute-representation) section).

_**Request URL parameters**_

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                         | Example       |
|------------|--------|-------------------------------------|---------------|
| `entityId` | string | Id of the entity to be updated      | `Room`        |
| `attrName` | string | Name of the attribute to be updated | `Temperature` |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                 | Example            |
|------------|----------|--------|---------------------------------------------------------------------------------------------|--------------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`             |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                    | `overrideMetadata` |

The values that `options` parameter can have for this specific request are:

| Options            | Description                                                                                                                                          |
|--------------------|------------------------------------------------------------------------------------------------------------------------------------------------------|
| `overrideMetadata` | Replace the existing metadata with the one provided in the request. See [Metadata update semantics](#metadata-update-semantics) section for details. |
| `forcedUpdate` | Update operation have to trigger any matching subscription, no matter if there is an actual attribute update or no instead of the default behavior, which is to updated only if attribute is effectively updated. Check also the `entityChange` [alteration type](user/subscriptions_alttype.md) for the same effect. |
| `flowControl`  | Enable flow control mechanism, to avoid saturation under high-load scenarios. It is explained in [this section in the documentation](admin/perf_tuning.md#updates-flow-control-mechanism). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The reques payload is an object representing the attribute identified by the attribute name given in the URL 
contained in the entity identified by the ID. The object follow structure described in the 
[JSON Attribute Representation](#json-attribute-representation) (and side [Partial Representations](#partial-representations) section).

Example:

```json
{
  "value": 25.0,
  "metadata": {
    "unitCode": {
      "value": "CEL"
    }
  }
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

#### Remove a Single Attribute [DELETE /v2/entities/{entityId}/attrs/{attrName}]

Removes an entity attribute from a given entity.

_**Request URL parameters**_

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                         | Example       |
|------------|--------|-------------------------------------|---------------|
| `entityId` | string | Id of the entity to be deleted      | `Room`        |
| `attrName` | string | Name of the attribute to be deleted | `Temperature` |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                 | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`        |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

### Attribute Value

#### Get Attribute Value [GET /v2/entities/{entityId}/attrs/{attrName}/value]

This operation returns the `value` property with the value of the attribute.

_**Request URL parameters**_

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                           | Example    |
|------------|--------|---------------------------------------|------------|
| `entityId` | string | Id of the entity to be retrieved      | `Room`     |
| `attrName` | string | Name of the attribute to be retrieved | `Location` |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                 | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`        |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                 | `skipForwarding` |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                         |
|-------------|---------------------------------------------------------------------------------------------------------------------|
| `skipForwarding` | when used, CB skips forwarding to CPrs. The query is evaluated using exclusively CB local context information. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK.
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

`Content-Type` header with `application/json` or `text/plain` (depending on the response payload)

_**Response payload**_

The response payload can be an object, array, string, number, boolean or null with the value of the attribute.

* If attribute value is JSON Array or Object:
  * If `Accept` header can be expanded to `application/json` or `text/plain` return the value as a JSON with a
    response type of application/json or text/plain (whichever is the first in `Accept` header or
    `application/json` in case of `Accept: */*`).
  * Else return a HTTP error "406 Not Acceptable: accepted MIME types: application/json, text/plain"
* If attribute value is a string, number, null or boolean:
  * If `Accept` header can be expanded to text/plain return the value as text. In case of a string, citation
    marks are used at the beginning and end.
  * Else return a HTTP error "406 Not Acceptable: accepted MIME types: text/plain"

Example:

```json
{
  "address": "Ronda de la Comunicacion s/n",
  "zipCode": 28050,
  "city": "Madrid",
  "country": "Spain"
}
```

#### Update Attribute Value [PUT /v2/entities/{entityId}/attrs/{attrName}/value]

The request payload is the new attribute value.

_**Request URL parameters**_

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                          | Example    |
|------------|--------|--------------------------------------|------------|
| `entityId` | string | Id of the entity to be updated.      | `Room`     |
| `attrName` | string | Name of the attribute to be updated. | `Location` |

_**Request query parameters**_

| Parameter  | Optional | Type   | Description                                                                                 | Example        |
|------------|----------|--------|---------------------------------------------------------------------------------------------|----------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`         |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                    | `forcedUpdate` |

The values that `options` parameter can have for this specific request are:

| Options        | Description                                                                                                                                          |
|----------------|------------------------------------------------------------------------------------------------------------------------------------------------------|
| `forcedUpdate` | Update operation have to trigger any matching subscription, no matter if there is an actual attribute update or no instead of the default behavior, which is to updated only if attribute is effectively updated. Check also the `entityChange` [alteration type](user/subscriptions_alttype.md) for the same effect. |
| `flowControl`  | Enable flow control mechanism, to avoid saturation under high-load scenarios. It is explained in [this section in the documentation](admin/perf_tuning.md#updates-flow-control-mechanism). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type.                                                                                     | `text/plain`       |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload of the request can be a JSON object or array, or plain text, according to the payload MIME type 
specified in the `Content-Type` HTTP header as follow:

* If the request payload MIME type is `application/json`, then the value of the attribute is set to
  the JSON object or array coded in the payload (if the payload is not a valid JSON document,
  then an error is returned).
* If the request payload MIME type is `text/plain`, then the following algorithm is applied to the
  payload:
  * If the payload starts and ends with citation-marks (`"`), the value is taken as a string
    (the citation marks themselves are not considered part of the string)
  * If `true` or `false`, the value is taken as a boolean.
  * If `null`, the value is taken as null.
  * If these first three tests 'fail', the text is interpreted as a number.
  * If not a valid number, then an error is returned and the attribute's value is unchanged.

Example:

```json
{
  "address": "Ronda de la Comunicacion s/n",
  "zipCode": 28050,
  "city": "Madrid",
  "country": "Spain"
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

### Types

#### List Entity Types [GET /v2/type]

If the `values` option is not in use, this operation returns a JSON array with the entity types.
Each element is a JSON object with information about the type:

* `type` : the entity type name.
* `attrs` : the set of attribute names along with all the entities of such type, represented in
  a JSON object whose keys are the attribute names and whose values contain information of such
  attributes (in particular a list of the types used by attributes with that name along with all the
  entities).
* `count` : the number of entities belonging to that type.

If the `values` option is used, the operation returns a JSON array with a list of entity type
names as strings.

Results are ordered by entity `type` in alphabetical order.

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                | Example |
|-----------|----------|--------|--------------------------------------------|---------|
| `limit`   | ✓        | number | Limit the number of types to be retrieved. | `10`    |
| `offset`  | ✓        | number | Skip a number of records.                  | `20`    |
| `options` | ✓        | string | Options dictionary.                        | `count` |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                              |
|----------|------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of types is returned in the HTTP header `Fiware-Total-Count` |
| `values` | When used, the response payload is a JSON array with a list of entity types              |
| `noAttrDetail` | When used, the request does not provide attribute type details. `types` list associated to each attribute name is set to `[]`. Using this option, Orion solves these queries much faster (in some cases saving from 30 seconds to 0.5 seconds). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

This request return a JSON array with an object for each different entity type found, that contains elements:
- `type`. The name of the entity type. The type itself.
- `attrs`. An object that contains all the attributes and the types of each attribute that belongs to that specific type.
- `count`. The amount of entities that have that specific entity type.

Example:

```json
[
  {
    "type": "Car",
    "attrs": {
      "speed": {
        "types": [ "Number" ]
      },
      "fuel": {
        "types": [ "gasoline", "diesel" ]
      },
      "temperature": {
        "types": [ "urn:phenomenum:temperature" ]
      }
    },
    "count": 12
  },
  {
    "type": "Room",
    "attrs": {
      "pressure": {
        "types": [ "Number" ]
      },
      "humidity": {
        "types": [ "percentage" ]
      },
      "temperature": {
        "types": [ "urn:phenomenum:temperature" ]
      }
    },
    "count": 7
  }
]
```

#### Retrieve entity information for a given type [GET /v2/types]

This operation returns a JSON object with information about the type:

* `attrs` : the set of attribute names along with all the entities of such type, represented in
  a JSON object whose keys are the attribute names and whose values contain information of such
  attributes (in particular a list of the types used by attributes with that name along with all the
  entities).
* `count` : the number of entities belonging to that type.

_**Request query parameters**_

| Parameter    | Optional | Type   | Description         | Example |
|--------------|----------|--------|---------------------|---------|
| `options` | ✓        | string | Options dictionary.                        | `noAttrDetail` |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                              |
|----------|------------------------------------------------------------------------------------------|
| `noAttrDetail` | When used, the request does not provide attribute type details. `types` list associated to each attribute name is set to `[]`. Using this option, Orion solves these queries much faster (in some cases saving from 30 seconds to 0.5 seconds). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

This request return a JSON with 2 fields for the entity type retrieved
- `attrs`. An object that contains a object for each type of attributes present on the entities that belongs to that specific type. 
   This object contains an array, `types`, with al the different types found for that attribute in all the entities of the type specified.
- `count`. The amount of entities that have that specific entity type.

Example:

```json
{
  "attrs": {
    "pressure": {
      "types": [ "Number" ]
    },
    "humidity": {
      "types": [ "percentage" ]
    },
    "temperature": {
      "types": [ "urn:phenomenum:temperature" ]
    }
  },
  "count": 7
}
```

## Subscriptions Operations

### Subscription payload datamodel

#### `subscription`

A subscription is represented by a JSON object with the following fields:

| Parameter      | Optional | Type    | Description                                                                                   |
|----------------|----------|---------|-----------------------------------------------------------------------------------------------|
| `id`           |          | string  | Subscription unique identifier. Automatically created at creation time.                       |
| `description`  | ✓        | string  | A free text used by the client to describe the subscription.                                  |
| [`subject`](#subscriptionsubject)   |          | object | An object that describes the subject of the subscription.                 |
| [`notification`](#subscriptionnotification) |          | object | An object that describes the notification to send when the subscription is triggered.         |
| `expires`      | ✓        | ISO8601 | Subscription expiration date in ISO8601 format. Permanent subscriptions must omit this field. |
| `status`       |          | string | Either `active` (for active subscriptions) or `inactive` (for inactive subscriptions). If this field is not provided at subscription creation time, new subscriptions are created with the `active` status, which can be changed by clients afterwards. For expired subscriptions, this attribute is set to `expired` (no matter if the client updates it to `active`/`inactive`). Also, for subscriptions experiencing problems with notifications, the status is set to `failed`. As soon as the notifications start working again, the status is changed back to `active`. Additionaly, `oneshot` value is available, firing the notification only once whenever the entity is updated after creating the subscription. Once a notification is triggered, the subscription transitions to "status": "inactive". |
| `throttling`   | ✓        | number | Minimal period of time in seconds which must elapse between two consecutive notifications. Orion implements this discarding notifications during the throttling guard period. Thus, notifications may be lost if they arrive too close in time. |

Referring to `throttling` field, it is implemented in a local way. In multi-CB configurations (HA scenarios), take into account that the last-notification
measure is local to each Orion node. Although each node periodically synchronizes with the DB in order to get potentially newer
values (more on this [here](../admin/perf_tuning.md#subscription-cache)) it may happen that a particular node has an old value, so throttling
is not 100% accurate.

#### `subscription.subject`

A `subject` contains the following subfields:

| Parameter                                    | Optional | Type   | Description                                                                     |
|----------------------------------------------|----------|--------|---------------------------------------------------------------------------------|
| `entities`                                   | ✓        | string | A list of objects, each one composed of the following subfields: <ul><li><code>id</code> or <code>idPattern</code> Id or pattern of the affected entities. Both cannot be used at the same time, but one of them must be present.</li> <li><code>type</code> or <code>typePattern</code> Type or type pattern of the affected entities. Both cannot be used at the same time. If omitted, it means "any entity type".</li></ul> |
| [`condition`](#subscriptionsubjectcondition) | ✓        | object | Condition to trigger notifications. If omitted, it means "any attribute change will trigger condition" |

#### `subscription.subject.condition`

A `condition` contains the following subfields:

| Parameter    | Optional | Type  | Description                                                                                                                   |
|--------------|----------|-------|-------------------------------------------------------------------------------------------------------------------------------|
| `attrs`      | ✓        | array | Array of attribute names that will trigger the notification.                                                                  |
| `expression` | ✓        | object| An expression composed of `q`, `mq`, `georel`, `geometry` and `coords` (see [List Entities](#list-entities-get-v2entities) operation above about this field) |
| `alterationTypes` | ✓   | array | Specify under which alterations (entity creation, entity modification, etc.) the subscription is triggered (see section [Subscriptions based in alteration type](#subscriptions-based-in-alteration-type)) |

Based on the `condition` field, the notification triggering rules are as follow:

* If `attrs` and `expression` are used, a notification is sent whenever one of the attributes in
  the `attrs` list changes and at the same time `expression` matches.
* If `attrs` is used and `expression` is not used, a notification is sent whenever any of the
  attributes in the `attrs` list changes.
* If `attrs` is not used and `expression` is used, a notification is sent whenever any of the
  attributes of the entity changes and at the same time `expression` matches.
* If neither `attrs` nor `expression` are used, a notification is sent whenever any of the
  attributes of the entity changes.

#### `subscription.notification`

A `notification` object contains the following subfields:

| Parameter          | Optional          | Type    | Description                                                                                                                                                                       |
|--------------------|-------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `attrs` or `exceptAttrs` |          | array | Both cannot be used at the same time. <ul><li><code>attrs</code>: List of attributes to be included in notification messages. It also defines the order in which attributes must appear in notifications when <code>attrsFormat</code> <code>value</code> is used (see [Notification Messages](#notification-messages) section). An empty list means that all attributes are to be included in notifications. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.</li><li><code>exceptAttrs</code>: List of attributes to be excluded from the notification message, i.e. a notification message includes all entity attributes except the ones listed in this field.</li><li>If neither <code>attrs</code> nor <code>exceptAttrs</code> is specified, all attributes are included in notifications.</li></ul>|
| [`http`](#subscriptionnotificationhttp), [`httpCustom`](#subscriptionnotificationhttpcustom), [`mqtt`](#subscriptionnotificationmqtt) or [`mqttCustom`](#subscriptionnotificationmqttcustom)| ✓                 | object | One of them must be present, but not more than one at the same time. It is used to convey parameters for notifications delivered through the transport protocol. |
| `attrsFormat`          | ✓                 | string | Specifies how the entities are represented in notifications. Accepted values are `normalized` (default), `keyValues`, `values` or `legacy`.<br> If `attrsFormat` takes any value different than those, an error is raised. See detail in [Notification Messages](#notification-messages) section. |
| `metadata`         | ✓                 | string  | List of metadata to be included in notification messages. See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.            |
| `onlyChangedAttrs` | ✓                 | boolean | If `true` then notifications will include only attributes that changed in the triggering update request, in combination with the `attrs` or `exceptAttrs` field. (default is `false` if the field is ommitted)) |
| `covered`          | ✓                 | boolean | If `true` then notifications will include all the attributes defined in `attrs` field, even if they are not present in the entity (in this, case, with `null` value). (default value is false). For further information see [Covered subscriptions](#covered-subscriptions) section |
| `timesSent`        | Only on retrieval | number  | Not editable, only present in GET operations. Number of notifications sent due to this subscription.                                                                              |
| `lastNotification` | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Last notification timestamp in ISO8601 format.                                                                                      |
| `lastFailure`      | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Last failure timestamp in ISO8601 format. Not present if subscription has never had a problem with notifications.                   |
| `lastSuccess`      | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Timestamp in ISO8601 format for last successful notification.  Not present if subscription has never had a successful notification. |
| `lastFailureReason`| Only on retrieval | string  | Not editable, only present in GET operations. Describes the cause of the last failure (i.e. the failure occurred at `lastFailure` time). Not included in MQTT subscriptions.|
| `lastSuccessCode`  | Only on retrieval | number  | Not editable, only present in GET operations. the HTTP code (200, 400, 404, 500, etc.) returned by receiving endpoint last time a successful notification was sent (i.e. the success occurred at `lastSuccess` time). Not included in MQTT subscriptions.|
| `failsCounter`     | Only on retrieval | number  | Not editable, only present in GET operations. The number of consecutive failing notifications associated to the subscription. `failsCounter` is increased by one each time a notification attempt fails and reset to 0 if a notification attempt successes (`failsCounter` is ommitted in this case).|
| `maxFailsLimit`    | ✓                 | number  | Establishes a maximum allowed number of consecutive fails. If the number of fails overpasses the value of `maxFailsLimit` (i.e. at a given moment `failsCounter` is greater than `maxFailsLimit`) then Orion automatically passes the subscription to `inactive` state. A subscripiton update operation (`PATCH /v2/subscription/subId`) is needed to re-enable the subscription (setting its state `active` again). |

Regarding `onlyChangedAttrs` field, as an example, if `attrs` is `[A, B, C]` for a given subscription, the default behavior 
(when `onlyChangedAttrs` is `false`) and the triggering update modified only A, then A, B and C are notified (in other 
words, the triggering update doesn't matter). However, if `onlyChangedAttrs` is `true` and the triggering update only 
modified A then only A is included in the notification.

Regarding `lastFailureReason` and `lastSuccessCode`, both can be used to analyze possible problems with notifications. 
See section in the [problem diagnosis procedures document](admin/diagnosis.md#diagnose-notification-reception-problems)
for more details.

Regarding `maxFailsLimit` field, in addition, when Orion automatically disables a subscription, a log trace in WARN 
level is printed. The line have the following format:

```
time=... | lvl=WARN | corr=... | trans=... | from=... | srv=... | subsrv=... | comp=Orion | op=... | msg= Subscription <subId> automatically disabled due to failsCounter (N) overpasses maxFailsLimit (M)
```

#### `subscription.notification.http`

A `http` object contains the following subfields:

| Parameter | Optional | Type   | Description                                                                                   |
|-----------|----------|--------|-----------------------------------------------------------------------------------------------|
| `url`     |          | string | URL referencing the service to be invoked when a notification is generated. An NGSIv2 compliant server must support the `http` URL schema. Other schemas could also be supported. |
| `timeout` | ✓        | number | Maximum time (in milliseconds) the subscription waits for the response. The maximum value allowed for this parameter is 1800000 (30 minutes). If `timeout` is defined to 0 or omitted, then the value passed as `-httpTimeout` CLI parameter is used. See section in the [Command line options](admin/cli.md#command-line-options) for more details. |

#### `subscription.notification.mqtt`

A `mqtt` object contains the following subfields:

| Parameter | Optional | Type   | Description                                                                                                                                |
|-----------|----------|--------|--------------------------------------------------------------------------------------------------------------------------------------------|
| `url`     |          | string | Represent the MQTT broker endpoint to use. URL must start with `mqtt://` and never contains a path (it only includes host and port)        |
| `topic`   |          | string | Represent the MQTT topic to use                                                                                                            |
| `qos`     | ✓        | number | MQTT QoS value to use in the notifications associated to the subscription (0, 1 or 2). If omitted then QoS 0 is used.                      |
| `user`    | ✓        | string | User name used to authenticate the connection with the broker.                                                                             |
| `passwd`  | ✓        | string | Passphrase for the broker authentication. It is always offuscated when retrieving subscription information (e.g. `GET /v2/subscriptions`). |

For further information about MQTT notifications, see the specific [MQTT notifications](user/mqtt_notifications.md) documentation.

#### `subscription.notification.httpCustom`

A `httpCustom` object contains the following subfields.

| Parameter | Optional | Type   | Description                                                                                   |
|-----------|----------|--------|-----------------------------------------------------------------------------------------------|
| `url`     |          | string | Same as in `http` above.                                                                      |
| `headers` | ✓        | object | A key-map of HTTP headers that are included in notification messages.                         |
| `qs`      | ✓        | object | A key-map of URL query parameters that are included in notification messages.                 |
| `method`  | ✓        | string | The method to use when sending the notification (default is POST). Only valid HTTP methods are allowed. On specifying an invalid HTTP method, a 400 Bad Request error is returned.|
| `payload` | ✓        | string | The payload to be used in notifications. If omitted, the default payload (see [Notification Messages](#notification-messages) sections) is used.|
| `timeout` | ✓        | number | Maximum time (in milliseconds) the subscription waits for the response. The maximum value allowed for this parameter is 1800000 (30 minutes). If `timeout` is defined to 0 or omitted, then the value passed as `-httpTimeout` CLI parameter is used. See section in the [Command line options](admin/cli.md#command-line-options) for more details. |

If `httpCustom` is used, then the considerations described in [Custom Notifications](#custom-notifications) section apply.

#### `subscription.notification.mqttCustom`

A `mqttCustom` object contains the following subfields.

| Parameter | Optional | Type   | Description                                                                                                                                |
|-----------|----------|--------|--------------------------------------------------------------------------------------------------------------------------------------------|
| `url`     |          | string | Represent the MQTT broker endpoint to use. URL must start with `mqtt://` and never contains a path (it only includes host and port)        |
| `topic`   |          | string | Represent the MQTT topic to use. Macro replacement is also performed for this field (i.e: a topic based on an attribute )                  |
| `qos`     | ✓        | number | MQTT QoS value to use in the notifications associated to the subscription (0, 1 or 2). If omitted then QoS 0 is used.                      |
| `user`    | ✓        | string | User name used to authenticate the connection with the broker.                                                                             |
| `passwd`  | ✓        | string | Passphrase for the broker authentication. It is always offuscated when retrieving subscription information (e.g. `GET /v2/subscriptions`). |
| `payload` | ✓        | string | The payload to be used in notifications. If omitted, the default payload (see [Notification Messages](#notification-messages) sections) is used.|

If `mqttCustom` is used, then the considerations described in [Custom Notifications](#custom-notifications) section apply. For further information about MQTT notifications, 
see the specific [MQTT notifications](user/mqtt_notifications.md) documentation.

### Subscription List

#### List Subscriptions [GET /v2/subscriptions]

Returns a list of all the subscriptions present in the system.

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                        | Example |
|-----------|----------|--------|----------------------------------------------------|---------|
| `limit`   | ✓        | number | Limit the number of subscriptions to be retrieved. | `10`    |
| `offset`  | ✓        | number | Skip a number of registrations.                    | `20`    |
| `options` | ✓        | string | Options dictionary.                                | `count` |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                                      |
|----------|--------------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of subscriptions is returned in the HTTP header `Fiware-Total-Count` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The payload is an array containing one object per subscription. Each subscription follows the JSON subscription representation 
format (described in [Subscription payload datamodel](#subscription-payload-datamodel) section).

Example:

```json
[
  {
    "id": "62aa3d3ac734067e6f0d0871",
    "description": "One subscription to rule them all",
    "subject": {                    
      "entities": [
        {
          "id": "Bcn_Welt",
          "type": "Room"
        }
      ],
      "condition": {
          "attrs": [ "temperature " ],
          "expression": {
            "q": "temperature>40"
          }
      }
    },
    "notification": {
      "httpCustom": {
        "url": "http://localhost:1234",
        "headers": {
          "X-MyHeader": "foo"
        },
        "qs": {
          "authToken": "bar"
        }
      },
      "attrsFormat": "keyValues",
      "attrs": ["temperature", "humidity"],
      "timesSent": 12,
      "lastNotification": "2015-10-05T16:00:00.00Z",
      "lastFailure": "2015-10-06T16:00:00.00Z"
    },
    "expires": "2025-04-05T14:00:00.00Z",
    "status": "failed",
    "throttling": 5
  }
]
```

#### Create Subscription [POST /v2/subscriptions]

Creates a new subscription.

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is a JSON object containing a subscription that follows the JSON subscription representation 
format (described in ["Subscription payload datamodel](#subscription-payload-datamodel) section).

Example:

```json
{
  "description": "One subscription to rule them all",
  "subject": {
    "entities": [
      {
        "idPattern": ".*",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [ "temperature" ],
      "expression": {
        "q": "temperature>40"
      }
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1234"
    },
    "attrs": ["temperature", "humidity"]
  },            
  "expires": "2025-04-05T14:00:00.00Z",
  "throttling": 5
}
```

_**Response code**_

* Successful operation uses 201 Created
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

* Return the header `Location` with the value of the path used to create the subscription (I.E : `/v2/subscriptions/62aa3d3ac734067e6f0d0871`) 
when the creation succeeds (Response code 201).


### Subscription By ID

#### Retrieve Subscription [GET /v2/subscriptions/{subscriptionId}]

Returns the subscription requested.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                            | Example                    |
|------------------|--------|----------------------------------------|----------------------------|
| `subscriptionId` | string | Id of the subscription to be retrieved | `62aa3d3ac734067e6f0d0871` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The payload is a JSON object containing a subscription that follows the JSON subscription representation 
format (described in ["Subscription payload datamodel](#subscription-payload-datamodel) section).

Example:

```json
{
  "id": "62aa3d3ac734067e6f0d0871",
  "description": "One subscription to rule them all",
  "subject": {
    "entities": [
      {
        "idPattern": ".*",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [ "temperature " ],
      "expression": {
        "q": "temperature>40"
      }
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1234"
    },
    "attrs": ["temperature", "humidity"],
    "timesSent": 12,
    "lastNotification": "2015-10-05T16:00:00.00Z"
    "lastSuccess": "2015-10-05T16:00:00.00Z"
  },
  "expires": "2025-04-05T14:00:00.00Z",
  "status": "active",
  "throttling": 5
}
```

#### Update Subscription [PATCH /v2/subscriptions/{subscriptionId}]

Only the fields included in the request are updated in the subscription.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `subscriptionId` | string | Id of the subscription to be updated | `62aa3d3ac734067e6f0d0871` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is a JSON object containing the fields to be modified of the subscrition following the JSON subscription 
representation format (described in ["Subscription payload datamodel](#subscription-payload-datamodel) section).

Example: 

```json
{
  "expires": "2025-04-05T14:00:00.00Z"
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

#### Delete subscription [DELETE /v2/subscriptions/{subscriptionId}]

Cancels subscription.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `subscriptionId` | string | Id of the subscription to be deleted | `62aa3d3ac734067e6f0d0871` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

## Registration Operations

A Context Registration allows to bind external context information sources so that they can
play the role of providers
of certain subsets (entities, attributes) of the context information space, including those located
at specific geographical areas.

A NGSIv2 server implementation may implement query and/or update forwarding to context information sources. In
particular, some of the following forwarding mechanisms could be implemented (not exhaustive list):

* Legacy forwarding (based on NGSIv1 operations)
* NGSI Context Source Forwarding Specification

Please check the corresponding specification in order to get the details.

### Registration payload datamodel

#### `registration`

A context registration is represented by a JSON object with the following fields:

| Parameter               | Optional | Type   | Description                                                                                                                                                                                 |
|-------------------------|----------|--------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `id`                    |          | string | Unique identifier assigned to the registration. Automatically generated at creation time.                                                                                                   |
| `description`           | ✓        | string | Description given to this registration.                                                                                                                                                     |
| [`provider`](#registrationprovider)              |          | object | Object that describes the context source registered.                                                                                                                                        |
| [`dataProvided`](#registrationdataprovided)          |          | object | Object that describes the data provided by this source.                                                                                                                                     |
| `status`       | ✓        | string | Enumerated field which captures the current status of this registration with the possibles values: [`active`, `inactive`, `expired` or `failed`]. If this field is not provided at registration creation time, new registrations are created with the `active` status, which may be changed by clients afterwards. For expired registrations, this attribute is set to `expired` (no matter if the client updates it to `active`/`inactive`). Also, for registrations experiencing problems with forwarding operations, the status is set to `failed`. As soon as the forwarding operations start working again, the status is changed back to `active`. |
| `expires`               | ✓        | ISO8601 | Registration expiration date in ISO8601 format. Permanent registrations must omit this field.                                                                                               |
| [`forwardingInformation`](#registrationforwardinginformation) |          | object | Information related to the forwarding operations made against the provider. Automatically provided by the implementation, in the case such implementation supports forwarding capabilities. |

#### `registration.provider`

The `provider` field contains the following subfields:

| Parameter      | Optional | Type   | Description                                                                                   |
|----------------|----------|--------|-----------------------------------------------------------------------------------------------|
| `http`         |          | object | It is used to convey parameters for providers that deliver information through the HTTP protocol.(Only protocol supported nowadays). <br>It must contain a subfield named `url` with the URL that serves as the endpoint that offers the providing interface. The endpoint must *not* include the protocol specific part (for instance `/v2/entities`). |
| `supportedForwardingMode`  |          | string | It is used to convey the forwarding mode supported by this context provider. By default `all`. Allowed values are: <ul><li><code>none</code>: This provider does not support request forwarding.</li><li><code>query</code>: This provider only supports request forwarding to query data.</li><li><code>update</code>: This provider only supports request forwarding to update data.</li><li><code>all</code>: This provider supports both query and update forwarding requests. (Default value).</li></ul> |

#### `registration.dataProvided`

The `dataProvided` field contains the following subfields:

| Parameter      | Optional | Type   | Description                                                                                   |
|----------------|----------|--------|-----------------------------------------------------------------------------------------------|
| `entities`     |          | array | A list of objects, each one composed of the following subfields: <ul><li><code>id</code> or <code>idPattern</code>: d or pattern of the affected entities. Both cannot be used at the same time, but one of them must be present.</li><li><code>type</code> or <code>typePattern</code>: Type or pattern of the affected entities. Both cannot be used at the same time. If omitted, it means "any entity type".</li></ul> |
| `attrs`        |          | array | List of attributes to be provided (if not specified, all attributes). |
| `expression`   |          | object | By means of a filtering expression, allows to express what is the scope of the data provided. Currently only geographical scopes are supported through the following subterms: <ul><li><code>georel</code>: Any of the geographical relationships as specified by the [Geographical queries](#geographical-queries) section. </li><li><code>geometry</code>: Any of the supported geometries as specified by the [Geographical queries](#geographical-queries) section.</li> <li><code>coords</code>: String representation of coordinates as specified by the [Geographical queries](#geographical-queries)section.</li></ul> |

#### `registration.forwardingInformation`

The `forwardingInformation` field contains the following subfields:

| Parameter        | Optional          | Type   | Description                                                                                                                                                                            |
|------------------|-------------------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `timesSent`      | Only on retrieval | number | Not editable, only present in GET operations. Number of request forwardings sent due to this registration.                                                                             |
| `lastForwarding` | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Last forwarding timestamp in ISO8601 format.                                                                                             |
| `lastFailure`    | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Last failure timestamp in ISO8601 format. Not present if registration has never had a problem with forwarding.                           |
| `lastSuccess`    | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Timestamp in ISO8601 format for last successful request forwarding. Not present if registration has never had a successful notification. |

### Registration list

#### List Registrations [GET /v2/registrations]

Lists all the context provider registrations present in the system.

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                        | Example |
|-----------|----------|--------|----------------------------------------------------|---------|
| `limit`   | ✓        | number | Limit the number of registrations to be retrieved. | `10`    |
| `offset`  | ✓        | number | Skip a number of registrations.                    | `20`    |
| `options` | ✓        | string | Options dictionary.                                | `count` |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                                      |
|----------|--------------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of registrations is returned in the HTTP header `Fiware-Total-Count` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

A JSON array containing all the registrations represented by an object for each registration following 
[Registratin Payload Datamodel](#registration-payload-datamodel)

Example:

```json
[
  {
    "id": "62aa3d3ac734067e6f0d0871",
    "description": "Example Context Source",
    "dataProvided": {
      "entities": [
        {
          "id": "Bcn_Welt",
          "type": "Room"
        }
      ],
      "attrs": [
        "temperature"
      ]
    },
    "provider": {
      "http": {
        "url": "http://contextsource.example.org"
      },
      "supportedForwardingMode": "all"
    },
    "expires": "2017-10-31T12:00:00",
    "status": "active",
    "forwardingInformation": {
      "timesSent": 12,
      "lastForwarding": "2017-10-06T16:00:00.00Z",
      "lastSuccess": "2017-10-06T16:00:00.00Z",
      "lastFailure": "2017-10-05T16:00:00.00Z"
    }
  }
]
```

#### Create Registration [POST /v2/registrations]

Creates a new context provider registration. This is typically used for binding context sources
as providers of certain data.

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is a JSON object containing a registration that follows the JSON registration representation 
format (described in ["Registration payload datamodel](#registration-payload-datamodel) section). 

Example:

```json
{
  "description": "Relative Humidity Context Source",
  "dataProvided": {
    "entities": [
      {
        "id": "room2",
        "type": "Room"
      }
    ],
    "attrs": [
      "relativeHumidity"
    ]
  },
  "provider": {
    "http":{ 
      "url": "http://localhost:1234"
    }
  }
}
```

_**Response code**_

* Successful operation uses 201 Created
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

The request return a header `Location` with the path of the registration (I.E: `/v2/registrations/62aa3d3ac734067e6f0d0871`) 
when the operation succeeds (Return code 201).

### Registration By ID

#### Retrieve Registration [GET /v2/registrations/{registrationId}]

Returns the registration requested.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                            | Example                    |
|------------------|--------|----------------------------------------|----------------------------|
| `registrationId` | string | Id of the subscription to be retrieved | `62aa3d3ac734067e6f0d0871` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example   |
|----------------------|----------|------------------------------------------------------------------------------------------------|-----------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`    |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`|

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The payload is a JSON object containing a registration that follows the JSON registration representation 
format (described in ["Registration payload datamodel](#registration-payload-datamodel) section). 

Example:

```json
{
      "id": "62aa3d3ac734067e6f0d0871",
      "description": "Example Context Source",
      "dataProvided": {
        "entities": [
          {
            "id": "Bcn_Welt",
            "type": "Room"
          }
        ],
        "attrs": [
          "temperature"
        ]
      },
      "provider": {
        "http": {
          "url": "http://contextsource.example.org"
        },
        "supportedForwardingMode": "all"
      },
      "expires": "2017-10-31T12:00:00",
      "status": "failed",
      "forwardingInformation": {
        "timesSent": 12,
        "lastForwarding": "2017-10-06T16:00:00.00Z",
        "lastFailure": "2017-10-06T16:00:00.00Z",
        "lastSuccess": "2017-10-05T18:25:00.00Z",
      }
}      
```

#### Update Registration [PATCH /v2/registrations/{registrationId}]

Only the fields included in the request are updated in the registration.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `registrationId` | string | Id of the subscription to be updated | `62aa3d3ac734067e6f0d0871` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is a JSON object containing the fields to be modified of the registration following the JSON registration 
representation format (described in ["Registration payload datamodel](#registration-payload-datamodel) section).

Example:

```json
{
    "expires": "2017-10-04T00:00:00"
}
```

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

#### Delete Registration [DELETE /v2/registrations/{registrationId}]

Cancels a context provider registration.

_**Request URL parameters**_

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `registrationId` | string | Id of the subscription to be deleted | `62aa3d3ac734067e6f0d0871` |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Response code**_

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

## Batch Operations

### Update operation

#### Update [POST /v2/op/update]

This operation allows to create, update and/or delete several entities in a single batch operation.

_**Request query parameters**_

| Parameter | Optional | Type   | Description         | Example     |
|-----------|----------|--------|---------------------|-------------|
| `options` | ✓        | string | Options dictionary. | `keyValues` |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                      |
|-------------|--------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the request payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `overrideMetadata` | Replace the existing metadata with the one provided in the request. See [Metadata update semantics](#metadata-update-semantics) section for details. |
| `forcedUpdate` | Update operation have to trigger any matching subscription, no matter if there is an actual attribute update or no instead of the default behavior, which is to updated only if attribute is effectively updated. Check also the `entityChange` [alteration type](user/subscriptions_alttype.md) for the same effect. |
| `flowControl`  | Enable flow control mechanism, to avoid saturation under high-load scenarios. It is explained in [this section in the documentation](admin/perf_tuning.md#updates-flow-control-mechanism). |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The payload is an object with two properties:

+ `actionType`, to specify the kind of update action to do: either `append`, `appendStrict`, `update`,
  `delete`, or `replace`.
+ `entities`, an array of entities, each entity specified using the JSON entity representation format
  (described in the section [JSON Entity Representation](#json-entity-representation)).

This operation is split in as many individual operations as entities in the `entities` vector, so
the `actionType` is executed for each one of them. Depending on the `actionType`, a mapping with
regular non-batch operations can be done:

* `append`: maps to `POST /v2/entities` (if the entity does not already exist) or `POST /v2/entities/<id>/attrs`
  (if the entity already exists).
* `appendStrict`: maps to `POST /v2/entities` (if the entity does not already exist) or
  `POST /v2/entities/<id>/attrs?options=append` (if the entity already exists).
* `update`: maps to `PATCH /v2/entities/<id>/attrs`.
* `delete`: maps to `DELETE /v2/entities/<id>/attrs/<attrName>` on every attribute included in the entity or
  to `DELETE /v2/entities/<id>` if no attribute were included in the entity.
* `replace`: maps to `PUT /v2/entities/<id>/attrs`.

Example:

```json
{
  "actionType": "append",
  "entities": [
    {
      "type": "Room",
      "id": "Bcn-Welt",
      "temperature": {
        "value": 21.7
        },
      "humidity": {
        "value": 60
      }
    },
    {
      "type": "Room",
      "id": "Mad_Aud",
      "temperature": {
        "value": 22.9
      },
      "humidity": {
        "value": 85
      }
    }
  ]
}
```

_**Response code**_

* Successful operation uses 204 No Content.
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

### Query operation

#### Query [POST /v2/op/query]

This operation execture a query among the existing entities based on filters provided in the request payload. 

_**Request query parameters**_

| Parameter | Optional | Type   | Description                                                               | Example              |
|-----------|----------|--------|---------------------------------------------------------------------------|----------------------|
| `limit`   | ✓        | number | Limit the number of entities to be retrieved.                             | `10`                 |
| `offset`  | ✓        | number | Skip a number of records.                                                 | `20`                 |
| `orderBy` | ✓        | string | Criteria for ordering results.See [Ordering Results](#ordering-results) section for details. | `temperature,!speed` |
| `options` | ✓        | string | Options dictionary.                                                       | `count`              |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                                      |
|----------|--------------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of entities returned in the response as an HTTP header named `Fiware-Total-Count` |
| `keyValues`  | When used, the response payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `values`  | When used, the response payload uses the `values` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `unique`  | When used, the response payload uses the `values` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |
| `skipForwarding` | When used, CB skips forwarding to CPrs. The query is evaluated using exclusively CB local context information. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The request payload may contain the following elements (all of them optional):

+ `entities`: a list of entities to search for. Each element is represented by a JSON object with the
  following elements:
    + `id` or `idPattern`: Id or pattern of the affected entities. Both cannot be used at the same
      time, but one of them must be present.
    + `type` or `typePattern`: Type or type pattern of the entities to search for. Both cannot be used at
      the same time. If omitted, it means "any entity type".
+ `attrs`: List of attributes to be provided (if not specified, all attributes).
+ `expression`: an expression composed of `q`, `mq`, `georel`, `geometry` and `coords` (see [List Entities](#list-entities-get-v2entities) operation above about this field).
+ `metadata`: a list of metadata names to include in the response.
   See [Filtering out attributes and metadata](#filtering-out-attributes-and-metadata) section for more detail.

Example:

```json
{
  "entities": [
    {
      "idPattern": ".*",
      "type": "Room"
    },
    {
      "id": "Car",
      "type": "P-9873-K"
    }
  ],
  "attrs": [
    "temperature",
    "humidity"
  ],
  "expression": {
      "q": "temperature>20"
  },
  "metadata": [
    "accuracy",
    "timestamp"
  ]
}
``` 

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.

_**Response headers**_

Successful operations return `Content-Type` header with `application/json` value.

_**Response payload**_

The response payload is an Array containing one object per matching entity, or an empty array `[]` if 
no entities are found. The entities follow the JSON entity representation format
(described in the section [JSON Entity Representation](#json-entity-representation)).

Example:

```json
[
  {
    "type": "Room",
    "id": "DC_S1-D41",
    "temperature": {
      "value": 35.6,
      "type": "Number"
    }
  },
  {
    "type": "Room",
    "id": "Boe-Idearium",
    "temperature": {
      "value": 22.5,
      "type": "Number"
    }
  },
  {
    "type": "Car",
    "id": "P-9873-K",
    "temperature": {
      "value": 40,
      "type": "Number",
      "accuracy": 2,
      "timestamp": {
        "value": "2015-06-04T07:20:27.378Z",
        "type": "DateTime"
      }
    }
  }
]
```

### Notify operation

#### Notify [POST /v2/op/notify]

This operation is intended to consume a notification payload so that all the entity data included by such notification is persisted, overwriting if necessary.
It is useful when one NGSIv2 endpoint is subscribed to another NGSIv2 endpoint (federation scenarios). 
The behavior must be exactly the same as `POST /v2/op/update` with `actionType` equal to `append`. 

_**Request query parameters**_

| Parameter | Optional | Type   | Description         | Example     |
|-----------|----------|--------|---------------------|-------------|
| `options` | ✓        | string | Options dictionary. | `keyValues` |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                      |
|-------------|--------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the request payload uses the `keyValues` simplified entity representation. See [Simplified Entity Representation](#simplified-entity-representation) section for details. |

_**Request headers**_

| Header               | Optional | Description                                                                                    | Example            |
|----------------------|----------|------------------------------------------------------------------------------------------------|--------------------|
| `Content-Type`       |          | MIME type. Required to be `application/json`.                                                  | `application/json` |
| `Fiware-Service`     | ✓        | Tenant or service. See subsection [Multitency](#multitenancy) for more information.            | `acme`             |
| `Fiware-ServicePath` | ✓        | Service path or subservice. See subsection [Service Path](#service-path) for more information. | `/project`         |

_**Request payload**_

The request payload must be an NGSIv2 notification payload, as described in section [Notification Messages](#notification-messages).

Example:

```json
{
  "subscriptionId": "5aeb0ee97d4ef10a12a0262f",
  "data": [{
    "type": "Room",
    "id": "DC_S1-D41",
    "temperature": {
      "value": 35.6,
      "type": "Number"
    }
  },
  {
    "type": "Room",
    "id": "Boe-Idearium",
    "temperature": {
      "value": 22.5,
      "type": "Number"
    }
  }]
}
```

_**Response code**_

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on [Error Responses](#error-responses) for
  more details.
