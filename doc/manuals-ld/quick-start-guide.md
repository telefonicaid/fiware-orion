# Orion-LD Quick Start Guide

Welcome to the Quick Start Guide to Orion-LD, the NGSI-LD context broker!

This guide is a brief guide to the most common characteristics of Orion-LD, with examples.
It is imperative to have a running instance of Orion-LD and MongoDB to play with for this exercise.

Orion-LD is an enhanced [Orion](https://github.com/telefonicaid/fiware-orion) and implements (apart from what Orion offers) the NGSI-LD API.
The NGSI-LD API is specified [here](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf) and while it's a pretty extensive
document, anybody that is really going to work with NGSI-LD should definitely read this document. And use it as a reference, of course.

The first thing to do is to thoroughly read [this quick guide to the @context](doc/manuals-ld/the-context.md).
There are also a number of tutorials about NGSI-LD:
* [FIWARE 601: Introduction to Linked Data](https://fiware-tutorials.readthedocs.io/en/latest/linked-data)
* [FIWARE 602: Linked Data Relationships and Data Models](https://fiware-tutorials.readthedocs.io/en/latest/relationships-linked-data)


Apart from learning about the context, this quick-start guide will show how to:
- Use `curl`, `JavaScript` and `Python` to send HTTP requests to Orion-LD
- Use the `mongo` command tool to inspect the contents of the database
- Create entities with contexts
- Retrieve entities with different contexts - to see "different results" for the very same entity retrieval !!!
- Filter the results - get only the entities that match your specific criteria

More information about the context will be provided as well, as this is crucial knowledge to work with Orion-LD.

## Creation of Entities
Now that we know how the context works, let's get our hands dirty and create an entity or five!
Start three terminal windows:
* broker
* mongo
* requests

In the **broker terminal**, start the Orion-LD context broker, and start it in the foreground (the `-fg` option):
```bash
orionld -fg
```

In the **mongo terminal**, first make sure that mongo is running. If not - start it.
The terminal will be used later to look at the contents of the database.

In the **request terminal** we will issue HTTP requests to the broker.

The first example will not supply and user context => only the Core Context will be applied.

Let's create an entity with two attributes:
* status (which is part of the Core Context)
* state  (which is **not** part of the Core Context)

We will be using `curl` which you have already installed if you have followed the [installation guide](doc/manuals-ld/installation-guide.md).
If not, make sure you have `curl` installed before proceeding.
However, there are code snippets also for Python and JavaScript (node.js), so if you prefer those ... 

Here goes:

### Entity Creation Example 1 - without context

#### curl
```bash
payload='{
  "id": "urn:entities:E1",
  "type": "T",
  "status": {
    "type": "Property",
    "value": "OK"
  },
  "state": {
    "type": "Property",
    "value": "OK"
  }
}'
curl localhost:1026/ngsi-ld/v1/entities -d "$payload" -H "Content-Type: application/json"
```

#### Python
```bash
import json
import requests

payload={
  "id": "urn:entities:E1",
  "type": "T",
  "status": {
    "type": "Property",
    "value": "OK"
  },
  "state": {
    "type": "Property",
    "value": "OK"
  }
}
response = requests.post(url='http://localhost:1026/ngsi-ld/v1/entities', headers={
    "content-type": "application/json"},  data=json.dumps(payload))
print(response.status_code)
```

#### JavaScript (Node.js)
To make requests with JavaScript you need to install [Node.js and NPM](https://nodejs.org/en/download/package-manager/) and create a NPM project:
```bash
mkdir myproject
cd myproject
npm init -y
touch index.js
```
And finally, install [axios](https://github.com/axios/axios) to make HTTP requests.
```
npm install axios
```
Let's go for the example.
```
const axios = require('axios')
const payload = {
    "id": "urn:entities:E2",
    "type": "T",
    "status": {
        "type": "Property",
        "value": "OK"
    },
    "state": {
        "type": "Property",
        "value": "OK"
    }
}
axios.post('http://localhost:1026/ngsi-ld/v1/entities', payload, 
            { headers: { "content-type": "application/json" } })
            .then(res => console.log(res.status))
            .catch(err => console.log(err))
```
A few notes about the payload:
* The entity `"id"` field **must** be a URI.
* The entity `"type"` field is **mandatory** - will be expanded.
* Attributes must be JSON Objects, and they **must** have a "type", whose value **must** be any of:
    * Property
    * Relationship
    * GeoProperty
  The attribute names will be expanded.
* Attributes that are of type *Property* **must** have a "value" field
* Attributes that are of type *Relationship* **must** have an "object" field and the value of that field **must** be a URI.

After issuing this command, the broker responds with a **201 Created** (which you won't see unless you ask `curl` to show the HTTP headers - how to do this is explained later)
and we can now look inside the mongo database to see what exactly has been stored:

```bash
mongo orion
> db.entities.findOne()
{
	"_id" : {
		"id" : "urn:entities:E1",
		"type" : "https://uri.etsi.org/ngsi-ld/default-context/T",
	},
	"attrs" : {
		"https://uri=etsi=org/ngsi-ld/status" : {
			"type" : "Property",
			"value" : "OK",
		},
		"https://uri=etsi=org/ngsi-ld/default-context/state" : {
			"type" : "Property",
			"value" : "OK",
		}
	}
}
```

The aim of this guide is not to teach about the data model or Orion/Orion-LD, so lots of stuff from the mongo content has been cut out to save lines.
What we will concentrate on here is the expansion of the entity type and the two attributes "state" and "status".
[ If you issue this command yourself, you will see more fields, especially "creDate/modDate" that are timestamps to store creation date and last modification date. ]

The entity type was "T".
"T" is not part of the core context, and no user context has been supplied, so, "T" wasn't found in the context.
What happens if a term to be expanded is not found in the context?  
It is expanded according to the value of the "@vocab" key of the core context. This is called the "Default URL".

The same has happened to the property "state".

"status" on the other hand is part of the Core Context, and it has been expanded accordingly.

But look at the expansions of the properties.
They aren't as the core context defines them!!!
All dots have been replaced with '=' !!!

This is part of the database model. Dots can't be part of any attribute name in the database as that would complicate filtering over sub-attributes.
We have to look at things in advance here, to understand this.
There is a mechanism to filter on properties-of-properties and the dot '.' is used as separator for property names.
Imagine you have an entity with a Property "P1" that in turn has a sub-property "P11" with the value 127.
This is how you find that entity:
```bash
GET /ngsi-ld/v1/entities?q=P1.P11==127
```

Now, if the dot '.' were to be allowed as part of the property name inside the database, this would be really difficult to implement, boarderline impossible.
So, we made the decision to use replace all dots with '=' before storing the name in the database.
We picked '=' as it's a forbidden character for attribute names and can't be used anyway.

By the way, the "P1.P11" in the q-expression would be expanded to something like "https://uri=etsi=org/ngsi-ld/default-context/P1.https://uri=etsi=org/ngsi-ld/default-context/P11",
if the Core context were used. And yes, it may look ugly, but it works :)

### Entity Creation Example 2 - with a user context in the payload
Now let's play a little with expansions, by using our own context that tries to overload the Core context (which is not possible, as you will see).
We will create our own context, defining:
* status (that is part of the Core context and thus cannot ve overloaded)
* state  (that is *not* part of the Core context)

The entity to be created will contain three attribute, one for each type of expansion:
* status (expanded according to the core context)
* state  (expanded according to the user context)
* state2 (expanded according to the default URL)

### CURL
```bash
payload='{
  "@context": {
    "status": "http://a.b.c/attrs/status",
    "state":  "http://a.b.c/attrs/state"
  },
  "id": "urn:entities:E2",
  "type": "T",
  "status": {
    "type": "Property",
    "value": "From Core Context"
  },
  "state": {
    "type": "Property",
    "value": "From User Context"
  },
  "state2": {
    "type": "Property",
    "value": "From Default URL"
  }
}'
curl localhost:1026/ngsi-ld/v1/entities -d "$payload" -H "Content-Type: application/ld+json"
```
### Python
```bash
import json
import requests

payload={
  "@context": {
    "status": "http://a.b.c/attrs/status",
    "state":  "http://a.b.c/attrs/state"
  },
  "id": "urn:entities:E2",
  "type": "T",
  "status": {
    "type": "Property",
    "value": "From Core Context"
  },
  "state": {
    "type": "Property",
    "value": "From User Context"
  },
  "state2": {
    "type": "Property",
    "value": "From Default URL"
  }
}
response = requests.post(url='http://localhost:1026/ngsi-ld/v1/entities', headers={
    "content-type": "application/ld+json"}, data=json.dumps(payload))
print(response.status_code)
```
### JavaScript (Node.js)
```bash
const axios = require('axios')
const payload = {
  "@context": {
    "status": "http://a.b.c/attrs/status",
    "state":  "http://a.b.c/attrs/state"
  },
  "id": "urn:entities:E2",
  "type": "T",
  "status": {
    "type": "Property",
    "value": "From Core Context"
  },
  "state": {
    "type": "Property",
    "value": "From User Context"
  },
  "state2": {
    "type": "Property",
    "value": "From Default URL"
  }
}
axios.post('http://localhost:1026/ngsi-ld/v1/entities', payload, 
            { headers: { "content-type": "application/ld+json" } })
            .then(res => console.log(res.status))
            .catch(err => console.log(err))
```
Note that the Content-Type now must be `application/ld+json`, as the payload data carries a context.
Let's see the database content after issuing this request:
```bash
mongo orion
> db.entities.findOne({"_id.id": "urn:entities:E2"})
```

This is the trimmed mongo output:
```
{
	"_id" : {
		"id" : "urn:entities:E2",
		"type" : "https://uri.etsi.org/ngsi-ld/default-context/T",
	},
	"attrs" : {
		"https://uri=etsi=org/ngsi-ld/status" : {
			"type" : "Property",
			"value" : "From Core Context",
		},
		"http://a=b=c/attrs/state" : {
			"type" : "Property",
			"value" : "From User Context",
		},
		"https://uri=etsi=org/ngsi-ld/default-context/state2" : {
			"type" : "Property",
			"value" : "From Default URL",
		}
	}
}
```

As you expected and as you can see:
* "status" has been expanded according to the Core context - defining it in the user context was useless
* "state" has been expanded according to the user context
* "T" and "state2" have both been expanded according to the default URL (the value of the @vocab item of the core context)

## Retrieval of Entities
We have created two entities, with a mix of contexts being used to expand the attribute names.
Let's retrieve the entities and see what exact attribute names we get.

### Entity Retrieval Example 1 - without context
No context will be used (== the Core context will be used):

#### CURL
```bash
curl localhost:1026/ngsi-ld/v1/entities?type=T
```

Note that you could alse type in the long name of the entity type:
```bash
curl localhost:1026/ngsi-ld/v1/entities?type=https://uri.etsi.org/ngsi-ld/default-context/T
```

That would be useful in case you look for entities whose types have been expanded with some other context.
Instead of passing in the entire context, you only pass the fully qualified name (FQN) of the *type*.

The output from the retieval command is as follows:
```
[{"@context":"https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld","id":"urn:entities:E1","type":"T","state":{"type":"Property","value":"OK"},"status":{"type":"Property","value":"OK"}},{"@context":"https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld","id":"urn:entities:E2","type":"T","http://a.b.c/attrs/state":{"type":"Property","value":"From User Context"},"state2":{"type":"Property","value":"From Default URL"},"status":{"type":"Property","value":"From Core Context"}}]
```

That's not very readable, is it?

Luckily, Orion-LD supports pretty printing and we'll issue the command like this instead:
```bash
curl 'localhost:1026/ngsi-ld/v1/entities?type=T&prettyPrint=yes&spaces=2'
```

This is the "new" output:
```json
[
  {
    "@context": "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
    "id": "urn:entities:E1",
    "type": "T",
    "state": {
      "type": "Property",
      "value": "OK"
    },
    "status": {
      "type": "Property",
      "value": "OK"
    }
  },
  {
    "@context": "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
    "id": "urn:entities:E2",
    "type": "T",
    "http://a.b.c/attrs/state": {
      "type": "Property",
      "value": "From User Context"
    },
    "state2": {
      "type": "Property",
      "value": "From Default URL"
    },
    "status": {
      "type": "Property",
      "value": "From Core Context"
    }
  }
]
```
That's more like it!

Two entities are found, as we filtered over entity type "T" and both our entities were created with that type.
The entities come with the context in the payload ... Why?
Well, curl adds an "Accept: */*" if no Accept header is stated on the command line, so, Orion-LD receives "Accept: */*" which means
that **any** MIME format is accepted and thus, **application/ld+json** is picked.

To inhibit this behaviour of `curl`, give it an Accept header, for example **application/json**:
```bash
curl 'localhost:1026/ngsi-ld/v1/entities?type=T&prettyPrint=yes&spaces=2' -H "Accept: application/json"
```

If you try this command you will see the entities without the "@context" member (the context is returned in the HTTP header "Link" instead).

You can also ask `curl` to not add any Accept header at all, by giving it an empty Accept header:
```bash
curl 'localhost:1026/ngsi-ld/v1/entities?type=T&prettyPrint=yes&spaces=2' -H "Accept:"
```

Orion-LD responds with mime-type *application/json* if no Accept header is given.
Orion-LD responds with mime-type *application/json* whenever possible (for example, for accept header `*/*` or `application/*`.

Back on track ...
The entity "urn:entities:E1" was created without any context, i.e. the Core Context, but when creating "urn:entities:E2" we gave a context.
So, why do both entities come back with the Core context???

Simple.
The context of the **GET request** was the core context, and that is what has been used to assemble the response.
It doesn't matter what context was used when creating/modifying the entities.

This is important:

**Each request uses the current context - the context used when issuing the request. Never mind what context was used before.**

After all, a context is nothing but a collection of aliases.
The real names and values are the longnames, which is what is stored in the database.

Now, what is important here is to look at the attribute names.
We have two entities, one with two attributes and one with three
* urn:entities:E1 (created with Core Context):
  * state
  * status
* urn:entities:E2 (created with user context):
  * state
  * state2
  * status

As you can see from the response of the GET request, only "state" of "urn:entities:E2" is returned as a long name.
Why is that?
Well, when the GET request processed what was found in the database, trying to match long names to short names,
only the Core context was used, as that's what was used in the GET request.

The entity urn:entities:E1 was created using the that same context, so its "state" attribute was expanded using the Default URL.
The broker is intelligent enough to see that, and is able to compact "https://uri.etsi.org/ngsi-ld/default-context/state" to "state" (same same with entity type "T").
The "state" attribute of entity urn:entities:E2 on the other hand was expanded using the user context of that request, to "http://a.b.c/attrs/state" and that
is not found anywhere in the core context that the GET request used. So, impossible to compact, the long name is returned.

"status" for both entities was expanded using the Core context and thus always found for compaction.
"state2" was expanded using the default URL and thus always found for compaction.

It's tricky, I know.
Please read through this example again, if needed, until it is 100% clear.

### Entity Retrieval Example 2 - with context
We said earlier that a context only lives within its own request.
This is true. Otherwise I wouldn't have said it! :)
However, requests with inline contexts that is not just a simple string, or that can't be reduced (more about that later)
to a simple string are saved by Orion-LD. Saved to later be served, if asked.

The context used in a request is normally returned in the response, and most often in the Link HTTP header.
For example, the response to a creation of an entity has no payload data, so the context is returned in the HTTP Link header.
The reason for this is for eventual proxy servers to be aware of the context used.

So, if an entity creation is issued with an inline context, like the one in "Entity Creation Example 1", how can the
broker return that complex context in the Link header?
Remember that the Link header must be a string that is a URL.

What Orion-LD does is that it creates a new context.
A context cache is already maintained inside the broker, to avoid to download the very same context over and over again. That would be silly!
So, in this situation, the incoming complex context is created inside the broker's context cache and it is given a name.
The full URL to reach this context is then included in the HTTP headers of the response, in the Link header, of course.

#### CURL
The context can later be retrieved with the following command:
```bash
GET /ngsi-ld/v1/jsonldContexts/<context-id>
```

So, let's try this, using the option `--dump-header` of `curl` so that we save the HTTP headers - that's where we'll find the URL to retrieve the context later:
```bash
payload='{
  "@context": {
    "P1": "http://a.b.c/attrs/P1",
    "P2":  "http://a.b.c/attrs/P2"
  },
  "id": "urn:entities:E3",
  "type": "T",
  "P1": {
    "type": "Property",
    "value": "P1 - from user context"
  },
  "P2": {
    "type": "Property",
    "value": "P2 - from user context"
  },
  "P3": {
    "type": "Property",
    "value": "P2 - from Default URL"
  }  
}'
curl localhost:1026/ngsi-ld/v1/entities -d "$payload" -H "Content-Type: application/ld+json" --dump-header /tmp/httpHeaders
```

Now check out the HTTP headers:
```bash
cat /tmp/httpHeaders
HTTP/1.1 201 Created
Connection: Keep-Alive
Content-Length: 0
Link: <http://xps:1026/ngsi-ld/v1/jsonldContexts/9ad26df4-0d28-11ea-98e1-9cb6d0961dcd>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
Location: /ngsi-ld/v1/entities/urn:entities:E3
Date: Fri, 22 Nov 2019 13:04:23 GMT
```
Especially, check out the **Link** header:
```
cat /tmp/httpHeaders | grep ^Link
Link: <http://xps:1026/ngsi-ld/v1/jsonldContexts/9ad26df4-0d28-11ea-98e1-9cb6d0961dcd>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
```

You will get a different ID (the ID in this example happens to be `9ad26df4-0d28-11ea-98e1-9cb6d0961dcd`), and a different hostname (mine is: `xps`,
but the rest will be the same, and you **will** be able to retrieve the context with a command similar to this
(change `9ad26df4-0d28-11ea-98e1-9cb6d0961dcd` for the ID you got previously):
```bash
curl localhost:1026/ngsi-ld/v1/jsonldContexts/9ad26df4-0d28-11ea-98e1-9cb6d0961dcd
```

Not only can you retrieve the context, but you can also use it in subsequent requests, for example a GET request.

So, let's retrieve the entity urn:entities:E3 to see the attribute names with and without the context used during creation.

Without context (expect long names):
```bash
curl localhost:1026/ngsi-ld/v1/entities/urn:entities:E3?prettyPrint=yes
```
Here's the response:
```json
{
  "@context": "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
  "id": "urn:entities:E3",
  "type": "T",
  "http://a.b.c/attrs/P1": {
    "type": "Property",
    "value": "P1 - from user context"
  },
  "http://a.b.c/attrs/P2": {
    "type": "Property",
    "value": "P2 - from user context"
  },
  "P3": {
    "type": "Property",
    "value": "P2 - from Default URL"
  }
}
```

[ Don't get confused by seeing the @context member in the payload - remember that curl adds "Accept: */*" and that the broker decides to respond with "application/ld+json" ]

With the context used during creation (expect short names) - change hostname 'xps' and context ID '9ad26df4-0d28-11ea-98e1-9cb6d0961dcd' to your own values:
```bash
curl 'xps:1026/ngsi-ld/v1/entities/urn:entities:E3?prettyPrint=yes' -H 'Link: <http://localhost:1026/ngsi-ld/v1/jsonldContexts/9ad26df4-0d28-11ea-98e1-9cb6d0961dcd>;rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"'
```
Here's the response:
```json
{
  "@context": "http://xps:1026/ngsi-ld/v1/jsonldContexts/9ad26df4-0d28-11ea-98e1-9cb6d0961dcd",
  "id": "urn:entities:E3",
  "type": "T",
  "P1": {
    "type": "Property",
    "value": "P1 - from user context"
  },
  "P2": {
    "type": "Property",
    "value": "P2 - from user context"
  },
  "P3": {
    "type": "Property",
    "value": "P2 - from Default URL"
  }
}
```

#### Python
```
payload = {
    "@context": {
        "status": "http://a.b.c/attrs/status",
        "state":  "http://a.b.c/attrs/state"
    },
    "id": "urn:entities:E3",
    "type": "T",
    "status": {
        "type": "Property",
        "value": "From Core Context"
    },
    "state": {
        "type": "Property",
        "value": "From User Context"
    },
    "state2": {
        "type": "Property",
        "value": "From Default URL"
    }
}

response = requests.post(url="http://localhost:1026/ngsi-ld/v1/entities", headers={
    "content-type": "application/ld+json"}, data=json.dumps(payload))
print(response.headers)
```

Result: 
```
{'Connection': 'Keep-Alive', 'Content-Length': '0', 'Location': '/ngsi-ld/v1/entities/urn:entities:E3', 'Date': 'Tue, 18 Feb 2020 
18:42:24 GMT'}
```

#### JavaScript (Node.js)
```
const axios = require('axios')
const payload = {
    "@context": {
        "status": "http://a.b.c/attrs/status",
        "state": "http://a.b.c/attrs/state"
    },
    "id": "urn:entities:E3",
    "type": "T",
    "status": {
        "type": "Property",
        "value": "From Core Context"
    },
    "state": {
        "type": "Property",
        "value": "From User Context"
    },
    "state2": {
        "type": "Property",
        "value": "From Default URL"
    }
}
axios
    .post("http://localhost:1026/ngsi-ld/v1/entities", payload, 
    { headers: { "content-type": "application/ld+json" } })
    .then(res => console.log(res.headers))
    .catch(err => console.log(err))
```
Result:
```
{ 
  connection: 'close',
  'content-length': '0',
  location: '/ngsi-ld/v1/entities/urn:entities:E3',
  date: 'Tue, 18 Feb 2020 18:52:47 GMT'
}
```

### Reducing of a context (only valid for the non-ngsild-compliant version of Orion-LD)
As explained, any NGSI-LD broker has the Core Context "omnipresent". No way to get rid of it. No way to override it.
Also, no meaning whatsoever to send it as part of the user context.
However, it seems to be common pravtice to include it, normally to have the request compatible to JSON-LD.
Let me just say one thing:
**NGSI-LD is not 100% JSON-LD**
It is similar, as similar as possible, but NGSI-LD has a default context, namely the Core context and there is no need to feed the broker with it.
Doing that is just a waste of time.
So, what Orion-LD does to minimize the waste of time is the following:
* If the user provided context is a simple string and the value of that string is the URL of the Core Context - then the request is treated as if it didn't have any user-provided context.
  The alternative would be to lookup inside the user context (that is the core context) and then lookup in the core context only to override what was just found.
  - TWO lookups in the Core context ... bad idea
* If the user provided context is an array containing a string whose value is the core context, then that item is REMOVED from the array.
* If the user provided context is an array **with only one item**, then instead of an array, the user-context is treated as that sole item (which could be a string or an object.

Especially note that the following user context:
```json
  "@context": [
    "any URL",
    "URL to core context"
  ]
```
will be transformed into this:
```json
  "@context": "any URL"
```

It's obvious that this transformation makes for a faster lookup, right?

## Filtering results
The NGSI-LD API does not allow for a `GET /ngsi-ld/v1/entities` to be issued without any restraining.
One of the following URI parameters must be present for the request to be allowed:
* ?type=<TYPE>
* ?attrs=<A1,A2,...An>
* ?id=<ID1, ID2, ... IDn>
* ?q=<Q-Filter>

This is to avoid *too many* entities to be returned by the broker (flooding the user).
Personally, I don't agree to all this.
The non-ngsild-compliant version of Orion-LD doesn't have this restriction.
To avoid flooding, an NGSI-LD broker implements _pagination_.

### Pagination
Using pagination, you can tell the broker to return the entities from a start index and you define the number of entities to be returned.
This is done using two URI parameters:
* offset
* limit

The default values of these two are:
* offset = 0
* limit  = 20

I.e., return the entities from the beginning, and a maximum of 20 entities.
To really prevent flooding, the URI parameter 'limit' isn't allowed a value over 1000.

Pagination isn't trivial, as new entities may very well be created between retievals.
Fortunately, there is a trivial fix to this problem - the entities are sorted by age.
So, if new entities come in, they are added to the "end of the list".

Pagination example to retrieve entities 12-52:

#### CURL
```
curl localhost:1026/ngsi-ld/v1/entities?type=T&offset=12&limit=40
```
#### Python
```bash
import requests

response = requests.get(url='http://localhost:1026/ngsi-ld/v1/entities?type=T&offset=12&limit=40')
print(response.json())
```
#### JavaScript (Node.js)
```bash
const axios = require('axios')
axios.get('http://localhost:1026/ngsi-ld/v1/entities?type=T&offset=12&limit=40')
        .then(res => console.log(res.data))
        .catch(err => console.log(err))
```

Note the `type=T`.
As was mentioned earlier, some limitation of the total number of entities is required by the NGSI-LD spec.

### Filtering by Entity Type
This is already mentioned in examples, but, using the URI parameter *type*, you can tell the broker to return only entities of a specific entity type:
#### CURL
```
curl localhost:1026/ngsi-ld/v1/entities?type=T
```
#### Python
```bash
import requests

response = requests.get(url='http://localhost:1026/ngsi-ld/v1/entities?type=T')
print(response.json())
```
#### JavaScript (Node.js)
```bash
const axios = require('axios')
axios.get('http://localhost:1026/ngsi-ld/v1/entities?type=T')
        .then(res => console.log(res.data))
        .catch(err => console.log(err))
```
Also mentioned is that you can supply the fully qualified name of the type:
#### CURL
```
curl localhost:1026/ngsi-ld/v1/entities?type=http:/www.mypage.org/entityTypes/T
```
#### Python
```bash
import requests

response = requests.get(url='http://localhost:1026/ngsi-ld/v1/entities?type=http:/www.mypage.org/entityTypes/T')
print(response.json())
```
#### JavaScript (Node.js)
```bash
const axios = require('axios')
axios.get('http://localhost:1026/ngsi-ld/v1/entities?type=http:/www.mypage.org/entityTypes/T')
        .then(res => console.log(res.data))
        .catch(err => console.log(err))
```
The advantage with this is that you no longer need to supply the context (which would otherwise be necessary to expand T into http:/www.mypage.org/entityTypes/T)

### Restricting attributes
Perhaps an entity you are interested in has plenty of attributes and you only want to see attributes A1 and A6.
There is a URI parameter to do this. It is called `attrs` and it is a list of attribute names:
#### CURL
```
curl localhost:1026/ngsi-ld/v1/entities?attrs=A1,A6
```
#### Python
```bash
import requests

response = requests.get(url='http://localhost:1026/ngsi-ld/v1/entities?attrs=A1,A6')
print(response.json())
```
#### JavaScript (Node.js)
```bash
const axios = require('axios')
axios.get('http://localhost:1026/ngsi-ld/v1/entities?attrs=A1,A6')
        .then(res => console.log(res.data))
        .catch(err => console.log(err))
```

### Query Language
The URI parameter `q` is a very powerful tool for filtering results.
You can for example query for entities that have:
* an attribute called X  (?q=X)
* no attribute called X (?q=!X)
* an attribute X that is equal to 50 (?q=X==50)
* an attribute X that is equal to some of the values in a list (?q=X==50,60,70,114)
* an attribute X that has a value inside a range (?q=X==50..74)

All these examples are with Integers, but you can also use Booleans and Strings.
You can also query over subattributes, or conditions inside the values of compound values (a compound value for an attribute is a valkue that is either an Array of an Object).
You can also combine query-conditions using AND and OR and ...

Please take a look at the [ETSI NGSi-LD specification](https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf) for a full explication of all this.
