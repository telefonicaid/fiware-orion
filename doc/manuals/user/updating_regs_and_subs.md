# Updating registrations

The response to a register context request (both in
[standard](#Register_Context_operation "wikilink") and
[convenience](#Convenience_Register_Context "wikilink")) includes a
registration ID (a 24 hexadecimal digit number):

      <?xml version="1.0"?>                                             {
      <registerContextResponse>                                             "duration": "PT24H",
        <registrationId>51bf1e0ada053170df590f20</registrationId>           "registrationId": "51bf1e0ada053170df590f20"
        <duration>PT24H</duration>                                      }
      </registerContextResponse>                                    
 
This ID can be used to update the registration. There is no special
operation to update a registration (in this sense, it is different from
context subscriptions and context availability subscriptions, which have
updateContextSubscription and updateContextAvailabilitySubscription
operations). The update is done issuing a new registerContextRequest,
with the *registrationId* set:

        (curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF       (curl localhost:1026/v1/registry/registerContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
      <?xml version="1.0"?>                                                                                                                    {
          <registerContextRequest>                                                                                                                 "contextRegistrations": [
            <contextRegistrationList>                                                                                                                  {
              <contextRegistration>                                                                                                                        "entities": [
                <entityIdList>                                                                                                                                 {
                  <entityId type="Room" isPattern="false">                                                                                                         "type": "Room",
                    <id>Room8</id>                                                                                                                                 "isPattern": "false",
                  </entityId>                                                                                                                                      "id": "Room8"
                </entityIdList>                                                                                                                                }
                <contextRegistrationAttributeList>                                                                                                         ],
                  <contextRegistrationAttribute>                                                                                                           "attributes": [
                    <name>humidity</name>                                                                                                                      {
                    <type>percentage</type>                                                                                                                        "name": "humidity",
                    <isDomain>false</isDomain>                                                                                                                     "type": "percentage",
                  </contextRegistrationAttribute>                                                                                                                  "isDomain": "false"
                </contextRegistrationAttributeList>                                                                                                            }
                <providingApplication>http://mysensors.com/Rooms</providingApplication>                                                                    ],
              </contextRegistration>                                                                                                                       "providingApplication": "http://mysensors.com/Rooms"
            </contextRegistrationList>                                                                                                                 }
            <duration>P1M</duration>                                                                                                               ],
            <registrationId>51bf1e0ada053170df590f20</registrationId>                                                                              "duration": "P1M",
          </registerContextRequest>                                                                                                                "registrationId": "51bf1e0ada053170df590f20"
      EOF                                                                                                                                      }
                                                                                                                                               EOF
  
This "update registration" replaces the existing registration associated
to that ID with the new content, including [expiration
recalculation](#Extending_duration "wikilink").

Surprisingly, there is no way in NGSI to cancel a registration. A
workaround is to update it with a non-existing entity and duration 0,
but in order to do an actual delete you will need to remove the
registration from the database (check [the administration manual about
managing
database](Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Database_administration "wikilink")).

# Updating subscriptions

You have previously seen in this document that [context
subscriptions](#Context_subscriptions "wikilink") and [context
availability
subscriptions](#Context_availability_subscriptions "wikilink") can be
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
-   entityIdList
-   attributeList
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

-   entityIdList (in fact, it is mandatory in
    every updateContextAvailabilitySubscription).
-   attributeList
-   duration
