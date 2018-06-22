# Update action types

Both `POST /v1/updateContext` (NGSIv1) and `POST /v2/op/update` use an `actionType` field.
Its value is as follows:

* [`append`](#append) (NGSIv2) or `APPEND` (NGSIv1)
* [`appendStrict`](#appendstrict) (NGSIv2) or `APPEND_STRICT` (NGSIv1)
* [`update`](#update) (NGSIv2) or `UPDATE` (NGSIv1)
* [`delete`](#delete) (NGSIv2) or `DELETE` (NGSIv1)
* [`replace`](#replace) (NGSIv2) or `REPLACE` (NGSIv1)

The actionType values are described in following subsections. 
In the case of NGSIv2, equivalences to RESTful operations are described as well
(see [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable/) for details).
Similar equivalences exist as convenicence operations in the case of NGSIv1 (the 
[final example](#example-about-creation-and-removal-of-attributes-in-ngsiv1) illustrates them).

## `append`

This action type is used for creation of entities, creation of attributes in existing entities
and for updating existing attributes in existing entities. In the latter case, it is equal to `update`.

In NGSIv2 it maps to `POST /v2/entities` (if the entity does not already exist)
or `POST /v2/entities/<id>/attrs` (if the entity already exists).

## `appendStrict`

This action type is used for creation of entities or attributes in existing entities.
Attempts to use it to update already existing attributes (as  `append` allows) will result in an error.

In NGSIv2 it maps to `POST /v2/entities` (if the entity does not already exist)
or `POST /v2/entities/<id>/attrs?options=append` (if the entity already exists).

## `update`

This action type is used for modification of already existing attributes. Attempts to use it to create
new entities or attributes (as `append` or `appendStrict` allow) will result in an error.

In NGSIv2 it maps to `PATCH /v2/entities/<id>/attrs`.

## `delete`

This action type is used for removal of attributes in existing entities (but without removing the
entity itself) or for deletion of entities.

In NGSIv2 it maps to `DELETE /v2/entities/<id>/attrs/<attrName>` on every attribute included
in the entity or to `DELETE /v2/entities/<id>` if the entity has no attributes.

## `replace`

This action type is used for replacement of attributes in existing entities, i.e. all the existing attributes are
removed and the ones included in the request are added.

In NGSIv2 it maps to `PUT /v2/entities/<id>/attrs`.

## Example about creation and removal of attributes in NGSIv1

We have seen how to use updateContext with the APPEND action type to [create
new entities](walkthrough_apiv1.md#entity-creation). In addition, APPEND can be
used to add a new attribute after entity creation. Let's illustrate this
with an example.

We start by creating a simple entity 'E1' with one attribute named 'A':
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
```

Now, in order to append a new attribute (let's name it 'B') we use
updateContext APPEND with an entityId matching 'E1':

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "TA",
                    "value": "1"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```
We can now check with a query to that entity that both attribute A and
B are there:

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool)<<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "B",
                "type": "TB",
                "value": "2"
            },
            {
                "name": "A",
                "type": "TA",
                "value": "1"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```


APPEND is interpreted as an UPDATE on existing context elements. However, you can use APPEND_STRICT instead of APPEND
as updateAction. Using APPEND_STRICT, existing attributes are not updated but an error is reported.
Note that if your APPEND_STRICT request includes several attributes (e.g. A and B), some of them existing and other not
existing (e.g. A exists and B doesn't exist), then the ones that doesn't exist are added (in this case, B is added) and
an error is reported for the existing ones (in this case, an error is reported about A that already exists).


You can also remove attributes in a similar way, using the DELETE action type.
For example, to remove attribute 'A', we will use (note the empty
contextValue element):

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "TA",
                    "value": ""
                }
            ]
        }
    ],
    "updateAction": "DELETE"
}
EOF
```

Now, a query on the entity shows attribute B:

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "B",
                "type": "TB",
                "value": "2"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```


You can also use convenience operations with POST and DELETE methods to
add and delete attributes. Try the following:

Add a new attribute 'C' and 'D':

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "attributes": [
        {
            "name": "C",
            "type": "TC",
            "value": "3"
        },
        {
            "name": "D",
            "type": "TD",
            "value": "4"
        }
    ]
}
EOF
```

Remove attribute 'B':
```
curl localhost:1026/v1/contextEntities/E1/attribute/B -s -S \
    --header 'Content-Type: application/json'  -X DELETE  \
    --header 'Accept: application/json'  | python -mjson.tool
```
Query entity (should see 'C' and 'D', but not 'B'):

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "C",
                "type": "TC",
                "value": "3"
            },
            {
                "name": "D",
                "type": "TD",
                "value": "4"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```

Apart from deleting individual attributes from a given entity,
you can also delete an entire entity, including all its attributes and
their corresponding metadata. In order to do so, the updateContext
operation is used, with DELETE as actionType and without any
list of attributes, as in the following example:

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1"
        }
    ],
    "updateAction": "DELETE"
}
EOF
```

You can also use the following equivalent convenience operation:
```
curl localhost:1026/v1/contextEntities/E1 -s -S \
    --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -X DELETE
```

