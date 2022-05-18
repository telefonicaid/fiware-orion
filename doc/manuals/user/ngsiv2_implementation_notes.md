# <a name="top"></a>NGSIv2 Implementation Notes

* [Forbidden characters](#forbidden-characters)
* [Update operators for attribute values](#update-operators-for-attribute-values)
* [Metadata update semantics](#metadata-update-semantics)
* [Custom payload decoding on notifications](#custom-payload-decoding-on-notifications)
* [Option to disable custom notifications](#option-to-disable-custom-notifications)
* [Non-modifiable headers in custom notifications](#non-modifiable-headers-in-custom-notifications)
* [Header removal in custom notifications](#header-removal-in-custom-notifications)
* [Limit to attributes for entity location](#limit-to-attributes-for-entity-location)
* [Supported GeoJSON types in `geo:json` attributes](#supported-geojson-types-in-geojson-attributes)
* [Legacy attribute format in notifications](#legacy-attribute-format-in-notifications)
* [Datetime support](#datetime-support)
* [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name)
* [Subscription payload validations](#subscription-payload-validations)
* [`alterationType` attribute](#alterationtype-attribute)
* [`actionType` metadata](#actiontype-metadata)
* [`ignoreType` metadata](#ignoretype-metadata)
* [`noAttrDetail` option](#noattrdetail-option)
* [Notification throttling](#notification-throttling)
* [Ordering between different attribute value types](#ordering-between-different-attribute-value-types)
* [Oneshot subscriptions](#oneshot-subscriptions)
* [Subscriptions based in alteration type](#subscriptions-based-in-alteration-type)
* [Custom notifications without payload](#custom-notifications-without-payload)
* [MQTT notifications](#mqtt-notifications)
* [Notify only attributes that change](#notify-only-attributes-that-change)
* [Covered subscriptions](#covered-subscriptions)
* [`timeout` subscriptions option](#timeout-subscriptions-option)
* [`lastFailureReason` and `lastSuccessCode` subscriptions fields](#lastfailurereason-and-lastsuccesscode-subscriptions-fields)
* [`failsCounter` and `maxFailsLimit` subscriptions fields](#failscounter-and-maxfailslimit-subscriptions-fields)
* [Ambiguous subscription status `failed` not used](#ambiguous-subscription-status-failed-not-used)
* [`forcedUpdate` option](#forcedupdate-option)
* [`flowControl` option](#flowcontrol-option)
* [Registrations](#registrations)
* [`skipForwarding` option](#skipforwarding-option)
* [`null` support in DateTime and geolocation types](#null-support-in-datetime-and-geolocation-types)
* [`keyValues` not supported in `POST /v2/op/notify`](#keyvalues-not-supported-in-post-v2opnotify)
* [Deprecated features](#deprecated-features)

This document describes some considerations to take into account
regarding the specific implementation done by Orion Context Broker
of the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

## Forbidden characters

From "Field syntax restrictions" section at NGSIv2 specification:

> In addition to the above rules, given NGSIv2 server implementations could add additional
> syntactical restrictions in those or other fields, e.g., to avoid cross script injection attacks.

The additional restrictions that apply to Orion are the ones describe in the
[forbidden characters](forbidden_characters.md) section of the manual.

Note that you can use "TextUnrestricted" attribut type (and special attribute type beyond
the ones defined in the NGSIv2 Specification) in order to skip forbidden characters checkings
in the attribute value. However, it could have security implications (possible script
injections attacks) so use it at your own risk!

[Top](#top)

## Update operators for attribute values

Some attribute value updates has special semantics, beyond the ones described in the
NGSIv2 specification. In particular we can do requests like this one:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$inc": 3 },
  "type": "Number"
}
```

which means *"increase the value of attribute A by 3"*.

This functionality is usefeul to reduce the complexity of applications and avoid
race conditions in applications that access simultaneously to the same piece of
context. More detail in [specific documentation](update_operators.md).

[Top](#top)

## Metadata update semantics

The metadata update semantics used by Orion Context Broker (and the
related `overrideMetadata` option are detailed in
[this section of the documentation](metadata.md#updating-metadata).

Moreover, from NGSIv2 specification section "Partial Representations":

> Attribute `metadata` may be omitted in requests, meaning that there are no metadata
> elements associated to the attribute.

Depending if `overrideMetadata` is used or not, this sentence has two interpretations:

* If `overrideMetadata` is not used (default behaviour) it is interpreted as
  "... meaning that there are no metadata elements associated to the attribute,
  **which need to be updated**"
* If `overrideMetadata` is used it is interpreted as
  "... meaning that there are no metadata elements associated to the attribute,
  **as a result of the the attribute update**"

[Top](#top)

## Custom payload decoding on notifications

Due to forbidden characters restriction, Orion applies an extra decoding step to outgoing
custom notifications. This is described in detail in [this section](forbidden_characters.md#custom-payload-special-treatment)
of the manual.

[Top](#top)

## Option to disable custom notifications

Orion can be configured to disable custom notifications, using the `-disableCustomNotifications` [CLI parameter](../admin/cli.md).

In this case:

* `httpCustom` is interpreted as `http`, i.e. all sub-fields except `url` are ignored
* No `${...}` macro substitution is performed.

[Top](#top)

## Non-modifiable headers in custom notifications

The following headers cannot be overwritten in custom notifications:

* `Fiware-Correlator`
* `Ngsiv2-AttrsFormat`

Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` will be
ignored.

[Top](#top)

## Header removal in custom notifications

It is not explicilty said in NGSIv2 specification ("Custom Notifications" section) but an empty
string value for a header key in the `headers` object will remove that header from notifications.
For instance the following configuration:

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

[Top](#top)

## Limit to attributes for entity location

From "Geospatial properties of entities" section at NGSIv2 specification:

> Client applications are responsible for defining which entity attributes convey geospatial properties
> (by providing an appropriate NGSI attribute type). Typically this is an entity attribute named `location`,
> but nothing prevents use cases where an entity contains more than one geospatial attribute. For instance,
> locations specified at different granularity levels or provided by different location methods with different
> accuracy. Nonetheless, it is noteworthy that spatial properties need special indexes which can be under resource
> constraints imposed by backend databases. Thus, implementations may raise errors when spatial index limits are
> exceeded. The recommended HTTP status code for those situations is `413`, *Request entity too large*, and the
> reported error on the response payload must be `NoResourcesAvailable`.

In the case of Orion, that limit is one (1) attribute.

However, you can set `ignoreType` metadata to `true` to mean that a given attribute contains an extra informative
location (more detail in [this section of the documentation](#ignoretype-metadata)). This disables Orion
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

[Top](#top)

## Supported GeoJSON types in `geo:json` attributes

NGSIv2 specification doesn't specify any limitation in the possible GeoJSON types to be used for
`geo:json` attributes. However, the current implementation in Orion (based in
the [MongoDB capabilities](https://www.mongodb.com/docs/manual/reference/geojson/)) introduces
some limitations.

We have successfully tested the following types:

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

With regards to `FeatureCollection`, it is only accepted at creation/update time only if it contains a single 
`Feature` (i.e. the `features` field has only one element). Otherwise , Orion would return an `BadRequest`error.

The only GeoJSON type not supported at all is `GeometryCollection`. You will get a "Database Error"
if you try to use them).

## Legacy attribute format in notifications

Apart from the values described for `attrsFormat` in the NGSIv2 specification, Orion also supports a
`legacy` value, in order to send notifications in NGSIv1 format. This way, users can benefit from the
enhancements of NGSIv2 subscriptions (e.g. filtering) with NGSIv1 legacy notification receivers.

Note that NGSIv1 is deprecated. Thus, we don't recommend to use `legacy` notification format any longer.

[Top](#top)

## Datetime support

From "Special Attribute Types" section at NGSIv2 specification:

> DateTime: identifies dates, in ISO8601 format. These attributes can be used with the query operators greater-than,
> less-than, greater-or-equal, less-or-equal and range.

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

The string "ISO8601" as type for attributes and metadata is also supported. The effect is the same as when using "DateTime".

[Top](#top)

## User attributes or metadata matching builtin name

(The content of this section applies to all builtins except `dateExpires` attribute. Check the document
[on transient entities](transient_entities.md) for specific information about `dateExpires`).

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

[Top](#top)

## Subscription payload validations

The particular validations that Orion implements on NGSIv2 subscription payloads are the following ones:

* **description**: optional (max length 1024)
* **subject**: mandatory
    * **entities**: mandatory
        * **id** or **idPattern**: one of them is mandatory (but both at the same time is not allowed). id
            must follow NGSIv2 restrictions for IDs. idPattern must be not empty and a valid regex.
        * **type** or **typePattern**: optional (but both at the same time is not allowed). type must 
            follow NGSIv2 restrictions for IDs. type must not be empty. typePattern must be a valid regex, and non-empty.
    * **condition**: optional (but if present it must have a content, i.e. `{}` is not allowed)
        * **attrs**: optional (but if present it must be a list; empty list is allowed)
        * **expression**: optional (but if present it must have a content, i.e. `{}` is not allowed)
            * **q**: optional (but if present it must be not empty, i.e. `""` is not allowed)
            * **mq**: optional (but if present it must be not empty, i.e. `""` is not allowed)
            * **georel**: optional (but if present it must be not empty, i.e. `""` is not allowed)
            * **geometry**: optional (but if present it must be not empty, i.e. `""` is not allowed)
            * **coords**: optional (but if present it must be not empty, i.e. `""` is not allowed)
* **notification**:
    * **http**: must be present if `httpCustom` is omitted, forbidden otherwise
        * **url**: mandatory (must be a valid URL)
    * **httpCustom**: must be present if `http` is omitted, forbidden otherwise
        * **url**: mandatory (must be not empty)
        * **headers**: optional (but if present it must have a content, i.e. `{}` is not allowed)
        * **qs**: optional (but if present it must have a content, i.e. `{}` is not allowed)
        * **method**: optional (but if present it must be a valid HTTP method)
        * **payload**: optional (empty string is allowed)
    * **attrs**: optional (but if present it must be a list; empty list is allowed)
    * **metadata**: optional (but if present it must be a list; empty list is allowed)
    * **exceptAttrs**: optional (but it cannot be present if `attrs` is also used; if present it must be a non-empty list)
    * **attrsFormat**: optional (but if present it must be a valid attrs format keyword)
* **throttling**: optional (must be an integer)
* **expires**: optional (must be a date or empty string "")
* **status**: optional (must be a valid status keyword)

[Top](#top)

## `alterationType` attribute

Appart from the attributes described in the "Builtin Attributes" section in the NGSIv2 specification,
Orion implements the `alterationType` attribute.

This attribute can be used only in notifications (in queries such `GET /v2/entities?attrs=alterationType`
is ignored) and can take the following values:

* `entityCreate` if the update that triggers the notification is a entity creation operation
* `entityUpdate` if the update that triggers the notification was an update but it wasn't an actual change
* `entityChange` if the update that triggers the notification was an update with an actual change
* `entityDelete` if the update that triggers the notification was a entity delete operation

The type of this attribute is `Text`

This builtin attribute is related with the [subscriptions based in alteration type](subscriptions_alttype.md) feature.

[Top](#top)

## `actionType` metadata

From NGSIv2 specification section "Builtin metadata", regarding `actionType` metadata:

> Its value depend on the request operation type: `update` for updates,
> `append` for creation and `delete` for deletion. Its type is always `Text`.

Current Orion implementation supports "update" and "append". The "delete" case will be
supported upon completion of [this issue](https://github.com/telefonicaid/fiware-orion/issues/1494).

[Top](#top)

## `ignoreType` metadata

Appart from the metadata described in the "Builtin metadata" section in the NGSIv2 specification,
Orion implements the `ignoreType` metadata.

When `ignoreType` with value `true` is added to an attribute, Orion will ignore the
semantics associated to the attribute type. Note that Orion ignored attribute type in general so
this metadata is not needed most of the cases, but there are two cases in which attribute
type has an special semantic for Orion (check NGSIv2 specification for details):

* `DateTime`
* Geo-location types (`geo:point`, `geo:line`, `geo:box`, `geo:polygon` and `geo:json`)

At the present moment `ignoreType` is supported only for geo-location types, this way allowing a
mechanism to overcome the limit of only one geo-location per entity (more details
in [this section of the documentation](#limit-to-attributes-for-entity-location). Support
for `ignoreType` in `DateTime` may come in the future.

[Top](#top)

## `noAttrDetail` option

The value `noAttrDetail` of the URI param `options` may be used in order to avoid NGSIv2 type browsing queries
(`GET /v2/types` and `GET /v2/types/<type>`) to provide attribute type details.
When used, the `types` list associated to each attribute name is set to `[]`.

Using this option, Orion solves these queries much faster, especially in the case of a large number of attributes, each one with a different type.
This can be very useful if your use case doesn't need the attribute type detail.
In some cases savings from 30 seconds to 0.5 seconds with the `noAttrDetails` option have been detected.

[Top](#top)

## Notification throttling

From NGSIv2 specification regarding subscription throttling:

> throttling: Minimal period of time in seconds which must elapse between two consecutive notifications. It is optional.

The way in which Orion implements this is discarding notifications during the throttling guard period. Thus, notifications may be lost
if they arrive too close in time. If your use case doesn't support losing notifications this way, then you should not use throttling.

In addition, Orion implements throttling in a local way. In multi-CB configurations, take into account that the last-notification
measure is local to each Orion node. Although each node periodically synchronizes with the DB in order to get potentially newer
values (more on this [here](../admin/perf_tuning.md#subscription-cache)) it may happen that a particular node has an old value, so throttling
is not 100% accurate.

[Top](#top)

## Ordering between different attribute value types

From NGISv2 specification "Ordering Results" section:

> Operations that retrieve lists of entities permit the `orderBy` URI parameter to specify 
> the attributes or properties to be used as criteria when ordering results

It is an implementation aspect how each type is ordered with regard to other types. In the case of Orion,
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

[Top](#top)

## Oneshot subscriptions

Apart from the `status` values defined for subscription in the NGSIv2 specification, Orion also allows to use `oneshot`. Please find details in [the oneshot subscription document](oneshot_subscription.md)

[Top](#top)

## Subscriptions based in alteration type

Apart from the sub-fields allowed in subscriptions `conditions` field according to NGSIv2 specifiction,
Orion supports the `alterationTypes` field to specify under which alterations (entity creation, entity
modification, etc.) the subscription is triggered.

Please find details in [this specific documentation](subscriptions_alttype.md)

## Custom notifications without payload

If `payload` is set to `null` within `httpCustom` field in custom notifcations, then the notifications
associated to that subscription will not include any payload (i.e. content-length 0 notifications).

Note this is not the same than using `payload` set to `""` or omitting the field. In that case,
the notification will be sent using the NGSIv2 normalized format.

[Top](#top)

## MQTT notifications

Apart from the `http` and `httpCustom` fields withint `notification` object in subscription described
in the NGISv2 specification, Orion also supports `mqtt` and `mqttCustom` for MQTT notifications. This
topic is described with more detail [in this specific document](mqtt_notifications.md).

[Top](#top)

## Notify only attributes that change

Orion supports an extra field `onlyChangedAttrs` (within `notification`) in subscriptions, apart of the ones described in
the NGSIv2 specification. This field takes a `true` or `false` value (default is `false`, if the field is ommitted). If
set to `true` then notifications associated to the subscription include only attributes that changed in the triggering
update request, in combination with the `attrs` or `exceptAttrs` field.

For instance, if `attrs` is `[A, B, C]` the default behavior  (when `onlyChangedAttrs` is `false`) and the triggering
update modified only A, then A, B and C are notified (in other words, the triggering update doesn't matter). However,
if `onlyChangedAttrs` is `true` and the triggering update only modified A then only A is included in the notification.

[Top](#top)

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


[Top](#top)


## `timeout` subscriptions option

Apart from the subscription fields described in NGSIv2 specification for `GET /v2/subscriptions` and
`GET /v2/subscriptions/subId` requests, Orion supports the `timeout` extra parameter within the `http` or `httpCustom`
field. This field specifies the maximum time the subscription waits for the response when using HTTP
notifications in milliseconds.

The maximum value allowed for this parameter is 1800000 (30 minutes). If 
`timeout` is defined to 0 or omitted, then the value passed as `-httpTimeout` CLI parameter is used. See section in the
[Command line options](../admin/cli.md#command-line-options) for more details.

[Top](#top)

## `lastFailureReason` and `lastSuccessCode` subscriptions fields

Apart from the subscription fields described in NGSIv2 specification for `GET /v2/subscriptions` and
`GET /v2/subscriptions/subId` requests, Orion supports this two extra fields within the `notification`
field:

* `lastFailureReason`: a text string describing the cause of the last failure (i.e. the failure
  occurred at `lastFailure` time).
* `lastSuccessCode`: the HTTP code (200, 400, 404, 500, etc.) returned by receiving endpoint last
  time a successful notification was sent (i.e. the success occurred at `lastSuccess` time).

Both can be used to analyze possible problems with notifications. See section in the
[problem diagnosis procedures document](../admin/diagnosis.md#diagnose-notification-reception-problems)
for more details.

Note these two fields are included in HTTP subscriptions, but not in MQTT ones. See
[MQTT notifications document](#mqtt_notifications.md) for more detail.

[Top](#top)

## `failsCounter` and `maxFailsLimit` subscriptions fields

Apart from the subscription fields described in NGSIv2 specification for `GET /v2/subscriptions` and
`GET /v2/subscriptions/subId` requests, Orion supports a `failsCounter` field within the `notification`
field. The value of this field is the number of consecutive failing notifications associated
to the subscription. `failsCounter` is increased by one each time a notification attempt fails and reset
to 0 if a notification attempt successes (`failsCounter` is ommitted in this case).

There is also an optional field `maxFailsLimit` (also within `notification` field) which establishes
a maximum allowed number of consecutive fails. If the number of fails overpasses the value of
`maxFailsLimit` (i.e. at a given moment `failsCounter` is greater than `maxFailsLimit`) then
Orion automatically passes the subscription to `inactive` state. A subscripiton update operation
(`PATCH /v2/subscription/subId`) is needed to re-enable the subscription (setting its state
`active` again).

In addition, when Orion automatically disables a subscription, a log trace in WARN level is printed
in this format:

```
time=... | lvl=WARN | corr=... | trans=... | from=... | srv=... | subsrv=... | comp=Orion | op=... | msg= Subscription <subId> automatically disabled due to failsCounter (N) overpasses maxFailsLimit (M)
```

[Top](#top)

## `flowControl` option
As extra URI param option to the ones included in the NGSIv2 specification, Orion implements flowControl,
than can be used to specify that an update operation have to use flow control, which can improve performance
and avoid saturacion in high-load scenarios. This only works if the ContextBroker has been started using
the [`-notifFlowControl` parameter](../admin/cli.md), otherwise is ignored. The flow control mechanism
is explained in [this section in the documentation](../admin/perf_tuning.md#updates-flow-control-mechanism).

The following requests can use the flowControl URI param option:

* `POST /v2/entities/E/attrs?options=flowControl`
* `POST /v2/entities/E/attrs?options=append,flowControl`
* `POST /v2/op/update?options=flowControl`
* `PUT /v2/entities/E/attrs?options=flowControl`
* `PUT /v2/entities/E/attrs/A?options=flowControl`
* `PUT /v2/entities/E/attrs/A/value?options=flowControl`
* `PATCH /v2/entities/E/attrs?options=flowControl`

[Top](#top)

## Ambiguous subscription status `failed` not used

NGSIv2 specification describes `failed` value for `status` field in subscriptions:

> `status`: [...] Also, for subscriptions experiencing problems with notifications, the status
> is set to `failed`. As soon as the notifications start working again, the status is changed back to `active`.

Status `failed` was removed in Orion 3.4.0 due to it is ambiguous:

* `failed` may refer to an active subscription (i.e. a subscription that will trigger notifications
  upon entity updates) which last notification sent was failed
* `failed` may refer to an inactive subscription (i.e. a subscription that will not trigger notifications
  upon entity update) which was active in the past and which last notification sent in the time it was
  active was failed

In other words, looking to status `failed` is not possible to know if the subscription is currently
active or inactive.

Thus, `failed` is not used by Orion Context Broker and the status of the subscription always clearly specifies
if the subscription is `active` (including the variant [`oneshot`](#oneshot-subscriptions)) or
`inactive` (including the variant `expired`). You can check the value of `failsCounter` in order to know if
the subscription failed in its last notification or not (i.e. checking that `failsCounter` is greater than 0).

[Top](#top)

## `forcedUpdate` option
As extra URI param option to the ones included in the NGSIv2 specification, Orion implements forcedUpdate, 
than can be used to specify that an update operation have to trigger any matching subscription (and send 
corresponding notification) no matter if there is an actual attribute update or not. Remember that the 
default behaviour (i.e. without using the forcedUpdate URI param option) is to updated only if attribute 
is effectively updated.

The following requests can use the forcedUpdate URI param option:

* `POST /v2/entities/E/attrs?options=forcedUpdate`
* `POST /v2/entities/E/attrs?options=append,forcedUpdate`
* `POST /v2/op/update?options=forcedUpdate`
* `PUT /v2/entities/E/attrs?options=forcedUpdate`
* `PUT /v2/entities/E/attrs/A?options=forcedUpdate`
* `PUT /v2/entities/E/attrs/A/value?options=forcedUpdate`
* `PATCH /v2/entities/E/attrs?options=forcedUpdate`

Check also the `entityChange` [alteration type](subscriptions_alttype.md) for the same effect,
but applyed to the subscription, not matter if the update request included the `forcedUpdate` option or not.

[Top](#top)

## Registrations

Orion implements registration management as described in the NGSIv2 specification, except
for the following aspects:

* `PATCH /v2/registration/<id>` is not implemented. Thus, registrations cannot be updated
  directly. I.e., updates must be done deleting and re-creating the registration. Please
  see [this issue](https://github.com/telefonicaid/fiware-orion/issues/3007) about this.
* `idPattern` is supported but only for the exact regular expression `.*`
* `typePattern` is not implemented.
* The `expression` field (within `dataProvided`) is not supported. The field is simply
  ignored. Please see [this issue](https://github.com/telefonicaid/fiware-orion/issues/3107) about it.
* The `inactive` value for `status` is not supported. I.e., the field is stored/retrieved correctly,
  but the registration is always active, even when the value is `inactive`. Please see
  [this issue](https://github.com/telefonicaid/fiware-orion/issues/3108) about it.

According to NGSIv2 specification:

> A NGSIv2 server implementation may implement query or update forwarding to context information sources.

The way in which Orion implements such forwarding is as follows:

* `POST /v2/op/query` for query forwarding
* `POST /v2/op/update` for update forwarding

More information on forwarding to context information sources can be found in [this specific document](context_providers.md).

Orion implements an additional field `legacyForwarding` (within `provider`) not included in the NGSIv2
specification. If the value of `legacyForwarding` is `true` then NGSIv1-based query/update will be used
for forwarding requests associated to that registration. Although NGSIv1 is deprecated, some Context Provider may
not have been migrated yet to NGSIv2, so this mode may prove useful.

[Top](#top)

## `skipForwarding` option

You can use `skipForwarding` option in queries (e.g. `GET /v2/entities?options=skipForwarding`) in order to skip
forwarding to CPrs. In this case, the query is evaluated using exclusively CB local context information.

Note that in updates `skipForwarding` has no effect (if you want an update to be interpreted locally to the CB
just use an update request with append/creation semantics).

[Top](#top)

## `null` support in DateTime and geolocation types

According to NGSIv2 specification:

* `DateTime` attributes and metadata: has to be strings in in ISO8601 format
* `geo:point`, `geo:line`, `geo:box`, `geo:polygon` and `geo:json`: attributes has to follow specific formatting rules
  (defined in the "Geospatial properties of entities section)

It is not clear in the NGSIv2 specification if the `null` value is supported in these cases or not.
Just to be clear, Orion supports that possibility.

With regards to `DateTime` attributes and metadata:

* A `DateTime` attribute or metadata with `null` value will not be taken into account in filters, i.e.
  `GET /v2/entities?q=T>2021-04-21`

With regards to `geo:` attributes:

* A `geo:` attribute with `null` value will not be taken into account in geo-queries, i.e. the entity will
  not be returned as a result of geo-query
* `geo:` attributes with `null` value doesn't count towards [the limit of one](#limit-to-attributes-for-entity-location)

[Top](#top)

## `keyValues` not supported in `POST /v2/op/notify`

The current Orion implementation doesn't support `keyValues` option in `POST /v2/op/notify` operation. If you attempt
to use it you would get a 400 Bad Request error.

[Top](#top)

## Deprecated features

Although we try to minimize the changes in the stable version of the NGSIv2 specification, a few changes
have been needed in the end. Thus, there is changed functionality that doesn't appear in the current
NGSIv2 stable specification document but that Orion still supports
(as [deprecated functionality](../deprecated.md)) in order to keep backward compatibility.

In particular:

* The usage of `dateCreated` and `dateModified` in the `options` parameter (introduced
in stable RC-2016.05 and removed in RC-2016.10) is still supported, e.g. `options=dateModified`. However,
you are highly encouraged to use `attrs` instead (i.e. `attrs=dateModified,*`).

* `POST /v2/op/update` accepts the same action types as NGSIv1, that is `APPEND`, `APPEND_STRICT`,
`UPDATE`, `DELETE` and `REPLACE`. However, they shouldn't be used, preferring always the following counterparts:
`append`, `appendStrict`, `update`, `delete` and `replace`.

* `attributes` field in `POST /v2/op/query` is deprecated. It is a combination of `attrs` (to select
which attributes to include in the response to the query) and unary attribute filter in `q` within
`expression` (to return only entities which have these attributes). Use them instead.

[Top](#top)
