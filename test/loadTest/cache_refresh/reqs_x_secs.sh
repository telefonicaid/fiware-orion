#!/bin/bash

# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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


# variables
LISTENER=localhost:8090
CB=None
RESET=false
notifQueueSize=N/A

# ========================================================
#
# usage - show the script usage
#
function usage(){
     echo " "
     echo "Usage: "$(basename $0)
     echo "   -u (usage) [OPTIONAL]"
     echo "   -reset (this parameter is used to reset the listener previously) [OPTIONAL]"
     echo "   --listener <listener endpoint> (listener endpoint host:port)[OPTIONAL] (default: localhost:8090)"
     echo "   --cb <CB endpoint> (listener endpoint host:port) [OPTIONAL] (default: None)"
     echo " "
     echo " Example:"
     echo "     "$(basename $0)" --listener localhost:8090 --cb localhost:8090 -reset"
     echo " "
     echo " Note: "
     echo "     - if cb parameter is not used the queue size is not displayed..."
     echo ""
     echo "                      ( use [ Ctrl+C ] to stop )"
     echo " "
     exit 0
}

# BEGIN
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];          then usage;
  elif [ "$1" == "--listener" ];  then LISTENER="$2"; shift;
  elif [ "$1" == "--cb" ];        then CB="$2"; shift;
  elif [ "$1" == "-reset" ];      then RESET="true"; shift;
  else
      echo $0: bad parameter/option: "'"${1}"'";
      usage
  fi
  shift
done

if [  "$RESET" == "true"  ]
  then
     curl -s $LISTENER/reset > /dev/null
     echo " WARN - The listener has been reseted... "
fi

if [ "$CB" != "None" ]
    then
        echo " INFO - the notifQueue size will be monitorized from ContextBroker..."
    else
        echo " INFO - the notifQueue size won't be monitorized "
fi

echo " INFO - Show requests x seconds (TPS)... [CTRL+C] to stop!"
echo "----------------------------------------------------------------------------"
echo "                  reqs       requests      tps from first     notifQueue"
echo "     seconds    each sec      total        to last request       size"
echo "-----------------------------------------------------------------------------"
sec=0
req=0


while true
do
  sleep 1s
  sec=$(($sec+1))

  # requests received in the listener
  resp=`curl -s $LISTENER/receive 2>&1`
  if [  "$resp" == "" ]
       then
          echo "ERROR - The listener ("$LISTENER") does not respond..."
          exit
  fi
  total=`echo $resp | jq '.requests' | tr -d \"`
  tps=`echo $resp | jq '.tps' | tr -d \"`

  # notifQueue size
  if [ "$CB" != "None" ]
    then
      stat=`curl -s $CB/statistics  2>&1`
      if [  "$stat" == "" ]
           then
              echo "ERROR - The CB ("$CB") does not respond..."
              exit
      fi
      # jq is a lightweight and flexible command-line JSON processor. See Dependency in README.md
      notifQueueSize=`echo $stat | jq '.notifQueue.size'`
  fi

  # report per second
  echo " --- [" $sec "] ----- [" $(($total-$req)) "] ----- [" $total "] ----- [" $tps "] ------ [" $notifQueueSize "]"
done
