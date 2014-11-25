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
 # iot_support at tid dot es


# Config
version="Subscription Massive Generator Test ONTIMEINTERVAL v0.0.4";


# version
#
function version()
{
    echo Version: $version
    exit $1
}

# usage
#
function usage()
{
    echo  "Usage info:  ./subscriptionTest_ONTIMEINTERVAL.sh [ -u (usage)], [-v (verbose)] and [ --version (version)]
      subscriptionTest_ONTIMEINTERVAL.sh  [-u (usage)]
      subscriptionTest_ONTIMEINTERVAL.sh  [--version (version)]
      subscriptionTest_ONTIMEINTERVAL.sh [-v (verbose)] [<endpoint port>] [<amount of subscriptions>] [<update time>] [<notification endpoint>]
    "
    echo "Example of use:
      ./subscriptionTest_ONTIMEINTERVAL.sh 127.0.0.1:1026 60 60 http://127.0.0.1:1028/accumulate
    "
    echo "Default config when launched without configuration:
      - default endpoint:  127.0.0.1:1026
      - default amount of subs:  10
      - default update time (s):  60
      - default notification endpoint:  http://127.0.0.1:1028/accumulate
    "
    exit $1
}

#CLI options
if [ "$1" == "-u" ]
then
  usage
  exit 1
fi

if [ "$1" == "--version" ]
then
  version
  exit 1
fi

if [ "$1" == "-v" ]
then
  echo "Verbose mode ON"
  vm=1
  shift
fi

# START info
echo "#### Subscription ONTIMEINTERVAL test STARTED! #### "
echo "Date: " $(date +"%m-%d-%Y %H:%M");

# Default config: Add 10 subscriptions on CHANGE over PRESSURE and sent to http://127.0.0.1:1028/accumulator server
n=0
max=10

# Sleep time between requests
stime=1

# TestScenario
notifEp='http://127.0.0.1:1028/accumulate'
subType='ONTIMEINTERVAL'
updateTime=60

# Config options
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
    updateTime=$3;
fi

if [ "$4" ]; then
    #echo "Notification endpoint set to: " $4 ;
    notifEp=$4;
fi

echo "### Summary of sets: "
echo " - Selected endpoint: " $endpoint;
echo " - Selected amount of subs: " $max;
echo " - Selected uptate every (s): " $updateTime;
echo " - Selected notification endpoint: " $notifEp;

echo "### Endpoint Version: "
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
                <condValue>PT'$updateTime'S</condValue>
            </condValueList>
        </notifyCondition>
    </notifyConditions>
</subscribeContextRequest>' 

while (( $n < $max ))
do
	sleep $stime;
	echo "### Timestamp " $n; 
	date;
	echo "### Suscription test: " $n; 
	n=$((n+1))
    echo "###"
    curl -H 'Content-Type: application/xml' -d"$payload"  $endpoint/NGSI10/subscribeContext
done

# DONE
echo " ##########  Subscription test DONE!"

#TIPS (Copy&Paste)
#Trigger sample for temperature at room1:
#curl -H 'Content-Type: application/xml' -d '<?xml version="1.0" encoding="UTF-8"?><updateContextRequest><contextElementList><contextElement><entityId type="Room" isPattern="false"><id>Room1</id></entityId><contextAttributeList><contextAttribute><name>temperature</name><type>centigrade</type><contextValue>23</contextValue></contextAttribute></contextAttributeList></contextElement></contextElementList><updateAction>APPEND</updateAction></updateContextRequest>' localhost:1026/NGSI10/updateContext

#DEACTIVATE Subscription
#curl -H 'Content-Type: application/xml' -d '<?xml version="1.0"?><unsubscribeContextRequest><subscriptionId>SUB_ID</subscriptionId></unsubscribeContextRequest>' localhost:1026/NGSI10/unsubscribeContext
#Note: SUB_ID is the Id of the subscription to deactivate

