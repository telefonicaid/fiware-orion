# Structured values for attributes

Apart from simple strings such as "22.5" or "yellow", you can use
complex structures as attribute values. In particular, an attribute can
be set to a vector or to a key-value map (usually referred to as an
"object") using [updateContext](walkthrough_apiv1.md#update-context-elements) (or
equivalent [convenience
operation](walkthrough_apiv1.md#Convenience_Update_Context)). These values are
retrieved with a [queryContext](walkthrough_apiv1.md#query-context-operation) on
that attribute (or equivalent [convenience
operation](walkthrough_apiv1.md#Convenience_Query_Context)) and notifyContext
notifications sent as a consequence of a NGSI10 subscriptions.

Vector or key-map values correspond directly to JSON vectors and JSON
objects, respectively. Thus, the following updateContext request sets
the value of attribute "A" to a vector and the value of attribute B to a
key-map object (we use UPDATE as actionType, but this can be used at
initial entity or attribute creation with APPEND).

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T1",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "T",
                    "value": [
                        "22",
                        {
                            "x": [
                                "x1",
                                "x2"
                            ],
                            "y": "3"
                        },
                        [
                            "z1",
                            "z2"
                        ]
                    ]
                },
                {
                    "name": "B",
                    "type": "T",
                    "value": {
                        "x": {
                            "x1": "a",
                            "x2": "b"
                        },
                        "y": [
                            "y1",
                            "y2"
                        ]
                    }
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
``` 

The value of the attribute is stored internally by Orion Context Broker
in a format-independent representation. 

Note that (for API version 1) in order to align JSON representations, the final "leaf"
elements of the structured attribute values after traversing all vectors
and key-maps are always considered as strings. Thus, take into account that
although you can use for instance a JSON number as a field value in updates
(such as `{"x": 3.2 }`), you will retrieve a string in queries and notifications 
(i.e. `{ "x": "3.2"}`).

In API version 2, the final "leaf" elements of the structured attribute values after traversing all vectors and key-maps uses the right native JSON type (number, boolean, etc.). Thus, take if you use for instance a JSON number as a field value in updates (such as `{"x": 3.2 }`), you will retrieve also a number in queries and notifications (i.e. `{ "x": 3.2}`).
