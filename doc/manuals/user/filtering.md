#<a name="top"></a>Filtering results

* [Introduction](#introduction)
* [NGSIv2 filtering]
  * [Simple Query Language](#simple-query-language)
  * [Geographical Queries](#geographical-queries)
* [NGSIv1 filtering]
  * [Existence type filter](#existence-type-filter)
  * [No-Existence type filter](#no-existence-type-filter)
  * [Entity type filter](#entity-type-filter)
  * [Geo-location filter](#geo-location-filter)
  * [Geo-location filter NGSIv2](#geo-location-filter-ngsiv2)
  * [String query filter](#string-filter)
    
## Introduction

NGSIv2 and NGSIv1 have different filtering mechanisms. For example, in NGSIv1,
filtering is heavily based on the use of the `scope` payload element. Both
approaches (NGSIv2 and NGSIv1) are described below, each in a separate section.

[Top](#top)

## NGSIv2 filtering

NGSIv2 implements Simple Query Language and Geographical Queries. They
can be used in both synchronous queries (e.g. `GET /v2/entities`) and
subscription notifications (in the `subject.condition.expression` field).

### Simple Query Language

The Simple Query Language allows to define conditions that entity attributes must match, e.g.
attribute "temperature" has to be greater than 40.

Examples are found in [this section of the API walthrough](walkthrough_apiv2#query-entity).
The full syntax definition is found in the "Simple Query Language"
section of the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

[Top](#top)

### Geographical Queries

Geographical Queries allow to filter by geographical location, e.g. all the entities located
closer than 15 km from the center of Madrid. Of course, properly located entities are mandatory.

Both topics (entity location and geographical queries) are dealt in detail in
the "Geospacial properties of entities" and "Geographical Queries" sections of the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

[Top](#top)

## NGSIv1 filtering

Orion Context Broker implements several filters
that can be used to filter the results in NGSI10 query operations. These
filters are typically used with [queryContext with patterns](walkthrough_apiv1.md#query-context-operation) or [the convenience operation to get all entities](walkthrough_apiv1.md#getting-all-entities).

As a general rule, filters used in standard operation use a scope
element:

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "myEntityType",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "restriction": {
        "scopes": [
            {
                "type": "FIWARE::Filter::foobar",
                "value": ""
            }
        ]
    }
}
EOF
```

while filters in convenience operations are included as parameters in
the URL:

```
curl localhost:1026/v1/contextEntities?filter=value -s -S  --header 'Content-Type: application/json'  \
    --header 'Accept: application/json' | python -mjson.tool
```
Filters are cumulative. In other words, you can use several scopes in
the same restriction (in the case of standard operations) or several URL
argument separated by '&' in order to specify several filters. The
result is a logic "and" between all of them.

[Top](#top)

### Existence type filter

The scope correspoding to this type is "FIWARE::Filter::Existence". 

```
{
    "restriction": {
        "scopes": [
            {
                "type": "FIWARE::Filter::Existence",
                "value": "entity::type"
            }
        ]
    }
}
```
  
The URL parameter corresponding to this filter is 'exist'.

    curl localhost:1026/v1/contextEntities?exist=entity::type ...

In the current version, the only parameter than can be checked for
existence is the entity type, corresponding to "entity::type".

[Top](#top)

### No-Existence type filter

The scope corresponding to this type is "FIWARE::Filter::Not::Existence".

```
... 
    {
        "restriction": {
            "scopes": [
                {
                    "type": "FIWARE::Filter::Not::Existence",
                    "value": "entity::type"
                }
            ]
        }
    }
...
```
 
The URL parameter corresponding to this filter is '!exist'.

    curl localhost:1026/v1/contextEntities?!exist=entity::type ...

In the current version, the only parameter than can be checked for
no-existence is the entity type, corresponding to "entity::type". Note
that this is the only way of selecting an "entity without type" (given
that queries without type resolve to "any type", as explained in the
[following section](empty_types.md#using-empty-types)).

[Top](#top)

### Entity type filter

There is no scope corresponding to this filter, given that you can use
the usual entity type:

```
...
    {
        "type": "Room",
        "isPattern": "...",
        "id": "..."
    }
...
```
The URL parameter corresponding to this filter is 'entity::type'.

    curl localhost:1026/v1/contextEntities?entity::type=Room ...

[Top](#top)

### Geo-location filter

The scope corresponding to this type is "FIWARE::Location". It is
described in detail in [the following section](geolocation.md#geo-located-queries).

In the current version of Orion, there is no equivalent convenience
operation filter.

[Top](#top)

### Geo-location filter NGSIv2

The scope corresponding to this type is "FIWARE::Location::NGSIv2". It is
described in detail in [the following section](geolocation.md#geo-located-queries-ngsiv2).

[Top](#top)

### String filter

The scope corresponding to this type is "FIWARE::StringQuery".

```
...
    {
        "restriction": {
            "scopes": [
                {
                    "type": "FIWARE::StringQuery",
                    "value": "temperature<24;humidity==75..90;status==running"
                }
            ]
        }
    }
...
```

This scope allows to express filtering conditions such as equality, unequality,
greater/less than, range or existence.

There isn't any URL parameter correspondence for this filter in NGSI v1. In NGSI v2
it corresponds to the `q` parameter:

    curl 'localhost:1026/v2/entities?q=temperature<24;humidity==75..90;status==running'

For a detailed syntax description of the `value` or `q` parameter, see [NGSIv2 specification
document](http://telefonicaid.github.io/fiware-orion/api/v2/stable).

You can use this scope in NGSI v1, but take into account that in order to set attribute
values to numbers, you need to use NGSI v2 (NGSI v1 always uses strings for values).

[Top](#top)
