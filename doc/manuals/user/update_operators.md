# Update operators for attribute values

## Introduction

The usual way of modifying context with Orion is through attribute updates with particular values.
However, in some cases, it is better to provide an operation to be evaluated on the existing attribute
value. Let's see it with an example.

Let's consider an entity `AccessControl1` with an attribute `count` that measures the number of people
that crosses the access control. A context-aware application is in charge of updating context
(eg. based in a sensor measures). Whenever somebody crosses the access, the application has to:

* Read the value of the attribute, e.g. `GET /v2/entities/attrs/count`. Let's consider the value is 43
* Calculate the  new value (43 plus 1 is 44)
* Update the attribute with the new value, e.g. `PUT /v2/entities/attrs/count { "value": 44, "type": "Number" }`

In sum, the application has to read, calculate and update which increases its complexity.

The problem gets worse if several context-aware applications are accessing simultaneously to the same
piece of context. For instance, instead of a single access count, we have a `Zone1` which has an
attribute `count` with an aggregate of all the people that enters into the zone no matter which
access (assuming that the zone has many accesses).

We have application A managing one of the accesses and application B managing other of the accesses.
Both applications update `count` attribute in `Zone1` when somebody crosses the respective accesses.
Most of the time there is no problem:

* Somebody crosses access managed by A
* Application A does the read-calculate-cycle and update `count` in `Zone1`. Let's consider the values
  was 43 before updating, now `count` value is 44
* After some time, somebody crosses access managed by B
* Application B does the read-calculate-cycle and update `count` in `Zone1`. So now `count` value is 45

However, if both access-crossing events occurs very close in time problems may occur:

* Somebody crosses access A at the same time somebody crosses access B
* Application A reads count 43
* Application B reads count before A can calculate and update, so getting same value 43
* Application A sum 1 to 43 and update 44
* Application B does the same, and update 44
* Thus, the count is wrong: it should be 45 but it's 44!

We call this kind of problem *race condition* (due to the different speed in which events take place in
application A and B). It can be solved with the Orion update operators functionality, in particular
with the increment operator.

So, both applications instead of updating directly a particular value (such as 44 or 45) send a
*"increment by 1"* update, this way:

```
POST /v2/entities/Zone1/attrs/count
{
  "value": { "$inc": 1 },
  "type": "Number"
}
```

The same case would be as follows now:

* At a given moment count in Zone1 is 43 and somebody crosses access A at the same time somebody
  crosses access B
* Application A updates `count` with `{"$inc": 1}`
* Application B updates `count` with `{"$inc": 1}`
* Orion ensures that operations are executed atomically. It does not matter if the increment done
by application A comes before increment done by application B or viceversa. The result is the same: 45

Thus, update operators solve both problems:

* Complexity. Now applications do not need to do a read-calculate-update cycle. Just updating
  (with the operator) is enough.
* Race conditions. This is solved due to Orion ensures that these operations are executed atomically
  on the context attributes. Many applications can update the same piece of context without problems.

## Supported operators

Orion update operators are based on a subset of the ones implemented by MongoDB
(described [here](https://docs.mongodb.com/manual/reference/operator/update/)). A description follows.

### `$inc`

Increase by a given value.

For instance, if the preexisting value of attribute A in entity E is 10 the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$inc": 2 },
  "type": "Number"
}
```

would change the value of attribute A to 12.

This operator only accept numeric values (either positive or negative, integer or decimal).

### `$mul`

Multiply by a given value

For instance, if the preexisting value of attribute A in entity E is 10 the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$mul": 2 },
  "type": "Number"
}
```

would change the value of attribute A to 20.

This operator only accept numeric values (either positive or negative, integer or decimal).

### `$min`

Updates value if current value is greater than the one provides.

For instance, if the preexisting value of attribute A in entity E is 10 the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$min": 2 },
  "type": "Number"
}
```

would change the value of attribute A to 2. However, the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$min": 20 },
  "type": "Number"
}
```

