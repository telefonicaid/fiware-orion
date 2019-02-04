# <a name="top"></a>NGSIv2 Implementation Notes

* [Forbidden characters](#forbidden-characters)
* [Custom payload decoding on notifications](#custom-payload-decoding-on-notifications)
* [Option to disable custom notifications](#option-to-disable-custom-notifications)
* [Non-modifiable headers in custom notifications](#non-modifiable-headers-in-custom-notifications)
* [Limit to attributes for entity location](#limit-to-attributes-for-entity-location)
* [Legacy attribute format in notifications](#legacy-attribute-format-in-notifications)
* [Datetime support](#datetime-support)
* [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name)
* [Subscription payload validations](#subscription-payload-validations)
* [`actionType` metadata](#actiontype-metadata)
* [`noAttrDetail` option](#noattrdetail-option)
* [Notification throttling](#notification-throttling)
* [Ordering between different attribute value types](#ordering-between-different-attribute-value-types)
* [Initial notifications](#initial-notifications)
* [Oneshot Subscription](#oneshot-subscriptions)
* [`lastFailureReason` and `lastSuccessCode` subscriptions fields](#lastfailurereason-and-lastsuccesscode-subscriptions-fields)
* [`forcedUpdate` option](#forcedupdate-option)
* [Registrations](#registrations)
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

Any attemp of doing so (e.g. `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` will be
ignored.

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

[Top](#top)

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
    * `hh:mm:ss.sss` or `hhmmss.sss`. At the present moment, Orion is able to process times including microseconds (or even
      smaller resolutions) although internally they are stored as `.00`. However, this may change in the future
      (see [related issue](https://github.com/telefonicaid/fiware-orion/issues/2670)).
    * `hh:mm:ss` or `hhmmss`.
    * `hh:mm` or `hhmm`. Seconds are set to `00` in this case.
    * `hh`. Minutes and seconds are set to `00` in this case.
    * If `<time>` is ommited, then hours, minutes and seconds are set to `00`.
* Regarding `<timezones>` it must follow any of the patterns described in [the ISO8601 specification](https://en.wikipedia.org/wiki/ISO_8601#Time_zone_designators):
    * `Z`
    * `±hh:mm`
    * `±hhmm`
    * `±hh`
* ISO8601 specifies that *"if no UTC relation information is given with a time representation, the time is assumed to be in local time"*.
  However, this is ambiguous when client and server are in different zones. Thus, in order to solve this ambiguety, Orion will always
  assume timezone `Z` when timezone designator is ommited.

Orion always provides datetime attributes/metadata using the format `YYYY-MM-DDThh:mm:ss.ssZ`. Note it uses UTC/Zulu
timezone (which is the best default option, as clients/receivers may be running in any timezone). This may change in the
future (see [related issue](https://github.com/telefonicaid/fiware-orion/issues/2663)).

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

## `actionType` metadata

From NGSIv2 specification section "Builtin metadata", regarding `actionType` metadata:

> Its value depend on the request operation type: `update` for updates,
> `append` for creation and `delete` for deletion. Its type is always `Text`.

Current Orion implementation supports "update" and "append". The "delete" case will be
supported upon completion of [this issue](https://github.com/telefonicaid/fiware-orion/issues/1494).

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

The way in which Orion implements this is discarding notifications during the throttling guard period. Thus, nofications may be lost
if they arrive too close in time. If your use case doesn't support losing notifications this way, then you should not use throttling.

In addition, Orion implements throttling in a local way. In multi-CB configurations, take into account that the last-notification
measure is local to each Orion node. Although each node periodically synchronizes with the DB in order to get potencially newer
values (more on this [here](perf_tuning.md#subscription-cache)) it may happen that a particular node has an old value, so throttling
is not 100% accurate.

[Top](#top)

## Ordering between different attribute value types

From NGISv2 specification "Ordering Results" section:

> Operations that retrieve lists of entities permit the `orderBy` URI parameter to specify 
> the attributes or properties to be be used as criteria when ordering results

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

## Initial notifications

The NGSIv2 specification describes in section "Subscriptions" the rules that trigger notifications
corresponding to a given subscription, based on updates to the entities covered by the subscription.
Apart from that kind of regular notifications, Orion may send also an initial notification at
subscription creation/update time.

Initial notification can be configurable using a new URI parameter option  `skipInitialNotification`. For instance `POST /v2/subscriptions?options=skipInitialNotification`. 

Check details in the document about [initial notifications](initial_notification.md)

[Top](#top)

## Oneshot subscriptions

Apart from the `status` values defined for subscription in the NGSIv2 specification, Orion also allows to use `oneshot`. Please find details in [the oneshot subscription document](oneshot_subscription.md)

[Top](#top)

# `lastFailureReason` and `lastSuccessCode` subscriptions fields

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

[Top](#top)

## Registrations

Orion implements registration management as described in the NGSIv2 specification, except
for the following aspects:

* `PATCH /v2/registration/<id>` is not implemented. Thus, registrations cannot be updated
  directly. I.e., updates must be done deleting and re-creating the registration. Please
  see [this issue](https://github.com/telefonicaid/fiware-orion/issues/3007) about this.
* `idPattern` and `typePattern` are not implemented.
* The only valid `supportedForwardingMode` is `all`. Trying to use any other value will end
  in a 501 Not Implemented error response. Please
  see [this issue](https://github.com/telefonicaid/fiware-orion/issues/3106) about this.
* The `expression` field (within `dataProvided`) is not supported. The field is simply
  ignored. Please see [this issue](https://github.com/telefonicaid/fiware-orion/issues/3107) about it.
* The `inactive` value for `status` is not supported. I.e., the field is stored/retrieved correctly,
  but the registration is always active, even when the value is `inactive`. Please see
  [this issue](https://github.com/telefonicaid/fiware-orion/issues/3108) about it.

According to NGSIv2 specification:

> A NGSIv2 server implementation may implement query or update forwarding to context information sources.

The way in which Orion implements such forwarding is as follows:

Orion implements an additional field `legacyForwarding` (within `provider`) not included in NGSIv2
specification. If the value of `legacyForwarding` is `true` then NGSIv1-based query/update will be used
for forwarding requests associated to that registration. Although NGSIv1 is deprecated, for the time being,
NGSIv2-based forwarding has not been defined (see [this issue](https://github.com/telefonicaid/fiware-orion/issues/3068)
about it) so the only valid option is to always use `"legacyForwarding": true` (otherwise a 501 Not Implemented
error response will be the result).

[Top](#top)

## `keyValues` not supported in `POST /v2/op/notify`

The current Orion implementation doesn't support `keyValues` option in `POST /v2/op/notify` operation. If you attempt
to use it you would get a 400 Bad Request error.

[Top](#top)

## Deprecated features

Although we try to minimize the changes in the stable version of the NGSIv2 specification, a few changes
have been needed in the end. Thus, there is changed functionality that doesn't appear in the current
NGSIv2 stable specification document but that Orion still supports
(as [deprecated functionality](../deprecated.md)) in order to keep backward compability.

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
