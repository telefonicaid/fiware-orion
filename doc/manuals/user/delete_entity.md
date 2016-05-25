# Deleting entities (using NGSIv2)

Entities can be deleted using `DELETE /v2/entities/{id}` (in the case the entity id
identifies univocally the entity) or `DELETE /v2/entities/{id}?type={entityType}`
(in the case the type is needed to identify the entity).

You can also use the batch operation `POST /op/update` to remove entities, using
`actionType` DELETE.

# Deleting entities (using NGSIv1)

Apart from [deleting individual attributes from a given entity](append_and_delete.md),
you can also delete an entire entity, including all its attributes with
their corresponding metadata. In order to do so, the updateContext
operation is used, with DELETE as actionType and with an empty
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
