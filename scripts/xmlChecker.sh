#!/bin/bash
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

#TEST_PATH="../test/unittests/testData"
TEST_PATH="../test/manual"
VERBOSE=0

function inc_n () {
   N=`expr $N + 1`
}

function inc_ok () {
   OK=`expr $OK + 1`
}

function inc_err () {
   ERR=`expr $ERR + 1`
}

function process_file() {
   if [ "$VERBOSE" -eq "1" ]; then
      xmllint $1 --schema $2
      RESULT=$?
   else
      xmllint $1 --schema $2 > /dev/null 2>&1
      RESULT=$?
   fi
   if [ "$RESULT" -eq "0" ]; then
      echo "$1: ok"
      inc_ok
   else
      echo "$1: FAILS (xmllint error: $RESULT)"
      inc_err
   fi
   inc_n
}

# Get the .xsd files
echo "Enter username: "
read USER

echo "Enter password: "
STTY_ORIG=`stty -g` 
stty -echo
read PASS
stty $STTY_ORIG

wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fi-ware.eu/scmrepos/svn/iot/trunk/schemes/Ngsi10_Operations_v07.xsd
wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fi-ware.eu/scmrepos/svn/iot/trunk/schemes/Ngsi9_Operations_v07.xsd
wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fi-ware.eu/scmrepos/svn/iot/trunk/schemes/Ngsi9_10_dataStructure_v07.xsd

# Check each "family" of XML (per operation)
N=0
OK=0
ERR=0

# NGSI 9
for FILE in $(ls $TEST_PATH/registerContext*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/discoverContextAvailability*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/subscribeContextAvailabilityR*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/updateContextAvailabilitySubscriptionR*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/unsubscribeContextAvailabilityR*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/notifyContextAvailabilityR*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done


# NGSI 10
for FILE in $(ls $TEST_PATH/queryContext*.xml); do
   process_file $FILE Ngsi10_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/updateContext*.xml); do
   process_file $FILE Ngsi10_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/subscribeContextR*.xml); do
   process_file $FILE Ngsi10_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/updateContextSubscriptionR*.xml); do
   process_file $FILE Ngsi10_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/unsubscribeContextR*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done
for FILE in $(ls $TEST_PATH/notifyContextR*.xml); do
   process_file $FILE Ngsi9_Operations_v07.xsd
done

# Cleaning
rm Ngsi10_Operations_v07.xsd
rm Ngsi9_Operations_v07.xsd
rm Ngsi9_10_dataStructure_v07.xsd

#Results
echo "----------------"
echo "Processed files: $N"
echo "Ok files: $OK"
echo "Err files: $ERR"

TOTAL_FILES=$(ls $TEST_PATH/*.xml | wc -l)
if [ "$TOTAL_FILES" -ne "$N" ]; then
   echo "----------------"
   echo "There are $TOTAL_FILES XML files in the directory, so some of them were not processed"
   exit 1
fi

if [ "$ERR" -ne "0" ]; then
   exit 1
fi

exit 0

