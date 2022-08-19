# Attribute Metadata

Apart from metadata elements to which Orion pays special attention (e.g.
`dateCreated`, etc.), users can attach their own metadata to entity attributes. These
metadata elements are processed by Orion in a transparent way: Orion simply
stores them in the database at update time and retrieves them at query and
notification time.

You can use any name for your custom metadata except for a few reserved names, used
for special metadata that are interpreted by Orion:

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

## Updating metadata

When an attribute is updated the following rules apply:

* Metadata included in the attribute update request *not previously existing* are added
  to the attribute
* Metadata included in the attribute update request *previous existing* are updated
  in the attribute
* Existing metadata not included in the request are not touched in the attribute (i.e. they keep the
  previous value).

For instance, let's consider an attribute `temperature` with metadata `unit` and `avg` which values
are at the present moment:

* `unit`: `"celsius"`
* `avg`: `25.4`

and Context Broker receives a request like this:

```
PUT /v2/entities/E/attrs/temperature
{
  "value": 26,
  "type": "Number",
  "metadata": {
    "avg": {
      "value": 25.6,
      "type": "Number"
    },
    "accuracy": {
      "value": 98.7,
      "type": "Number"
    }
  }
}
```

After processing the update, the metadata at the attribute `temperature` would be:

* `unit`: `"celsius"` (existing and not touched by the request)
* `avg`: `25.6` (existing but touched by the request)
* `accuracy`: `98.7` (metadata added by the request)

The rationale behind the "stikyness" of metadata in this default behaviour is described in
more detail in [this issue at Orion repository](https://github.com/telefonicaid/fiware-orion/issues/4033)

### `overrideMetadata` option

You can override the default behaviour using the `overrideMetadata` option. In that case,
the metadata in the request replace the previously ones existing in the attribute.
For instance, with the same initial situation than before, but adding the
`overrideMetadata` option to the request:

```
PUT /v2/entities/E/attrs/temperature?options=overrideMetadata
{
  "value": 26,
  "type": "Number",
  "metadata": {
    "avg": {
      "value": 25.6,
      "type": "Number"
    },
    "accuracy": {
      "value": 98.7,
      "type": "Number"
    }
  }
}
```

After processing the update, the metadata at the attribute `temperature` would be:

* `avg`: `25.6` (existing but touched by the request)
* `accuracy`: `98.7` (metadata added by the request)

Note that `unit` metadata has been removed.

The `overrideMetadata` option can be use also to cleanup the metadata of a given
attribute omitting the `metadata` field in the request (equivalently, using
`"metadata":{}`), e.g.:

```
PUT /v2/entities/E/attrs/temperature?options=overrideMetadata
{
  "value": 26,
  "type": "Number"
}
```

Note `overrideMetadata` option is ignored in the update attribute value operation
(e.g. `PUT /v2/entities/E/attrs/temperature/value`) as in that case the operation
semantics makes explicit that only the value is going to be updated
(leaving `type` and `metadata` attribute fields untouched).
