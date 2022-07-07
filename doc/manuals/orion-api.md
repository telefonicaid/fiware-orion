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
    - [Special Attribute Types](#special-attribute-types)
    - [Builtin Attributes](#builtin-attributes)
    - [Special Metadata Types](#special-metadata-types)
    - [Builtin Metadata](#builtin-metadata)
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
    - [Notification Messages](#notification-messages)
    - [Custom Notifications](#custom-notifications)
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
            - [`subscription.notification`](#subscriptionnotification)
            - [`subscription.notification.http`](#subscriptionnotificationhttp)
            - [`subscription.notification.httpCustom`](#subscriptionnotificationhttpcustom)
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
  attribute and whose representation is described in the "JSON Attribute Representation" section
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
(see the "Partial Representations" section below) may leave some of them out.
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
  associated to the attribute. In responses, this property is set to
  `{}` if the attribute doesn't have any metadata.


## Special Attribute Types

Generally speaking, user-defined attribute types are informative; they are processed by the NGSIv2
server in an opaque way. Nonetheless, the types described below are used to convey a special
meaning:

* `DateTime`:  identifies dates, in ISO8601 format. These attributes can be used with the query
  operators greater-than, less-than, greater-or-equal, less-or-equal and range. For instance
  (only the referred entity attribute is shown):

```
{
  "timestamp": {
    "value": "2017-06-17T07:21:24.238Z",
    "type: "DateTime"
  }
}
```

* `geo:point`, `geo:line`, `geo:box`, `geo:polygon` and `geo:json`. They have special semantics
  related with entity location. See "Geospatial properties of entities" section.

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

Like regular attributes, they can be used in `q` filters and in `orderBy`.
However, they cannot be used in resource URLs.

## Special Metadata Types

Generally speaking, user-defined metadata types are informative; they are processed by the NGSIv2
server in an opaque way. Nonetheless, the types described below are used to convey a special
meaning:

* `DateTime`:  identifies dates, in ISO8601 format. This metadata can be used with the query
  operators greater-than, less-than, greater-or-equal, less-or-equal and range. For instance
  (only the referred attribute metadata is shown):

```
"metadata": {
      "dateCreated": {
        "value": "2019-09-23T03:12:47.213Z",
        "type": "DateTime"
      }
}
```

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

In case a client attempts to use a field that is invalid from a syntax point of view, the client
gets a "Bad Request" error response, explaining the cause.

## Attribute names restrictions

The following strings must not be used as attribute names:

* `id`, as it would conflict with the field used to represent entity id.

* `type`, as it would conflict with the field used to represent entity type.

* `geo:distance`, as it would conflict with the string used in `orderBy` for proximity to
  center point.

* Builtin attribute names (see specific section on "Builtin Attributes")

* `*`, as it has a special meaning as "all the custom/user attributes" (see section on
  "Filtering out attributes and metadata").

## Metadata names restrictions

The following strings must not be used as metadata names:

* Builtin metadata names (see specific section on "Builtin Metadata")

* `*`, as it has a special meaning as "all the custom/user metadata" (see section on
  "Filtering out attributes and metadata").

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
+ Attempt to exceed spatial index limit results in `NoResourceAvailable` (`413`). See "Geospatial properties of entities"
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

Client applications are responsible for defining which entity attributes convey geospatial
properties (by providing an appropriate NGSI attribute type). Typically this is an entity attribute
named `location`, but nothing prevents use cases where an entity contains more than one geospatial
attribute. For instance, locations specified at different granularity levels or provided by
different location methods with different accuracy. 
Nonetheless, it is noteworthy that spatial properties
need special indexes which can be under resource constraints imposed by backend databases.
Thus, implementations may raise errors when spatial index limits are exceeded.
The recommended HTTP status code for those situations is ``413``, *Request entity too large*, and
the reported error on the response payload must be ``NoResourcesAvailable``.

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
 The following geometries (see Simple Location Format) must be supported:

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

```
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

```
{
  "subscriptionId": "12345",
  "data": [ [23, 70], [24] ]
}
```

Notifications must include the `Ngsiv2-AttrsFormat` HTTP header with the value of the format of the
associated subscription, so that notification receivers are aware of the format without
needing to process the notification payload.

## Custom Notifications

NGSIv2 clients can customize HTTP notification messages using a simple template mechanism. The
`notification.httpCustom` property of a subscription allows to specify the following fields
to be templatized:

* `url`
* `headers` (both header name and value can be templatized)
* `qs` (both parameter name and value can be templatized)
* `payload`

The fifth field `method`, lets the NGSIv2 clients select the HTTP method to be used for delivering
the notification, but note that only valid HTTP verbs can be used: GET, PUT, POST, DELETE, PATCH,
HEAD, OPTIONS, TRACE, and CONNECT.


Macro substitution for templates is based on the syntax `${..}`. In particular:

* `${id}` is replaced by the `id` of the entity
* `${type}` is replaced by the `type` of the entity
* Any other `${token}` is replaced by the value of the attribute whose name is `token` or with
  an empty string if the attribute is not included in the notification. If the value is a number,
  a bool or null then its string representation is used. If the value is a JSON array or object
  then its JSON representation as string is used.

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

Some considerations to take into account:

* It is the NGSIv2 client's responsability to ensure that after substitution, the notification is a
  correct HTTP message (e.g. if the Content-Type header is application/xml, then the payload must
  correspond to a well-formed XML document). Specifically, if the resulting URL after applying the
  template is malformed, then no notification is sent.
* In case the data to notify contains more than one entity, a separate notification (HTTP message)
  is sent for each of the entities (contrary to default behaviour, which is to send all entities in
  the same HTTP message).

Note that if a custom payload is used for the notification (the field `payload` is given in the
corresponding subscription), then a value of `custom` is used for the `Ngsiv2-AttrsFormat` header
in the notification.

# API Routes

## Group API Entry Point

### Retrieve API Resources [GET /v2]

This resource does not have any attributes. Instead it offers the initial
API affordances in the form of the links in the JSON body.

It is recommended to follow the “url” link values,
[Link](https://tools.ietf.org/html/rfc5988) or Location headers where
applicable to retrieve resources. Instead of constructing your own URLs,
to keep your client decoupled from implementation details.


+ Response 200 (application/json)

    + Attributes (object)
        + entities_url: /v2/entities (required, string) - URL which points to the entities resource
        + types_url: /v2/types (required, string) - URL which points to the types resource
        + subscriptions_url: /v2/subscriptions (required, string) - URL which points to the
          subscriptions resource
        + registrations_url: /v2/registrations (required, string) - URL which points to the
          registrations resource

## Entities Operations

### Entities List

#### List Entities [GET /v2/entities]

Retrieves a list of entities that match different criteria by id, type, pattern matching (either id or type)
and/or those which match a query or geographical query (see [Simple Query Language](#simple_query_language) and 
[Geographical Queries](#geographical_queries)). A given entity has to match all the criteria to be retrieved
(i.e., the criteria is combined in a logical AND way). Note that pattern matching query parameters are incompatible
(i.e. mutually exclusive) with their corresponding exact matching parameters, i.e. `idPattern` with `id` and
`typePattern` with `type`.

The response payload is an array containing one object per matching entity. Each entity follows
the JSON entity representation format (described in "JSON Entity Representation" section).

**Request query parameters**

This requests accepts the following URL parameters to customize the request response.

<!-- Use this tool to prettify the table: http://markdowntable.com/ -->
| Parameter     | Optional | Type   | Description                                                                                                                                                                                                            | Example                           |
|---------------|----------|--------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|
| `id`          | ✓        | string | A comma-separated list of elements. Retrieve entities whose ID matches one of the elements in the list. Incompatible with `idPattern`.                                                                                 | Boe_Idearium                      |
| `type`        | ✓        | string | A comma-separated list of elements. Retrieve entities whose type matches one of the elements in the list. Incompatible with `typePattern`.                                                                             | Room                              |
| `idPattern`   | ✓        | string | A correctly formatted regular expression. Retrieve entities whose ID matches the regular expression. Incompatible with `id`.                                                                                           | Bode_.*                           |
| `typePattern` | ✓        | string | A correctly formatted regular expression. Retrieve entities whose type matches the regular expression. Incompatible with `type`.                                                                                       | Room_.*                           |
| `q`           | ✓        | string | temperature>40 (optional, string) - A query expression, composed of a list of statements separated by `;`, i.e., q=statement1;statement2;statement3. See [Simple Query Language specification](#simple_query_language) | temperature>40                    |
| `mq`          | ✓        | string | A query expression for attribute metadata, composed of a list of statements separated by `;`, i.e., mq=statement1;statement2;statement3. See [Simple Query Language specification](#simple_query_language)             | temperature.accuracy<0.9          |
| `georel`      | ✓        | string | Spatial relationship between matching entities and a reference shape. See [Geographical Queries](#geographical_queries).                                                                                               | near                              |
| `geometry`    | ✓        | string | Geographical area to which the query is restricted.See [Geographical Queries](#geographical_queries).                                                                                                                  | point                             |
| `limit`       | ✓        | number | Limits the number of entities to be retrieved                                                                                                                                                                          | 20                                |
| `offset`      | ✓        | number | Establishes the offset from where entities are retrieved                                                                                                                                                               | 20                                |
| `coords`      | ✓        | string | List of latitude-longitude pairs of coordinates separated by ';'. See [Geographical Queries](#geographical_queries)                                                                                                    | 41.390205,2.154007;48.8566,2.3522 |
| `attrs`       | ✓        | string | Comma-separated list of attribute names whose data are to be included in the response. The attributes are retrieved in the order specified by this parameter. If this parameter is not included, the attributes are retrieved in arbitrary order. See "Filtering out attributes and metadata" section for more detail.                                                                                                                                                                                                                                                      | seatNumber                        |
| `metadata`    | ✓        | string | A list of metadata names to include in the response. See "Filtering out attributes and metadata" section for more detail.                                                                                              | accuracy                          |
| `orderBy`     | ✓        | string | Criteria for ordering results. See "Ordering Results" section for details.                                                                                                                                             | temperature,!speed                |
| `options`     | ✓        | string |  A comma-separated list of options for the query. See the following table                                                                                                                                                              | count                     |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                                                    |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `count`     | when used, the total number of entities is returned in the response as an HTTP header named `Fiware-Total-Count`.                                                              |
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details.                             |
| `values`    | when used, the response payload uses the `values` simplified entity representation. See "Simplified Entity Representation" section for details.                                |
| `unique`    | when used, the response payload uses the `values` simplified entity representation. Recurring values are left out. See "Simplified Entity Representation" section for details. |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-Type is `application/json`

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

The payload is an object representing the entity to be created. The object follows
the JSON entity representation format (described in a "JSON Entity Representation" section).

**Request query parameters**

| Parameter | Optional | Type   | Description                                                              | Example |
|-----------|----------|--------|--------------------------------------------------------------------------|---------|
| `options` | ✓        | string | A comma-separated list of options for the query. See the following table | upsert  |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                        |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. |
| `upsert`    | when used, entity is updated if already exits. If upsert is not used and the entity already exist a `422 Unprocessable Entity` error is returned.  |

**Request payload**

Content-Type is `application/json`

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

**Response**

* Successful operation uses 201 Created (if upsert option is not used) or 204 No Content (if
  upsert option is used). Response includes a `Location` header with the URL of the
  created entity.
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 201:

Headers:
* Location: /v2/entities/Bcn-Welt?type=Room

Example response 204

Headers:
* Location: /v2/entities/Bcn-Welt?type=Room


### Entity by ID

#### Retrieve Entity [GET /v2/entities/{entityId}]

The response is an object representing the entity identified by the ID. The object follows
the JSON entity representation format (described in "JSON Entity Representation" section).

This operation must return one entity element only, but there may be more than one entity with the
same ID (e.g. entities with same ID but different types).
In such case, an error message is returned, with the HTTP status code set to 409 Conflict. 

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                      | Example |
|------------|--------|----------------------------------|---------|
| `entityId` | string | Id of the entity to be retrieved | `Room`  |


**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                                                                                                                                                                                                                                                                                                             | Example      |
|------------|----------|--------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                                                                                                                                                                                                                                                                                              | `Room`       |
| `attrs`    | ✓        | string | Comma-separated list of attribute names whose data must be included in the response. The attributes are retrieved in the order specified by this parameter. If this parameter is not included, the attributes are retrieved in arbitrary order, and all the attributes of the entity are included in the response. See "Filtering out attributes and metadata" section for more detail. | seatNumber   |
| `metadata` | ✓        | string | A list of metadata names to include in the response. See "Filtering out attributes and metadata" section for more detail.                                                                                                                                                                                                                                                               | accuracy     |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                                                                                                                                                                                                                                                                                                                | count        |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                                                    |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details.                             |
| `values`    | when used, the response payload uses the `values` simplified entity representation. See "Simplified Entity Representation" section for details.                                |
| `unique`    | when used, the response payload uses the `values` simplified entity representation. Recurring values are left out. See "Simplified Entity Representation" section for details. |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for more details.

Example response 200:

Content-Type is `application/json`

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

Just like the general request of getting an entire entity, this operation must return only one
entity element. If more than one entity with the same ID is found (e.g. entities with
same ID but different type), an error message is returned, with the HTTP status code set to
409 Conflict.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                      | Example |
|------------|--------|----------------------------------|---------|
| `entityId` | string | Id of the entity to be retrieved | `Room`  |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                                                                                                                                                                                                                                                                                                             | Example      |
|------------|----------|--------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                                                                                                                                                                                                                                                                                              | `Room`       |
| `attrs`    | ✓        | string | Comma-separated list of attribute names whose data must be included in the response. The attributes are retrieved in the order specified by this parameter. If this parameter is not included, the attributes are retrieved in arbitrary order, and all the attributes of the entity are included in the response. See "Filtering out attributes and metadata" section for more detail. | seatNumber   |
| `metadata` | ✓        | string | A list of metadata names to include in the response. See "Filtering out attributes and metadata" section for more detail.                                                                                                                                                                                                                                                               | accuracy     |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                                                                                                                                                                                                                                                                                                                | count        |


The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                                                    |
|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | when used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details.                             |
| `values`    | when used, the response payload uses the `values` simplified entity representation. See "Simplified Entity Representation" section for details.                                |
| `unique`    | when used, the response payload uses the `values` simplified entity representation. Recurring values are left out. See "Simplified Entity Representation" section for details. |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-Type is `application/json`

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

The request payload is an object representing the attributes to append or update. The object follows
the JSON entity representation format (described in "JSON Entity Representation" section), except
that `id` and `type` are not allowed.

The entity attributes are updated with the ones in the payload, depending on
whether the `append` operation option is used or not.

* If `append` is not used: the entity attributes are updated (if they previously exist) or appended
  (if they don't previously exist) with the ones in the payload.
* If `append` is used (i.e. strict append semantics): all the attributes in the payload not
  previously existing in the entity are appended. In addition to that, in case some of the
  attributes in the payload already exist in the entity, an error is returned.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                    | Example |
|------------|--------|--------------------------------|---------|
| `entityId` | string | Id of the entity to be updated | `Room`  |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                 | Example      |
|------------|----------|--------|---------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`       |
| `options`  | ✓        | string | A comma-separated list of options for the query. See the following table                    | append       |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                                                                        |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. |
| `append`    | Force an append operation.                                                                                                                         |

**Request payload**

Content-Type is `application/json`

Example:

```json
{
   "ambientNoise": {
     "value": 31.5
   }
}
```

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

#### Update Existing Entity Attributes [PATCH /v2/entities/{entityId}/attrs]

The request payload is an object representing the attributes to update. The object follows
the JSON entity representation format (described in "JSON Entity Representation" section), except
that `id` and `type` are not allowed.

The entity attributes are updated with the ones in the payload. In addition to that, if one or more
attributes in the payload doesn't exist in the entity, an error is returned.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                    | Example |
|------------|--------|--------------------------------|---------|
| `entityId` | string | Id of the entity to be updated | `Room`  |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                                                                                                                            | Example      |
|------------|----------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                                                                                                            | `Room`       |
| `options`  | ✓        | string | Only `keyValues` option is allowed for this method. When used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. | keyValues    |

**Request payload**

Content-Type is `application/json`

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

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

#### Replace all entity attributes [PUT /v2/entities/{entityId}/attrs]

The request payload is an object representing the new entity attributes. The object follows
the JSON entity representation format (described in a "JSON Entity Representation" above), except
that `id` and `type` are not allowed.

The attributes previously existing in the entity are removed and replaced by the ones in the
request.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                      | Example |
|------------|--------|----------------------------------|---------|
| `entityId` | string | Id of the entity to be modified. | `Room`  |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                                                                                                                            | Example      |
|------------|----------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                                                                                                            | `Room`       |
| `options`  | ✓        | string | Only `keyValues` option is allowed for this method. When used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. | keyValues    |

**Request payload**

Content-Type is `application/json`

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

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

#### Remove Entity [DELETE /v2/entities/{entityId}]

Delete the entity.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter  | Type   | Description                     | Example |
|------------|--------|---------------------------------|---------|
| `entityId` | string | Id of the entity to be deleted. | `Room`  |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                 | Example      |
|------------|----------|--------|---------------------------------------------------------------------------------------------|--------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`       |

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

### Attributes

#### Get attribute data [GET /v2/entities/{entityId}/attrs/{attrName}]

Returns a JSON object with the attribute data of the attribute. The object follows the JSON
representation for attributes (described in "JSON Attribute Representation" section).

**Request URL parameters**

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                           | Example       |
|------------|--------|---------------------------------------|---------------|
| `entityId` | string | Id of the entity to be retrieved      | `Room`        |
| `attrName` | string | Name of the attribute to be retrieved | `temperature` |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                                               | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id.                               | `Room`        |
| `metadata` | ✓        | string | A list of metadata names to include in the response. See "Filtering out attributes and metadata" section for more detail. | `accuracy`    |

**Response**

* Successful operation uses 200 OK.
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-Type is `application/json`

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
(described in "JSON Attribute Representation" section).

Response:

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

**Request URL parameters**

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                         | Example       |
|------------|--------|-------------------------------------|---------------|
| `entityId` | string | Id of the entity to be updated      | `Room`        |
| `attrName` | string | Name of the attribute to be updated | `Temperature` |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                 | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`        |

**Request payload**

Content-Type is `application/json`

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

#### Remove a Single Attribute [DELETE /v2/entities/{entityId}/attrs/{attrName}]

Removes an entity attribute.

Response:

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

**Request URL parameters**

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                         | Example       |
|------------|--------|-------------------------------------|---------------|
| `entityId` | string | Id of the entity to be deleted      | `Room`        |
| `attrName` | string | Name of the attribute to be deleted | `Temperature` |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                 | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`        |


### Attribute Value

#### Get Attribute Value [GET /v2/entities/{entityId}/attrs/{attrName}/value]

This operation returns the `value` property with the value of the attribute.

* If attribute value is JSON Array or Object:
  * If `Accept` header can be expanded to `application/json` or `text/plain` return the value as a JSON with a
    response type of application/json or text/plain (whichever is the first in `Accept` header or
    `application/json` in case of `Accept: */*`).
  * Else return a HTTP error "406 Not Acceptable: accepted MIME types: application/json, text/plain"
* If attribute value is a string, number, null or boolean:
  * If `Accept` header can be expanded to text/plain return the value as text. In case of a string, citation
    marks are used at the beginning and end.
  * Else return a HTTP error "406 Not Acceptable: accepted MIME types: text/plain"

**Request URL parameters**

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                           | Example    |
|------------|--------|---------------------------------------|------------|
| `entityId` | string | Id of the entity to be retrieved      | `Room`     |
| `attrName` | string | Name of the attribute to be retrieved | `Location` |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                 | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`        |

**Response**

* Successful operation uses 200 OK.
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Response 200 example:

Content-Type is `application/json`

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

The payload MIME type in the request is specified in the `Content-Type` HTTP header.

**Request URL parameters**

Those parameter are part of the URL request. They are mandatory. 

| Parameter  | Type   | Description                          | Example    |
|------------|--------|--------------------------------------|------------|
| `entityId` | string | Id of the entity to be updated.      | `Room`     |
| `attrName` | string | Name of the attribute to be updated. | `Location` |

**Request query parameters**

| Parameter  | Optional | Type   | Description                                                                                 | Example       |
|------------|----------|--------|---------------------------------------------------------------------------------------------|---------------|
| `type`     | ✓        | string | Entity type, to avoid ambiguity in case there are several entities with the same entity id. | `Room`        |

**Request payload**

Content-type is `application/json` or `text/plain`

```json
{
  "address": "Ronda de la Comunicacion s/n",
  "zipCode": 28050,
  "city": "Madrid",
  "country": "Spain"
}
```

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
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

**Request query parameters**

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

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-Types is `application/json`

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

**Request query parameters**

| Parameter    | Optional | Type   | Description  | Example |
|--------------|----------|--------|--------------|---------|
| `entityType` |          | string | Entity Type. | `Room`  |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Response 200 example:

Content-Type is `application/json`

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

| Parameter      | Optional | Type   | Description                                                                                   |
|----------------|----------|--------|-----------------------------------------------------------------------------------------------|
| `id`           |          | string | Subscription unique identifier. Automatically created at creation time.                       |
| `description`  | ✓        | string | A free text used by the client to describe the subscription.                                  |
| [`subject`](#subscriptionsubject)      |          | object | An object that describes the subject of the subscription.                                     |
| [`notification`](#subscriptionnotification) |          | object | An object that describes the notification to send when the subscription is triggered.         |
| `expires`      | ✓        | ISO8601 | Subscription expiration date in ISO8601 format. Permanent subscriptions must omit this field. |
| `status`       |          | string | Either `active` (for active subscriptions) or `inactive` (for inactive subscriptions). If this field is not provided at subscription creation time, new subscriptions are created with the `active` status, which can be changed by clients afterwards. For expired subscriptions, this attribute is set to `expired` (no matter if the client updates it to `active`/`inactive`). Also, for subscriptions experiencing problems with notifications, the status is set to `failed`. As soon as the notifications start working again, the status is changed back to `active`.                                                                                              |
| `throttling`   | ✓        | number | Minimal period of time in seconds which must elapse between two consecutive notifications.    |

#### `subscription.subject`

A `subject` contains the following subfields:

| Parameter      | Optional | Type   | Description                                                                                   |
|----------------|----------|--------|-----------------------------------------------------------------------------------------------|
| `entities`     | ✓        | array| A list of objects, each one composed of the following subfields: <ul><li><code>id</code> or <code>idPattern</code> Id or pattern of the affected entities. Both cannot be used at the same time, but one of them must be present.</li> <li><code>type</code> or <code>typePattern</code> Type or type pattern of the affected entities. Both cannot be used at the same time. If omitted, it means "any entity type".</li></ul> |
| `condition`    | ✓        | object| Condition to trigger notifications. This field is optional and it may contain two properties, both optional: <ul><li><code>attrs</code> Array of attribute names that will trigger the notification. </li> <li><code>expression</code> An expression composed of <code>q</code>, <code>mq</code>, <code>georel</code>, <code>geometry</code> and <code>coords</code> (see "List entities" operation above about this field)</li></ul> |

#### `subscription.notification`

A `notification` object contains the following subfields:

| Parameter              | Optional          | Type   | Description                                                                                                                                                                                                                                                     |
|------------------------|-------------------|--------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `attrs` or `exceptAttrs` |          | array | Both cannot be used at the same time. <ul><li><code>attrs</code>: List of attributes to be included in notification messages. It also defines the order in which attributes must appear in notifications when <code>attrsFormat</code> <code>value</code> is used (see "Notification Messages" section). An empty list means that all attributes are to be included in notifications. See "Filtering out attributes and metadata" section for more detail.</li><li><code>exceptAttrs</code>: List of attributes to be excluded from the notification message, i.e. a notification message includes all entity attributes except the ones listed in this field.</li><li>If neither <code>attrs</code> nor <code>exceptAttrs</code> is specified, all attributes are included in notifications.</li></ul>|
| [`http`](#subscriptionnotificationhttp) or [`httpCustom`](#subscriptionnotificationhttpcustom) | ✓                 | object | One of them must be present, but not both at the same time. It is used to convey parameters for notifications delivered through the HTTP protocol.                                                                                                              |
| `attrsFormat`          | ✓                 | string | Specifies how the entities are represented in notifications. Accepted values are `normalized` (default), `keyValues` or `values`.<br> If `attrsFormat` takes any value different than those, an error is raised. See detail in "Notification Messages" section. |
| `metadata`             | ✓                 | string | List of metadata to be included in notification messages. See "Filtering out attributes and metadata" section for more detail.                                                                                                                                  |
| `timesSent`            | Only on retrieval | number | Not editable, only present in GET operations. Number of notifications sent due to this subscription.                                                                                                                                                            |
| `lastNotification`     | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Last notification timestamp in ISO8601 format.                                                                                                                                                                    |
| `lastFailure`          | Only on retrieval | ISO8601  | Not editable, only present in GET operations. Last failure timestamp in ISO8601 format. Not present if subscription has never had a problem with notifications.                                                                                                 |
| `lastSuccess`          | Only on retrieval | ISO8601 | Not editable, only present in GET operations. Timestamp in ISO8601 format for last successful notification.  Not present if subscription has never had a successful notification.                                                                               |

#### `subscription.notification.http`

An `http` object contains the following subfields:

| Parameter | Optional | Type   | Description                                                                                   |
|-----------|----------|--------|-----------------------------------------------------------------------------------------------|
| `url`     |          | string | URL referencing the service to be invoked when a notification is generated. An NGSIv2 compliant server must support the `http` URL schema. Other schemas could also be supported. |

#### `subscription.notification.httpCustom`

An `httpCustom` object contains the following subfields.

| Parameter | Optional | Type   | Description                                                                                   |
|-----------|----------|--------|-----------------------------------------------------------------------------------------------|
| `url`     |          | string | Same as in `http` above.                                                                      |
| `headers` | ✓        | object | A key-map of HTTP headers that are included in notification messages.                         |
| `qs`      | ✓        | object | A key-map of URL query parameters that are included in notification messages.                 |
| `method`  | ✓        | string | The method to use when sending the notification (default is POST). Only valid HTTP methods are allowed. On specifying an invalid HTTP method, a 400 Bad Request error is returned.|
| `payload` | ✓        | string | The payload to be used in notifications. If omitted, the default payload (see "Notification Messages" sections) is used.|

If `httpCustom` is used, then the considerations described in "Custom Notifications" section apply.

Notification rules are as follow:

* If `attrs` and `expression` are used, a notification is sent whenever one of the attributes in
  the `attrs` list changes and at the same time `expression` matches.
* If `attrs` is used and `expression` is not used, a notification is sent whenever any of the
  attributes in the `attrs` list changes.
* If `attrs` is not used and `expression` is used, a notification is sent whenever any of the
  attributes of the entity changes and at the same time `expression` matches.
* If neither `attrs` nor `expression` are used, a notification is sent whenever any of the
  attributes of the entity changes.

### Subscription List

#### List Subscriptions [GET /v2/subscriptions]

Returns a list of all the subscriptions present in the system.

**Request query parameters**

| Parameter | Optional | Type   | Description                                        | Example |
|-----------|----------|--------|----------------------------------------------------|---------|
| `limit`   | ✓        | number | Limit the number of subscriptions to be retrieved. | `10`    |
| `offset`  | ✓        | number | Skip a number of registrations.                    | `20`    |
| `options` | ✓        | string | Options dictionary.                                | `count` |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                                      |
|----------|--------------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of subscriptions is returned in the HTTP header `Fiware-Total-Count` |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

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
The subscription is represented by a JSON object as described at the beginning of this section.

**Request payload**

Content-Type is `application/json`

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

**Response**

* Successful operation uses 201 Created
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 201:

Headers: 
* Location: /v2/subscriptions/62aa3d3ac734067e6f0d0871


### Subscription By ID

#### Retrieve Subscription [GET /v2/subscriptions/{subscriptionId}]

The response is the subscription represented by a JSON object as described at the beginning of this
section.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                            | Example                    |
|------------------|--------|----------------------------------------|----------------------------|
| `subscriptionId` | string | Id of the subscription to be retrieved | `62aa3d3ac734067e6f0d0871` |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-Type is `application/json`

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

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `subscriptionId` | string | Id of the subscription to be updated | `62aa3d3ac734067e6f0d0871` |

**Request payload**

Content-Type is `application/json`

```json
{
  "expires": "2025-04-05T14:00:00.00Z"
}
```

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

#### Delete subscription [DELETE /v2/subscriptions/{subscriptionId}]

Cancels subscription.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `subscriptionId` | string | Id of the subscription to be deleted | `62aa3d3ac734067e6f0d0871` |

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
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
| `http`         |          | string | It is used to convey parameters for providers that deliver information through the HTTP protocol.(Only protocol supported nowadays). <br>It must contain a subfield named `url` with the URL that serves as the endpoint that offers the providing interface. The endpoint must *not* include the protocol specific part (for instance `/v2/entities`). |
| `supportedForwardingMode`  |          | string | It is used to convey the forwarding mode supported by this context provider. By default `all`. Allowed values are: <ul><li><code>none</code>: This provider does not support request forwarding.</li><li><code>query</code>: This provider only supports request forwarding to query data.</li><li><code>update</code>: This provider only supports request forwarding to update data.</li><li><code>all</code>: This provider supports both query and update forwarding requests. (Default value).</li></ul> |

#### `registration.dataProvided`

The `dataProvided` field contains the following subfields:

| Parameter      | Optional | Type   | Description                                                                                   |
|----------------|----------|--------|-----------------------------------------------------------------------------------------------|
| `entities`     |          | array | A list of objects, each one composed of the following subfields: <ul><li><code>id</code> or <code>idPattern</code>: d or pattern of the affected entities. Both cannot be used at the same time, but one of them must be present.</li><li><code>type</code> or <code>typePattern</code>: Type or pattern of the affected entities. Both cannot be used at the same time. If omitted, it means "any entity type".</li></ul> |
| `attrs`        |          | array | List of attributes to be provided (if not specified, all attributes). |
| `expression`   |          | object | By means of a filtering expression, allows to express what is the scope of the data provided. Currently only geographical scopes are supported through the following subterms: <ul><li><code>georel</code>: Any of the geographical relationships as specified by the Geoqueries section of this specification. </li><li><code>geometry</code>: Any of the supported geometries as specified by the Geoqueries section of this specification.</li> <li><code>coords</code>: String representation of coordinates as specified by the Geoqueries section of this specification.</li></ul> |

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

**Request query parameters**

| Parameter | Optional | Type   | Description                                        | Example |
|-----------|----------|--------|----------------------------------------------------|---------|
| `limit`   | ✓        | number | Limit the number of registrations to be retrieved. | `10`    |
| `offset`  | ✓        | number | Skip a number of registrations.                    | `20`    |
| `options` | ✓        | string | Options dictionary.                                | `count` |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                                      |
|----------|--------------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of registrations is returned in the HTTP header `Fiware-Total-Count` |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

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
The registration is represented by a JSON object as described at the beginning of this section.

**Request payload** 

Content-Type is `application/json`

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

**Response**

* Successful operation uses 201 Created
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 201:

Headers: 
* Location: /v2/registrations/62aa3d3ac734067e6f0d0871

### Registration By ID

#### Retrieve Registration [GET /v2/registrations/{registrationId}]

The response is the registration represented by a JSON object as described at the beginning of this
section.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                            | Example                    |
|------------------|--------|----------------------------------------|----------------------------|
| `registrationId` | string | Id of the subscription to be retrieved | `62aa3d3ac734067e6f0d0871` |

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-Type is `application/json`

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

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `registrationId` | string | Id of the subscription to be updated | `62aa3d3ac734067e6f0d0871` |

**Request payload** 

Content-Type is `application/json`

```json
{
    "expires": "2017-10-04T00:00:00"
}
```

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

#### Delete Registration [DELETE /v2/registrations/{registrationId}]

Cancels a context provider registration.

**Request URL parameters**

This parameter is part of the URL request. It is mandatory. 

| Parameter        | Type   | Description                          | Example                    |
|------------------|--------|--------------------------------------|----------------------------|
| `registrationId` | string | Id of the subscription to be deleted | `62aa3d3ac734067e6f0d0871` |

**Response**

* Successful operation uses 204 No Content
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

## Batch Operations

### Update operation

#### Update [POST /v2/op/update]

This operation allows to create, update and/or delete several entities in a single batch operation.
The payload is an object with two properties:

+ `actionType`, to specify the kind of update action to do: either `append`, `appendStrict`, `update`,
  `delete`, or `replace`.
+ `entities`, an array of entities, each entity specified using the JSON entity representation format
  (described in the section "JSON Entity Representation").

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

**Request query parameters**

| Parameter | Optional | Type   | Description         | Example     |
|-----------|----------|--------|---------------------|-------------|
| `options` | ✓        | string | Options dictionary. | `keyValues` |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                      |
|-------------|--------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the request payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. |

**Request payload**

Content-type is `application/json`

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

**Response**

* Successful operation uses 204 No Content.
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

### Query operation

#### Query [POST /v2/op/query]

The response payload is an Array containing one object per matching entity, or an empty array `[]` if 
no entities are found. The entities follow the JSON entity representation format
(described in the section "JSON Entity Representation").

The payload may contain the following elements (all of them optional):

+ `entities`: a list of entities to search for. Each element is represented by a JSON object with the
  following elements:
    + `id` or `idPattern`: Id or pattern of the affected entities. Both cannot be used at the same
      time, but one of them must be present.
    + `type` or `typePattern`: Type or type pattern of the entities to search for. Both cannot be used at
      the same time. If omitted, it means "any entity type".
+ `attrs`: List of attributes to be provided (if not specified, all attributes).
+ `expression`: an expression composed of `q`, `mq`, `georel`, `geometry` and `coords` (see "List
   entities" operation above about this field).
+ `metadata`: a list of metadata names to include in the response.
   See "Filtering out attributes and metadata" section for more detail.

**Request query parameters**

| Parameter | Optional | Type   | Description                                                               | Example              |
|-----------|----------|--------|---------------------------------------------------------------------------|----------------------|
| `limit`   | ✓        | number | Limit the number of entities to be retrieved.                             | `10`                 |
| `offset`  | ✓        | number | Skip a number of records.                                                 | `20`                 |
| `orderBy` | ✓        | string | Criteria for ordering results.See "Ordering Results" section for details. | `temperature,!speed` |
| `options` | ✓        | string | Options dictionary.                                                       | `count`              |

The values that `options` parameter can have for this specific request are:

| Options  | Description                                                                                      |
|----------|--------------------------------------------------------------------------------------------------|
| `count`  | When used, the total number of entities returned in the response as an HTTP header named `Fiware-Total-Count` |
| `keyValues`  | When used, the response payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. |
| `values`  | When used, the response payload uses the `values` simplified entity representation. See "Simplified Entity Representation" section for details. |
| `unique`  | When used, the response payload uses the `values` simplified entity representation. See "Simplified Entity Representation" section for details. |

**Request payload**

Content-Type is `application/json`

```
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

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.

Example response 200:

Content-type is `application/json`

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
This operation is useful when one NGSIv2 endpoint is subscribed to another NGSIv2 endpoint (federation scenarios). 
The request payload must be an NGSIv2 notification payload. 
The behavior must be exactly the same as `POST /v2/op/update` with `actionType` equal to `append`. 

**Request query parameters**

| Parameter | Optional | Type   | Description         | Example     |
|-----------|----------|--------|---------------------|-------------|
| `options` | ✓        | string | Options dictionary. | `keyValues` |

The values that `options` parameter can have for this specific request are:

| Options     | Description                                                                                      |
|-------------|--------------------------------------------------------------------------------------------------|
| `keyValues` | When used, the request payload uses the `keyValues` simplified entity representation. See "Simplified Entity Representation" section for details. |

**Request payload**

Content-Type is `application/json`

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

**Response**

* Successful operation uses 200 OK
* Errors use a non-2xx and (optionally) an error payload. See subsection on "Error Responses" for
  more details.
