#!/bin/bash

# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
# author: 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

if [  "$1" == ""  ]
  then
    echo "ERROR - No url defined (Mandatory)"
    echo "usage:"
    echo "    ./reqs_x_secs.sh <url> [-reset]"
    echo "    example: ./reqs_x_secs.sh localhost:4567 "
    echo ""
    echo " the -reset parameter is used to reset the listener previously."
    echo ""
    echo "           ( use [ Ctrl+C ] to stop )"
    exit
fi

if [  "$2" == "-reset"  ]
  then
     curl -s $1/reset > /dev/null
     echo ""
     echo " WARN - The listener has been reset... "
     echo ""
fi


echo "Show requests x seconds (TPS)... [CTRL+C] to stop!"
echo "--------------------------------------------------------------"
echo "                  reqs       requests      tps from first"
echo "     seconds    each sec      total        to last request"
echo "---------------------------------------------------------------"
sec=0
req=0


while true
do
  sleep 1s
  sec=$(($sec+1))
  resp=`curl -s $1/receive 2>&1`
  total=`echo $resp | sed 's/^{"requests": "\(.*\)","tps":\(.*\)"}/\1/'`
  tps=`echo $resp | sed 's/^{"requests": "\(.*\)","tps": "\(.*\)"}/\2/'`
  if [[  "$total" != ""  && "$tps" != "" ]]
     then
        echo " --- [" $sec "] ----- [" $(($total-$req)) "] ----- [" $total "] ----- [" $tps "]"
        req=$total
     else
       echo "Error - The listener ("$1") does not respond..."
       exit
  fi
done

