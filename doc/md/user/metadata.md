# Attribute Metadata

## Custom attribute metadata

Apart from metadata elements to which Orion pays special attention (e.g.
ID, location, etc.), users can attach their own metadata to entity
attributes. These metadata elements are processed by Orion in a
transparent way: it simply stores them in the database at updateContext
(and notifyContext time in [federeated
scenarios](#Context_Broker_Federation "wikilink")) and retrieve it in
queryContext or notifyContext.

For example, to create an entity Room1 with attribute "temperature", and
associate metadata "accuracy" to "temperature":

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                          {
          <contextElement>                                                                                                              "type": "Room",
            <entityId type="Room" isPattern="false">                                                                                    "isPattern": "false",
              <id>Room1</id>                                                                                                            "id": "Room1",
            </entityId>                                                                                                                 "attributes": [
            <contextAttributeList>                                                                                                      {
              <contextAttribute>                                                                                                          "name": "temperature",
                <name>temperature</name>                                                                                                  "type": "float",
                <type>float</type>                                                                                                        "value": "26.5",
                <contextValue>26.5</contextValue>                                                                                         "metadatas": [
                <metadata>                                                                                                                {
                  <contextMetadata>                                                                                                         "name": "accuracy",
                    <name>accuracy</name>                                                                                                   "type": "float",
                    <type>float</type>                                                                                                      "value": "0.8"
                    <value>0.8</value>                                                                                                    }
                  </contextMetadata>                                                                                                      ]
                </metadata>                                                                                                             }
              </contextAttribute>                                                                                                       ]
            </contextAttributeList>                                                                                                   }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "APPEND"
        <updateAction>APPEND</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
Metadata can be updated regardless of the attribute value being updated
or not. For example, next updateContext shows how "accuracy" is updated
to 0.9 although the value of the temperature iself is still 26.5:

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                          {
          <contextElement>                                                                                                              "type": "Room",
            <entityId type="Room" isPattern="false">                                                                                    "isPattern": "false",
              <id>Room1</id>                                                                                                            "id": "Room1",
            </entityId>                                                                                                                 "attributes": [
            <contextAttributeList>                                                                                                      {
              <contextAttribute>                                                                                                          "name": "temperature",
                <name>temperature</name>                                                                                                  "type": "float",
                <type>float</type>                                                                                                        "value": "26.5",
                <contextValue>26.5</contextValue>                                                                                         "metadatas": [
                <metadata>                                                                                                                {
                  <contextMetadata>                                                                                                         "name": "accuracy",
                    <name>accuracy</name>                                                                                                   "type": "float",
                    <type>float</type>                                                                                                      "value": "0.9"
                    <value>0.9</value>                                                                                                    }
                  </contextMetadata>                                                                                                      ]
                </metadata>                                                                                                             }
              </contextAttribute>                                                                                                       ]
            </contextAttributeList>                                                                                                   }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "UPDATE"
        <updateAction>UPDATE</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
Metadata can be added after attribute creation. For example, if we want
to add metadata "average" to "temperature" (in addition to the existing
"precision"):

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                          {
          <contextElement>                                                                                                              "type": "Room",
            <entityId type="Room" isPattern="false">                                                                                    "isPattern": "false",
              <id>Room1</id>                                                                                                            "id": "Room1",
            </entityId>                                                                                                                 "attributes": [
            <contextAttributeList>                                                                                                      {
              <contextAttribute>                                                                                                          "name": "temperature",
                <name>temperature</name>                                                                                                  "type": "float",
                <type>float</type>                                                                                                        "value": "26.5",
                <contextValue>26.5</contextValue>                                                                                         "metadatas": [
                <metadata>                                                                                                                {
                  <contextMetadata>                                                                                                         "name": "average",
                    <name>average</name>                                                                                                    "type": "float",
                    <type>float</type>                                                                                                      "value": "22.4"
                    <value>22.4</value>                                                                                                   }
                  </contextMetadata>                                                                                                      ]
                </metadata>                                                                                                             }
              </contextAttribute>                                                                                                       ]
            </contextAttributeList>                                                                                                   }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "UPDATE"
        <updateAction>UPDATE</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
We can check that temperature includes both attributes

      curl localhost:1026/v1/contextEntities/Room1 -s -S | xmllint --format -       curl localhost:1026/v1/contextEntities/Room1 -s -S --header 'Accept: application/json' | python -mjson.tool

      <?xml version="1.0"?>                              {
      <contextElementResponse>                             "contextElements": [
        <contextElement>                                   {
          <entityId type="Room" isPattern="false">           "type": "Room",
            <id>Room1</id>                                   "isPattern": "false",
          </entityId>                                        "id": "Room1",
          <contextAttributeList>                             "attributes": [
            <contextAttribute>                               {
              <name>temperature</name>                         "name": "temperature",
              <type>float</type>                               "type": "float",
              <contextValue>26.5</contextValue>                "value": "26.5",
              <metadata>                                       "metadatas": [
                <contextMetadata>                              {
                  <name>average</name>                           "name": "average",
                  <type>float</type>                             "type": "float",
                  <value>22.4</value>                            "value": "22.4"
                </contextMetadata>                             },
                <contextMetadata>                              {
                  <name>accuracy</name>                          "name": "accuracy",
                  <type>float</type>                             "type": "float",
                  <value>0.9</value>                             "value": "0.9"
                </contextMetadata>                             }
              </metadata>                                      ]
            </contextAttribute>                              }
          </contextAttributeList>                            ]
        </contextElement>                                  }
        <statusCode>                                       ],
          <code>200</code>                                 "statusCode": {
          <reasonPhrase>OK</reasonPhrase>                    "code": "200",
        </statusCode>                                        "reasonPhrase": "OK"
      </contextElementResponse>                            }
                                                         }
Note that, from the point of view of [ONCHANGE
subscription](#ONCHANGE "wikilink"), changing the metadata of a given
attribute or adding a new metadata element is considered a change even
if attribute value itself hasn't changed. Metadata elements cannot be
deleted once introduced: in order to delete metadata elements you have
to remove the entity attribute (using [updateContext
DELETE](#Adding_and_removing_attributes_with_APPEND_and_DELETE_in_updateContext "wikilink")),
then re-create it ([using updateContext
APPEND](#Adding_and_removing_attributes_with_APPEND_and_DELETE_in_updateContext "wikilink")).

You can use any name for your custom metadata except for the ones used
for some metadata names that are interpreted by Orion:

-   [ID](#Metadata_ID_for_attributes "wikilink")
-   [location](#Defining_location_attribute "wikilink")
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

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                            {
          <contextElement>                                                                                                                "type": "Room",
            <entityId type="Room" isPattern="false">                                                                                      "isPattern": "false",
              <id>Room1</id>                                                                                                              "id": "Room1",
            </entityId>                                                                                                                   "attributes": [
            <contextAttributeList>                                                                                                          {
              <contextAttribute>                                                                                                              "name": "temperature",
                <name>temperature</name>                                                                                                      "type": "float",
                <type>float</type>                                                                                                            "value": "23.5",
                <contextValue>23.5</contextValue>                                                                                             "metadatas": [
                <metadata>                                                                                                                      {
                   <contextMetadata>                                                                                                              "name": "ID",
                      <name>ID</name>                                                                                                             "type": "string",
                      <type>string</type>                                                                                                         "value": "ground"
                      <value>ground</value>                                                                                                     }
                   </contextMetadata>                                                                                                         ]
                </metadata>                                                                                                                 },
              </contextAttribute>                                                                                                           {
              <contextAttribute>                                                                                                              "name": "temperature",
                <name>temperature</name>                                                                                                      "type": "float",
                <type>float</type>                                                                                                            "value": "23.8",
                <contextValue>23.8</contextValue>                                                                                             "metadatas": [
                <metadata>                                                                                                                    {
                   <contextMetadata>                                                                                                            "name": "ID",
                      <name>ID</name>                                                                                                           "type": "string",
                      <type>string</type>                                                                                                       "value": "wall"
                      <value>wall</value>                                                                                                     }
                   </contextMetadata>                                                                                                         ]
                </metadata>                                                                                                                 }
              </contextAttribute>                                                                                                         ]
            </contextAttributeList>                                                                                                     }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "APPEND"
        <updateAction>APPEND</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
Now, we can query for temperature to get both instances:

      (curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format -) <<EOF       (curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                      {
      <queryContextRequest>                                                                                                         "entities": [
        <entityIdList>                                                                                                                {
          <entityId type="Room" isPattern="false">                                                                                      "type": "Room",
            <id>Room1</id>                                                                                                              "isPattern": "false",
          </entityId>                                                                                                                   "id": "Room1"
        </entityIdList>                                                                                                               }
        <attributeList>                                                                                                             ],
           <attribute>temperature</attribute>                                                                                       "attributes": [
        </attributeList>                                                                                                              "temperature"
      </queryContextRequest>                                                                                                        ]
      EOF                                                                                                                         }
                                                                                                                                  EOF
  
We can update an specific instance (e.g. ground), letting the other
untouched:

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                            {
          <contextElement>                                                                                                                "type": "Room",
            <entityId type="Room" isPattern="false">                                                                                      "isPattern": "false",
              <id>Room1</id>                                                                                                              "id": "Room1",
            </entityId>                                                                                                                   "attributes": [
            <contextAttributeList>                                                                                                        {
              <contextAttribute>                                                                                                            "name": "temperature",
                <name>temperature</name>                                                                                                    "type": "float",
                <type>float</type>                                                                                                          "value": "30",
                <contextValue>30</contextValue>                                                                                             "metadatas": [
                <metadata>                                                                                                                  {
                   <contextMetadata>                                                                                                          "name": "ID",
                      <name>ID</name>                                                                                                         "type": "string",
                      <type>string</type>                                                                                                     "value": "ground"
                      <value>ground</value>                                                                                                 }
                   </contextMetadata>                                                                                                       ]
                </metadata>                                                                                                               }
              </contextAttribute>                                                                                                         ]
            </contextAttributeList>                                                                                                     }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "UPDATE"
        <updateAction>UPDATE</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
 
Check it using again queryContext (ground has changed to 30ºC but wall
has its initial value of 23.8º C).

To avoid ambiguities, you cannot mix the same attribute with and without
ID. The following entity creation will fail:

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                            {
          <contextElement>                                                                                                                "type": "Room",
            <entityId type="Room" isPattern="false">                                                                                      "isPattern": "false",
              <id>Room2</id>                                                                                                              "id": "Room1",
            </entityId>                                                                                                                   "attributes": [
            <contextAttributeList>                                                                                                          {
              <contextAttribute>                                                                                                              "name": "temperature",
                <name>temperature</name>                                                                                                      "type": "float",
                <type>float</type>                                                                                                            "value": "23.5",
                <contextValue>23.5</contextValue>                                                                                             "metadatas": [
                <metadata>                                                                                                                      {
                   <contextMetadata>                                                                                                              "name": "ID",
                      <name>ID</name>                                                                                                             "type": "string",
                      <type>string</type>                                                                                                         "value": "ground"
                      <value>ground</value>                                                                                                     }
                   </contextMetadata>                                                                                                         ]
                </metadata>                                                                                                                 },
              </contextAttribute>                                                                                                           {
              <contextAttribute>                                                                                                              "name": "temperature",
                <name>temperature</name>                                                                                                      "type": "float",
                <type>float</type>                                                                                                            "value": "23.8"
                <contextValue>23.8</contextValue>                                                                                           }
              </contextAttribute>                                                                                                         ]
            </contextAttributeList>                                                                                                     }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "APPEND"
        <updateAction>APPEND</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       

       ...                                                                                                  ...
       <statusCode>                                                                                         "statusCode": {
         <code>472</code>                                                                                     "code": "472",
         <reasonPhrase>request parameter is invalid/not allowed</reasonPhrase>                                "details": "action: APPEND - entity: (Room1, Room) - offending attribute: temperature",
         <details>action: APPEND - entity: (Room1, Room) - offending attribute: temperature</details>         "reasonPhrase": "request parameter is invalid/not allowed"
       </statusCode>                                                                                        }
       ...                                                                                                  ...
  Finally, you can use also the following convenience operations with
attributes using ID metadata:

-   GET /v1/contextEntities/Room1/attributes/temperature/ground: to get
    an specific attribute identified by ID
-   PUT /v1/contextEntities/Room1/attributes/temperature/ground (using
    as payload updateContextElementRequest, as [described in a previous
    section](#Convenience_Update_Context "wikilink")).
-   DELETE /v1/contextEntities/Room1/attributes/temperature/ground: to
    remove an specific attribute identified by ID (see DELETE attribute
    semantics [described in a previous
    section](#Adding_and_removing_attributes_with_APPEND_and_DELETE_in_updateContext "wikilink")).