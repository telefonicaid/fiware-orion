# Subscriptions based in alteration type

By default, a subscription is triggered (i.e. the notification associated to it is sent) when
the triggered condition (expressed in the `subject` and `conditions` fields of the subscription, e.g.
covered entities, list of attributes to check, filter expression, etc.) during a create or actual
update entity operation.

However, this default behaviour can be changed so a notification can be sent, for instance,
only when an entity is created or only when an entity is deleted, but not when the entity is
updated.

In particular, the `alterationTypes` field is used, as sub-field of `conditions`. The value
of this field is an array which elements specify a list of alteration types upon which the
subscription is triggered. At the present moment, the following alteration types are supported:

* `entityUpdate`: notification is sent whenever a entity covered by the subscription is updated
  (no matter if the entity actually changed or not)
* `entityChange`: notification is sent whenever a entity covered by the subscription is updated
  and it actually changes (or if it is not an actual update, but [`forcedUpdate` option](ngsiv2_implementation_notes.md#forcedupdate-option)
  is used in the update request)
* `entityCreate`: notification is sent whenever a entity covered by the subscription is created
* `entityDelete`: notification is sent whenever a entity covered by the subscription is deleted

For instance:

```
  "conditions": {
    "alterationTypes": [ "entityCreate", "entityDelete" ],
    ...
  }
```

will trigger subscription when an entity creation or deletion takes place, but not when an
update takes place. The elements in the `alterationTypes` array are interpreted in OR sense.

Default `alterationTypes` (i.e. the one for subscription not explicitly specifying it)
is `["entityCreate", "entityChange"]`.

The particular alteration type can be got in notifications using the
[`alterationType` builtin attribute](ngsiv2_implementation_notes.md#alterationtype-attribute).
