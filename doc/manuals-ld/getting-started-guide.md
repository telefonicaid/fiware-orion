# Orion-LD Quick Start Guide

Welcome to the Quick Start Guide to Orion-LD, the NGSI-LD context broker!

This guide is a walk-through of the most common characteristics of Orion-LD, with plenty of examples.
It would be a good idea to have an instance of Orion-LD that you can start and restart for this exercise.

We will learn about contexts, and how to:
- Use `curl` to send HTTP requests to Orion-LD
- Use the `mongo` command tool to inspect the contents of the database
- Create entities with contexts
- Retrieve entities with different contexts - to see "different results" for the very same entity retrieval !!!
- Filtering of the results - get only the entities that match your criteria
- Subscriptions and Notifications

## Contexts
A _context_, in its simplest form, is nothing but a list of key-values:

```json
"@context": {
  "P1": "https://a.b.c/attributes/P1",
  "P2": "https://a.b.c/attributes/P2",
  "P3": "https://a.b.c/attributes/P3",
  ...
}
```
The key (e.g. "P1") is an alias for the longer name ("https://a.b.c/attributes/P1" for the key "P1").
That's all there is to it, just a way of avoiding to write really long strings and also, importantly, a freedom for the user to
defines his/her own aliases for the longnames.

When is this useful?
To write shortnames instead of longnames, the advantage is clear.

But, what about the "freedom for the user to defines his/her own aliases"?
Nothing like an example to illustrate this:

```
I am a father to my children and a husband to my wife, a son to my parents and a grandson to my grandparents. Etc.

The way I see my three children is that the are my children.
The way my three children see eachother is that the other two are their siblings.

The way I see my mother is that she's my mother.
The way my wife sees my mother is that she's her mother-in-law.
```
Same human beings, different viewpoints.
That's pretty much what a context is, a viewpoint

What is expanded, with the help of the context, inside the payload data is:
* The Entity Type
* The Property Names
* The Relationship Names

However, contexts can be a little more complex, where the value is an object instead of just a string:
```json
"@context": {
  "P1": {
    "@id": "https://a.b.c/attributes/P1",
    "@type": "vocab"
  },
  ...
```

If an alias has a complex value in the @context, and that complex value contains a member "@type" that equals "@vocab",
then also the value of that attribute (an attribute is a Property or a Relationship) is expanded according to the context.
But, only if found inside that very same context. If not found, the value is untouched.

If you want to learn more about contexts, please refer to documentation on JSON-LD. There is plenty out there ...

## Creation of Entities
