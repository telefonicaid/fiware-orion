source harnessFunctions.sh

echo "1. Make a subscription for E/att on tenant t_01"
echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++"
url="/t_01/NGSI10/subscribeContext"
payload='<?xml version="1.0" encoding="UTF-8"?>
<subscribeContextRequest>
  <entityIdList>
    <entityId type="Test" isPattern="false">
      <id>E</id>
    </entityId>
  </entityIdList>
  <attributeList/>
  <reference>http://127.0.0.1:'${LISTENER_PORT}'/notify</reference>
  <duration>PT1H</duration>
  <notifyConditions>
    <notifyCondition>
      <type>ONCHANGE</type>
      <condValueList>
        <condValue>att</condValue>
      </condValueList>
    </notifyCondition>
  </notifyConditions>
</subscribeContextRequest>'
curlXml ${url} "${payload}"
echo
echo


echo "2. Make a subscription for E/att on tenant t_02"
echo "+++++++++++++++++++++++++++++++++++++++++++++++++++++"
url="/t_02/NGSI10/subscribeContext"
payload='<?xml version="1.0" encoding="UTF-8"?>
<subscribeContextRequest>
  <entityIdList>
    <entityId type="Test" isPattern="false">
      <id>E</id>
    </entityId>
  </entityIdList>
  <attributeList/>
  <reference>http://127.0.0.1:'${LISTENER2_PORT}'/notify</reference>
  <duration>PT1H</duration>
  <notifyConditions>
    <notifyCondition>
      <type>ONCHANGE</type>
      <condValueList>
        <condValue>att</condValue>
      </condValueList>
    </notifyCondition>
  </notifyConditions>
</subscribeContextRequest>'
curlXml ${url} "${payload}"
echo
echo


echo "3. Update t_01/E/att"
echo "++++++++++++++++++++"
url="/t_01/NGSI10/updateContext"
payload='<?xml version="1.0" encoding="UTF-8"?>
<updateContextRequest>
  <contextElementList>
    <contextElement>
      <entityId type="Test" isPattern="false">
        <id>E</id>
      </entityId>
      <contextAttributeList>
        <contextAttribute>
          <name>att</name>
          <type>test</type>
          <contextValue>accu_t01</contextValue>
        </contextAttribute>
      </contextAttributeList>
    </contextElement>
  </contextElementList>
  <updateAction>APPEND</updateAction>
</updateContextRequest>'
curlXml ${url} "${payload}"
echo
echo


echo "4. Update t_02/E/att"
echo "++++++++++++++++++++"
url="/t_02/NGSI10/updateContext"
payload='<?xml version="1.0" encoding="UTF-8"?>
<updateContextRequest>
  <contextElementList>
    <contextElement>
      <entityId type="Test" isPattern="false">
        <id>E</id>
      </entityId>
      <contextAttributeList>
        <contextAttribute>
          <name>att</name>
          <type>test</type>
          <contextValue>accu_t02</contextValue>
        </contextAttribute>
      </contextAttributeList>
    </contextElement>
  </contextElementList>
  <updateAction>APPEND</updateAction>
</updateContextRequest>'
curlXml ${url} "${payload}"
echo
echo


echo "5. Dump accumulator t_01"
echo "+++++++++++++++++++++++++"
curl localhost:${LISTENER_PORT}/dump
echo
echo


echo "6. Dump accumulator t_02"
echo "+++++++++++++++++++++++++"
curl localhost:${LISTENER2_PORT}/dump
echo


