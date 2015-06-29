# Structured values for attributes

Apart from simple strings such as "22.5" or "yellow", you can use
complex structures as attribute values. In particular, an attribute can
be set to a vector or to a key-value map (usually referred to as an
"object") using [updateContext](#Update_context_elements "wikilink") (or
equivalent [convenience
operation](#Convenience_Update_Context "wikilink")). These values are
retrieved with a [queryContext](#Query_Context_operation "wikilink") on
that attribute (or equivalent [convenience
operation](#Convenience_Query_Context "wikilink")) and notifyContext
notifications sent as a consequence of a NGSI10 subscriptions.

Vector or key-map values correspond directly to JSON vectors and JSON
objects, respectively. Thus, the following updateContext request sets
the value of attribute "A" to a vector and the value of attribute B to a
key-map object (we use UPDATE as actionType, but this can be used at
initial entity or attribute creation with APPEND).

    (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
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
                   "value": [ "22" , 
                              {
                                 "x": [ "x1", "x2"], 
                                 "y": "3" 
                              }, 
                              [ "z1", "z2" ] 
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
                      "y": [ "y1", "y2" ]
                   }
               }
               ]
           }
       ],
       "updateAction": "UPDATE"
     }
    EOF

In the case of XML, the structure is a bit less "natural" than in JSON,
but equivalent:

    (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF
    <?xml version="1.0" encoding="UTF-8"?>
     <updateContextRequest>
     <contextElementList>
       <contextElement>
         <entityId type="T1" isPattern="false">
           <id>E1</id>
         </entityId>
         <contextAttributeList>
           <contextAttribute>
             <name>A</name>
             <type>T</type>
             <contextValue type="vector">
                <item>22</item>
                <item>
                  <x type="vector">
                    <item>x1</item>
                    <item>x2</item>
                  </x>
                  <y>3</y>
                </item>
                <item type="vector">
                  <item>z1</item>
                  <item>z2</item>
                </item>
             </contextValue>
           </contextAttribute>
           <contextAttribute>
             <name>B</name>
             <type>T</type>
             <contextValue>
                <x>
                  <x1>a</x1>
                  <x2>b</x2>
                </x>
                <y type="vector">
                   <item>y1</item>
                   <item>y2</item>
                </y>
             </contextValue>
           </contextAttribute>
         </contextAttributeList>
       </contextElement>
     </contextElementList>
     <updateAction>UPDATE</updateAction>
     </updateContextRequest>
    EOF

The particular rules applied to XML format are the following ones:

-   Tags **not using** the "type" attribute equal to "vector" represent
    key-map object, taking into account the following:
    -   Each child tag is considered a key-map pair of the object, whose
        key is the tag name and the value is the content of the tag. The
        value can be either a string or a tag (which can represent
        either a vector or a key-map object).
    -   It is not allowed to have 2 child tags with the same name (a
        parse error is generated in that case).
-   Tags **using** the "type" attribute equal to "vector" represent a
    vector, taking into account the following:
    -   Each child tag is considered a vector element, whose value is
        the content of the tag. The value can be either a string or a
        tag (which can represent either a vector or a key-map object).
        The name of the tag is not taken into account (vector elements
        have no name, otherwise the vector wouldn't be an actual vector,
        but a key-map).
    -   In updateContext, you can use whatever name you want for
        child tags. However, all the child tags must have the same name
        (otherwise a parse error is generated).
-   Except for "type", XML attributes are not allowed within the
    sub-tree of an structured value (a parse error is generated as
    a consequence).

The value of the attribute is stored internally by Orion Context Broker
in a format-independent representation. Thus, you can updateContext a
structured attribute using JSON, then queryContext that attribute using
XML (or vice-versa) and you get the equivalent result. The internal
format-independent representation ignores the tag names for vector items
set using XML (as described above), so the
queryContextResponse/notifyContextRequest in XML uses always a
predefined tag name for that: <item> (that may not match the tag name
used in the updateContext operation setting that value; in fact, the
updateContext operation could have been done with JSON, in which case
the tag name for vector items has no sense at all).

Note that in order to align XML/JSON representations, the final "leaf"
elements of the structured attribute values after travesing all vectors
and key-maps are always considered as strings. String is the "natural"
type for elements in XML, but not in JSON (that allow other types, such
as integer). Thus, take into account that although you can use for
instance a JSON integer as a field value in updates (such as {"x": 3 }),
you will retrieve a string in queries and notifications (i.e. { "x": "3"
}).