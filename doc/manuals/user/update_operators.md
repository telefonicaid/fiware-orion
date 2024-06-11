# Update operators for attribute values

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
