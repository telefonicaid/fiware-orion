# Websockets interface

## What is this?

This interface is a new way to talk with Orion using websockets, this is point to improve the comunication with a faster and efficient protocol, now we have a persistent connection with a open socket. 

This feature is in a very early phase, at this moment you can create, delete, modify and update entities and you can subscribe for notifications, but is not well tested and up to date is only a wrapped version of REST API, there is not a binary protocol.

## How I can use it?

In the websocket protocol we need wrap some parts of REST API in JSON format in order to get a complete websocket message, to do this
we can use the following JSON template:

```
{
    "verb": "HTTP verb",
    "url": "/uri/used/to/this/operation",
    "params": {"key": "value"},
    "headers": {"key": "value"},
    "payload" : {Put a normal REST message here}
}
```
Description:

* verb: is the HTTP verb used to this operation.

* url: is the URL used in this operation, for example: `/v2/entities` or `/v2/subscriptions`

* params: these are the options pased normally in the URL after the `?` symbol and they are a key-value pair. For example if you use `http://server/v2?options=keyValues` on websockets you must use `"params": {"options": "keyValues"}`

* headers: these are the headers sent to server in a REST message, you can use this to sent information about the tenant, servicePath, etc.

* payload: the payload is a valid JSON with the REST message to be processed.

### How to connect with Orion

To use Orion through websockets we need connect with port 9010 using "ngsiv2-json" as protocol name. For example in JavaScript the code is as follow

` var websocket = new WebSocket('ws://127.0.0.1:9010', 'ngsiv2-json');`

### Subscriptions in websockets

Subscriptions in websockets are basically the same as in the REST protocol, but they have three important differences:
1. Life cycle, a susbcription die when connection die.
2. The callback are always `ws://`, and no, this is not a protocol prefix, you must use exactly this string.
3. Through websockets you can create subcriptions for REST or websocket notifications, but using REST you can't create a subscription for websocket notification. 


### Examples

These are some examples of websocket message for Orion:

* Create an entity using `keyValue` option
	
```
{
	"verb" : "POST",
	"url" : "/v2/entities",
	"params" : {"options" : "keyValues"},
    "headers": {"tenant": "my_tenant"},
	"payload" : {"type": "1", "id": "1","temp": 1}
}
```
    
* Subscribe for notifications

```
{
	"verb": "POST",
	"url": "/v2/subscriptions",
    "headers": {"tenant": "my_tenant"},
	"payload": {
		"description": "subscription example",
		"subject": {
			"entities": [{
				"id": "1",
				"type": "1"
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
A very important detail is the callback field, always must be "ws://" this say to Orion how notificacation must be send.

* Update an entity

```
{
	"verb" : "POST",
	"url" : "/v2/entities/1",
    "headers": {"tenant": "my_tenant"},
	"params" : {"options" : "keyValues"},
	"payload" : {"temp": 1}
}
```

* Delete entity
```
{
	"headers": {"tenant": "my_tenant"},
	"verb" : "DELETE",
	"url" : "/v2/entities/1"
}
```

