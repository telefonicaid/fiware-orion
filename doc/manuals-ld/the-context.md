# A Guide to The Context
A _context_, in its simplest form, is nothing but a list of key-values:

```json
"@context": {
  "P1": "https://a.b.c/attributes/P1",
  "P2": "https://a.b.c/attributes/P2",
  "P3": "https://a.b.c/attributes/P3",
  ...
}
```

The key (e.g. "P1") is an alias for the longer name ("https://a.b.c/attributes/P1").
That's all there is to it, just a way of avoiding to write really long strings and also, importantly, a freedom for the user to
define his/her own aliases for the longnames.

When is this useful?
To write short names instead of long names, the advantage is clear.

But, what about the "freedom for the user to define his/her own aliases"?
Nothing like an example to illustrate this:

```
I am a father to my children and a husband to my wife, a son to my parents and a grandson to my grandparents.

The way I see my three children is that the are my children.
The way my three children see each other is that the other two are their siblings.

The way I see my mother is that she's my mother.
The way my wife sees my mother is that she's her mother-in-law.
```
Same human beings, different "viewpoints".
That's pretty much what a context is, a "viewpoint".

## Expansion and Compaction
An NGSi-LD broker uses the context to expand and compact the shortnames that are part of the payload data or
that comes in as a URI parameter.

What is expanded/compacted, with the help of the context, inside the payload data is:
* The Entity Type
* The Property Names (in all levels)
* The Relationship Names (in all levels)

The term **Attribute** is used to refer to Properties, Relationships, Properties-of-Properties, etc.

So, for example, if you issue a query `GET /ngsi-ld/v1/entities?type=T2`, then T2 will be expanded according to the current context
and then looked up to find any matching entities.
All entities are stored in a fully expanded way. Can't be any other way. The expanded form is the **real** value of the entity type, attribute name, value, etc.
Before returning the resulting payload, all expandable/compactable items (entity type, attribute name, value, etc.) are compacted according to the context.

So, in this GET example:
* T2 is expanded (to "https://uri.etsi.org/ngsi-ld/default-context/T2", for example)
* All entities with that type are retrieved from storage
* All those entities are compacted (attribute names, entity type ...) => "https://uri.etsi.org/ngsi-ld/default-context/T2" goes back to being "T2"
* The response is composed from the compacted entities

Orion-LD uses MongoDB for storage and if you look inside, you will see the expanded form of the entities.


## The Core Context
Orion-LD, just like any NGSI-LD compliant broker, has a built-in, default context.
This built-in context is called the **Core Context** and it is found [here](https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld).

The Core Context defines the core of any NGSI-LD broker and its definitions (key-values) override any user-supplied definition.
When a term (entity type or attribute name) is looked up for expansion or compaction in the context, the entire context is searched to the end, and the last hit
overrides any previous hit. The Core Context is the last context to be searched, and it will thus override any other context.


## The Default URL
What if a term isn't found anywhere, during the lookup in the context?
Well, all entity types and attribute names **are** expanded, and if no match is found in the provided contest (nor in the Core Context),
then a special field - "@vocab" - of the Core Context is used.
The value of "@vocab" is **The Default URL**.
If you look inside the Core Contest, ypou'll find "@vocab":

```json
"@vocab": "https://uri.etsi.org/ngsi-ld/default-context/"
```

So, if an attribute has the name "P13" and "P13" isn't found antwhere, then "P13" will end up expanded according to the Default URL:

```text
"https://uri.etsi.org/ngsi-ld/default-context/P13"
```


## Content-Type and the Context
There are two different ways to supply the context in a NGSI-LD request:

* via an HTTP header called `Link`,
* as part of the payload.

If the context is passed in the `Link` HTTP header, then the `Context-Type` must be **application/json**.
If in the payload data, then the `Context-Type` must be **application/ld+json**.
If this is not fulfilled, Orion-LD will respond with an error.


## Passing the Context in an Orion-LD Request
To pass the context `http://context.store.es/myContexts/context1.jsonld` in the `Link` header using `curl` (e.g. to create an entity):

```bash
export payload='{ "id": "xxx", "type": "TTT", ... }'
curl localhost:1026/ngsi-ld/v1/entities -d "$payload" -H 'Link: <http://context.store.es/myContexts/context1.jsonld>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"' -H "Content-Type: application/json"
```

If instead you wish to pass the context inside the payload data, it may look like this:
```bash
export payload='{ "id": "xxx", "type": "TTT", "@context": "url-to-context", ... }'
curl localhost:1026/ngsi-ld/v1/entities -d "$payload" -H "Content-Type: application/ld+json"
```

## Compound Contexts
Contexts inside the payload data can be expressed in a variety of ways:
* A JSON String (a URL that refers the context)
    ```json
    "@context": "url-to-context"`
    ```
* A JSON Object that *is* the context, i.e. a list of key-values
    ```json
    "@context":	{
      "P1": "https://a.b.c/attributes/P1",
      "P2": "https://a.b.c/attributes/P2",
      ...
    }
    ```
* A JSON Array of string (that are URLs refering contexts)
    ```json
    "@context":	[
      "url-to-context1",
      "url-to-context2",
      ...
    ]
    ```
* A JSON Array of a combination of strings and objects.
    ```json
    "@context":	[
      "url-to-context1",
      {
        "P1": "https://a.b.c/attributes/P1",
        "P2": "https://a.b.c/attributes/P2",
        ...
      }
    ]
    ```


## Value Expansion
The value of a key-value in a context can be a little more complex that just a string. E.g.:
```json
"@context": {
  "P1": {
    "@id": "https://a.b.c/attributes/P1",
    "@type": "vocab"
  },
  ...
}
```

If an alias has a complex value in the @context, and that complex value contains a member "@type" that equals "@vocab",
then also the value of that attribute (an attribute is a Property or a Relationship) is expanded according to the context.
But, only if found inside that very same context. If not found, the value is untouched.


If you want to learn more about contexts, please refer to documentation on JSON-LD. There is plenty out there.
