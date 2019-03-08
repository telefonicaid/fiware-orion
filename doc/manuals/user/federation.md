# Context Broker Federation

This section described "push" federation (in the sense notifyContext
sent by one Orion instance are processed by other Orion instance).
However, the [Context Providers and request forwarding](context_providers.md)
functionality can be used to implement a kind of "pull" federation (in
which one Orion instance forwards a query/update to another Orion
instance). Note that an important difference between two approaches is
that in the "push" mode all the Orion instances update its local state,
while in the "pull" approach all the intermediate Orion instances acts
as "proxy" without storing the data locally.

Apart from processing updates (usually issued by a client application)
Orion Context Broker can process notifications with the same semantics.
This opens the door to interesting federation scenarios (one example is
the [FIWARE Lab context management platform](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FIWARE_Lab_Context_Management_Platform)).

![](Federation.png "Federation.png")

Let's illustrate with an example. Consider the following setup: three
context broker instances running in the same machine (of course, this is
not a requirement but makes things simpler to test this feature), in
ports 1030, 1031 and 1032 respectively and using different databases
(named A, B and C to be brief). Let's start each instance (run each
command in a separate terminal):

    contextBroker -fg -port 1030 -db orion1030
    contextBroker -fg -port 1031 -db orion1031
    contextBroker -fg -port 1032 -db orion1032

Next, let's send create a subscription in A (to make B subscribe to updates
made in A). Note that the URL used in the reference has to be
"/v2/op/notify":

```
curl localhost:1030/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "temperature"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1031/v2/op/notify"
    }
  }
}
EOF
```


Next, let's send a create a subscription in B (to make C subscribe to updates
made in B). The subscription is basically the same, only the port in the
curl line and reference elements are different.

```
curl localhost:1031/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "temperature"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1032/v2/op/notify"
    }
  }
}
EOF
```

Now, let's create an entity in context broker A.

```
curl localhost:1030/v2/entities -s -S -H'Content-Type: application/json' -d @- <<EOF
{
  "id": "Room1",
  "type": "Room",
  "temperature": {
    "value": 23,
    "type": "Number"
  }
}
EOF
```

Given the subscriptions in place, a notification is
automatically sent from A to B. That event at B causes in sequence a
notification to be sent to C. So, at the end, the creation of an
entity in A causes the creation of the same entity (with the same
attribute values) in C. You can check it by doing a query to C:

```
curl localhost:1032/v2/entities -s -S H-header 'Accept: application/json' -d @- | python -mjson.tool
```

which response is:

```
[
  {
    "id": "Room1",
    "type": "Room",
    "temperature": {
      "value": 23,
      "type": "Number"
    }
  }
]
```

The semantics of the notification request are the same than `POST /v2/entities?options=upsert`.
So, if the entity exists, is upated. If the entities doesn't exist, it is created. Thus,
federation doesn't provide exact mirroring: if the entity is deleted in
first Context Broker the entity will not be deleted in the second Context Broker.

Note that Orion Context Broker could send an [initial notification](initial_notification.md)
when the federation subscription is done. In some cases, this initial notification could be
unprocessable by the receiver Context Broker. In particular, we have found cases in which
the initial notification includes more elements in the service path headers than the legally
allowed (see [documentation about service path](service_path.md)), thus generating a
`"too many service paths - a maximum of ten service paths is allowed"` error. However, note
that only this initial notification is ignored, regular notifications after it doesn't have
this problems and are correctly processed by the receiving Context Broker.
