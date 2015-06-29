# Adding and removing attributes and entities with APPEND and DELETE in updateContext

We have seen how to use updateContext with APPEND action type to [create
new entities](#Entity_Creation "wikilink"). In addition, APPEND can be
used to add a new attribute after entity creation. Let's illustrate this
with an example.

We start creating a simple entity 'E1' with one attribute named 'A':

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                            {
          <contextElement>                                                                                                                "type": "T",
            <entityId type="T" isPattern="false">                                                                                         "isPattern": "false",
              <id>E1</id>                                                                                                                 "id": "E1",
            </entityId>                                                                                                                   "attributes": [
            <contextAttributeList>                                                                                                        {
              <contextAttribute>                                                                                                            "name": "A",
                <name>A</name>                                                                                                              "type": "TA",
                <type>TA</type>                                                                                                             "value": "1"
                <contextValue>1</contextValue>                                                                                            }
              </contextAttribute>                                                                                                         ]
            </contextAttributeList>                                                                                                     }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "APPEND"
        <updateAction>APPEND</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
Now, in order to append a new attribute (let's name it 'B') we use
updateContext APPEND with an entityId matching 'E1':

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                            {
          <contextElement>                                                                                                                "type": "T",
            <entityId type="T" isPattern="false">                                                                                         "isPattern": "false",
              <id>E1</id>                                                                                                                 "id": "E1",
            </entityId>                                                                                                                   "attributes": [
            <contextAttributeList>                                                                                                        {
              <contextAttribute>                                                                                                            "name": "B",
                <name>B</name>                                                                                                              "type": "TB",
                <type>TB</type>                                                                                                             "value": "2"
                <contextValue>2</contextValue>                                                                                            }
              </contextAttribute>                                                                                                         ]
            </contextAttributeList>                                                                                                     }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "APPEND"
        <updateAction>APPEND</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
Now we can check with a query to that entity that both attributes A and
B are there:

      curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/xml' | xmllint --format -       curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool

      <?xml version="1.0"?>                          {
      <contextElementResponse>                           "contextElement": {
        <contextElement>                                     "attributes": [
          <entityId type="" isPattern="false">                   {
            <id>E1</id>                                              "name": "B",
          </entityId>                                                "type": "TB",
          <contextAttributeList>                                     "value": "2"
            <contextAttribute>                                   },
              <name>A</name>                                     {
              <type>TA</type>                                        "name": "A",
              <contextValue>1</contextValue>                         "type": "TA",
            </contextAttribute>                                      "value": "1"
            <contextAttribute>                                   }
              <name>B</name>                                 ],
              <type>TB</type>                                "id": "E1",
              <contextValue>2</contextValue>                 "isPattern": "false",
            </contextAttribute>                              "type": ""
          </contextAttributeList>                        },
        </contextElement>                                "statusCode": {
        <statusCode>                                         "code": "200",
          <code>200</code>                                   "reasonPhrase": "OK"
          <reasonPhrase>OK</reasonPhrase>                }
        </statusCode>                                }
      </contextElementResponse>                  
  
We can also remove attributes in a similar way, using the DELETE action
type. For example, to remove attribute 'A' we will use (note the empty
contextValue element):

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                            {
          <contextElement>                                                                                                                "type": "T",
            <entityId type="T" isPattern="false">                                                                                         "isPattern": "false",
              <id>E1</id>                                                                                                                 "id": "E1",
            </entityId>                                                                                                                   "attributes": [
            <contextAttributeList>                                                                                                        {
              <contextAttribute>                                                                                                            "name": "A",
                <name>A</name>                                                                                                              "type": "TA",
                <type>TA</type>                                                                                                             "value": ""
                <contextValue/>                                                                                                           }
              </contextAttribute>                                                                                                         ]
            </contextAttributeList>                                                                                                     }
          </contextElement>                                                                                                           ],
        </contextElementList>                                                                                                         "updateAction": "DELETE"
        <updateAction>DELETE</updateAction>                                                                                         }
      </updateContextRequest>                                                                                                       EOF
      EOF                                                                                                                       
  
