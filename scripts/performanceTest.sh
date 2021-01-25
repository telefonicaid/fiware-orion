# Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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
function perfCalc
{
  REQ="$1"
  SUM=0
  for F in $(orionLogView | egrep 'TPUT: Entire request' | awk -F: '{ print $5 }')
  do
    SUM=$(echo "$SUM + $F" | bc)
  done
  AVG=$(echo "$SUM * 0.01" | bc)
  TPUT=$(echo "1 / $AVG" | bc)
  echo $REQ: $TPUT reqs/second
}

ft ngsild_performance_POST_entities_100.test
perfCalc "POST /entities"

ft ngsild_performance_GET_entity_out_of_100.test
perfCalc "GET /entities/{EID} among 100"

ft ngsild_performance_GET_entity_out_of_10.test
perfCalc "GET /entities/{EID} among 10"
