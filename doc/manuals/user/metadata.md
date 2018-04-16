# Attribute Metadata

Apart from metadata elements to which Orion pays special attention (e.g.
`dateCreated`, etc.), users can attach their own metadata to entity attributes. These
metadata elements are processed by Orion in a transparent way: Orion simply
stores them in the database at update time and retrieves them at query and
notification time.

You can use any name for your custom metadata except for a few reserved names, used
for special metadata that are interpreted by Orion:

-   [ID](#metadata-id-for-attributes) (deprecated, but still "blocked" as forbidden keyword)
-   location, which is currently [deprecated](../deprecated.md), but still supported
-   The ones defined in "System/builtin in metadata" section in the NGSIv2 spec

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
to remove the entity attribute (see [DELETE action type](update_action_types.md#delete)),
then re-create it (see [APPEND action type](update_action_types.md#append)).


## Metadata in notifications

By default, all custom (user) metadata are included in notifications. However, the field `metadata`
can be used to filter the list. In addition, it can be used to specify that the following special
metadata (not included by default) must be included.

* previousValue
* actionType

Details about their meaning can be found in the ""System/builtin in metadata"" section in
the NGSIv2 specification.

Note that using the following

```
"metadata": [ "previousValue" ]
```

will cause to include `previousValue` but will exclude user metadata that
attributes in the notification may have. If you want to get `previousValue`
*and* any other "regular" metadata then use:

```
"metadata": [ "previousValue", "*" ]
```

Note that you can also use `"metadata": [ "*" ]` although it doesn't make much sense, as
it gives the same result as not including `metadata` at all (remember that the default
behaviour is to include all user metadata).

See details in "Filtering out attributes and metadata" section in the NGSIv2 specification.
