#<a name="top"></a>NGSIv2 Implementation Notes

* [Forbidden characters](#forbidden-characters)
* [Custom payload decoding on notifications](#custom-payload-decoding-on-notifications)
* [Option to disable custom notifications](#option-to-disable-custom-notifications)
* [Limit to attributes for entity location](#limit-to-attributes-for-entity-location)
* [Legacy attribute format in notifications](#legacy-attribute-format-in-notifications)
* [Scope functionality](#scope-functionality)
* [Error responses](#error-responses)
* [Subscription payload validations](#subscription-payload-validations)
* [`actionType` metadata](#actiontype-metadata)
* [`noAttrDetail` option](#noattrdetail-option)
* [Notification throttling](#notification-throttling)
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

[Top](#top)

## Scope functionality

Orion implements a `scope` field in the `POST /v2/op/update` operation (you can see
[an example in the NGSIv2 walkthrough](walkthrough_apiv2.md#batch-operations)). However, note that this syntax is
somewhat experimental and it hasn't been consolidated in the NGSIv2 specification.

[Top](#top)

## Error responses

The error response rules defined in https://github.com/telefonicaid/fiware-orion/issues/1286 takes precedence over
the ones described in "Error Responses" section in the NGSIv2 specification. In particular, Orion Context
Broker never responds with "InvalidModification (422)", using "Unprocessable (422)" instead.

[Top](#top)

## Subscription payload validations

The particular validations that Orion implements on NGSIv2 subscription payloads are the following ones:

* **description**: optional (max length 1024)
* **subject**: mandatory
    * **entities**: mandatory
        * **id** or **idPattern**: one of them is mandatory (but both at the same time is not allowed). id
            must follow NGSIv2 restrictions for IDs. idPattern must be not empty and a valid regex.
        * **type**: optional (but if present it must follow NGSIv2 restrictions for IDs)
    * **condition**: optional (but if present it must have a content, i.e. `{}` is not allowed)
        * **attrs**: optional (but if present it must be a list; empty list is allowed)
        * **expression**: optional (but if present it must have a content, i.e. `{}` is not allowed)
            * **q**: optional (but if present it must be not empty, i.e. `""` is not allowed)
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

From NGSIv2 specification section ""System/builtin in metadata"", regarding `actionType` metadata:

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

## Deprecated features

Although we try to minimize the changes in the stable version of the NGSIv2 specification, a few changes
have been needed in the end. Thus, there is changed functionality that doesn't appear in the current
NGSIv2 stable specification document but that Orion still supports
(as [deprecated functionality](../deprecated.md)) in order to keep backward compability.

In particular, the usage of `dateCreated` and `dateModified` in the `options` parameter (introduced
in stable RC-2016.05 and removed in RC-2016.10.) is still supported, e.g. `options=dateModified`. However,
you are highly encouraged to use `attrs` instead (i.e. `attrs=dateModified,*`).

[Top](#top)
