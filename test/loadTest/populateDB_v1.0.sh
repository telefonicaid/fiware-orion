#!/bin/bash

#Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
# 
# Author: Ivan Arias



###########################################
# Script to populate a DB with N entities #
###########################################

if [ -z "$4" ]; then    
    echo "It is necesary to define url, tenant, elements_number and step"
	echo "  usage:  populateDB_v1.0.sh <http://hostname:port> <tenant> <100000> <10>"
	exit
fi

ENDPOINT=$1
TENANT=$2
ELEMENTS_NUMBER=$3
STEP=$4
MAX_ELEMENT=$(expr $ELEMENTS_NUMBER \* $STEP )

for (( COUNTER=0; COUNTER<=$MAX_ELEMENT; COUNTER+=$STEP)); do
(curl $ENDPOINT/NGSI10/updateContext -s -S --header 'Content-Type: application/xml' --header 'Fiware-Service: indexTest' -d @- | xmllint --format - ) <<EOF
<?xml version="1.0" encoding="UTF-8"?>
    <updateContextRequest>
        <contextElementList>
            <contextElement>
                <entityId type="Room" isPattern="false">
                    <id>Room1_$COUNTER</id> 
                </entityId>
                <contextAttributeList>
                    <contextAttribute>
                        <name>temperature</name>
                        <type>centigrade</type>
                        <contextValue>$RANDOM</contextValue>
                    </contextAttribute>
                    <contextAttribute>
                        <name>pressure</name>
                        <type>mmHg</type>
                        <contextValue>$RANDOM</contextValue>
                    </contextAttribute>
                </contextAttributeList>
            </contextElement>
        </contextElementList>
        <updateAction>APPEND</updateAction>
    </updateContextRequest>
EOF
done

echo
echo "--------------------------------------------------------------------------------------------------------------"
echo "Database populate finished in: "$ENDPOINT" and tenant: "$TENANT
echo "-- elements number ( "$ELEMENTS_NUMBER" ) with step ( "$STEP" ) = maximum element (" $MAX_ELEMENT ") --"
echo "--------------------------------------------------------------------------------------------------------------"
echo 
