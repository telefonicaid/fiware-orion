#<a name="top"></a>NGSIv2 Implementation Notes

This document describes some considerations to take into account
regarding the specific implementation done by Orion Context Broker
of the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

## Forbidden characters

From "Field syntax restrictions" section at NGSIv2 specification:

> In addition to the above rules, given NGSIv2 server implementations could add additional
> syntactical restrictions in those or other fields, e.g., to avoid cross script injection attacks.

The additional restrictions that apply to Orion are the ones describe in the
[forbidden characters](forbidden_characters.md) section of the manual.

## Custom payload decoding on notifications

Due to forbidden characters restriction, Orion applies an extra decoding step to outgoing
custom notifications. This is described in detail in [this section](forbidden_characters.md#custom-payload-special-treatment)
of the manual.

## Option to disable custom notifications

Orion can be configured to disable custom notifications, using the `-disableCustomNotifications` [CLI parameter](../admin/cli.md).

In this case:

* `httpCustom` is interpreted as `http`, i.e. all sub-fields except `url` are ignored
* No `${...}` macro substitution is performed.

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

## Legacy attribute format in notifications

Apart from the values described for `attrsFormat` in the NGSIv2 specification, Orion also supports a
`legacy` value, in order to send notifications in NGSIv1 format. This way, users can benefit from the
enhancements of NGSIv2 subscriptions (e.g. filtering) with NGSIv1 legacy notification receivers.

## Disable attribute detail in GET types operation

Not yet implemented, but is expected that Orion will implmement the `noAttrsType` option for the
`GET /v2/types` operation in order to not include attribute details (whose aggregation could be
costly in terms of performance).

Related with: https://github.com/telefonicaid/fiware-orion/issues/2073

## Metadata vector and object values not implemented yet

NGSIv2 specification allows metadata value to be JSON Array or Object. However, such datatypes are yet to be
implemented for metadata values in Orion.

## Default type for entities, attributes and metadata

Currently, Orion uses the string `none` as default for entities/attributes/metadata at creation/update time.
However, this may change in the future, as described in https://github.com/telefonicaid/fiware-orion/issues/2223.

## Scope functionality

Orion implements a `scope` field in the `POST /v2/op/update` operation (you can see
[an example in the NGSIv2 walkthrough](walkthrough_apiv2.md#batch-operations)). However, note that this syntax is
somewhat experimental and it hasn't been consolidated in the NGSIv2 specification.

## Error responses

The error response rules defined in https://github.com/telefonicaid/fiware-orion/issues/1286 takes precedence over
the ones described in "Error Responses" section in the NGSIv2 specification. In particular, Orion Context
Broker never responds with "InvalidModification (422)", using "Unprocessable (422)" instead.

# Subscription payload validations

The particular validations that Orion implements on NGSIv2 subscription payloads are the following ones:

* **description**: optional (max length 1024)
* **subject**: mandatory
  * **entities**: mandatory
    * **id** or **idPattern**: one of them is mandatory (but both at the same time is not allowed). id
      must follow NGSIv2 restrictions for IDs. idPattern must be not empty and a valid regex.
    * **type**: optional (but if appears it must follow NGSIv2 restrictions for IDs)
  * **condition**: optional (but if appears it must have a content, i.e. `{}` is not allowed)
    * **attrs**: optional (but if appears it must be a list; empty list is allowed)
    * **expression**: optional (but if appears it must have a content, i.e. `{}` is not allowed)
      * **q**: optional (but if appears it must be not empty, i.e. `""` is not allowed)
      * **georel**: optional (but if appears it must be not empty, i.e. `""` is not allowed)
      * **geometry**: optional (but if appears it must be not empty, i.e. `""` is not allowed)
      * **coords**: optional (but if appears it must be not empty, i.e. `""` is not allowed)
* **notification**:
  * **http**: must be present if `httpCustom` is omitted, forbidden otherwise
    * **url**: mandatory (must be a valid URL)
  * **httpCustom**: must be present if `http` is omitted, forbidden otherwise
    * **url**: mandatory (must be not empty)
    * **headers**: optional (but if appears it must have a content, i.e. `{}` is not allowed)
    * **qs**: optional (but if appears it must have a content, i.e. `{}` is not allowed)
    * **method**: optional (but if apperas it must be a valid HTTP method)
    * **payload**: optional (empty string is allowed)
  * **attrs**: optional (but if appears it must be a list; empty list is allowed)
  * **exceptAttrs**: optional (but it cannot appear if `attrs` is also used; if appears it must be a non-empty list)
  * **attrsFormat**: optional (but if appears it must be a valid attrs format keyword)
* **throttling**: optional (must be an integer)
* **expires**: optional (must be a date)
* **status**: optional (must be a valid status keyword)