Now, a query to the entity shows attribute B:

      curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/xml' | xmllint --format -       curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool

      <?xml version="1.0"?>                          {
      <contextElementResponse>                           "contextElement": {
        <contextElement>                                     "attributes": [
          <entityId type="" isPattern="false">                   {
            <id>E1</id>                                              "name": "B",
          </entityId>                                                "type": "TB",
          <contextAttributeList>                                     "value": "2"
            <contextAttribute>                                   }
              <name>B</name>                                 ],
              <type>TB</type>                                "id": "E1",
              <contextValue>2</contextValue>                 "isPattern": "false",
            </contextAttribute>                              "type": ""
          </contextAttributeList>                        },
        </contextElement>                                "statusCode": {
        <statusCode>                                         "code": "200",
          <code>200</code>                                   "reasonPhrase": "OK"
          <reasonPhrase>OK</reasonPhrase>                }
        </statusCode>                                }
      </contextElementResponse>                  
  
You can also use convenience operations with POST and DELETE verbs to
add and delete attributes. Try the following:

Add a new attribute 'C' and 'D':

      (curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/xml' -X POST -d @- | xmllint --format - ) << EOF       (curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                                      {
      <appendContextElementRequest>                                                                                                                 "attributes" : [
        <contextAttributeList>                                                                                                                        {
          <contextAttribute>                                                                                                                            "name" : "C",
            <name>C</name>                                                                                                                              "type" : "TC",
            <type>TC</type>                                                                                                                             "value" : "3"
            <contextValue>3</contextValue>                                                                                                            },
          </contextAttribute>                                                                                                                         {
          <contextAttribute>                                                                                                                            "name" : "D",
            <name>D</name>                                                                                                                              "type" : "TD",
            <type>TD</type>                                                                                                                             "value" : "4"
            <contextValue>4</contextValue>                                                                                                            }
          </contextAttribute>                                                                                                                       ]
        </contextAttributeList>                                                                                                                   }
      </appendContextElementRequest>                                                                                                              EOF
      EOF                                                                                                                                     
  
Remove attribute 'B':

      curl localhost:1026/v1/contextEntities/E1/attributes/B -s -S --header 'Content-Type: application/xml' -X DELETE | xmllint --format -       curl localhost:1026/v1/contextEntities/E1/attribute/B -s -S --header 'Content-Type: application/json' -X DELETE --header 'Accept: application/json' | python -mjson.tool

Query entity (should see 'C' and 'D', but not 'B'):
  
      curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/xml' | xmllint --format -       curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool

      <?xml version="1.0"?>                          {
      <contextElementResponse>                           "contextElement": {
        <contextElement>                                     "attributes": [
          <entityId type="" isPattern="false">                   {
            <id>E1</id>                                              "name": "C",
          </entityId>                                                "type": "TC",
          <contextAttributeList>                                     "value": "3"
            <contextAttribute>                                   },
              <name>C</name>                                     {
              <type>TC</type>                                        "name": "D",
              <contextValue>3</contextValue>                         "type": "TD",
            </contextAttribute>                                      "value": "4"
            <contextAttribute>                                   }
              <name>D</name>                                 ],
              <type>TD</type>                                "id": "E1",
              <contextValue>4</contextValue>                 "isPattern": "false",
            </contextAttribute>                              "type": ""
          </contextAttributeList>                        },
        </contextElement>                                "statusCode": {
        <statusCode>                                         "code": "200",
          <code>200</code>                                   "reasonPhrase": "OK"
          <reasonPhrase>OK</reasonPhrase>                }
        </statusCode>                                }
      </contextElementResponse>                  
  
# Deleting entities

Apart from deleting individual attributes from a given entity (see
[previous section on that
topic](#Adding_and_removing_attributes_with_APPEND_and_DELETE_in_updateContext "wikilink")),
you can also delete an entire entity, including all its attributes with
their corresponding metadata. In order to do so, the updateContext
operation is used, with DELETE as actionType and with an empty
attributeList, as in the following example:

      (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0" encoding="UTF-8"?>                                                                                        {
      <updateContextRequest>                                                                                                          "contextElements": [
        <contextElementList>                                                                                                           {
          <contextElement>                                                                                                                "type": "T",
            <entityId type="T" isPattern="false">                                                                                         "isPattern": "false",
              <id>E1</id>                                                                                                                 "id": "E1"
            </entityId>                                                                                                                 }
            <contextAttributeList/>                                                                                                   ],
          </contextElement>                                                                                                           "updateAction": "DELETE"
        </contextElementList>                                                                                                       }
        <updateAction>DELETE</updateAction>                                                                                         EOF
      </updateContextRequest>                                                                                                   
      EOF                                                                                                                       
 
You can also use the following equivalent convenience operation:

      curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/xml' -X DELETE       curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -X DELETE


