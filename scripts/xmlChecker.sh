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

TEST_DATA_PATH="../test/unittests/testData"
TEST_MANUAL_PATH="../test/manual"
TEST_HARNESS_PATH="../test/testharness"
VERBOSE=0

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
      OK=$OK+1
   else
      echo "$1: FAILS (xmllint error: $RESULT)"
      ERR=$ERR+1
   fi
   N=$N+1
}

function do_check() {

   # NGSI 9
   for FILE in $(ls $1/registerContext*.xml); do
      process_file $FILE Ngsi9_Operations_v07.xsd
   done
   for FILE in $(ls $1/discoverContextAvailability*.xml); do
      process_file $FILE Ngsi9_Operations_v07.xsd
   done
   for FILE in $(ls $1/subscribeContextAvailabilityR*.xml); do
      process_file $FILE Ngsi9_Operations_v07.xsd
   done
   for FILE in $(ls $1/updateContextAvailabilitySubscriptionR*.xml); do
      process_file $FILE Ngsi9_Operations_v07.xsd
   done
   for FILE in $(ls $1/unsubscribeContextAvailabilityR*.xml); do
      process_file $FILE Ngsi9_Operations_v07.xsd
   done
   for FILE in $(ls $1/notifyContextAvailabilityR*.xml); do
      process_file $FILE Ngsi9_Operations_v07.xsd
   done

   # NGSI 10
   for FILE in $(ls $1/queryContext*.xml); do
      process_file $FILE Ngsi10_Operations_v07.xsd
   done
   for FILE in $(ls $1/updateContext*.xml); do
      process_file $FILE Ngsi10_Operations_v07.xsd
   done
   for FILE in $(ls $1/subscribeContextR*.xml); do
      process_file $FILE Ngsi10_Operations_v07.xsd
   done
   for FILE in $(ls $1/updateContextSubscriptionR*.xml); do
      process_file $FILE Ngsi10_Operations_v07.xsd
   done
   for FILE in $(ls $1/unsubscribeContextR*.xml); do
      process_file $FILE Ngsi10_Operations_v07.xsd
   done
   for FILE in $(ls $1/notifyContextR*.xml); do
      process_file $FILE Ngsi10_Operations_v07.xsd
   done

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
typeset -i N
typeset -i OK
typeset -i ERR
N=0
OK=0
ERR=0

# Check "regular files"
do_check $TEST_DATA_PATH
do_check $TEST_MANUAL_PATH
N_ONLY_FILES=$N

# Check fragments in test harness
TMP_DIR=$(mktemp -d /tmp/xmlCheck.XXXXX)
for FILE in $(find $TEST_HARNESS_PATH -name *.test); do
    PREFIX=$(basename ${FILE%.*})
    ./xmlExtractor.py $FILE $TMP_DIR $PREFIX
done

for FILE in $(ls $TMP_DIR/*ngsi9*); do
    process_file $FILE Ngsi9_Operations_v07.xsd
done

for FILE in $(ls $TMP_DIR/*ngsi10*); do
    process_file $FILE Ngsi10_Operations_v07.xsd
done

# Cleaning
rm Ngsi10_Operations_v07.xsd
rm Ngsi9_Operations_v07.xsd
rm Ngsi9_10_dataStructure_v07.xsd
rm -rf $TMP_DIR

#Results
echo "----------------"
echo "Processed files/fragments: $N"
echo "Ok files/fragments: $OK"
echo "Err files/fragments: $ERR"

typeset -i N_DATA
typeset -i N_MANUAL
typeset -i TOTAL_FILES
N_DATA=$(ls $TEST_DATA_PATH/*.xml | wc -l)
N_MANUAL=$(ls $TEST_MANUAL_PATH/*.xml | wc -l)
TOTAL_FILES=$N_DATA+$N_MANUAL
#TOTAL_FILES=`expr $N_DATA + $N_MANUAL`

if [ "$TOTAL_FILES" -ne "$N" ]; then
   echo "----------------"
   echo "There are $TOTAL_FILES XML files in $TEST_DATA_PATH and $TEST_MANUAL_PATH directories, but only $N_ONLY_FILES"
   echo "were processed. This typically happens when some .xml files are not following the name convention used by"
   echo "the xmlChecker.sh script."
   exit 1
fi

if [ "$ERR" -ne "0" ]; then
   exit 1
fi

exit 0

