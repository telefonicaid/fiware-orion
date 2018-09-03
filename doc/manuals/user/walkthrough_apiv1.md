# <a name="top"></a>FIWARE NGSI APIv1 Walkthrough

* [Introduction](#introduction)
* [Before starting...](#before-starting)
    * [Example Case](#example-case)
    * [Starting the broker for the tutorials](#starting-the-broker-for-the-tutorials)
    * [Starting accumulator server](#starting-accumulator-server)
    * [Issuing commands to the broker](#issuing-commands-to-the-broker)
* [Context management using NGSI10](#context-management-using-ngsi10)
    * [NGSI10 standard operations](#ngsi10-standard-operations)
	    * [Entity Creation](#entity-creation)
	    * [Query Context operation](#query-context-operation)
	    * [Update Context elements](#update-context-elements)
            * [Context Subscriptions](#context-subscriptions)
	    * [Summary of NGSI10 standard operations URLs](#summary-of-ngsi10-standard-operations-urls)
    * [NGSI10 convenience operations](#ngsi10-convenience-operations)
	    * [Convenience Entity Creation](#convenience-entity-creation)
	    * [Convenience Query Context](#convenience-query-context)
	    * [Getting all entities](#getting-all-entities)
	    * [Browsing all types and detailed information on a type](#browsing-all-types-and-detailed-information-on-a-type)
	    * [Convenience Update Context](#convenience-update-context)
	    * [Convenience operations for context subscriptions](#convenience-operations-for-context-subscriptions)
	    * [Summary of NGSI10 convenience operations URLs](#summary-of-ngsi10-convenience-operations-urls)
* [Context availability management using NGSI9](#context-availability-management-using-ngsi9)
    * [NGSI9 standard operations](#ngsi9-standard-operations)
	    * [Register Context operation](#register-context-operation)
	    * [Discover Context Availability operation](#discover-context-availability-operation)
	    * [Context availability subscriptions](#context-availability-subscriptions)
	    * [Summary of NGSI9 standard operations URLs](#summary-of-ngsi9-standard-operations-urls)
    * [NGSI9 convenience operations](#ngsi9-convenience-operations)
	    * [Convenience Register Context](#convenience-register-context)
	    * [Only-type entity registrations using convenience operations](#only-type-entity-registrations-using-convenience-operations)
	    * [Convenience Discover Context Availability](#convenience-discover-context-availability)
	    * [Convenience operations for context availability subscriptions](#convenience-operations-for-context-availability-subscriptions)
	    * [Summary of NGSI9 convenience operations URLs](#summary-of-ngsi9-convenience-operations-urls) 

## Introduction

Note that there is also an [NGSIv2 version of this walkthrough](walkthrough_apiv2.md). In general, you should use NGSIv2
(i.e. the other document), except if you need context management availability functionality (aka NGSI9), not
yet developed in NGSIv2. In the case of doubt, you should use NGSIv2.

This walkthrough adopts a practical approach that we hope will help our
readers to get familiar with the Orion Context Broker and have some fun
in the process :).

The walkthrough can be also found (partially) in Apiary format [here](http://telefonicaid.github.io/fiware-orion/api/v1/).

The first two sections on [Context management using
NGSI10](#context-management-using-ngsi10) and [Context
availability management using
NGSI9](#context-availability-management-using-ngsi9) are the
main ones. They describe the basic context broker functionality, both
for context management (information about entities, such as the
temperature of a car) and context availability management (information
not about the entities themselves, but about the providers of that
information). Some remarks to take into account in order to use this stuff:

-   Context management and context availability management are
    independent functionalities (corresponding to different parts of the
    NGSI interface, NGS10 and NGSI9 respectively), so you can use the
    broker for one purpose, the other, or both of them.
-   Note that each main section is divided in two sub-sections: the
    first being about standard operations while the second is about
    convenience operations. In fact, each sub-section is an independent
    tutorial (for a total of 4 tutorials summing up both sections) that
    can be done in a step-by-step manner, just copy-pasting the commands
    from this document.
-   Before starting (or if you get lost in the middle and
    need to start from scratch :) ), restart Orion Context Broker as
    described in [starting the broker for the
    tutorials](#starting-the-broker-for-the_tutorials).
-   It is recommended to start with the part on standard operations,
    and then do the part on convenience operations (some
    explanations and concepts described in the former are needed for
    the latter).

It is recommended to get familiar with the theoretical concepts on which
the NGSI model is based before starting. E.g. entities, attributes, etc.
Have a look at the FIWARE documentation about this, e.g. [this public
presentation](http://bit.ly/fiware-orion).

[Top](#top)

## Before starting...

Before starting, let's introduce the example case that is used in the
tutorials and how to run and interact with Orion Context Broker.

[Top](#top)

#### Example Case

Let's assume we have a building with several rooms and that we want to
use Orion Context Broker to manage its context information. The rooms
are Room1, Room2, Room3 and Room4 and each room has two sensors:
temperature and (atmospheric) pressure (except Room4, which only has a
pressure sensor). In addition, let's consider that we have two cars
(Car1 and Car2) with sensors able to measure speed and location (in GPS
sense).

![](Orion-example-case.png "Orion-example-case.png")

Most of the time we will use Room1 and Room2 in the tutorials. Room3,
Room4, Car1 and Car2 will be used only in the section regarding [context
availability
subscriptions](#context-availability-subscriptions).

The Orion Context Broker interacts with context producer applications
(which provide sensor information) and a context consumer application
(which processes that information, e.g. to show it in a graphical user
interface). We will play the role of both kinds of applications in the
tutorials.

[Top](#top)

### Starting the broker for the tutorials

Before starting, you need to install the broker as described in the
[Installation and Administration Guide](../admin/install.md).

The tutorials assume that you don't have any previous content in the
Orion Context Broker database. In order to do so, follow the [delete
database
procedure](../admin/database_admin.md#delete-complete-database).

To start the broker (as root or using the sudo command):

```
/etc/init.d/contextBroker start
```

To restart the broker (as root or using the sudo command):

```
/etc/init.d/contextBroker restart
```

[Top](#top)

### Starting accumulator server

Some parts of the tutorial (the ones related with subscriptions and
notifications) require some process to play the role of the consumer
application able to receive notifications. To that end, download the
accumulator script, available [at
GitHub](https://github.com/telefonicaid/fiware-orion/blob/master/scripts/accumulator-server.py).
It is a very simple "dummy" application that just listens to a given URL
(the example below uses localhost:1028/accumulate, but a different
host and/or port can be specified) and echoes whatever it receives in the
terminal window where it is executed. Run it using the following
command:

```
# cd /dir/where/accumulator-server/is/downloaded
# chmod a+x accumulator-server.py
# ./accumulator-server.py --port 1028 --url /accumulate --host ::1 -v
```

[Top](#top)

### Issuing commands to the broker

To issue requests to the broker, we use the `curl` command line tool.
We have chosen `curl` because it is almost ubiquitous in any GNU/Linux
system and simplifies including examples in this document that can
easily be copied and pasted. Of course, it is not mandatory to use it,
you can use any REST client tool instead (e.g.
[RESTClient](http://restclient.net/)). Indeed, in a real case, you will
probably interact with the Orion Context Broker using a programming
language library implementing the REST client part of your application.

The basic patterns for all the curl examples in this document are the
following:

-   For POST:

```
curl localhost:1026/<operation_url> -s -S [headers]' -d @- <<EOF
[payload]
EOF
```

-   For PUT:

```
curl localhost:1026/<operation_url> -s -S [headers] -X PUT -d @- <<EOF
[payload]
EOF
```

-   For GET:

```
curl localhost:1026/<operation_url> -s -S [headers]
```

-   For DELETE:

```
curl localhost:1026/<operation_url> -s -S [headers] -X DELETE
```

Regarding \[headers\] you have to include the following ones:

-   Accept header to specify which payload format
    you want to receive in the response. You should explicitly specify JSON.

```
curl ... --header 'Accept: application/json' ...
```

-   Only in the case of using payload in the request (i.e. POST or PUT),
    you have to use Context-Type header to specify the format (JSON).

```
curl ... --header 'Content-Type: application/json' ...
```

Some additional remarks:

-   We are using multi-line shell commands to provide the input to curl,
    using EOF to mark the beginning and the end of the multi-line block
    (*here-documents*). In
    some cases (GET and DELETE) we omit "-d @-" as they don't
    use payload.

-   In our examples we assume that the broker is listening on port 1026.
    Adjust this in the curl command line if you are using a
    different setting.

-   In order to pretty-print JSON in responses, you can use Python with
    msjon.tool (examples along with tutorial are using this style):

```
(curl ... | python -mjson.tool) <<EOF
...
EOF
```

-   Check that curl is installed in your system using:

```
which curl
```

[Top](#top)

## Context management using NGSI10

### NGSI10 standard operations

This section describes the different standard NGSI10 operations that the
Orion Context Broker supports, showing examples of requests and
responses. We use the term "standard" as they are directly derived from
the OMA NGSI specification, to distinguish them from the other family of
operations ("convenience") which has been defined by the FIWARE project
to ease the usage of NGSI implementations.

**Don't forget to restart the broker before starting this tutorial as
described [previously in this
document](#starting-the-broker-for-the-tutorials)**

At the end of this section, you will have the basic knowledge to create
applications (both context producers and consumers) using Orion Context
Broker with NGSI10 standard operations:

-   updateContext
-   queryContext
-   subscribeContext
-   updateContextSubscription
-   unsubscribeContext

[Top](#top)

#### Entity Creation

Orion Context Broker will start in an empty state, so first of all we
need to make it aware of the existence of certain entities. In
particular, we are going to "create" Room1 and Room2 entities, each one
with two attributes (temperature and pressure). We do this using the
updateContext operation with APPEND action type (the other main action
type, UPDATE, [will be discussed in a next
section](#update-context-elements)).

First, we are going to create Room1. Let's assume that at entity
creation time temperature and pressure of Room1 are 23 ºC and 720 mmHg
respectively.

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "23"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "720"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
} 
EOF
```

The updateContext request payload contains a list of contextElement
elements. Each contextElement is associated to an entity, whose
identification is provided by the `id`, `type` and `isPattern` fields (in this case
the identification for Room1 is provided) and contains a list of attributes. 
Each element in the attributes list provides the value for a given attribute 
(identified by name) of the entity. 
Apart from the list of contextElement elements, the payload includes
also an updateAction element. We use APPEND, which means that we want 
to add new information.

Orion Context Broker doesn't perform any checking on types (e.g. it doesn't
check that when a context producer application updates the value of the
temperature, this value is formatted as a float like "25.5" or "-40.23"
and not something like "hot").

Upon receipt of this request, the broker will create the entity in its
internal database, set the values for its attributes and will response
with the following:

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

As you can see, it follows the same structure as the request, just to
acknowledge that the request was correctly processed for these context
elements. You probably wonder why contextValue elements are empty in
this case, but actually you don't need the values in the response
because you were the one to provide them in the request.

Next, let's create Room2 in a similar way (in this case, setting
temperature and pressure to 21 ºC and 711 mmHg respectively).

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool ) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "21"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "711"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```

The response to this request is:

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Apart from simple values (i.e. strings) for attribute values, you can
also use complex structures or custom metadata. These are advance
topics, described in [this
section](structured_attribute_valued.md#structured-attribute-values ) and [this
other](metadata.md#custom-attribute-metadata ), respectively.

[Top](#top)

#### Query Context operation

Now let's play the role of a consumer application, wanting to access the
context information stored by Orion Context Broker to do something
interesting with it (e.g. show a graph with the room temperature in a
graphical user interface). The NGSI10 queryContext request is used in
this case, e.g. to get context information for Room1:

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ]
} 
EOF
```

The response includes all the attributes belonging to Room1 and we can
check that temperature and pressure have the values that we set at
entity creation with updateContext (23ºC and 720 mmHg).

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "720"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

If you use an empty attributes element in the request, the response will include all the attributes of the entity. If you include an actual list of attributes (e.g. temperature) only that are retrieved, as shown in the following request:

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ]
} 
EOF
```

which response is as follows:

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Moreover, a powerful feature of Orion Context Broker is that you can use
a regular expression for the entity ID. For example, you can query
entities which ID starts with "Room" using the regex "Room.\*". In this
case, you have to set isPattern to "true" as shown below:

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        },
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2"
        }
    ],
    "attributes": [
        "temperature"
    ]
} 
EOF
```
```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "true",
            "id": "Room.*"
        }
    ],
    "attributes": [
        "temperature"
    ]
} 
EOF
```
  

Both produce the same response:

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "21"
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Finally, note that you will get an error in case you try to query a
non-existing entity or attribute, as shown in the following cases below:

```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room5"
        }
    ]
} 
EOF
```
```
(curl localhost:1026/v1/queryContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "humidity"
    ]
} 
EOF
```

Both requests will produce the same error response:

```
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context elements found"
    }
}
```

Additional comments:

-   You can also use geographical scopes in your queries. This is an
    advance topic, described in [this
    section](geolocation.md#geolocation-capabilities).
-   Note that by default only 20 entities are returned (which is fine
    for this tutorial, but probably not for a real
    utilization scenario). In order to change this behaviour, see [the
    section on pagination](pagination.md#pagination ) in this manual.
-   In the case of JSON
    responses, you can use the *?attributeFormat=object* URI parameter
    to get attributes as a JSON object (i.e. key-values map) instead of
    a vector (default behaviour):

```
(curl 'localhost:1026/v1/queryContext?attributeFormat=object' -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ]
}
EOF
```

[Top](#top)

#### Update context elements

You can update the value of entities attributes using the updateContext
operation with UPDATE action type. The basic rule to take into account
with updateContext is that APPEND creates new context elements, while
UPDATE updates already existing context elements (however, Orion
interprets APPEND as UPDATE if the entity already
exists; you can avoid that using [APPEND_STRICT](update_action_types.md#append_strict)).

Now we will play the role of a context producer application, i.e. a
source of context information. Let's assume that this application in a
given moment wants to set the temperature and pressure of Room1 to 26.5
ºC and 763 mmHg respectively, so it issues the following request:

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
     --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "26.5"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "763"
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
} 
EOF
```

As you can see, the structure of the request is exactly the same we used
for [updateContext with APPEND for creating
entities](#entity-creation), except we use UPDATE now as
action type.

Upon receipt of this request, the broker will update the values for the
entity attributes in its internal database and will response with the
following:


```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
      
```

Again, the structure of the response is exactly the same one we used for
[updateContext with APPEND for creating
entities](#entity-creation ).

The updateContext operation is quite flexible as it allows you to update
as many entities and attributes as you want: it is just a matter of
which contextElements you include in the list. You could even update the
whole database of Orion Context Broker (maybe including thousands of
entities/attributes) in just one updateContext operation (at least in
theory).

To illustrate this flexibility, we will show how to update Room2 in two
separated updateContext request (setting its temperature to 27.4 ºC and
its pressure to 755 mmHg), each one targeting just one attribute. This
also illustrates that you don't need to include all the attributes of an
entity in the updateContext, just the ones you want to update (the other
attributes maintain their current value).

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2",
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": "27.4"
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
} 
EOF
```
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2",
            "attributes": [
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": "755"
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
} 
EOF 
```
  
The responses for these requests are respectively:

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": ""
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```
  
```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": ""
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Now, you can use queryContext operation [as previously
described](#query-context-operation) to check that Room1 and
Room2 attributes has been actually updated.

Apart from simple values (i.e. strings) for attribute values, you can
also use complex structures. This is an advance topic, described in
[this section](structured_attribute_valued.md#structured-attribute-values).

Apart from APPEND or UPDATE there are additional possibilities for the
`actionType` field, e.g. REPLACE to replace entity attributes (if your
entity has the attributes A and B and you send an updateContext REPLACE
request with A, then the entity at the end will have only A, i.e., the attribute B
has been removed). Have a look at [the section about action types](update_action_types.md)
for the complete list.

[Top](#top)

#### Context subscriptions

The NGSI10 operations you know up to now (updateContext and
queryContext) are the basic building blocks for synchronous context
producer and context consumer applications. However, Orion Context
Broker has another powerful feature that you can take advantage of: the
ability to subscribe to context information so when "something" happens
(we will explain the different cases for that "something") your
application will get an asynchronous notification. In that way, you
don't need to continuously repeat queryContext requests (i.e. polling),
the Orion Context Broker will let you know the information when it
comes.

Before starting to play with feature, [start the accumulator
server](#starting-accumulator-server-for-the-tutorials) to
capture notifications.

Subscriptions are used when you want to be notified when some
attribute changes. Let's consider the following example:

```
(curl localhost:1026/v1/subscribeContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "pressure"
            ]
        }
    ],
    "throttling": "PT5S"
}
EOF
```

Let's examine in detail the different elements included in the payload:

-   entities and attributes define which context elements will be included
    in the notification message.
    In this example, we are specifying that the notification has to include
    the temperature attribute for entity Room1.
-   The callback URL to send notifications is defined with the
    reference element. We are using the URL of the accumulator-server.py
    program started before. Only one reference can be included per
    subscribeContext request. However, you can have several
    subscriptions on the same context elements (i.e. same entity
    and attribute) without any problem. Default URL schema (in the
    case you don't specify any) is "http", e.g. using "localhost:1028"
    as reference will be actually interpreted as
    "<http://localhost:1028>".
-   Subscriptions have a duration, specified using the [ISO
    8601](http://www.wikipedia.org/wiki/ISO_8601) standard format. Once
    that duration is expired, the subscription is simply ignored
    (however, it is still stored in the broker database and needs to be
    purged using the procedure described in the [administration
    manual](../admin/database_admin.md#deleting-expired-documents)).
    You can extend the duration of a subscription by updating it, as
    described [later in this document](duration.md#extending-duration).
    We are using "P1M" which means "one month".
-   The notifyCondition element defines the "trigger" for
    the subscription. It uses the type ONCHANGE. The condValues vector contains a list of
    attribute names. They define the
    "triggering attributes", i.e. attributes that upon creation/change
    due to [Entity Creation](#entity-creation) or
    [Update context elements](#update-context-elements) trigger
    the notification. The rule is that if at least one of the attributes
    in the list changes (e.g. some kind of "OR" condition), then a
    notification is sent. But note that a notification includes the
    attributes in the attribute vector, which doesn't necessarily
    include any attribute in the condValue. For example, in this case,
    when Room1 pressure changes, the Room1 temperature value is notified,
    but not pressure itself. If you want also pressure to be notified,
    the request would need to include
    "pressure" within the attribute vector
    (or to use an empty attribute vector, which you already know means "all
    the attributes in the entity"). Now, this example here, to be
    notified of the value of *temperature* each time the value of
    *pressure* changes may not be too useful. The example is chosen this
    way only to show the enormous flexibility of subscriptions.
-   You can leave the condValue list empty (or even omit it) to make a notification
    trigger on any entity attribute change (regardless of the name of the attribute).
-   The throttling element is used to specify a minimum
    inter-notification arrival time. So, setting throttling to 5 seconds
    as in the example above, makes a notification not to be sent
    if a previous notification was sent less than 5 seconds ago, no
    matter how many actual changes take place in that period. This is to give the 
    notification receptor a means to protect itself against context producers
    that update attribute values too frequently. In multi-CB configurations, take
    into account that the last-notification measure is local to each CB node. Although
    each node periodically synchronizes with DB in order to get potentially newer
    values (more on this [here](../admin/perf_tuning.md#subscription-cache)) it may happen that
    a particular node has an old value, so throttling is not 100% accurate.

The response corresponding to that request contains a subscription ID (a
24 hexadecimal number used for updating and cancelling the subscription
-Write it down because you will need it later in this tutorial) and a
duration/throttling acknowledgement:

```
{
    "subscribeResponse": {
        "duration": "P1M",
        "subscriptionId": "51c0ac9ed714fb3b37d7d5a8"
    }
}
```

Let's have a look now at accumulator-server.py. We will see one (and
just one by the moment, no matter how much you wait)
notifyContextRequest, similar to this one:

```
POST http://localhost:1028/accumulate
Content-Length: 492
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8",
    "originator": "localhost",
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "26.5"
                    }
                ],
                "type": "Room",
                "isPattern": "false",
                "id": "Room1"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Orion Context Broker notifies NGSI10 subscribeContext using the POST
HTTP method (on the URL used as reference for the subscription) with a
notifyContextRequest payload. Apart from the subscriptionId element
(that matches the one in the response to subscribeContext request) and
the originator element, there is a contextResponses vector which is
the same that the one used in the [queryContext
responses](#query-context-operation).

Currently, the originator is always "localhost". We will look into a
more flexible way of using this in a later version.

You may wonder why accumulator-server.py is getting this message if you
don't actually do any update. This is due to the *initial notification*,
which details are described [here](initial_notification.md).

Now, do the following exercise, based on what you know from [update
context](#update-context-elements): Do the following 4
updates in sequence, letting pass more than 5 seconds between one
and the next (to avoid losing notifications due to throttling):

-   update Room1 temperature to 27: nothing happens, as temperature is
    not the triggering attribute
-   update Room1 pressure to 765: you will get a notification with the
    current value of Room1 temperature (27)
-   update Room1 pressure to 765: nothing happens, as the broker is
    clever enough to know that the value previous to the updateContext
    request was also 765 so no actual update has occurred and
    consequently no notification is sent.
-   update Room2 pressure to 740: nothing happens, as the subscription
    is for Room1, not Room2.

Next, try to check how throttling is enforced. Update Room1 pressure
fast, without letting pass 5 seconds and you will see that the second
notification doesn't arrive to accumulator-server.py.

Subscriptions can be updated using the NGSI10 updateContextSubcription.
The request includes a subscriptionId that identifies the subscription
to modify and the actual update payload. For example, if we want to
change the duration interval to extend duration
(of course, replace the subscriptionId value after copy-paste with the
one that you have got in the subscribeContext response in the previous
step) command:

```
(curl localhost:1026/v1/updateContextSubscription -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8",
    "duration": "P2M"
}
EOF
```

The response is very similar to the one for subscribeContext request:

```
{
    "subscribeResponse": {
        "subscriptionId": "51c0ac9ed714fb3b37d7d5a8",
        "duration": "P2M"
    }
}
```

Finally, you can cancel a subscription using the NGSI10
unsubscribeContext operation, that just uses de subscriptionId in the
request payload (replace the subscriptionId value after copy-paste with
the one that you get in the subscribeContext response in the previous
step):

```
(curl localhost:1026/v1/unsubscribeContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8"
}
EOF
```

The response is just an acknowledgement of that the cancellation was
successful.

```
{
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    },
    "subscriptionId": "51c0ac9ed714fb3b37d7d5a8"
}
```

You can do some more updates and look at accumulator-server.py to check that the
notification flow has stopped.

[Top](#top)

#### Summary of NGSI10 standard operations URLs

Each standard operation has a unique URL. All of them use the POST
method. The summary is below:

-   <host:port>/v1/updateContext
-   <host:port>/v1/queryContext
-   <host:port>/v1/subscribeContext
-   <host:port>/v1/updateContextSubscription
-   <host:port>/v1/unsubscribeContext

[Top](#top)

### NGSI10 convenience operations

This section describes the different convenience operations described as
part of the FIWARE NGSI REST API NGSI10 that Orion Context Broker
supports, showing examples of requests and responses. Convenience
operations are a set of operations that have been defined by FIWARE
project to ease the usage of NGSI implementations as a complement to the
standard operations defined in the OMA NGSI specification.

**Don't forget to restart the broker before starting this tutorial as
described [previously in this
document](#starting-the-broker-for-the-tutorials)**.

At the end of this section, you will have learnt to use convenience
operations as a handy alternative to some standard operations described
in [the previous
section](#tutorial-on-ngsi10-standard-operations). It is
highly recommended to do that tutorial before, to get familiar with
update and query context, etc. and to be able to compare between the two
approaches.

[Top](#top)

#### Convenience Entity Creation

Orion Context Broker will start in an empty state, so first of all we
need to make it aware of the existence of certain entities. Thus, let's
first create Room1 entity with temperature and pressure attributes (with
its initial values)

```
(curl localhost:1026/v1/contextEntities/Room1 -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF

{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "23"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "720"
        }
    ]
} 
EOF
```

the response is:

```
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ],
    "id": "Room1",
    "isPattern": "false",
    "type": ""
}
```

Now, let's do the same with Room2:

```
(curl localhost:1026/v1/contextEntities/Room2 -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "21"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "711"
        }
    ]
}
EOF
```

which response is:

```
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ],
    "id": "Room2",
    "isPattern": "false",
    "type": ""
}
```

You can also create an attribute (and the containing entity along the
way) in the following way (additional attributes could be added after
that, as described in [this
section](update_action_types.md#append)):

```
(curl localhost:1026/v1/contextEntities/Room3/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -X POST -d @- | python -mjson.tool) <<EOF
{
    "value" : "21"
} 
EOF
```

Compared to [entity creation based on standard
operation](#entity_creation) we observe the following
differences:

-   We are using the POST verb on the /v1/contextEntities/{EntityID}
    resource to create new entities
-   We cannot create more than one entity at a time using convenience
    operation requests.
-   The payload of requests and responses in convenience operations are
    very similar to the ones used in standard operations, since
    contextAttribute and contextResponse  elements are the same.
-   You can replace
    "Room1" by "/type/Room/id/Room1" in the URl to define the type (in
    general: "/type/<type>/id/<id>").

As alterative, you can use "POST /v1/contextEntitites" to create entities. In this case, the
entity information (ID and type) is included in the payload and the URL
is independent of that fields, as shown below:

```
(curl localhost:1026/v1/contextEntities -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -X POST -d @- | python -mjson.tool) <<EOF
{
    "id": "Room1",
    "type": "Room",
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "23"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "720"
        }
    ]
}
EOF
```

Apart from simple values (i.e. strings) for attribute values, you can
also use complex structures or custom metadata. These are advance
topics, described in [this
section](Structured_attribute_valued.md#structured-attribute-values) and [this
other](metadata.md#custom-attribute-metadata "), respectively.

[Top](#top)

#### Convenience Query Context

Finally, let's describe convenience operations for querying context
information. We can query all the attribute values of a given entity,
e.g. Room1 attributes:

```
curl localhost:1026/v1/contextEntities/Room1 -s -S 
    --header 'Accept: application/json' | python -mjson.tool
```
which response is:
```
{
    "contextElement": {
        "attributes": [
            {
                "name": "temperature",
                "type": "float",
                "value": "23"
            },
            {
                "name": "pressure",
                "type": "integer",
                "value": "720"
            }
        ],
        "id": "Room1",
        "isPattern": "false",
        "type": "Room"
    },
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
```
We can also query a single attribute of a given entity, e.g. Room2
temperature:

```
curl localhost:1026/v1/contextEntities/Room2/attributes/temperature -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

which response is:

```
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "21"
        }
    ],
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
```

Comparing to [standard queryContext
operation](#query-context-operation) we observe the following
differences:

-   Convenience operations use the GET method without payload in the
    request (simpler than standard operation)
-   The response contextElementResponse element used in the response of
    the convenience operation to query all the attributes of an entity
    has the same structure as the one that appears inside the responses
    for standard queryContext. However, the contextAttributeResponse
    element in the response of the convenience operation used as
    response to the query of a single attribute of an entity is new.
-   You can replace
    "Room1" by "/type/Room/id/Room1" in the URl to define the type (in
    general: "/type/<type>/id/<id>").

You can also query by all the entities belonging to the same type, either 
all the attributes or a particular one, as shown below. First, create an 
couple of entities of type Car using standard updateContext APPEND operations
(given that, as described in previous section, you cannot create entities with
types using convenience operations):

```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Car",
            "isPattern": "false",
            "id": "Car1",
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "value": "75"
                },
                {
                    "name": "fuel",
                    "type": "float",
                    "value": "12.5"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
} 
EOF
```
```
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
     --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "Car",
            "isPattern": "false",
            "id": "Car2",
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "value": "90"
                },
                {
                    "name": "fuel",
                    "type": "float",
                    "value": "25.7"
                }
            ]
        }
    ],
    "updateAction": "APPEND"
}
EOF
```

Request to get all the attributes:

```
curl localhost:1026/v1/contextEntityTypes/Car -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

Response:

```
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "75"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "12.5"
                    }
                ],
                "id": "Car1",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "90"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "25.7"
                    }
                ],
                "id": "Car2",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
```

Request to get only one attribute (e.g. speed):

```
curl localhost:1026/v1/contextEntityTypes/Car/attributes/speed -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

Response:

``` 
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "75"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "12.5"
                    }
                ],
                "id": "Car1",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "value": "90"
                    },
                    {
                        "name": "fuel",
                        "type": "float",
                        "value": "25.7"
                    }
                ],
                "id": "Car2",
                "isPattern": "false",
                "type": "Car"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
} 

``` 

Additional comments:

-   You can also use geographical scopes in your queries. This is an
    advanced topic, described in [this
    section](geolocation.md#geolocation-capabilities).
-   You can use the *?attributeFormat=object* URI parameter
    to get attributes as a JSON object (i.e. key-values map) instead of
    a vector (default behaviour):

``` 
curl localhost:1026/v1/contextEntities/Room1?attributeFormat=object -s -S \
    --header 'Accept: application/json' | python -mjson.tool

```


```
 {
    "contextResponses": [
        {
            "contextElement": {
                "attributes": {
                    "pressure": {
                        "type": "integer",
                        "value": "720"
                    },
                    "temperature": {
                        "type": "float",
                        "value": "23"
                    }
                },
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"'
            }
        }
    ]
}

``` 

[Top](#top)

#### Getting all entities

You can get all the entities using the following convenience operation:

```
curl localhost:1026/v1/contextEntities -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool
```
In our case, it will return both Room1 and Room2:

``` 
{
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "720"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": ""
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        },
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "21"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "711"
                    }
                ],
                "id": "Room2",
                "isPattern": "false",
                "type": ""
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
``` 

Additional comments:

-   Getting all the entities stored in Orion isn't a really good idea
    (except if you have a limited number of entities). Have a look at
    the [section on filters](filtering.md#filters ).
-   Note that by default, only 20 entities are returned (which is fine
    for this tutorial, but probably not for a real
    utilization scenario). In order to change this behaviour, see [the
    section on pagination](pagination.md#pagination ) in this manual.
-   You can use the
    *?attributeFormat=object* URI parameter to get attributes as a JSON
    object (i.e. key-values map) instead of a vector (default
    behaviour), as described in the [previous
    section](#convenience-query-context).

[Top](#top)

#### Browsing all types and detailed information on a type

The following operation can be used to get a list of all entity types
existing at Orion Context Broker in a given moment:

``` 
curl localhost:1026/v1/contextTypes -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool
``` 

The response will be:

``` 
{
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    },
    "types": [
        {
            "attributes": [
                "speed",
                "fuel",
                "temperature"
            ],
            "name": "Car"
        },
        {
            "attributes": [
                "pressure",
                "hummidity",
                "temperature"
            ],
            "name": "Room"
        }
    ]
}
``` 
As you can see, attribute information for each type is provided. Some
important remarks:

-   Given that NGSI doesn't force all the entities of a given type to
    have the same set of attributes (i.e. entities of the same type
    could have a different attributes set) the attributes set per type
    returned by this operation is the union set of the attribute sets of
    each entity belonging to that type.
-   If you are not interested in attributes information, you can use the
    *?collapse=true* parameter in order to get only a list of types.

In addition, you can use the following operation to get detailed
information of a given type (by the time being, that information consists 
of a list of all its attributes):

``` 
curl localhost:1026/v1/contextTypes/Room -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' | python -mjson.tool
``` 

The response will be:

``` 
{
    "attributes": [
        "hummidity",
        "pressure",
        "temperature"
    ],
    "name": "Room",
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    }
}
``` 
       
Note that [pagination mechanism](pagination.md#pagination) also works in
the operations described above.

In addition, note that this convenience operation doesn't have any standard operation counterpart.

[Top](#top)

#### Convenience Update Context

Let's set the Room1 temperature and pressure values:

``` 
(curl localhost:1026/v1/contextEntities/Room1/attributes -s -S \
    --header 'Content-Type: application/json'  --header 'Accept: application/json' \
    -X PUT -d @- | python -mjson.tool) << EOF
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "26.5"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "763"
        }
    ]
}
EOF
``` 

the response is:

``` 
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
``` 

Now, let's do the same with Room2:

``` 
(curl localhost:1026/v1/contextEntities/Room2/attributes -s -S \
    --header 'Content-Type: application/json'  --header 'Accept: application/json' \
    -X PUT -d @- | python -mjson.tool) << EOF
{
    "attributes": [
        {
            "name": "temperature",
            "type": "float",
            "value": "27.4"
        },
        {
            "name": "pressure",
            "type": "integer",
            "value": "755"
        }
    ]
} 
EOF
``` 
 
which response is:

``` 
{
    "contextResponses": [
        {
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "value": ""
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "value": ""
                }
            ],
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}
``` 

You can update a single attribute of a given entity in the following way:

``` 
(curl localhost:1026/v1/contextEntities/Room2/attributes/temperature -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -X PUT -d @- | python -mjson.tool) <<EOF
{
    "value": "26.3"
} 
EOF
``` 

Comparing to [standard updateContext
operation](#update-context-elements) we observe the following
differences:

-   We cannot update more than one entity at a time using convenience
    operation requests.
-   The payload of request and response in convenience operations are
    very similar to the ones used in standard operations, the
    contextAttributeList and contextResponse elements are the same.
-   You can replace
    "Room1" by "/type/Room/id/Room1" in the URl to define the type (in
    general: "/type/<type>/id/<id>").

Apart from simple values (i.e. strings) for attribute values, you can
also use complex structures or custom metadata. These are advance
topics, described in [this
section](structured_attribute_valued.md#structured-attribute-values ) and [this
other](metadata.md#custom-attribute-metadata ), respectively.

[Top](#top)

#### Convenience operations for context subscriptions

You can use the following convenience operations to manage context
subscriptions:

-   POST /v1/contextSubscriptions, to create the subscription, using the
    same payload as [standard susbcribeContext
    operation](#context-subscriptions ).
-   PUT /v1/contextSubscriptions/{subscriptionID}, to update the
    subscription identified by {subscriptionID}, using the same payload
    as [standard updateContextSubscription
    operation](#context-subscriptions). The ID in the payload must
    match the ID in the URL.
-   DELETE /v1/contextSubscriptions/{subscriptionID}, to cancel the
    subscription identified by {subscriptionID}. In this case, payload
    is not used

[Top](#top)

#### Summary of NGSI10 convenience operations URLs

Convenience operations use a URL to identify the resource and a HTTP
verb to identify the operation on that resource following the usual REST
convention: GET is used to retrieve information, POST is used to create
new information, PUT is used to update information and DELETE is used to
destroy information.

You find a summary in [the following
document](https://docs.google.com/spreadsheet/ccc?key=0Aj_S9VF3rt5DdEhqZHlBaGVURmhZRDY3aDRBdlpHS3c#gid=0).

[Top](#top)

## Context availability management using NGSI9

### NGSI9 standard operations

This section describes the different standard NGSI9 operations that the
Orion Context Broker supports, showing examples of requests and
responses. We use the term "standard" as they are directly derived from
the OMA NGSI specification, to distinguish them from the other family of
operations ("convenience") which has been defined by the FIWARE project
to ease the usage of NGSI implementations.

**Don't forget to restart the broker before starting this tutorial as
described [previously in this
document](#starting-the-broker-for-the-tutorials)**.

At the end of this section, you will have the basic knowledge to create
applications (both context producers and consumers) using Orion Context
Broker with NGSI9 standard operations:

-   registerContext
-   discoverContextAvailability
-   subscribeContextAvailability
-   updateContextAvailabilitySubscription
-   unsubscribeContextAvailability

[Top](#top)

#### Register Context operation

First of all you have to register Room1 and Room2. In order to do so, we
use the following NGSI9 registerContext operation:

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room1"
                },
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room2"
                }
            ],
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "isDomain": "false"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M"
}
EOF
``` 

The payload includes a list of contextRegistration elements, each one
with the following information:

-   A list of entities to be registered. In our case, they are the
    *Room1* and *Room2* entities. For each entity we specify a *type*
    (in this case, we are using "Room" as type) and an *ID* (which are
    "Room1" and "Room2" respectively). The *isPattern* field is not
    actually used in registerContext, so it always has a value
    of "false".
-   A list of attributes to register for the entities. In our case, they
    are the temperature and pressure attributes. For each one, we define
    a *name*, a *type* and whether it is a *domain attribute* or not.
    -   Orion Context Broker doesn't perform any checking on types (e.g.
        it doesn't check that when a context producer application
        updates the value of the temperature, this value is formatted as
        a float like "25.5" or "-40.23" and not something like "hot").
        In addition, domain attributes are not supported, so isDomain
        must always be set to "false".
-   The URL of the providing application. By "providing application" (or
    Context Provider) we mean the URL that represents the actual context
    information for the entities and attributes being registered. In our
    example we are assuming that all the sensors are provided by
    <http://mysensors.com/Rooms> (of course, this is a fake URL :). More
    information on providing application [later in this
    manual](context_providers.md#registering-context-providers-and-request-forwarding).

Note that in this case we are registering both rooms using just one
contextRegistration element, but we could also have used two
contextRegistrations, each one for a different Room. This would
typically be the case in which both rooms have different providing
applications (e.g. <http://mysensors.com/Rooms1> and
<http://mysensors.com/Rooms2>). Moreover, we would use four different
contextRegistrations in case each sensor were associated to different
providing applications (e.g. <http://mysensors.com/Rooms1/temperature>,
<http://mysensors.com/Rooms1/pressure>,
<http://mysensors.com/Rooms2/temperature> and
<http://mysensors.com/Rooms2/pressure>).

Finally, note that the payload includes a duration element. The duration
element sets the duration of the registration so after that time has
passed it can be considered as expired (however, [duration can be
extended](duration.md#extending-duration)). We use the [ISO 8601
standard](http://www.wikipedia.org/wiki/ISO_8601) for duration format.
We are using "P1M" which means "one month" (a very large amount,
probably enough time to complete this tutorial :).

We will get the following response :

``` 
{
    "duration": "P1M",
    "registrationId": "52a744b011f5816465943d58"
}
``` 

The registrationId (whose value will be different when you run the
request, as it is generated using the timestamp of the current time :)
is a 24 hexadecimal digit which provides an unique reference to the
registration. It is used for updating the registration as explained
[later in this manual](context_providers.md#updating-registrations).

[Top](#top)

#### Discover Context Availability operation

So now the broker has registration information about Room1 and Room2.
How can we access that information? Using the NGSI9
discoverContextAvailability operation. For example, we can discover
registrations for Room1 using:

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ]
} 
EOF
``` 

This would produce the following response:

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": "float"
                    },
                    {
                        "isDomain": "false",
                        "name": "pressure",
                        "type": "integer"
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": "Room"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

Note that we used an empty attributes in the request. Doing so, the discover searches for Room1, no
matter which attributes have been registered. If we want to be more
precise, we can include the name of an attribute to search for:

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header  'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

which produces the following response:

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": "float"
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": "Room"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

If the broker doesn't have any registration information, it will return
a response telling so. Thus, the following request:

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "humidity"
    ]
}
EOF
``` 

would produce the following response:

``` 
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context element registrations found"
    }
}
``` 

You can also search for a list of entities, e.g. to discover temperature
in both Room1 and Room2:

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        },
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room2"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

which will produce the following response:

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": "float"
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": "Room"
                    },
                    {
                        "id": "Room2",
                        "isPattern": "false",
                        "type": "Room"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

Finally, a powerful feature of Orion Context Broker is that you can use
a regular expression for the entity ID. For example, you can discover
entities whose ID starts with "Room" using the regex "Room.\*". In this
case, you have to set isPattern to "true" as shown below:

``` 
(curl localhost:1026/v1/registry/discoverContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "true",
            "id": "Room.*"
        }
    ],
    "attributes": [
        "temperature"
    ]
}
EOF
``` 

This will produce the exact same response as the previous example.

Note that by default only 20 registrations are returned (which is fine
for this tutorial, but probably not for a real utilization scenario). In
order to change this behaviour, see [the section on
pagination](pagination.md#pagination ) in this manual.

[Top](#top)

#### Context availability subscriptions

The NGSI9 operations you know up to now (registerContext and
discoverContextAvailability) are the basic building blocks for
synchronous context producer and context consumer applications. However,
Orion Context Broker has another powerful feature that you can take
advantage of: the ability to context information availability so when
"something" happens (we will explain the different cases for that
"something") your application will get an asynchronous notification. In
that way, you don't need to continuously repeat
discoverContextAvailability requests (i.e. polling), the Orion Context
Broker will let you know the information when it comes.

We assume that the accumulator-server.py program is still running.
Otherwise, start it as described in [the previous
section](#starting-accumulator-server-for-the-tutorials).

Context availability subscriptions are used when we want to be notified
not about context information (i.e. the values of attributes of some
entities) but about the availability of the context sources themselves.
We will clarify what this means with an example.

Let's consider that your context consumer application wants to be
notified each time the Orion Context Broker gets aware of a new Room
registration, e.g. because a new Room icon has to be drawn in the
graphical user interface that the application is offering to final
users. Thus, each time a new entity of type "Room" is registered in the
broker (using [registerContext
operation](#register-context-operation)), the broker must be
able to send notifications.

In order to configure this behavior, we use the following NGSI9
subscribeContextAvailability request:

``` 
(curl localhost:1026/v1/registry/subscribeContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "attributes": [
        "temperature"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M"
}
EOF
``` 



The payload has the following elements:

-   entities and attributes define which context availability information we are
    interested in. They are used to select the context registrations to
    include in the notifications.
    In this case, we are stating that we are interested in context
    availability about "temperature" attribute in any entity of type
    "Room" ("any" is represented by the ".\*" pattern, which is a
    regular expression that matches any string).
-   The callback URL to send notifications is defined with the
    *reference* element. We are using the URL of the
    accumulator-server.py program started before. Only one reference can
    be included per subscribeContextAvailability request. However, you
    can have several subscriptions on the same context availability
    elements (i.e. same entity and attribute) without
    any problem. Default URL schema (in the case you don't specify any)
    is "http", e.g. using "localhost:1028" as reference will be actually
    interpreted as "<http://localhost:1028>".
-   Subscriptions have a duration (specified in the duration elements in
    the same format as [registerContext
    request](#register-context-operation)). Once that
    duration expires, the subscription is ignored (however, it is still
    stored in the broker database and needs to be purged using the
    procedure described in the [administration
    manual](../admin/database_admin.md#deleting-expired-documents)).
    You can extend the duration of a subscription by updating it, as
    described [later in this document](duration.md#extending-duration ).
    We are using "P1M" which means "one month".

As you can see, the structure of subscriptionContextAvailability is
similar to the structure of [NGSI10
subscribeContext](#context-subscriptions), although in this
case we don't use notifyConditions nor throttling.

The response to the subscribeContextAvailability request is a
subscription ID (a 24 hexadecimal number used for updating and
cancelling the subscription - write it down because you will need it in
later steps of this tutorial) and a duration acknowledgement. Again,
pretty similar to a subscribeContext.

``` 
{
    "duration": "P1M",
    "subscriptionId": "52a745e011f5816465943d59"
}
```             

Looking at accumulator-server.py, we will see the following initial
notification:

``` 
POST http://localhost:1028/accumulate
Content-Length: 638
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json
                                                                                        
{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Room",
                        "isPattern": "false",
                        "id": "Room1"
                    },
                    {
                        "type": "Room",
                        "isPattern": "false",
                        "id": "Room2"
                    }
                ],
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 

Orion Context Broker notifies NGSI9 subscribeContextAvailability using
the POST HTTP method (on the URL used as reference for the subscription)
with a notifyContextAvailabilityRequest payload. Apart from the
subscriptionId element (that matches the one in the response to
subscribeContextAvailability request) and the originator element, the
contextResponse vector is the same than the one used in [the
discoverContextAvailability
responses](#discover-context-availability-operation).

Currently, the originator is always "localhost". We will look into a
more flexible way of using this in a later version.

The initial notification includes all the currently registered entities
that match the entity/attribute used in
subscribeContextAvailability request. That is, the registration
corresponding to Room1 and Room2 temperature. Note that, although Room1
and Room2 registered two attributes (temperature and pressure) only
temperature is shown, as the attribute vector in
subscribeContextAvailability only includes temperature.

The NGSI specification is not clear on if an initial
notifyContextAvailabilityRequest has to be sent in this case or not. On
one hand, some developers have told us that it might be useful to know
the initial registrations before starting to receive notifications due
to new registrations. On the other hand, an application can get the
initial status using discoverContextAvailability. Thus, this behavior
could be changed in a later version. What is your opinion? :)

Let's see what happens when we register a new room (Room3) with
temperature and pressure:

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room3"
                }
            ],
            "attributes": [
                {
                    "name": "temperature",
                    "type": "float",
                    "isDomain": "false"
                },
                {
                    "name": "pressure",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M"
}
EOF
``` 
As expected, the accumulator-server.py will be notified of the new
registration. Again, although Room3 registration includes temperature
and pressure, only the first attribute is included in the notification.

``` 
POST http://localhost:1028/accumulate
Content-Length: 522
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Room",
                        "isPattern": "false",
                        "id": "Room3"
                    }
                ],
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
``` 
We can also check that context registrations not matching the
subscription doesn't trigger any notifications. For example, let's
register a room (Room4) with only attribute pressure (remember that the
subscription only includes temperature in attribute vector).

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room4"
                }
            ],
            "attributes": [
                {
                    "name": "pressure",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M"
} 
EOF
``` 

You can now check that no new notification arrives to
accumulator-server.py.

As with context subscriptions, context availability subscriptions can be
updated (using the NGSI9 updateContextAvailabilitySubscription). The
request includes the *subscriptionId* that identifies the subscription
to modify, and the actual update payload. For example, let's change
subscription entities to something completely different: cars instead of
rooms and all the attributes are removed (i.e. an empty attribute 
element). As always you have to replace the *subscriptionId* value after
copy-pasting with the value you got from the
subscribeContextAvailability response in the previous step).

```
(curl localhost:1026/v1/registry/updateContextAvailabilitySubscription -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Car",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "duration": "P1M",
    "subscriptionId": "52a745e011f5816465943d59"
}
EOF                                                                                   
```
The response is very similar to the one for subscribeContextAvailability
request:

```
{
    "duration": "P1M",
    "subscriptionId": "52a745e011f5816465943d59"
}
```

Given that there are currently no car entities registered, you will not
receive any initial notification. So. let's register two cars: Car1 with
an attribute named speed and Car2 with an attribute named location.

```
(curl localhost:1026/v1/registry/registerContext -s -S  --header 'Content-Type: application/json' \
    --header 'Accept: application/json'  -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car1"
                }
            ],
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
}
EOF
```

```
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car2"
                }
            ],
            "attributes": [
                {
                    "name": "location",
                    "type": "ISO6709",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
}
EOF
```
As both registrations match the entity and attribute used in
the updateContextAvailabilitySubscription, we will get a notification
for each car registration, as can be seen in accumulator-server.py:

```
POST http://localhost:1028/accumulate
Content-Length: 529
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Car",
                        "isPattern": "false",
                        "id": "Car1"
                    }
                ],
                "attributes": [
                    {
                        "name": "speed",
                        "type": "integer",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
```

```
POST http://localhost:1028/accumulate
Content-Length: 535
User-Agent: orion/0.9.0
Host: localhost:1028
Accept: application/json
Content-Type: application/json

{
    "subscriptionId": "52a745e011f5816465943d59",
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "type": "Car",
                        "isPattern": "false",
                        "id": "Car2"
                    }
                ],
                "attributes": [
                    {
                        "name": "location",
                        "type": "ISO6709",
                        "isDomain": "false"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
```

Finally, you can cancel a subscription using the NGSI9
unsubscribeContextAvailability operation, just using the subscriptionId
in the request payload (replace the subscriptionId value after
copy-pasting with the one you received in the
subscribeContextAvailability response in the previous step).

```
(curl localhost:1026/v1/registry/unsubscribeContextAvailability -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) <<EOF
{
    "subscriptionId": "52a745e011f5816465943d59"
}
EOF
```
 
The response is just an acknowledgement that the cancellation was
successful.

```
{
    "statusCode": {
        "code": "200",
        "reasonPhrase": "OK"
    },
    "subscriptionId": "52a745e011f5816465943d59"
}
```

After cancelling, you can try to register a new car (e.g. Car3) to check
that no new notification is sent to accumulator-server.py.

[Top](#top)

#### Summary of NGSI9 standard operations URLs

Each standard operation has a unique URL. All of them use the POST
method. The summary is below:

-   <host:port>/v1/registry/registerContext
-   <host:port>/v1/registry/discoverContextAvailability
-   <host:port>/v1/registry/subscribeContextAvailability
-   <host:port>/v1/registry/updateContextAvailabilitySubscription
-   <host:port>/v1/registry/unsubscribeContextAvailability

[Top](#top)

### NGSI9 convenience operations

The following section describes the different convenience operations
described as part of the FIWARE NGSI REST API NGSI9 that Orion Context
Broker supports, showing examples of requests and responses. Convenience
operations are a set of operations that have been defined by FIWARE
project to ease the usage of NGSI implementations as a complement to the
standard operations defined in the OMA NGSI specification.

**Don't forget to restart the broker before starting this tutorial as
described [previously in this
document](#starting-the-broker-for-the-tutorials)**.

At the end of this section, you will have learnt to use convenience
operations as a handy alternative to some standard operations described
in [the previous
section](#tutorial-on-ngsi9-standard-operations). It is
highly recommended to do that tutorial before, to get familiar with
register, discover, etc. to be able to compare between the two
approaches.

[Top](#top)

#### Convenience Register Context

First of all, we register Room1 and Room2 with attributes temperature
and pressure, using the following commands:

``` 
(curl localhost:1026/v1/registry/contextEntities/Room1/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
```
(curl localhost:1026/v1/registry/contextEntities/Room1/attributes/pressure -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \ 
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
```
(curl localhost:1026/v1/registry/contextEntities/Room2/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
```
(curl localhost:1026/v1/registry/contextEntities/Room2/attributes/pressure -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \ 
    -d @- | python -mjson.tool) << EOF
{
  "duration" : "P1M",
  "providingApplication" : "http://mysensors.com/Rooms"
}
EOF
```
  
So, what's the difference compared to [standard registerContext
operation](#register-context-operation)?

-   We needed four requests, instead of just one request in the standard
    operation case.
-   We are using more operations, but the payload used in each operation
    is much simpler. This payload is a simplified version of the payload
    in registerContext, including only duration and
    providing application.
-   You can
    replace "Room1" by "/type/Room/id/Room1" in the URl to define the
    type (in general: "/type/<type>/id/<id>").
-   From the Orion Context Broker perspective, there are 4 independent
    registrations (i.e. 4 different registration IDs) to all
    effects (e.g. [updating](updating_regs_and_subs.md#updating-registrations),
    [extending duration](duration.md#extending-duration )).
-   It is possible to use /v1/registry/contextEntities/Room1 (without
    the attribute part). In that case, you are registering an entity
    without attributes. Note you cannot specify attributes in the
    registerProviderRequest element.

The response to each of these requests is the same as the response to a
standard registerContext (one response for each of the four requests,
with a different ID):

```
{
    "duration": "P1M",
    "registrationId": "51c1f5c31612797e4fe6b6b6"
}
```                               

[Top](#top)

#### Only-type entity registrations using convenience operations

You can use the NGSI9 "contextEntityTypes" convenience operations to
register entity types without an specific ID. Let's illustrate with an
example.

Let's register the "Funny" entity type (note that we are not specifying
any entity ID):

```
(curl localhost:1026/v1/registry/contextEntityTypes/Funny -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration": "P1M",
  "providingApplication": "http://mysensors.com/Funny"
}
EOF
```

Now, let's discover on that type:

```
curl localhost:1026/v1/registry/contextEntityTypes/Funny -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

which response is:

```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "entities": [
                    {
                        "id": "",
                        "isPattern": "false",
                        "type": "Funny"
                    }
                ],
                "providingApplication": "http://mysensors.com/Funny"
            }
        }
    ]
}
```

As you can see, the ID element is empty (it makes sense, as we didn't
specify any ID at registration).

Moreover, you can register attributes in these registrations, e.g:

```
(curl localhost:1026/v1/registry/contextEntityTypes/MoreFunny/attributes/ATT -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' \
    -d @- | python -mjson.tool) << EOF
{
  "duration": "P1M",
  "providingApplication": "http://mysensors.com/Funny"
}
EOF
```

```
curl localhost:1026/v1/registry/contextEntityTypes/MoreFunny/attributes/ATT -s -S \
    --header 'Accept: application/json' | python -mjson.tool
```

```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "ATT",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "",
                        "isPattern": "false",
                        "type": "MoreFunny"
                    }
                ],
                "providingApplication": "http://mysensors.com/Funny"
            }
        }
    ]
}
```

[Top](#top)

#### Convenience Discover Context Availability

Using convenience operations you can discover registration information
for a single entity or for an entity-attribute pair. For example, to
discover registrations for Room1 (no matter the attributes):

```
curl localhost:1026/v1/registry/contextEntities/Room1 -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```

which produces the following response:
```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        },
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "pressure",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room1",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
```
Now, let's discover registrations for Room2-temperature:

```
curl localhost:1026/v1/registry/contextEntities/Room2/attributes/temperature -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```

The response is as follows:

```
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "temperature",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room2",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        },
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "pressure",
                        "type": ""
                    }
                ],
                "entities": [
                    {
                        "id": "Room2",
                        "isPattern": "false",
                        "type": ""
                    }
                ],
                "providingApplication": "http://mysensors.com/Rooms"
            }
        }
    ]
}
```

Discovery of not registered elements (e.g. Room5 or the humidity of
Room1) will produce an error. E.g. the following requests:

```
curl localhost:1026/v1/registry/contextEntities/Room3 -s -S  \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
```
curl localhost:1026/v1/registry/contextEntities/Room2/attributes/humidity -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
will produce the following error response:

```
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context element found"
    }
}
```

Compared to [standard discoverContextAvailability
operation](#discover-context-availability-operation ):

-   Convenience operations use the GET method without needing any
    payload in the request (simpler than the standard operation).
    However, there are two differences in the content. First, each
    attribute always comes in a different contextRegistrationResponse
    element (as each attribute corresponds to a different registration,
    [as explained before](#convenience-register-context)).
    Secondly, as registrations done using convenience operations aren't
    typed, the type fields are empty for entities and attributes.
-   You can replace
    "Room1" by "/type/Room/id/Room1" in the URl to define the type (in
    general: "/type/<type>/id/<id>").
-   It is not possible to use convenience operations to discover lists
    of entities, entity patterns, nor lists of attributes.

You can also
discover by all the entities belonging to the same type, either all the
attributes or a particular one, as shown below. First, register an
couple of entities of type Car using standard registerContext operations
(given that, as described in previous section, you cannot register
entities with types using convenience operations):

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car1"
                }
            ],
            "attributes": [
                {
                    "name": "speed",
                    "type": "integer",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
}
EOF
``` 
``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Car",
                    "isPattern": "false",
                    "id": "Car2"
                }
            ],
            "attributes": [
                {
                    "name": "fuel",
                    "type": "float",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Cars"
        }
    ],
    "duration": "P1M"
} 
EOF
```

Request without specifying attributes:

```
curl localhost:1026/v1/registry/contextEntityTypes/Car -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
Response:

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "speed",
                        "type": "integer"
                    }
                ],
                "entities": [
                    {
                        "id": "Car1",
                        "isPattern": "false",
                        "type": "Car"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        },
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "fuel",
                        "type": "float"
                    }
                ],
                "entities": [
                    {
                        "id": "Car2",
                        "isPattern": "false",
                        "type": "Car"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
``` 

Request specifying one attribute (e.g. speed):

```
curl localhost:1026/v1/registry/contextEntityTypes/Car/attributes/speed -s -S \
    --header 'Content-Type: application/json' --header 'Accept: application/json' | python -mjson.tool
```
Response:

``` 
{
    "contextRegistrationResponses": [
        {
            "contextRegistration": {
                "attributes": [
                    {
                        "isDomain": "false",
                        "name": "speed",
                        "type": "integer"
                    }
                ],
                "entities": [
                    {
                        "id": "Car1",
                        "isPattern": "false",
                        "type": "Car"
                    }
                ],
                "providingApplication": "http://mysensors.com/Cars"
            }
        }
    ]
}
``` 

Note that by default only 20 registrations are returned (which is fine
for this tutorial, but probably not for a real utilization scenario). In
order to change this behaviour, see [the section on
pagination](pagination.md#pagination ) in this manual.

[Top](#top)

#### Convenience operations for context availability subscriptions

You can use the following convenience operations to manage context
availability subscriptions:

-   POST /v1/registry/contextAvailabilitySubscriptions, to create the
    subscription, using the same payload as [standard
    susbcribeAvailabilityContext
    operation](#context-availability-subscriptions ).
-   PUT /v1/registry/contextAvailabilitySubscriptions/{subscriptionID},
    to update the subscription identified by {subscriptionID}, using the
    same payload as [standard updateContextAvailabilitySubscription
    operation](#context-availability-subscriptions ). The ID
    in the payload must match the ID in the URL.
-   DELETE
    /v1/registry/contextAvailabilitySubscriptions/{subscriptionID}, to
    cancel the subscription identified by {subscriptionID}. In this
    case, payload is not used.

[Top](#top)

#### Summary of NGSI9 convenience operations URLs

Convenience operations use a URL to identify the resource and a HTTP
verb to identify the operation on that resource following the usual REST
convention: GET is used to retrieve information, POST is used to create
new information, PUT is used to update information and DELETE is used to
destroy information.

You find a summary in [the following
document](https://docs.google.com/spreadsheet/ccc?key=0Aj_S9VF3rt5DdEhqZHlBaGVURmhZRDY3aDRBdlpHS3c#gid=0).
<span style="color:red">.

[Top](#top)
