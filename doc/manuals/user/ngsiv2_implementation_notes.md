# <a name="top"></a>NGSIv2 Implementation Notes

* [Update operators for attribute values](#update-operators-for-attribute-values)
* [User attributes or metadata matching builtin name](#user-attributes-or-metadata-matching-builtin-name)
* [Subscription payload validations](#subscription-payload-validations)
* [`actionType` metadata](#actiontype-metadata)
* [Oneshot subscriptions](#oneshot-subscriptions)
* [Subscriptions based in alteration type](#subscriptions-based-in-alteration-type)
* [Covered subscriptions](#covered-subscriptions)
* [Ambiguous subscription status `failed` not used](#ambiguous-subscription-status-failed-not-used)
* [Registrations](#registrations)
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

## Oneshot subscriptions

Apart from the `status` values defined for subscription in the NGSIv2 specification, Orion also allows to use `oneshot`. Please find details in [the oneshot subscription document](oneshot_subscription.md)

[Top](#top)

## Subscriptions based in alteration type

Apart from the sub-fields allowed in subscriptions `conditions` field according to NGSIv2 specifiction,
Orion supports the `alterationTypes` field to specify under which alterations (entity creation, entity
modification, etc.) the subscription is triggered.

Please find details in [this specific documentation](subscriptions_alttype.md)

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
