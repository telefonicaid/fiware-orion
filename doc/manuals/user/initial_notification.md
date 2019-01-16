Initial notification
Introduction
Considering a given subscription, Orion Context Broker notifies whenever an update occurs and the subscription conditions are met on the updated entity. That is, the entity an attribute being update are covered by the subscription and other optional filters (attribute values, geolocation, etc.) also match. Thus, notifications are synchronous to entity updates.

However, there is a special case of notification, named initial notification, which is sent synchronous to the subscription creation (or update) transaction. This notification includes all the entities covered by the subscription being created (or updated). Different from non-initial notification, it may include several entities, depending on the existing context. The semantics for the initial notification are in fact very close to the ones in a synchronous query (GET /v2/entities).

Let's illustrate with an example. At a given moment, the following entities exist at Orion (response to GET /v2/entities):

[
    {
        "id": "Room1",
        "temperature": {
            "metadata": {},
            "type": "Float",
            "value": 23
        },
        "type": "Room"
    },
    {
        "id": "Room2",
        "temperature": {
            "metadata": {},
            "type": "Float",
            "value": 33
        },
        "type": "Room"
    },
    {
        "id": "Room3",
        "temperature": {
            "metadata": {},
            "type": "Float",
            "value": 43
        },
        "type": "Room"
    }
]
Then, the following subscription is created:

POST /v2/subscriptions

{
  "subject": {
    "entities": [
      {
        "idPattern": ".*",
        "type": "Room"
      }
    ],
    "condition": {
      "expression": {
	     "q": "temperature<35"
	  }
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/accumulate"
    },
    "attrs": [
      "temperature"
    ]
  }
}
That subscription creation triggers an initial notification like this one (simplified):

POST /accumulate HTTP/1.1

{
	"subscriptionId": "59edf55231cee478fe9fff1e",
	"data": [{
		"id": "Room1",
		"type": "Room",
		"temperature": {
			"type": "Float",
			"value": 23,
			"metadata": {}
		}
	}, {
		"id": "Room2",
		"type": "Room",
		"temperature": {
			"type": "Float",
			"value": 33,
			"metadata": {}
		}
	}]
}
Note that Room3 is not include, as it doesn’t match the attribute value filter.

Rationale
Initial notification are sent because the Orion Context Broker considers the transition from "non existing subscription" to "subscribed" as a change.

The original NGSI specification from OMA is not clear on if an initial notification has to be sent in this case or not. On one hand, some developers think it might be useful to know the initial values before starting to receive notifications due to actual changes. On the other hand, an application can get the initial status using synchronous queries so no initial notification is required.

Between both options, We have opted to implement the more flexible approach, which provide the initial notification to users who can exploit is and can be ignore for who doesn't need it. 

For ignoring the initial notification, a URI parameter `options=skipInitialNotification` can be used. Using this URI parameter, initial notification can be skipped at Subscription creation/updation time. 

Additional considerations
Note that initial notification is not always sent. It is sent only in the case some entity matches the subscription criteria at subscription creation/update time. If the matching occurs, it is possible to avoid it by using URI parameter. 

Initial notification uses the same entities limit that synchronous queries. That is, as much as 20 entities are included in the initial notification, no matter how much entities got covered by the subscription. Different from queries (which have limit and orderBy options), this number or the ordering criteria cannot be configured. How to apply this kind of pagination to initial notifications is yet a matter of discussion (see dicussion) as it could be problematic to send a large amount of entities in the same notification. Thus at the present moment, in the case of having a large set of entities covered by the subscription, it is not recommend to rely in initial notification to get the initial context for you application (use synchronous queries instead).

As happens with non-initial notification, the Fiware-ServicePath header is included in initial notification. However, note that an initial notification potentially includes several (more than one) entities, which may belong to different service paths. Thus, a list of comma separated values is used (the first element in the Fiware-ServicePath list corresponding to the first entity, etc.). Note that given the limit of possible entities (20) is greater than the number of allowed service paths elements in the list (10) this may lead to produce "illegal" initial notification when consumed by other Context Broker in a federation scenario. The means to solve this situation (i.e. grouping all the entities belonging to the same service path in the same notification and send how many initial notifications as different service paths) are part of our current backlog.




