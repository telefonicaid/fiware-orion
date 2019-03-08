# <a name="top"></a>Filtering results

* [Introduction](#introduction)
* [Simple Query Language](#simple-query-language)
* [Geographical Queries](#geographical-queries)
    
## Introduction

NGSIv2 implements Simple Query Language and Geographical Queries. They
can be used in both synchronous queries (e.g. `GET /v2/entities`) and
subscription notifications (in the `subject.condition.expression` field).

[Top](#top)

## Simple Query Language

The Simple Query Language allows to define conditions that entity attributes must match, e.g.
attribute "temperature" has to be greater than 40. It can be also used to define conditions
on the attribute metadata, e.g. metadata "accuracy" of attribute "metadata" has to be greater than 0.9.

Examples are found in [this section of the API walkthrough](walkthrough_apiv2#query-entity).
The full syntax definition is found in the "Simple Query Language"
section of the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

[Top](#top)

## Geographical Queries

Geographical Queries allow to filter by geographical location, e.g. all the entities located
closer than 15 km from the center of Madrid. Of course, properly located entities are mandatory.

Both topics (entity location and geographical queries) are dealt in detail in
the "Geospacial properties of entities" and "Geographical Queries" sections of the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

[Top](#top)
