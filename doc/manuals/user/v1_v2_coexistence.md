# Considerations on NGSIv1 and NGSIv2 coexistence

NGSIv1 is the API offered by Orion Context Broker from its very first version. 
[NGSIv2](http://telefonicaid.github.io/fiware-orion/api/v2/stable) development started 
in July 2015 in Orion 0.23.0. Although at the end NGSIv1 will be deprecated and 
removed from the code in some future Orion version (so only NGSIv2 will remain) 
this is a big work and both API versions will be coexisting during some time. 

This document explains some consideration to take into account regarding such coexistence.

## Native JSON types

NGSIv2 allows to create/update attributes (and metadata) whose values use JSON native 
types (number, boolean, string, etc.). Unfortunately, NGSIv1 uses a JSON parser that converts 
numbers and boolean values to string at creation/update time. Thus, an attempt of 
setting `A=2` using NGSIv1 will actually store `A="2"` in the Orion database.

However, NGSIv1 rendering is able to correctly retrieve attribute values stored using 
non-string JSON native types. Thus, if you set `A=2` using NGSIv2 and retrieve that 
attribute using NGSIv1, you will get `A=2`.

## Filtering

You can use the filtering capabilities developed for NGSIv2 (`GET /v2/entities?q=<query>`) also 
in NGSIv1 using a Scope element in the payload of `POST /v1/queryContext`. See 
[the following section](filtering.md#string-filters) for details.

However, take into account that some of the filters (e. g. greater/less, range, etc.) are thought
for numeric values. Thus, in order to work properly, these filters (although using a 
`POST /v1/queryContext`) needs that the attributes to which they refer were created using NGSIv2 operations.

In addition, note that NGSIv2 geo-query filters can be used also in NGSIv1. See
[the following section](geolocation.md#geo-located-queries-ngsiv2) for details

## Checking ID fields

NGSIv2 introduces syntax restrictions for ID fields (such as entity id/type, attribute name/type
or metadata name/type) which are described in the "Field syntax restrictions" section in the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable). In order to
keep backward compatibility, these restrictions are not used in the NGSIv1 API by default, but
you can enable them using the `-strictNgsiv1Ids` [CLI parameter](../admin/cli.md).

Related with this topic, note that NGSIv1 allows entities/attributes/metadatas without types
and with types equal to the empty string (`""`). However, NGSIv2 ID fields (including types) have
a minimum length of 1 character. Thus, entities created with NGSIv1 but rendered using NGSIv2 operation
will automatically replace these cases with the string value `none` (which is the default type in NGSIv2).

## `orderBy` parameter

The `orderBy` parameter defined for NGSIv2 can be used also in NGSIv1 queryContext operation (see
details in the [pagination documentation](pagination.md). However, note that the "geo:distance"
order can be used only in NGSIv2.

## NGSIv1 notification with NGSIv2 subscriptions

NGSIv2 allows several notification modes depending on the `attrsFormat` field associated to the
subscription. Apart from the values described in the NGSIv2 specification, Orion also support
`legacy` value in order to send notifications in NGSIv1 format. This way, users can have the
enhancements of NGSIv2 subscriptions (e.g. filtering or notification metadata marks) with
NGSIv1 legacy notifications receivers.

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
  other entities/attributes not being updated due to failing or missing CPrs), 404 Not Found is returned to the client.
  The `error` field in this case is `PartialUpdate` and the `description` field contains information about which entity
  attributes failed to update.

