# Updating registrations

The response to a register context request (both in
[standard](walkthrough_apiv1.md#register_context_operation) and
[convenience](walkthrough_apiv1.md#convenience_context_operation)) includes a
registration ID (a 24 hexadecimal digit number):

``` 
{
    "duration": "PT24H",
    "registrationId": "51bf1e0ada053170df590f20"
}
``` 

This ID can be used to update the registration. There is no special
operation to update a registration (in this sense, it is different from
context subscriptions and context availability subscriptions, which have
updateContextSubscription and updateContextAvailabilitySubscription
operations). The update is done issuing a new registerContextRequest,
with the *registrationId* set:

``` 
(curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextRegistrations": [
        {
            "entities": [
                {
                    "type": "Room",
                    "isPattern": "false",
                    "id": "Room8"
                }
            ],
            "attributes": [
                {
                    "name": "humidity",
                    "type": "percentage",
                    "isDomain": "false"
                }
            ],
            "providingApplication": "http://mysensors.com/Rooms"
        }
    ],
    "duration": "P1M",
    "registrationId": "51bf1e0ada053170df590f20"
}
EOF
```
This "update registration" replaces the existing registration associated
to that ID with the new content, including [expiration
recalculation](duration.md#extending-duration).

Surprisingly, there is no way in NGSI to cancel a registration. A
workaround is to update it with a non-existing entity and duration 0,
but in order to do an actual delete you will need to remove the
registration from the database (check [the administration manual about
managing
database](../admin/database_admin.md#database-administration)).

# Updating subscriptions

You have previously seen in this document that [context
subscriptions](walkthrough_apiv1.md#register-context-operation) and [context
availability
subscriptions](walkthrough_apiv1.md#convenience-register-context) can be
updated. However, differently from registerContext, not everything can
be updated. Let's look at this closely, depending on the type of
subscription.

## What can be updated in a context subscription?

The payload for updateContextSubscription is similar to the one for a
subscribeContext request. However, not all the fields can be included,
as not everything can be updated. In particular, the following fields
cannot be updated:

-   subscriptionId (although you must include it in
    updateContextSubscription to refer to the subscription)
-   entities 
-   attributes
-   reference

However, the following fields can be modified:

-   notifyConditions
-   throttling
-   duration
-   restriction

## What can be updated in a context availability subscription?

The payload used by an updateContextAvailabilitySubscription is pretty
similar to the one in the subscribeContextAvailability request. However,
not all the fields can be included, as not everything is updatable. In
particular, the following is not updatable:

-   subscriptionId (although you must include it in
    updateContextAvailabilitySubscription to refer to the subscription)
-   reference

Thus, the following is updatable:

-   entities (in fact, it is mandatory in
    every updateContextAvailabilitySubscription).
-   attributes
-   duration
