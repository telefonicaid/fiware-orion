source ../harnessFunctions.sh

ucr='<?xml version="1.0" encoding="UTF-8"?>
<updateContextRequest>
  <contextElementList>
    <contextElement>
      <entityId type="Room" isPattern="false">
        <id>entity01</id>
      </entityId>
      <contextAttributeList>
        <contextAttribute>
          <name>temperature</name>
          <type>degree</type>
          <contextValue>11</contextValue>
        </contextAttribute>
        <contextAttribute>
          <name>lightstatus</name>
          <type>light</type>
          <contextValue>L23</contextValue>
        </contextAttribute>
      </contextAttributeList>
    </contextElement>
  </contextElementList>
  <updateAction>APPEND</updateAction>
</updateContextRequest>'

qcr='<?xml version="1.0" encoding="UTF-8"?>
<queryContextRequest>
  <entityIdList>
    <entityId type="Room" isPattern="false">
      <id>entity01</id>
    </entityId>
  </entityIdList>
  <attributeList>
    <attribute>temperature</attribute>
    <attribute>occupancy</attribute>
    <attribute>lightstatus</attribute>
  </attributeList>
  <restriction>
    <attributeExpression>Attribute Expression</attributeExpression>
    <scope>
      <operationScope>
        <scopeType>st1</scopeType>
        <scopeValue>sv1</scopeValue>
      </operationScope>
      <operationScope>
        <scopeType>FIWARE_Location</scopeType>
        <scopeValue>
          <circle>
            <radius>10</radius>
            <centerLatitude>45</centerLatitude>
            <centerLongitude>45</centerLongitude>
            <inverted>0</inverted>
          </circle>
        </scopeValue>
      </operationScope>
      <operationScope>
        <scopeType>st2</scopeType>
        <scopeValue>sv2</scopeValue>
      </operationScope>
    </scope>
  </restriction>
</queryContextRequest>'

scr='<?xml version="1.0" encoding="UTF-8"?>
<subscribeContextRequest>
  <entityIdList>
    <entityId type="Room" isPattern="false">
      <id>ConferenceRoom</id>   
    </entityId>
    <entityId type="Room" isPattern="false">
      <id>OfficeRoom</id>   
    </entityId>
  </entityIdList>
  <attributeList>
    <attribute>temperature</attribute>
    <attribute>lightstatus</attribute>
  </attributeList>
  <reference>http://127.0.0.1:1028</reference>
  <duration>P5Y</duration>
  <restriction>
    <attributeExpression>testRestriction</attributeExpression>
    <scope>
      <operationScope>
        <scopeType>scope1</scopeType>
        <scopeValue>sval1</scopeValue>
      </operationScope>
      <operationScope>
        <scopeType>scope2</scopeType>
        <scopeValue>sval2</scopeValue>
      </operationScope>
      <operationScope>
        <scopeType>FIWARE_Location</scopeType>
        <scopeValue>
          <circle>
            <radius>11</radius>
            <centerLatitude>55</centerLatitude>
            <centerLongitude>55</centerLongitude>
            <inverted>0</inverted>
          </circle>
        </scopeValue>
      </operationScope>
    </scope>
  </restriction>
  <notifyConditions>
    <notifyCondition>
      <type>ONCHANGE</type>
      <condValueList>
        <condValue>temperature</condValue>
        <condValue>lightstatus</condValue>
      </condValueList>
      <restriction>restriction</restriction>
    </notifyCondition>
  </notifyConditions>
  <throttling>P5Y</throttling>
</subscribeContextRequest>'

ucsr='<?xml version="1.0" encoding="UTF-8"?>
<updateContextSubscriptionRequest>
  <duration>P50Y</duration>
  <restriction>
    <attributeExpression>AttriTest</attributeExpression>
    <scope>
      <operationScope>
        <scopeType>st1</scopeType>
        <scopeValue>sv1</scopeValue>
      </operationScope>
      <operationScope>
        <scopeType>FIWARE_Location</scopeType>
        <scopeValue>
          <circle>
            <radius>12</radius>
            <centerLatitude>65</centerLatitude>
            <centerLongitude>65</centerLongitude>
            <inverted>0</inverted>
          </circle>
        </scopeValue>
      </operationScope>
    </scope>
  </restriction>
  <subscriptionId>111222333444555666777888</subscriptionId>
  <notifyConditions>
    <notifyCondition>
      <type>ONCHANGE</type>
      <condValueList>
        <condValue>CondValue3</condValue>
        <condValue>CondValue4</condValue>
      </condValueList>
    </notifyCondition>
  </notifyConditions>
  <throttling>P5Y</throttling>
</updateContextSubscriptionRequest>'

echo '1: +++++++++++++++++++++++++++++++++++++'
curl localhost:${BROKER_PORT}/ngsi10/queryContext -s -S --header "Content-Type: application/xml" -d "$qcr" 
echo '2: +++++++++++++++++++++++++++++++++++++'
curl localhost:${BROKER_PORT}/ngsi10/updateContext -s -S --header "Content-Type: application/xml" -d "$ucr"
echo '3: +++++++++++++++++++++++++++++++++++++'
curl localhost:${BROKER_PORT}/ngsi10/queryContext -s -S --header "Content-Type: application/xml" -d "$qcr"
echo '4: +++++++++++++++++++++++++++++++++++++'
curl localhost:${BROKER_PORT}/ngsi10/subscribeContext -s -S --header "Content-Type: application/xml" -d "$scr"
echo '5: +++++++++++++++++++++++++++++++++++++'
grep 'circleRadius: Got a circleRadius' /tmp/contextBrokerLog | awk -F'] ' '{ print $2 }'
echo '6: +++++++++++++++++++++++++++++++++++++'

