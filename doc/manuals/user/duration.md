# Duration (for registration and subscriptions)

## Default duration

If you don't specify a duration in registerContext, subscribeContext or
subscribeContextAvailability a default of 24 hours is used. You will get
a confirmation of the duration in these cases in the response, e.g. for
a registerContext:

      {
      "duration": "PT24H",
         "registrationId": "52f38a64261c371af12b8565"
      }

## Extending duration

We have seen that registrations and subscriptions (both context and
context availability ones) have a duration. The expiration date is
calculated using the following formula:

-   expiration = current-time + duration

The behavior of the broker regarding expired elements is the following:

-   For registrations: an expired registration is not taken into account
    in discoverContextAvailability request processing, but it is still
    [updatable](../admin/database_admin.md#updating-registrations).
-   For subscriptions: an expired subscription is not taken into account
    to send new notifications based on it, but it is still updatable
    (using updateContextSubscription/updateContextSubscriptionAvailability)
    and it can be canceled
    (using unsubscribeContext/unsubscribeContextAvailability).

Note that Orion Context Broker doesn't remove expired elements from the
database, but that they can be easily deleted [as described in the
administration manual](../admin/database_admin.md#deleting-expired-documents).

Finally, take into account that the expiration is *recalculated* on
updates, not *expanded*. Let's clarify this with an example. Let's
suppose that at 18:30 you do a subscription with PT1H duration (i.e. one
hour). Thus, it will expire at 19:30. Next, at 19:00 you do an update
using again PT1H as duration. So, that hour period is not added to 19:30
(the previous expiration limit) but added to the 19:00 (the current
time). Thus, the new expiration time is 20:00.
