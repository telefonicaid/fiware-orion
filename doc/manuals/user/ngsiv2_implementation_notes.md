# <a name="top"></a>NGSIv2 Implementation Notes

* [Update operators for attribute values](#update-operators-for-attribute-values)
* [Datetime support](#datetime-support)
* [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name)
* [Subscription payload validations](#subscription-payload-validations)
* [`actionType` metadata](#actiontype-metadata)
* [`ignoreType` metadata](#ignoretype-metadata)
* [Ordering between different attribute value types](#ordering-between-different-attribute-value-types)
* [Oneshot subscriptions](#oneshot-subscriptions)
* [Subscriptions based in alteration type](#subscriptions-based-in-alteration-type)
* [Custom notification extra macros](#custom-notification-extra-macros)
* [Custom notification with JSON payload](#custom-notification-with-json-payload)
* [Custom notifications without payload](#custom-notifications-without-payload)
* [MQTT notifications](#mqtt-notifications)
* [Covered subscriptions](#covered-subscriptions)
* [Ambiguous subscription status `failed` not used](#ambiguous-subscription-status-failed-not-used)
* [Registrations](#registrations)
* [`null` support in DateTime and geolocation types](#null-support-in-datetime-and-geolocation-types)
* [`keyValues` not supported in `POST /v2/op/notify`](#keyvalues-not-supported-in-post-v2opnotify)
* [Deprecated features](#deprecated-features)

This document describes some considerations to take into account
regarding the specific implementation done by Orion Context Broker
of the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

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

## `actionType` metadata

From NGSIv2 specification section "Builtin metadata", regarding `actionType` metadata:

> Its value depend on the request operation type: `update` for updates,
> `append` for creation and `delete` for deletion. Its type is always `Text`.

Current Orion implementation supports "update" and "append". The "delete" case will be
supported upon completion of [this issue](https://github.com/telefonicaid/fiware-orion/issues/1494).

[Top](#top)

## `ignoreType` metadata

Apart from the metadata described in the "Builtin metadata" section in the NGSIv2 specification,
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

[Top](#top)

## Custom notification extra macros

Apart from the `${...}` macros described in "Custom Notifications" section in the NGSIv2
specification, the following ones can be used:

* `${service}` is replaced by the service (i.e. `fiware-service` header value) in the
  update request triggering the subscription.
* `${servicePath}` is replaced by the service path (i.e. `fiware-servicepath` header value) in the
  update request triggering the subscription.
* `${authToken}` is replaced by the authorization token (i.e. `x-auth-token` header value) in the
  update request triggering the subscription.

In the rare case an attribute was named in the same way of the above (e.g. an attribute which
name is `service`) then the attribute value takes precedence.

[Top](#top)

## Custom notification with JSON payload

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

Some notes to take into account:

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

[Top](#top)

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
for non existing attributes (the default behaviour when `covered` is not set to `true` is to replace by the
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
