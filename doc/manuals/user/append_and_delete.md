# Adding and removing attributes and entities with APPEND and DELETE in updateContext

We have seen how to use updateContext with APPEND action type to [create
new entities](walkthrough_apiv1.md#entity-creation). In addition, APPEND can be
used to add a new attribute after entity creation. Let's illustrate this
with an example.

We start creating a simple entity 'E1' with one attribute named 'A':
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
```                                                                                                                  

Now, in order to append a new attribute (let's name it 'B') we use
updateContext APPEND with an entityId matching 'E1':

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "TA",
                    "value": "1"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
} 
EOF
``` 
Now we can check with a query to that entity that both attributes A and
B are there:

``` 
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool)<<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "B",
                "type": "TB",
                "value": "2"
            },
            {
                "name": "A",
                "type": "TA",
                "value": "1"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
``` 


APPEND is interpreted as UPDATE in existing context elements. However, you can use APPEND_STRICT instead of APPEND as updateAction. In that case,
existing attributes are not updated and an error is reported in that case. Note that if your APPEND_STRICT request includes several attributes
(e.g. A and B), some of them existing and some other not existing (e.g. A exists and B doesn't exist) the ones that doesn't exist are added (in
this case, B is added) and an error is reported for the existing ones (in this case, an error is reported about A already exists).


We can also remove attributes in a similar way, using the DELETE action
type. For example, to remove attribute 'A' we will use (note the empty
contextValue element):

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "TA",
                    "value": ""
                }
            ]
        }
    ],
    "updateAction": "DELETE"
} 
EOF
```

Now, a query to the entity shows attribute B:

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "B",
                "type": "TB",
                "value": "2"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
```
        
  
You can also use convenience operations with POST and DELETE verbs to
add and delete attributes. Try the following:

Add a new attribute 'C' and 'D':

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "attributes": [
        {
            "name": "C",
            "type": "TC",
            "value": "3"
        },
        {
            "name": "D",
            "type": "TD",
            "value": "4"
        }
    ]
} 
EOF
```

Remove attribute 'B':
```
curl localhost:1026/v1/contextEntities/E1/attribute/B -s -S \
    --header 'Content-Type: application/json'  -X DELETE  \
    --header 'Accept: application/json'  | python -mjson.tool
```
Query entity (should see 'C' and 'D', but not 'B'):

```
(curl localhost:1026/v1/contextEntities/E1 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool) <<EOF
{
    "contextElement": {
        "attributes": [
            {
                "name": "C",
                "type": "TC",
                "value": "3"
            },
            {
                "name": "D",
                "type": "TD",
                "value": "4"
            }
        ],
        "id": "E1",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
EOF
``` 

To delete an entire entity, please check [this document](delete_entity.md).
