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
However, this may change in the future, as described in https://github.com/telefonicaid/fiout it, but the ware-orion/issues/2223.

## Scope functionality

Orion implements a `scope` field in the `POST /v2/op/update` operation (you can see
[an example in the NGSIv2 walkthrough](walkthrough_apiv2.md#batch-operations)). However, note that this syntax is
somewhat experimental and it hasn't been consolidated in the NGSIv2 specification.

## NGSIv2 query update forwarding to Context Providers

Context availability management functionality (i.e. operations to register Context Providers) is still to be
implemented for NGSIv2. However, you can [register providers using NGSIv1 operations](context_providers.md) 
and have your NGSIv2-based updates and queries being forwarded to Context Providers, getting the response in NGSIv2.
The forwarded message in the CB to CPr communication, and its response, is done using NGSIv1.


However, the following considerations have to be taken into account:

* Query filtering (e.g. `GET /v2/entities?q=temperature>40`) is not supported on query forwarding. First, Orion 
  doesn't include the filter in the `POST /v1/queryContext` operation forwarded to CPr. Second, Orion doesn't filter 
  the CPr results before responding them back to client. An issue corresponding to this limitation has been created:
  https://github.com/telefonicaid/fiware-orion/issues/2282
* On forwarding, any type of entity in the NGSIv2 update/query matches registrations without entity type. However, the 
  opposite doesn't work, so if you have registrations with types, then you must use `?type` in NGSIv2  update/query in
  order to obtain a match.
* In the case of partial updates (e.g. `POST /v2/op/entities` resulting in some entities/attributes being updated and 
  other entities/attributes not being updated due to failing or missing CPrs), 404 No Content is returned to the client and
  information about all attributes not being updated is provided in the payload.
