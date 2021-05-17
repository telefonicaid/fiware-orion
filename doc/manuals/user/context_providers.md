# Context Providers registration and request forwarding

The register context operation uses the
concept of "context provider" which is a URL that identifies the
_source_ of the context information for the entities/attributes included
in that registration.

This _source_ (url) is provided by the `provider` object:

```
...
"provider": {
  "http": {
    "url": "http://mysensors.com/Rooms"
  }
}
...
```
  
If Orion receives a query or update operation and it cannot find the targeted context
element locally (i.e. in its internal database) *but* a Context Provider
is registered for that context element, then Orion will forward the
query/update request to the Context Provider. In this case, Orion acts
as a pure "NGSI proxy" (i.e. Orion doesn't cache the result of the query
internally) and, from the point of view of the client issuing the
original request, the process is mostly transparent. The Context
Provider is required to implement the NGSI API (at least partially) to
support the query/update operation.

Let's illustrate this with an example.

![](QueryContextWithContextProvider.png "QueryContextWithContextProvider.png")


* First (message number 1), the application (perhaps on behalf of a
  Context Provider) registers the Context Provider in Orion for the
  Street4 temperature. Let's assume that the Context Provider exposes
  its API on <http://sensor48.mycity.com/v2>
      
```
curl localhost:1026/v2/registrations -s -S -H 'Content-Type: application/json' -H 'Accept: application/json' -d @-  <<EOF
{
  "dataProvided": {
    "entities": [
      {
        "id": "Street4",
        "type": "Street"
      }
    ],
    "attrs": [
      "temperature"
    ]
  },
  "provider": {
    "http": {
      "url": "http://sensor48.mycity.com/v2"
    }
  }
}
EOF
```
      
      
* Next, consider that a client queries the temperature (message number 2).

      
```
curl localhost:1026/v2/entities/Street4/attrs/temperature?type=Street -s -S \
    -H 'Accept: application/json' -d @- | python -mjson.tool
``` 

* Orion doesn't know the Street4 temperature, but it knows (due to
  the registration in the previous step) that the Context Provider at
  <http://sensor48.mycity.com/v2> does know about the Street4 temperature, so it forwards the query
  (message number 3) to the URL
  <http://sensor48.mycity.com/v2/entities> (i.e., the URL used in
  the `url` field at registration time, and adding "/entities" to the URL PATH).

If NGSIv2 forwarding is used, the forwarded query would be something like the following one.
You may wonder why `GET /v2/entities` is not used (as in the client request). This is due to
this operation has limitations when the query includes more than one entity of different types
(`POST /v2/op/query` is fully expressive and doesn't have that limitation).


```
POST http://sensor48.mycity.com/v2/op/query

{
  "entities": [
    {
      "id": "Street4",
      "type": "Street"
    }
  ],
  "attrs": [
    "temperature"
  ]
}
```

* The Context Provider at <http://sensor48.mycity.com/v2> would respond
  with the payload (message number 4):

``` 
[
  {
    "id": "Street4",
    "type": "Street",
    "temperature": {
      "value": "16",
      "type": "float"
    }
  }
]
``` 

* Orion forwards the response to the client (message number 5).

``` 
{
   "value": 16,
   "type": "Number
}
```

Some additional comments:

-   The `-httpTimeout` [CLI parameter](admin/cli.md)
    is used to set the CPr timeout. If a request forwarded to a CPr is
    taking more than that timeout, then Orion closes the connection and
    assumes that the CPr is not responding.
-   In case a given request involves more than one Context Provider (e.g. an
    update including 3 context elements, each one being an entity
    managed by a different Context Provider), Orion will forward the
    corresponding "piece" of the request to each Context Provider,
    gathering all the results before responding to the client. The current
    implementation processes multiple forwards in sequence, i.e. Orion awaits
    the response from the previous CPr (or timeout expiration) before sending
    the forward request to the next.
-   You can use the `-cprForwardLimit` [CLI parameter](admin/cli.md) to limit
    the maximum number of forwarded requests to Context Providers for a single client request.
    You can use 0 to disable Context Providers forwarding completely.
-   On forwarding, any type of entity in the NGSIv2 update/query matches registrations without entity type. However, the
    opposite doesn't work, so if you have registrations with types, then you must use `?type` in NGSIv2  update/query in
    order to obtain a match. Otherwise you may encounter problems, like the one described in this
    [post at StackOverflow](https://stackoverflow.com/questions/48163972/orion-cb-doesnt-update-lazy-attributes-on-iot-agent).
-   Query filtering (e.g. `GET /v2/entities?q=temperature>40`) is not supported on query forwarding. First, Orion
    doesn't include the filter in the `POST /v2/op/query` operation forwarded to CPr. Second, Orion doesn't filter
    the CPr results before responding them back to client. An issue corresponding to this limitation has been created:
    https://github.com/telefonicaid/fiware-orion/issues/2282
-   In the case of partial updates (e.g. `POST /v2/op/update` resulting in some entities/attributes being updated and
    other entities/attributes not being updated due to failing or missing CPrs), a 404 Not Found is returned to the client.
    The `error` field in this case is `PartialUpdate`. The `description` field contains a list of attributes corresponding
    to the partial update. In the case of NGSIv1 CPr, the CPr response to CB returns exactly the attributes that
    weren't updated in a partial update. In the case of NGSIv2 CPr, the CPr response to Orion doesn't provide information
    about which attributes were updated and which ones were not in a partial update. Thus, in a general case the list
    of attributes in `description` contains *at least one* attribute that was not updated (to a maximum of all them). If
    only NGSIv1 CPrs are involved in the fowarding, the list precisely identifies only the attributes that weren't updated.
