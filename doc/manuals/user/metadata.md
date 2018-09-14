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
-   The ones defined in "Builtin metadata" section in the NGSIv2 spec

## Custom attribute metadata

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

At the moment NGSIv2 doesn't define an operation
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