would not change attribute value.

Apart from numbers, other value types are supported (eg, strings).

### `$max`

Updates value if current value is lesser than the one provides.

For instance, if the preexisting value of attribute A in entity E is 10 the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$max": 12 },
  "type": "Number"
}
```

would change the value of attribute A to 12. However, the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$max": 4 },
  "type": "Number"
}
```

would not change attribute value.

Apart from numbers, other value types are supported (eg, strings).

### `$push`

To be used with attributes which value is an array, add an item to the array.

For instance, if the preexisting value of attribute A in entity E is `[1, 2, 3]` the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$push": 3 },
  "type": "Number"
}
```

would change the value of attribute A to `[1, 2, 3, 3]`

### `$addToSet`

Similar to push but avoids duplications.

For instance, if the preexisting value of attribute A in entity E is `[1, 2, 3]` the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$addToSet": 4 },
  "type": "Number"
}
```

would change the value of attribute A to `[1, 2, 3, 4]`. However, the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$addToSet": 3 },
  "type": "Number"
}
```

would not change attribute value.

### `$pull`

To be used with attributes which value is an array, removes all occurrences of the item
passed as parameter.

For instance, if the preexisting value of attribute A in entity E is `[1, 2, 3]` the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$pull": 2 },
  "type": "Number"
}
```

would change the value of attribute A to `[1, 3]`.

### `$pullAll`

To be used with attributes which value is an array. The parameter is also an array. All
the occurrences of any of the members of the array used as parameter are removed.

For instance, if the preexisting value of attribute A in entity E is `[1, 2, 3]` the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$pullAll": [2, 3] },
  "type": "Number"
}
```

would change the value of attribute A to `[1]`.

### `$set`

To be used with attributes which value is an object to add/update a sub-key in the
object without modifying any other sub-keys.

For instance, if the preexisting value of attribute A in entity E is `{"X": 1, "Y": 2}` the
following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$set": {"Y": 20, "Z": 30} },
  "type": "Number"
}
```

would change the value of attribute A to `{"X": 1, "Y": 20, "Z": 30}`.

For consistence, `$set` can be used with values that are not an object, such as:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$set": "foo" },
  "type": "Number"
}
```

which has the same effect than a regular update, i.e.:

```
POST /v2/entities/E/attrs/A
{
  "value": "foo",
  "type": "Number"
}
```

### `$unset`

To be used with attributes which value is an object to remove a sub-key from the
object without modifying any other sub-keys.

For instance, if the preexisting value of attribute A in entity E is `{"X": 1, "Y": 2}` the
following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$unset": {"X": 1} },
  "type": "Number"
}
```

would change the value of attribute A to `{"Y": 20}`.

The actual value of the sub-key used with `$unset` is not relevant. A value of 1 is recommented
for simplity but the following request would also work and would be equivalent to the one above:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$unset": {"X": null} },
  "type": "Number"
}
```

Note that if the value of `$unset` is not an object, it will be ignored. Not existing sub-keys
are also ignored.

### Combining `$set` and `$unset`

You can combine the usage of `$set` and `$unset` in the same attribute update.

For instance, if the preexisting value of attribute A in entity E is `{"X": 1, "Y": 2}` the
following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$set": {"Y": 20, "Z": 30}, "$unset": {"X": 1} },
  "type": "Number"
}
```

would change the value of attribute A to `{"Y": 20}`.

The sub-keys in the `$set` value cannot be at the same time in the `$unset` value or
the other way around. For instance the following request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$set": {"X": 20, "Z": 30}, "$unset": {"X": 1} },
  "type": "Number"
}
```

would result in error.

## How Orion deals with operators

Orion doesn't execute the operation itself, but pass it to MongoDB, which is the one actually
executing in the attribute value stored in the database. Thus, the execution semantics are the
ones described in [MongoDB documentation](https://docs.mongodb.com/manual/reference/operator/update/)
for the equivalent operands.

If the operation results in error at MongoDB level, the error is progressed as is as a
500 Internal Error in the client response. For instance, `$inc` operator only support numerical values in
MongoDB. So if we send this request:

```
POST /v2/entities/E/attrs/A
{
  "value": { "$inc": "foo" },
  "type": "Number"
}
```

The result would be this error:

```
500 Internal Server Error

