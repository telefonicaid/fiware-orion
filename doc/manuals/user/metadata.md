# Attribute Metadata

Apart from metadata elements to which Orion pays special attention (e.g.
ID, etc.), users can attach their own metadata to entity attributes. These
metadata elements are processed by Orion in a transparent way: Orion simply
stores them in the database at update time and retrieves them at query and
notification time.

You can use any name for your custom metadata except for a few reserved names, used
for special metadata that are interpreted by Orion:

-   [ID](#metadata-id-for-attributes)
-   location, which is currently [deprecated](../deprecated.md), but still supported
-   ngsi:onArrival, used by the "Notification metadata marks" functionality in NGSIv2
-   ngsi:onChange, used by the "Notification metadata marks" functionality in NGSIv2
-   ngsi:onSubscription, used by the "Notification metadata marks" functionality in NGSIv2

Its management is slightly different in NGSIv1 and NGSIv2, so it is
described in different sections.

## Custom attribute metadata (using NGSIv2)

For example, to create an entity Room1 with attribute "temperature", and
associate metadata "accuracy" to "temperature":

```
curl localhost:1026/v2/entities -s -S --header 'Content-Type: application/json' \
    -d @- <<EOF
{
  "id": "Room1",
  "type": "Room",
  "temperature": {
    "value": 26.5,
    "type": "Float",
    "metadata": {
      "accuracy": {
        "value": 0.8,
        "type": "Float"
      }
    }
  }
}
EOF
```

Unlike NGSIv1, at the moment NGSIv2 doesn't define an operation
to update metadata regardless of the attribute value being updated
or not. In addition, it doesn't define an operation to add metadata after
attribute creation. In other words, the whole metadata array is updated
along with attribute value and type in the `PUT /v2/entities/{id}/attrs/{attrName}` operation.

We can check that temperature includes the metadata:

```
curl localhost:1026/v2/entities/Room1 -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

which response is

```
{
    "id": "Room1",
    "temperature": {
        "metadata": {
            "accuracy": {
                "type": "Float",
                "value": 0.8
            }
        },
        "type": "Float",
        "value": 26.5
    },
    "type": "Room"
}
```

At the moment, NGSIv2 doesn't allow to delete individual metadata elements once introduced.
However, you can delete all metadata updating the attribute with `metadata` set to `{}`.

Note that, from the point of view of [subscriptions](walkthrough_apiv2.md#subscriptions),
changing the metadata of a given attribute is considered a change even
though the attribute value itself hasn't changed.

## Custom attribute metadata (using NGSIv1)

For example, to create an entity Room1 with attribute "temperature", and
associate metadata "accuracy" to "temperature":

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "accuracy",
                            "type": "float",
                            "value": "0.8"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF 
``` 
  
Metadata can be updated regardless of the attribute value being updated
or not. For example, next updateContext shows how "accuracy" is updated
to 0.9 although the value of the temperature iself is still 26.5:

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "accuracy",
                            "type": "float",
                            "value": "0.9"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
``` 
      
Metadata can be added after attribute creation. For example, if we want
to add metadata "average" to "temperature" (in addition to the existing
"precision"):

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "average",
                            "type": "float",
                            "value": "22.4"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
```
We can check that temperature includes both attributes

``` 
(curl localhost:1026/v1/contextEntities/Room1 -s -S \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5",
                    "metadatas": [
                        {
                            "name": "average",
                            "type": "float",
                            "value": "22.4"
                        },
                        {
                            "name": "accuracy",
                            "type": "float",
                            "value": "0.9"
                        }
                    ]
                }
            ]
        }
    ],
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
``` 
      
Note that, from the point of view of [subscriptions](walkthrough_apiv1.md#context-subscriptions), changing the metadata of a given
attribute or adding a new metadata element is considered a change even
if attribute value itself hasn't changed. Metadata elements cannot be
deleted once introduced: in order to delete metadata elements you have
to remove the entity attribute (using [updateContext
DELETE](append_and_delete.md#adding-and-removing-attributes-with-append-and-delete-in-updatecontext)),
then re-create it ([using updateContext
APPEND](append_and_delete.md#adding-and-removing-attributes-with-append-and-delete-in-updatecontext)).

## Metadata ID for attributes

This functionality is not included in the NGSIv2 API.

Sometimes, you could want to model attributes belonging to an entity
which share the same name and type. For example, let's consider an
entity "Room1" which has two temperature sensors: one in the ground and
other in the wall. We can model this as two instances of the attribute
"temperature", one with ID "ground" and the other with the ID "wall". We
use the metadata ID for this purpose. Let's illustrate with an example.

First, we create the Room1 entity:

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "23.5",
                    "metadatas": [
                        {
                            "name": "ID",
                            "type": "string",
                            "value": "ground"
                        }
                    ]
                },
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "23.8",
                    "metadatas": [
                        {
                            "name": "ID",
                            "type": "string",
                            "value": "wall"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
``` 

Now, we can query for temperature to get both instances:

``` 
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

We can update an specific instance (e.g. ground), letting the other
untouched:

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "30",
                    "metadatas": [
                        {
                            "name": "ID",
                            "type": "string",
                            "value": "ground"
                        }
                    ]
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
``` 

Check it using again queryContext (ground has changed to 30ºC but wall
has its initial value of 23.8º C).

To avoid ambiguities, you cannot mix the same attribute with and without
ID. The following entity creation will fail:

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "23.5",
                    "metadatas": [
                        {
                            "name": "ID",
                            "type": "string",
                            "value": "ground"
                        }
                    ]
                },
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "23.8"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
``` 

```
...
    {
	"statusCode": {
	    "code": "472",
	    "details": "action: APPEND - entity: (Room1, Room) - offending attribute: temperature",
	    "reasonPhrase": "request parameter is invalid/not allowed"
	}
    }
...
```
      
Finally, you can use also the following convenience operations with
attributes using ID metadata:

-   GET /v1/contextEntities/Room1/attributes/temperature/ground: to get
    an specific attribute identified by ID
-   PUT /v1/contextEntities/Room1/attributes/temperature/ground (using
    as payload updateContextElementRequest, as [described in a previous
    section](walkthrough_apiv1.md#convenience-update-context)).
-   DELETE /v1/contextEntities/Room1/attributes/temperature/ground: to
    remove an specific attribute identified by ID (see DELETE attribute
    semantics [described in a previous
    section](append_and_delete.md#adding-and-removing-attributes-with-append-and-delete-in-updatecontext)).


## Metadata in notifications

By default, all custom (user) metadata are included in notifications. However, the field `metadata`
can be used to filter the list. In addition, it can used to specify that the following special
metadata (not included by default) has to be included.

* previousValue
* actionType

Details about their meaning can be found in "Special metadata in notifications" section in
the NGSIv2 specification).

Note that using the following

```
"metadata": [ "previousValue" ]
```

will cause to include `previousValue` but will remove any other metadata that
attributes in the notification may potentially have. If you want to get `previousValue`
*and* any other "regular" metadata then use:

```
"metadata": [ "previousValue", "*" ]
```

Note that you can also use `"metadata": [ "*" ]` although it doesn't have too much sense, as
you will get the same result that not including `metadata` at all (remember that default
behaviour is to include all custom metadata).
