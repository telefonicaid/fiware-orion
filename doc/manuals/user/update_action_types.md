# Update action types

`POST /v2/op/update` use an `actionType` field. Its value is as follows:

* [`append`](#append)
* [`appendStrict`](#appendstrict)
* [`update`](#update)
* [`delete`](#delete)
* [`replace`](#replace)

The actionType values are described in following subsections. 
Equivalences to RESTful operations are described as well in the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/).

## `append`

This action type is used for creation of entities, creation of attributes in existing entities
and for updating existing attributes in existing entities. In the latter case, it is equal to `update`.

It maps to `POST /v2/entities` (if the entity does not already exist)
or `POST /v2/entities/<id>/attrs` (if the entity already exists).

## `appendStrict`

This action type is used for creation of entities or attributes in existing entities.
Attempts to use it to update already existing attributes (as  `append` allows) will result in an error.

It maps to `POST /v2/entities` (if the entity does not already exist)
or `POST /v2/entities/<id>/attrs?options=append` (if the entity already exists).

## `update`

This action type is used for modification of already existing attributes. Attempts to use it to create
new entities or attributes (as `append` or `appendStrict` allow) will result in an error.

It maps to `PATCH /v2/entities/<id>/attrs`.

## `delete`

This action type is used for removal of attributes in existing entities (but without removing the
entity itself) or for deletion of entities.

It maps to `DELETE /v2/entities/<id>/attrs/<attrName>` on every attribute included
in the entity or to `DELETE /v2/entities/<id>` if the entity has no attributes.

## `replace`

This action type is used for replacement of attributes in existing entities, i.e. all the existing attributes are
removed and the ones included in the request are added.

It maps to `PUT /v2/entities/<id>/attrs`.
