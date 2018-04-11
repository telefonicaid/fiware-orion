# <a name="top"></a>Considerations on NGSIv1 and NGSIv2 coexistence

NGSIv1 is the API offered by Orion Context Broker from its very first version. 
[NGSIv2](http://telefonicaid.github.io/fiware-orion/api/v2/stable) development started 
in July 2015 in Orion 0.23.0. Although at the end NGSIv1 will be deprecated and 
removed from the code in some future Orion version (so only NGSIv2 will remain) 
this is a big work and both API versions will be coexisting during some time. 

This document explains some consideration to take into account regarding such coexistence.

* [Native JSON types](#native-json-types)
* [Filtering](#filtering)
* [Differences in the support to `DateTime` attribute type](#differences-in-the-support-to-datetime-attribute-type)
* [Checking ID fields](#checking-id-fields)
* [`orderBy` parameter](#orderby-parameter)
* [NGSIv1 notification with NGSIv2 subscriptions](#ngsiv1-notification-with-ngsiv2-subscriptions)
* [NGSIv2 query update forwarding to Context Providers](#ngsiv2-query-update-forwarding-to-context-providers)
* [Getting registrations created with NGSIv1 using NGSIv2 operations](#getting-registrations-created-with-NGSIv1-using-NGSIv2-operations)
* [Context availability subscriptions](#context-availability-subscriptions)

## Native JSON types

NGSIv2 allows to create/update attributes (and metadata) whose values use JSON native 
types (number, boolean, string, etc.). By default, NGSIv1 uses a JSON parser that converts
numbers and boolean values to string at creation/update time. Thus, an attempt of 
setting `A=2` using NGSIv1 will actually store `A="2"` in the Orion database. However,
some degree of native types is possible in NGSIv1 storing, using
the [autocast feature](ngsiv1autocast.md).

No matter if autocast is enabled or not, NGSIv1 rendering is able to correctly retrieve
attribute values stored using non-string JSON native types. Thus, if you set `A=2`
using NGSIv2 and retrieve that attribute using NGSIv1, you will get `A=2`.

[Top](#top)

## Filtering

You can use the filtering capabilities developed for NGSIv2 (`GET /v2/entities?q=<query>`) also 
in NGSIv1 using a Scope element in the payload of `POST /v1/queryContext`. See 
[the following section](filtering.md#string-filters) for details.

However, take into account that some of the filters (e. g. greater/less, range, etc.) are thought
for numeric values. Thus, in order to work properly, these filters (although using a 
`POST /v1/queryContext`) needs that the attributes to which they refer were created using NGSIv2 operations.

In addition, note that NGSIv2 geo-query filters can be used also in NGSIv1. See
[the following section](geolocation.md#geo-located-queries-ngsiv2) for details

[Top](#top)

## Differences in the support to `DateTime` attribute type

NGSIv2 supports the `DateTime` attribute type to identify dates. These attributes can be used with the query operators
greater-than, less-than, greater-or-equal, less-or-equal and range. See "Special Attribute Types" section at
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable)) and ["DateTime support"  section
in the NGSIv2 implementation notes](ngsiv2_implementation_notes.md#datetime-support).

However, note that `DateTime` attribute type has *not* any special interpretation in the NGSIv1 API, i.e. the
attribute value is treated as any other string without any special meaning. That has two implications:

* Attributes created/updated using the NGSIv1 with type `DateTime` are treated as normal strings.
* Attributes created using NGSIv2 (or which last update has been using NGSIv2) with type `DateTime`, then
  updated using NGSIv1 will "lose" their date nature and they are treated as normal string.
* Attributes created using NGSIv1 (or which last update has been using NGSIv1) with type `DateTime`, then
  updated using NGSIv2 will "gain" their date nature so they can be used in date filters, etc.

[Top](#top)

## Checking ID fields

NGSIv2 introduces syntax restrictions for ID fields (such as entity id/type, attribute name/type
or metadata name/type) which are described in the "Field syntax restrictions" section in the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable). In order to
keep backward compatibility, these restrictions are not used in the NGSIv1 API by default, but
you can enable them using the `-strictNgsiv1Ids` [CLI parameter](../admin/cli.md).

Note that even when `-strictNgsiv1Ids` is used the Orion DB may contain entities/attributes/medatada
that don't conform with NGSIv2 ID rules. This may happen if such entities/attributes/metadata have
been created by Orion in moments in which it has run without `-strictNgsiv1Ids` enabled (maybe an old
version previous to the implementation of this feature). In that case, getting such
entities/attributes/metadata using NGSIv2 API may result in inconstent results (e.g. to use `GET /v2/entities`
and get entities with whitespaces in some entity ids).

The following [issue at github](https://github.com/telefonicaid/fiware-orion/issues/1733) has been created
to cope with this situation in the future. However, in general it is as good practise to follow always the
"Field sysntax restrictions" for IDs even if you are not forced to do so in NGSIv1. Doing so you would avoid
any problem when managing context information, no matter which version of the API (either NGSIv1 or NGSIv2)
you use.

[Top](#top)

## `orderBy` parameter

The `orderBy` parameter defined for NGSIv2 can be used also in NGSIv1 queryContext operation (see
details in the [pagination documentation](pagination.md). However, note that the "geo:distance"
order can be used only in NGSIv2.

[Top](#top)

## NGSIv1 notification with NGSIv2 subscriptions

NGSIv2 allows several notification modes depending on the `attrsFormat` field associated to the
subscription. Apart from the values described in the NGSIv2 specification, Orion also support
`legacy` value in order to send notifications in NGSIv1 format. This way, users can have the
enhancements of NGSIv2 subscriptions (e.g. filtering or system/builtin metadata in notifications) with
NGSIv1 legacy notifications receivers.

[Top](#top)

## NGSIv2 query update forwarding to Context Providers

You can [register providers using either NGSIv1 or NGSIv2 operations](context_providers.md) and have your NGSIv2-based updates and
queries being forwarded to Context Providers, getting the response in NGSIv2. The forwarded message in the CB
to CPr communication, and its response, is done using NGSIv1, although [an NGSIv2-based forwarding mechanism will
be defined in the future](https://github.com/telefonicaid/fiware-orion/issues/3068).

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

[Top](#top)

## Getting registrations created with NGSIv1 using NGSIv2 operations

In general, there is no problem creating registrations using NGSIv1 (in particular, `POST /v1/registry/registerContext`) and then retrieving them using NGSIv2 (in particular, `GET /v2/registrations` or `GET /v2/registrations/<id>`).

Note that NGSIv1 considers the concept of "context registration". A registration is composed of several context
registrations, each one being composed of a set of entities and attributes. NGSIv2 proposes a much simpler approach,
without context registration as intermediate element, i.e. a registration is associated to a set of entities and
attributes directly.

In the case of retrieving a registration created using NGSIv1, and that has more than one context registration, with
`GET /v2/registrations` or `GET /v2/registrations/<id>` only the first one is considered. In other words,
the `dataProvided` element in the response to `GET /v2/registrations` or `GET /v2/registrations/<id>` is
filled using the first context registration (the following ones, if any, are ignored).

This doesn't have to be a problem, as most NGSIv1 registrations use only one context registration (there isn't any
practical advantage of having more than one, from a functional point of view, e.g. forwarding). However, this is
[pending on a more definitive solution](https://github.com/telefonicaid/fiware-orion/issues/3044).

[Top](#top)

## Context availability subscriptions

Note that context availability subscriptions and notifications are not included in NGSIv2. They have been intentionally left out due to it is a feature in NGSIv1 that is rarely used and doesn't worth the effort of being included in NGSIv2.

[Top](#top)