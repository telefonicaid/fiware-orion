		
# v0.2
# USAGE:  ./subscriptTest_.sh endpoint:port amountOfSuscriptions conditionValue notificationEndpoint
# EXAMPLE ./subscriptionTest_ONx.sh 127.0.0.1:1026 100 pressure http://127.0.0.1:1028/accumulate

# START
echo "#### Suscription ONCHANGE test STARTED! #### "
echo "Date: " $(date +"%m-%d-%Y %H:%M"); 


# Default config: 100 suscriptions and updated on CHANGE over PRESSURE and sent to http://127.0.0.1:1028/accumulator server

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

while (( $n < $max ))
do
	sleep $stime;
	echo "### Timestamp " $n; 
	date;
	echo "### Suscription test: " $n; 
	n=$((n+1))

echo "###"
curl -H 'Content-Type: application/xml' -d '<?xml version="1.0"?><subscribeContextRequest><entityIdList><entityId type="Room" isPattern="false"><id>Room1</id></entityId></entityIdList><attributeList><attribute>temperature</attribute></attributeList><reference>'$notifEp'</reference><duration>P1M</duration><notifyConditions><notifyCondition><type>'$subType'</type><condValueList><condValue>'$condValue'</condValue></condValueList></notifyCondition></notifyConditions><throttling>PT0S</throttling></subscribeContextRequest>' $endpoint/NGSI10/subscribeContext
done

# DONE
echo " ##########  Suscription test DONE!"


#TIPS 
#Trigger sample for pressure: 
#curl -H 'Content-Type: application/xml' -d '<?xml version="1.0" encoding="UTF-8"?><updateContextRequest><contextElementList><contextElement><entityId type="Room" isPattern="false"><id>Room1</id></entityId><contextAttributeList><contextAttribute><name>pressure</name><type>mmHg</type><contextValue>'`date +%N`'</contextValue></contextAttribute></contextAttributeList></contextElement></contextElementList><updateAction>UPDATE</updateAction></updateContextRequest>' orion/NGSI10/updateContext

#DEACTIVATE Subscription:
#curl -H 'Content-Type: application/xml' -d '<?xml version="1.0"?><unsubscribeContextRequest><subscriptionId>530b8d95286043dd1c801939</subscriptionId></unsubscribeContextRequest>' ORION/NGSI10/unsubscribeContext


