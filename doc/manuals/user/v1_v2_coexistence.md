# Considerations on NGSIv1 and NGSIv2 coexistence

NGSIv1 is the API offered by Orion Context Broker from its very first version. 
[NGSIv2](http://telefonicaid.github.io/fiware-orion/api/v2/) development started 
in July 2015 in Orion 0.23.0. Although at the end NGSIv1 will be deprecated and 
removed from the code in some future Orion version (so only NGSIv2 will remain) 
this is a big work and both API versions will be coexisting during some time. 

This document explains some consideration to take into account regarding such coexistence.

## Native JSON types

NGSIv2 allows to create/update attributes (and metadata) whose values use JSON native 
types (number, boolean, string, etc.). However, NGSIv1 uses a JSON parser that converts 
numbers and boolean values to string at creation/update time. Thus, an attempt of 
setting `A=2` using NGSIv1 will actually store `A="2"` in the Orion database.

However, NGSIv1 rendering is able to retrieve attributes values stored using 
non-string JSON native types correctly. Thus, if you set `A=2` using NGSIv2 and retrieve that 
attribute using NGSIv1 you will get `A=2`. Currently this work for attribute simple
values, i.e. compound attribute values or metadata values always use string-based rendering.

## Filtering

You can use the filtering capabilities developed for NGSIv2 (`GET /v2/entities?q=<query>`) also 
in NGSIv1 using a Scope element in the payload of `POST /v1/queryContext`. See 
[the following section](filtering.md#string-filter) for details.

However, take into account that some of the filters (e. g. greater/less, range, etc.) are thought
for numeric values. Thus, in order to work properly, these filters (although using a 
`POST /v1/queryContext`) needs that the attributes to which they refer were created using NGSIv2 operations.

In addition, note that NGSIv2 geo-query filters can be used also in NGSIv1. See
[the following section](geolocation.md#geo-located-queries-ngsiv2) for details

## Checking ID fields

NGSIv2 introduces syntax restrictions for ID fields (such as entity id/type, attribute name/type
or metadata name/type) which are described in the "Field syntax restrictions" section in the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/). In order to
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
subscription. Appart of the values described in the NGSIv2 specification, Orion also support
`legacy` value in order to send notifications in NGSIv1 format. This way, users can have the
enhancements in NGSIv2 subscriptions (e.g. filtering) with NGSIv1 legacy notifications receivers.

