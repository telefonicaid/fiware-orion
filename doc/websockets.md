# Websockets interface

## What is this?

The websockets interface is a new way to communicate with Orion using websockets, this is point to improve the comunication with a faster and more efficient protocol, because now we have a persistent connection with an open socket. Also websockets are a way of easing the interaction between Orion and client applications running in browsers and smartphone.

This feature is in a very early phase, at this moment you can create, delete, modify and update entities and you can subscribe for notifications, but it is not well tested and up to date is only a wrapped version of the REST API, there is no a binary protocol implemented yet.

## How I can use it?

In the websocket protocol we need to wrap some parts of the REST API in JSON format in order to get a complete websocket message. To do this
we can use the following JSON template:

```
{
    "verb": "HTTP verb",
    "url": "/uri/used/to/this/operation",
    "params": {"key": "value"},
    "headers": {"key": "value"},
    "payload" : {Payload corresponding to the operation according to the REST API}
}
```
Description:

* verb: HTTP verb used to this operation.

* url: URL used in this operation, for example: `/v2/entities` or `/v2/subscriptions`

* params: these are the options passed normally in the URL after the `?` symbol and they are a key-value pair. For example if you use `http://server/v2?options=keyValues` on websockets you must use `"params": {"options": "keyValues"}`

* headers: these are the headers sent to the server in a REST message, you can use this to send information about the tenant, servicePath, etc.

* payload: the payload is a valid JSON with the REST message to be processed, according to the specific operation as described in the REST API specification.

### How to connect with Orion

To use Orion through websockets we need to connect to the port 9010 using "ngsiv2-json" as protocol name. For example in JavaScript the code to create the connection is as follows:

` var websocket = new WebSocket('ws://127.0.0.1:9010', 'ngsiv2-json');`

### Subscriptions in websockets

Subscriptions in websockets are basically the same as in the REST protocol, but they have three important differences:
1. Life cycle, a susbcription die when connection ends.
2. The callback are always `ws://`, and no, this is not a protocol prefix, you must use exactly this string.
3. Through websockets you can create subcriptions for REST or websocket (only for the very connection that sends the subscription request) notifications. However, note that using REST you can't create a subscription for websocket notification as in that case the connection used to create the subscripiton is closed when the operation finished.


### Examples

These are some examples of websocket message for Orion:

* Create an entity using `keyValue` option
	
```
{
	"verb" : "POST",
	"url" : "/v2/entities",
	"params" : {"options" : "keyValues"},
    "headers": {"Fiware-service": "my_Fiware-service"},
	"payload" : {"type": "T", "id": "E1","temp": 1}
}
```
    
* Subscribe for notifications

```
{
	"verb": "POST",
	"url": "/v2/subscriptions",
    "headers": {"Fiware-service": "my_Fiware-service"},
	"payload": {
		"description": "subscription example",
		"subject": {
			"entities": [{
				"id": "E1",
				"type": "T"
			}],
			"condition": {
				"attributes": [
					"temp"
				],
				"expression": {
					"q": "temp>40"
				}
			}
		},
		"notification": {
			"callback": "ws://",
			"attributes": [
				"temp"
			],
			"throttling": 5
		},
		"expires": "2017-04-05T14:00:00.00Z"
	}
}
```
callback field, which must always be exactly "ws://" - this tells Orion how notificacations are to be sent.

* Update an entity

```
{
	"verb" : "POST",
	"url" : "/v2/entities/E1",
        "headers": {"Fiware-service": "my_Fiware-service"},
	"params" : {"options" : "keyValues"},
	"payload" : {"temp": 1}
}
```

* Delete entity
```
{
	"headers": {"Fiware-service": "my_Fiware-service"},
	"verb" : "DELETE",
	"url" : "/v2/entities/E1"
}
```

