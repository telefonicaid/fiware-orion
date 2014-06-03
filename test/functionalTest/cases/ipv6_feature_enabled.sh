source harnessFunctions.sh

echo "1: ++++++++++++++++++++"
curl :::${BROKER_PORT}/version | xmllint --format -

echo "2: ++++++++++++++++++++"
curl localhost:${BROKER_PORT}/version | xmllint --format -

echo "3: ++++++++++++++++++++"
SUB_OUT=$((curl :::${BROKER_PORT}/NGSI10/subscribeContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format -) <<EOF
<?xml version="1.0"?>
<subscribeContextRequest>
  <entityIdList>
        <entityId type="Room" isPattern="false">
          <id>OfficeRoom6</id>
        </entityId>
  </entityIdList>
  <attributeList>
        <attribute>temperature</attribute>
        <attribute>lightstatus</attribute>
  </attributeList>
  <reference>http://:::${LISTENER_PORT_V6}/notify</reference>
  <duration>PT1H</duration>
  <notifyConditions>
        <notifyCondition>
          <type>ONCHANGE</type>
          <condValueList>
                <condValue>temperature</condValue>
                <condValue>lightstatus</condValue>
          </condValueList>          
        </notifyCondition>
  </notifyConditions>
</subscribeContextRequest>
EOF)
SUB_ID_V6=$(echo "$SUB_OUT" | grep subscriptionId | awk -F '>' '{print $2}' | awk -F '<' '{print $1}' | grep -v '^$' )
echo "$SUB_OUT"

echo "4: ++++++++++++++++++++"
SUB_OUT=$((curl localhost:${BROKER_PORT}/NGSI10/subscribeContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format -) <<EOF
<?xml version="1.0"?>
<subscribeContextRequest>
  <entityIdList>
        <entityId type="Room" isPattern="false">
          <id>OfficeRoom4</id>
        </entityId>
  </entityIdList>
  <attributeList>
        <attribute>temperature</attribute>
        <attribute>lightstatus</attribute>
  </attributeList>
  <reference>http://localhost:${LISTENER_PORT}/notify</reference>
  <duration>PT1H</duration>
  <notifyConditions>
        <notifyCondition>
          <type>ONCHANGE</type>
          <condValueList>
                <condValue>temperature</condValue>
                <condValue>lightstatus</condValue>
          </condValueList>
        </notifyCondition>
  </notifyConditions>
</subscribeContextRequest>
EOF)
SUB_ID_V4=$(echo "$SUB_OUT" | grep subscriptionId | awk -F '>' '{print $2}' | awk -F '<' '{print $1}' | grep -v '^$' )
echo "$SUB_OUT"

echo "5: ++++++++++++++++++++"
(curl :::${BROKER_PORT}/NGSI10/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF
<?xml version="1.0"?>
<updateContextRequest>
  <contextElementList>
        <contextElement>
          <entityId type="Room" isPattern="false">
                <id>OfficeRoom6</id>
          </entityId>
          <contextAttributeList>
            <contextAttribute>
          <name>pressure</name>
                  <type>clima</type>
                  <contextValue>p2300</contextValue>
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
</updateContextRequest>
EOF

# Get notifications count
mongoCmd ${BROKER_DATABASE_NAME} "db.csubs.findOne({_id: ObjectId(\"$SUB_ID_V6\")}, {_id: 0, count: 1})"

echo "6: ++++++++++++++++++++"
(curl localhost:${BROKER_PORT}/NGSI10/updateContext -s -S --header 'Content-Type: application/xml' -d @- | xmllint --format - ) <<EOF
<?xml version="1.0"?>
<updateContextRequest>
  <contextElementList>
        <contextElement>
          <entityId type="Room" isPattern="false">
                <id>OfficeRoom4</id>
          </entityId>
          <contextAttributeList>
            <contextAttribute>
          <name>pressure</name>
                  <type>clima</type>
                  <contextValue>p2300</contextValue>
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
</updateContextRequest>
EOF

# Get notifications count
mongoCmd ${BROKER_DATABASE_NAME} "db.csubs.findOne({_id: ObjectId(\"$SUB_ID_V4\")}, {_id: 0, count: 1})"

echo "7: ++++++++++++++++++++"
#Get accumulated data
curl :::${LISTENER_PORT_V6}/dump -s -S 

echo "8: ++++++++++++++++++++"
#Get accumulated data
curl localhost:${LISTENER_PORT}/dump -s -S