{"error":"InternalServerError","description":"Database Error &#40;collection: orion.entities - update&#40;&#41;: &lt;{ &quot;_id.id&quot; : &quot;E&quot;, &quot;_id.type&quot; : &quot;T&quot;, &quot;_id.servicePath&quot; : &quot;/&quot; },{ &quot;$set&quot; : { &quot;attrs.A.type&quot; : &quot;Number&quot;, &quot;attrs.A.mdNames&quot; : [  ], &quot;attrs.A.creDate&quot; : 1631801113.0986146927, &quot;attrs.A.modDate&quot; : 1631801407.5359125137, &quot;modDate&quot; : 1631801407.5359227657, &quot;lastCorrelator&quot; : &quot;cbe6923c-16f7-11ec-977e-000c29583ca5&quot; }, &quot;$unset&quot; : { &quot;attrs.A.md&quot; : 1, &quot;location&quot; : 1, &quot;expDate&quot; : 1 }, &quot;$inc&quot; : { &quot;attrs.A.value&quot; : &quot;foo&quot; } }&gt; - exception: Cannot increment with non-numeric argument: {attrs.A.value: &quot;foo&quot;}&#41;"}
```

which decoded is:

```
"error":"InternalServerError","description":"Database Error (collection: orion.entities - update(): <{ "_id.id" : "E", "_id.type" : "T", "_id.servicePath" : "/" },{ "$set" : { "attrs.A.type" : "Number", "attrs.A.mdNames" : [  ], "attrs.A.creDate" : 1631801113.0986146927, "attrs.A.modDate" : 1631801407.5359125137, "modDate" : 1631801407.5359227657, "lastCorrelator" : "cbe6923c-16f7-11ec-977e-000c29583ca5" }, "$unset" : { "attrs.A.md" : 1, "location" : 1, "expDate" : 1 }, "$inc" : { "attrs.A.value" : "foo" } }> - exception: Cannot increment with non-numeric argument: {attrs.A.value: "foo"})"}
```

and if we look at the end, we can see the error reported by MongoDB:

```
Cannot increment with non-numeric argument: {attrs.A.value: "foo"})"}
```

In addition, note that Orion assumes that the value for the attribute in the request
is a JSON object which just one key (the operator). If you do a weird thing something like this:

```
POST /v2/entities/E/attrs/A
{
  "value": {
    "x": 1
    "$inc": 1,
    "$mul": 10
  },
  "type": "Number"
}
```

you will get (randomly, in principle) one among this ones:

* A gets increased its value by 1
* A gets multiply its value by 10
* A gets is value updated to (literally) this JSON object: `{ "x": 1, "$inc": 1, "$mul": 10 }`

So be careful of avoiding these situations.

The only exception to "use only one operator" rule is the case of `$set` and
`$unset`, that can be used together [as described above](#combining-set-and-unset).

## Current limitations

### Create or replace entities

Update operators cannot be used in entity creation or replace operations. For instance if
you create an entity this way:

```
POST /v2/entities/E/attrs/A
{
  "id": "E",
  "type": "T",
  "A": {
    "value": { "$inc": 2 },
    "type": "Number"
  }
}
```

the attribute A in the just created entity will have as value (literally) this JSON object: `{ "$inc": 2 }`.

However, note that the case of adding new attributes to existing entities will work. For instance if
we already have an entity E with attributes A and B and we append C this way:

```
POST /v2/entities/E
{
  "C": {
    "value": { "$inc": 2 },
    "type": "Number"
  }
}
```

then C will be created with value `2`.
