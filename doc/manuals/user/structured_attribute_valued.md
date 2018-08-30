# Structured values for attributes

Apart from simple values such as 22.5 or "yellow", you can use
complex structures as attribute values. In particular, an attribute can
be set to a vector or to a key-value map (usually referred to as an
"object") at creation/update time. These values are
retrieved at query and notification time.

Vector or key-map values correspond directly to JSON vectors and JSON
objects, respectively. Thus, the following create entity request sets
the value of attribute "A" to a vector and the value of attribute B to a
key-map object (we show it in an entity creation operation, but the
same applies to attribute updates).

```
curl localhost:1026/v2/entities -s -S --header 'Content-Type: application/json' \
   -d @- <<EOF
{
  "id": "E1",
  "type": "T",
  "A": {
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
  "B": {
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
}
EOF
```

The value of the attribute is stored internally by Orion Context Broker
in a format-independent representation. 
