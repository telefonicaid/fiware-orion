 # Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
 #
 # This file is part of Orion Context Broker.
 #
 # Orion Context Broker is free software: you can redistribute it and/or
 # modify it under the terms of the GNU Affero General Public License as
 # published by the Free Software Foundation, either version 3 of the
 # License, or (at your option) any later version.
 #
 # Orion Context Broker is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 # General Public License for more details.
 #
 # You should have received a copy of the GNU Affero General Public License
 # along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
 #
 # For those usages not covered by this license please contact with
 # fermin at tid dot es

# v0.3
# USAGE:  ./subscriptionTest_ONx.sh endpoint:port amountOfSubscriptions conditionValue notificationEndpoint
# EXAMPLE ./subscriptionTest_ONx.sh 127.0.0.1:1026 100 pressure http://127.0.0.1:1028/accumulate

# START
echo "#### Subscription ONCHANGE test STARTED! #### "
echo "Date: " $(date +"%m-%d-%Y %H:%M"); 


# Default config: 100 subscriptions on CHANGE over PRESSURE and sent to http://127.0.0.1:1028/accumulator server
#config
n=0
max=10

#sleep time between requests
stime=0

#TestScenario
notifEp='http://localhost:1028/accumulate'
subType='ONCHANGE'
condValue='pressure'

#Inputs and defaults

if [ "$1" ]; then
    #echo "CB endpoint set to: " $1 ;
    endpoint=$1;
    else
    endpoint=127.0.0.1:1026;
fi

if [ "$2" ]; then
    #echo "Amount of Subs set to: " $2 ;
    max=$2;
fi

if [ "$3" ]; then
    #echo "ConditionValue set to: " $3 ;
    condValue=$3;
fi

if [ "$4" ]; then
    #echo "Notification endpoint set to: " $4 ;
    notifEp=$4;
fi

echo "### Summary of sets: "
echo " - Selected endpoint: " $endpoint;
echo " - Selected amount of subs: " $max;
echo " - Selected condition Value: " $condValue;
echo " - Selected notification endpoint: " $notifEp;

echo "### Version: "
curl $endpoint/version


payload='<?xml version="1.0"?>
    <subscribeContextRequest>
        <entityIdList>
            <entityId type="Room" isPattern="false">
                <id>Room1</id>
            </entityId>
        </entityIdList>
        <attributeList>
            <attribute>temperature</attribute>
        </attributeList>
        <reference>'$notifEp'</reference>
        <duration>P1M</duration>
        <notifyConditions>
            <notifyCondition>
                <type>'$subType'</type>
            <condValueList>
                <condValue>'$condValue'</condValue>
            </condValueList>
            </notifyCondition>
        </notifyConditions>
        <throttling>PT0S</throttling>
    </subscribeContextRequest>'



while (( $n < $max ))
do
	sleep $stime;
	echo "### Timestamp " $n; 
	date;
	echo "### Subscription test: " $n;
	n=$((n+1))
    echo "###"
    curl -H 'Content-Type: application/xml' -d"$payload" $endpoint/NGSI10/subscribeContext
    
done

# DONE
echo " ##########  Subscription test DONE!"


#TIPS (Copy&Paste)
#Trigger sample for pressure: 
#curl -H 'Content-Type: application/xml' -d '<?xml version="1.0" encoding="UTF-8"?><updateContextRequest><contextElementList><contextElement><entityId type="Room" isPattern="false"><id>Room1</id></entityId><contextAttributeList><contextAttribute><name>pressure</name><type>mmHg</type><contextValue>'`date +%N`'</contextValue></contextAttribute></contextAttributeList></contextElement></contextElementList><updateAction>APPEND</updateAction></updateContextRequest>' orion/NGSI10/updateContext

#DEACTIVATE Subscription:
#curl -H 'Content-Type: application/xml' -d '<?xml version="1.0"?><unsubscribeContextRequest><subscriptionId>SUB_ID</subscriptionId></unsubscribeContextRequest>' ORION/NGSI10/unsubscribeContext
#Note: SUB_ID is the Id of the subscription to deactivate

