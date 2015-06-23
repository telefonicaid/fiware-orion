# Context Broker Federation

This section described "push" federation (in the sense notifyContext
sent by one Orion instance are processed by other Orion instance).
However, the [registring Context Providers and request
forwarding](#Registring_Context_Providers_and_request_forwarding "wikilink")
functionality can be used to implement a kind of "pull" federation (in
which one Orion instance fowards a query/update to another Orion
instance). Note that an importand difference between two approaches is
that in the "push" mode all the Orion instances update its local state,
while in the "push" approach all the intermediate Orion instances acts
as "proxy" without storing the data locally.

Apart from processing updateContext and registerContext (usually issued
by a client application) Orion Context Broker can process
notifyContextRequest and notifyContextAvailabilityRequest with the same
semantics. This opens the door to interesting federation scenarios (one
example is the [FI-LAB context management platform](# "wikilink")).

![](Federation.png "Federation.png")

Consider the following setup: three context broker instances running in
the same machine (of course, this is not a requirement but makes things
simpler to test this feature), in ports 1030, 1031 and 1032 respectively
and using different databases (named A, B and C to be brief). Let's
start each instance (run each command in a separate terminal):

    contextBroker -fg -port 1030 -db orion1030
    contextBroker -fg -port 1031 -db orion1031
    contextBroker -fg -port 1032 -db orion1032

Next, let's send a subscribeContext to A (to make B subscribe to updates
made in A). Note that the URL used in the reference has to be
"/v1/notifyContext":

    (curl localhost:1030/v1/subscribeContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format -) <<EOF
    <?xml version="1.0"?>
    <subscribeContextRequest>
      <entityIdList>
        <entityId type="Room" isPattern="false">
          <id>Room1</id>
        </entityId>
      </entityIdList>
      <reference>http://localhost:1031/v1/notifyContext</reference>
      <duration>P1M</duration>
      <notifyConditions>
        <notifyCondition>
          <type>ONCHANGE</type>
          <condValueList>
            <condValue>temperature</condValue>
          </condValueList>
        </notifyCondition>
      </notifyConditions>
      <throttling>PT5S</throttling>
    </subscribeContextRequest>
    EOF

Next, let's send a subscribeContext to B (to make C subscribe to updates
made in B). The subscription is basically the same, only the port in the
curl line and reference elements are different.

    (curl localhost:1031/v1/subscribeContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format -) <<EOF
    <?xml version="1.0"?>
    <subscribeContextRequest>
      <entityIdList>
        <entityId type="Room" isPattern="false">
          <id>Room1</id>
        </entityId>
      </entityIdList>
      <reference>http://localhost:1032/v1/notifyContext</reference>
      <duration>P1M</duration>
      <notifyConditions>
        <notifyCondition>
          <type>ONCHANGE</type>
          <condValueList>
            <condValue>temperature</condValue>
          </condValueList>
        </notifyCondition>
      </notifyConditions>
      <throttling>PT5S</throttling>
    </subscribeContextRequest>
    EOF

Now, let's create an entity in context broker A.

    (curl localhost:1030/v1/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF
    <?xml version="1.0" encoding="UTF-8"?>
    <updateContextRequest>
      <contextElementList>
        <contextElement>
          <entityId type="Room" isPattern="false">
            <id>Room1</id>
          </entityId>
          <contextAttributeList>
            <contextAttribute>
              <name>temperature</name>
              <type>float</type>
              <contextValue>23</contextValue>
            </contextAttribute>
          </contextAttributeList>
        </contextElement>
      </contextElementList>
      <updateAction>APPEND</updateAction>
    </updateContextRequest>
    EOF

Given the subscriptions in place, a notifyContextRequest is
automatically sent from A to B. That event at B causes in sequence a
notifyContextRequest to be sent to C. So, at the end, the creation of an
entity in A causes the creation of the same entity (with the same
attribute values) in C. You can check it by doing a queryContext to C:

    (curl localhost:1032/v1/queryContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format -) <<EOF
    <?xml version="1.0" encoding="UTF-8"?>
    <queryContextRequest>
      <entityIdList>
        <entityId type="Room" isPattern="false">
          <id>Room1</id>
        </entityId>
      </entityIdList>
      <attributeList/>
    </queryContextRequest>
    EOF

    <?xml version="1.0"?>
    <queryContextResponse>
      <contextResponseList>
        <contextElementResponse>
          <contextElement>
            <entityId type="Room" isPattern="false">
              <id>Room1</id>
            </entityId>
            <contextAttributeList>
              <contextAttribute>
                <name>temperature</name>
                <type>float</type>
                <contextValue>23</contextValue>
              </contextAttribute>
            </contextAttributeList>
          </contextElement>
          <statusCode>
            <code>200</code>
            <reasonPhrase>OK</reasonPhrase>
          </statusCode>
        </contextElementResponse>
      </contextResponseList>
    </queryContextResponse>

In the current context broker version, the semantics of
nofityContextRequest are the same that [updateContext
APPEND](#Entity_Creation "wikilink") or, if the context element already
exist, the semantics of [updateContext
UPDATE](#Update_context_elements "wikilink"). Thus, federation doesn't
provide exact mirroring: a updateContext DELETE to one context broker
will not produce the same effect in the federated context broker.

This mechanism works similarly with registerContext and
subscribeContextAvailability. In this case, the URL for the reference
element is "/v1/registry/notifyContextAvailability".