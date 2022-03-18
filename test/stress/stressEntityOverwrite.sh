# Copyright 2021 FIWARE Foundation e.V.
#
# This file is part of Orion-LD Context Broker.
#
# Orion-LD Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion-LD Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# orionld at fiware dot org
#
# Author: Ken Zangelin


#
# Input from stress.sh
#
requests=$1
threads=$2
entities=$3
attributes=$4

echo "2.1. Creating $entities entities E1-E$entities, with attributes P1 and R1"
echo "======================================================================"
typeset -i eNo
eNo=1

rm -f /tmp/httpHeaders
rm -f /tmp/httpHeaders.out

while [ $eNo -le $entities ]
do
  eId=$(printf "urn:ngsi-ld:entities:E%d" $eNo)
  eNo=$eNo+1

  payload='{
    "id": "'$eId'",
    "type": "T",
    "P1": {
      "type": "Property",
      "value": 1
    },
    "R1": {
      "type": "Relationship",
      "object": "urn:E1:R1:step1"
    }
  }'

  curl localhost:1026/ngsi-ld/v1/entities -d "$payload" -H "Content-Type: application/json" --dump-header /tmp/httpHeaders.out 2> /dev/null
  cat /tmp/httpHeaders.out >> /tmp/httpHeaders
done

entitiesCreated=$(grep Location: /tmp/httpHeaders | wc -l)
echo -n "Entities created: $entitiesCreated"

rm -f	/tmp/httpHeaders
rm -f /tmp/entitiesCreated
echo
echo


echo "2.2. Asking the broker how many entities it has"
echo "==============================================="
curl "localhost:1026/ngsi-ld/v1/entities?type=T&count=true&limit=0" --dump-header /tmp/httpHeaders.out > /dev/null 2>&1
cat /tmp/httpHeaders.out | grep NGSILD-Results-Count
rm -f /tmp/httpHeaders.out
echo
echo


echo "2.3. Creating the payload body file with $attributes attributes for Apache benchmarking tool in step 7"
echo "========================================================================================"
typeset -i aNo
aNo=0
echo '{'                                  > /tmp/body.json

while [ $aNo -le $attributes ]
do
  attrName='attr_'$aNo
  attr='"'$attrName'":  {
    "type": "Property",
    "value": '$aNo',
    "P1": {
      "type": "Property",
      "value": '$aNo'
    },
    "R1": {
      "type": "Relationship",
      "object": "urn:E1:R1:step1"
    }
  }'

  if [ $aNo != $attributes ]
  then
    attr=${attr},
  fi
  
  echo "$attr" >> /tmp/body.json
  aNo=$aNo+1
done
echo "}" >> /tmp/body.json
echo
echo


echo "13. Running Apache HTTP server benchmarking tool (ab) with $threads threads and a total of $requests requests overwriting the entity"
echo "============================================================================================================================="
ab -c $threads -d -n $requests -T application/json -p /tmp/body.json -m POST  http://localhost:1026/ngsi-ld/v1/entities/urn:ngsi-ld:entities:E1/attrs
r=$?
echo
echo

exit $r
