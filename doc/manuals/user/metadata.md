# Attribute Metadata

## Custom attribute metadata

Apart from metadata elements to which Orion pays special attention (e.g.
ID, location, etc.), users can attach their own metadata to entity
attributes. These metadata elements are processed by Orion in a
transparent way: it simply stores them in the database at updateContext
(and notifyContext time in [federeated scenarios](federation.md#context-broker-federation)) and retrieve it in
queryContext or notifyContext.

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
      
Note that, from the point of view of [ONCHANGE
subscription](walkthrough_apiv1.md#onchange), changing the metadata of a given
attribute or adding a new metadata element is considered a change even
if attribute value itself hasn't changed. Metadata elements cannot be
deleted once introduced: in order to delete metadata elements you have
to remove the entity attribute (using [updateContext
DELETE](append_and_delete.md#adding-and-removing-attributes-with-append-and-delete-in-updatecontext)),
then re-create it ([using updateContext
APPEND](append_and_delete.md#adding-and-removing-attributes-with-append-and-delete-in-updatecontext)).

You can use any name for your custom metadata except for the ones used
for some metadata names that are interpreted by Orion:

-   [ID](#metadata-id-for-attributes)
-   [location](geolocation.md#Defining_location_attribute )
-   creData (reserved for future use)
-   modDate (reserved for future use)

## Metadata ID for attributes

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
