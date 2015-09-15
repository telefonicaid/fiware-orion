# Using empty types

You can use empty types in entities in NGSI9/NGSI10 operations. In fact,
convenience operations implicitly use empty types in this way by default
You can use the `/type/<type>/id/<id>` pattern instead of `<id>` in
convenience operations URLs to specify a type).

Moreover, you can use empty entity types in discover context
availability or query context operations. In this case, the absence of
type in the query is interpreted as "any type".

For example, let's consider having the following context in Orion
Context Broker:

-   Entity 1:
    -   ID: Room1
    -   Type: Room
-   Entity 2:
    -   ID: Room1
    -   Type: Space

A discoveryContextAvailability/querycontext using:

```
  ...
  "entities": [
      {
          "type": "",
          "isPattern": "false",
          "id": "Room1"
      }
  ]
  ...
```

will match both Entity 1 and Entity 2.

Regarding attributes, they can be created without type in updateContext
APPEND. If attribute type is left empty in subsequent updateContext UPDATEs, then
the type is not updated and attribute keeps its previous type.
