# Transient Entities

A transient entity is a regular entity (i.e. it has id/type, a set of attributes, etc.) but with an expiration timestamp.
When such point in time is reached the entity is automatically deleted from the context managed by Orion.

Thus, a first and very important piece of advice: **be careful if you use transient entities as once
the expiration time has come, the entity will be automatically deleted from database and there is
no way of recovering it**. Ensure the information you set in a transient entity is not relevant once 
the entity has expired (i.e. deleted).

In addition, **have a look to the [backward compatibility considerations section](#backward-compatibility-considerations)
in the case you are already using attributes with the exact name `dateExpires`**.

## The `dateExpires` attribute

The expiration timestamp of an entity is defined by means of the `dateExpires` NGSIv2 builtin attribute. This is an
attribute of `DateTime` type, according to the [NGSIv2 specification](https://fiware.github.io/specifications/ngsiv2/stable/).
Its value is the datetime when the entity will expire.

As any other NGSIv2 builtin attribute, `dateExpires` is not shown by default, you need to use `attrs` URI parameter (in GET
based queries) or `"attrs"` field (in `POST /v2/op/query`) in order to get it. Please have a look to sections "Builtin Attributes"
and "Filtering out attributes and metadata" in the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable)
for more details.

## Valid transitions

### Create entity with `dateExpires` attribute

An entity is created with transient nature if it includes the `dateExpires` attribute. For instance:

```
POST /v2/entities
{
  "id": "t1",
  "type": "Ticket",
  ...
  "dateExpires": {
    "value": "2028-07-07T21:35:00Z",
    "type": "DateTime"
  }
}
```

creates an entity that will expire at July 7th, 2028 at 21:35 UTC.

Additional considerations:

* `dateExpires` has to have a valid `DateTime` value (check NGSIv2 specification for details).
Otherwise a 400 Bad Request would be returned.

* If `dateExpires` is set in the past, the entity is created expired (a bit weird, but functionally correct).

### Add `dateExpires` attribute to entity that previously doesn't have it

We can add the `dateExpires` attribute to a regular entity (e.g. "t2"). For instance:

```
POST /v2/entities/t2/attrs
{
  "dateExpires": {
    "value": "2028-10-12T14:23:00Z",
    "type": "DateTime"
  }
}
```

will make that entity to expire on October 12th, 2028 at 14:23 UTC.

Additional considerations:

* `dateExpires` has to have a valid `DateTime` syntax (check NGSIv2 specification for details).
Otherwise a 400 Bad Request would be returned.

* If `dateExpires` is set in the past, the entity gets automatically expired.

### Update `dateExpires` attribute in entity that previously has it

Context Broker allows several ways of updating an attribute value (check NGSIv2 specification for
details). For instance using PUT on the attribute resource URL:

```
PUT /v2/entities/t2/attrs/dateExpires
{  
  "value": "2028-12-31T23:59:00Z",
  "type": "DateTime"
}
```

will change expiration date to December 31th, 2028 at 23:59 UTC.

Additional considerations:

* `dateExpires` has to have a valid `DateTime` syntax (check NGSIv2 specification for details).
Otherwise a 400 Bad Request would be returned.

* If `dateExpires` is set in the past, the entity gets automatically expired

### Remove `dateExpires` attribute from entity

Finally, you can remove `dateExpires` attribute from a transient entity:

```
DELETE /v2/entities/t2/attrs/dateExpires
```

This become the entity into a regular (i.e. not transient) entity and will not be deleted due to expiration. 

## Deletion of expired entities

Expiration relies on the MongoDB feature to [expire documents at a specific clock time](https://docs.mongodb.com/manual/tutorial/expire-data/#expire-documents-at-a-specific-clock-time). This is based in a background thread that wakes up every 60 seconds,
so your transient entities may remain in the database up to 60 seconds (or a bit more, if the MongoDB load is high) after
expiration date (see [MongoDB documentation](https://docs.mongodb.com/manual/core/index-ttl/#timing-of-the-delete-operation)
for more details).

The default sleep interval for the TTL monitor thread can be changed in MongoDB, but that topic is out of the scope of this
document. Have a look to [this link](http://hassansin.github.io/working-with-mongodb-ttl-index#ttlmonitor-sleep-interval) for more detail.

**Once a transient entity is removed, it cannot be recovered.**

## Backward compatibility considerations

Transient entity were introduced in Orion 1.15.0. Up to Orion 1.14.0 `dateExpires` is interpreted as a regular
attribute without any special semantic. So, what would happen in the case you are already using an attribute
named `dateExpires` in your application before upgrading to Orion 1.15.0?

Existing entities using `dateExpires` will keep using it in the same way until the attribute gets updated.
That is, if `dateExpires` is not a `DateTime` (e.g. a number, a regular string, etc.) it will keep with
the same value (e.g. in GET operations, etc.). If `dateExpires` is a `DateTime` that datetime will not be
interpreted as an expiration date (i.e. the entity will not be deleted after the datetime passes).

However, even in the case the attribute would keep its previous value without any special semantic, note
that `dataExpires` becomes a builtin attribute, so it is not shown except if explicitly requested with
`attrs` URI parameter (in GET based queries) or `"attrs"` field (in `POST /v2/op/query` and subscriptions).

Once `dateExpires` attribute get updated for first time, it will start to mean an expiration date on the given
entity, with the behaviour described in previous section. Please, **take this into account in the case
you were implementing client-side expiration based on the value of that attribute, as your entities could
be automatically deleted in an unwanted way**.
