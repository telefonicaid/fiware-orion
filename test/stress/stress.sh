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



# -----------------------------------------------------------------------------
#
# Make sure we are where we need to be
#
if [ ! -f scripts/dbReset.sh ]
then
  echo "You need to be on the Orion-LD repository root to run this script"
  exit 1
fi



# -----------------------------------------------------------------------------
#
# usage
#
function usage()
{
  sfile="Usage: "$(basename $0)
  empty=$(echo $sfile | tr 'a-zA-z/0-9.:' ' ')
  echo "$sfile [-u (usage)]"
  echo "$empty [-v (verbose)]"
  echo "$empty [--attributes|-a (number of attributes for the entities in the test - default: 1000)]"
  echo "$empty [--maxmem|-m (max RAM (in bytes) for the broker - default: 6000000000 (6 gigabytes))]"
  echo "$empty [--entities|-e (number of entities to create before the 'real' test starts - default: 200)]"
  echo "$empty [--threads|-t (number of threads for the load test - default: 10)]"
  echo "$empty [--requests|-r (total number of requests for the load test - default: 10000)]"
  echo "$empty <test case> (path to the file to be executed in this stress test)"
  exit 1
}



# -----------------------------------------------------------------------------
#
# Parse options
#
verbose=off
attributes=1000
brokerMaxMem=6000000000
entities=200
threads=10
requests=10000
testCase=/X
while [ "$#" != 0 ]
do
    if   [ "$1" == "-u" ];             then usage;
    elif [ "$1" == "-v" ];             then verbose=on;
    elif [ "$1" == "--attributes" ] || [ "$1" == "-a" ]; then attributes="$2";   shift;
    elif [ "$1" == "--maxmem" ]     || [ "$1" == "-m" ]; then brokerMaxMem="$2"; shift;
    elif [ "$1" == "--entities" ]   || [ "$1" == "-e" ]; then entities="$2";     shift;
    elif [ "$1" == "--threads" ]    || [ "$1" == "-t" ]; then threads="$2";      shift;
    elif [ "$1" == "--requests" ]   || [ "$1" == "-r" ]; then requests="$2";     shift;
    else
        if [ $testCase != "/X" ]
        then
            echo "Error: only one test case at a time"
            echo "Offending parameter: $1"
            exit 1
        fi
        testCase=$1
        fi
  shift
done



# -----------------------------------------------------------------------------
#
# Making sure the test case file is A-OK
#
if [ $testCase == "" ]
then
    echo "Please supply a test file for the stress test"
    usage
fi

if [ ! -f $testCase ]
then
    echo "Error: invalid test case (not a file): $testCase"
    exit 1
fi

if [ ! -x $testCase ]
then
    echo "Error: invalid test case (not executable): $testCase"
    exit 1
fi


#
# All OK - let the stre3ss commence!
#


echo "1.1. Killing Orion-LD, if running"
echo "================================="
killall orionld
echo
echo


echo "1.2. Cleaning database - default tenant only"
echo "============================================"
scripts/dbReset.sh orion
echo
echo


echo "1.3. Please start Orion-LD, on port 1026, and press <CR>"
echo "========================================================"
echo "E.g.:"
echo "* valgrind -v --leak-check=full --track-origins=yes --trace-children=yes --suppressions=test/valgrind/suppressions.supp orionld -fg -noswap"
echo "* gdb --args orionld -fg -noswap"
echo "* orionld -fg -noswap"
echo
echo "Also, in case the stress test case ($testCase) uses any tenants other than the default one, perhaps dbReset.sh should be used?"
echo
echo -n "<Press CR once the broker is up and running> "
read X


echo "1.4. Enter the PID of the broker, and press <CR>"
echo "================================================"
ps aux | grep orionld | grep -v grep | grep -v 'emacs ' | grep -v 'gdb '
echo -n	"<Enter the PID of the broker + CR ('0' for no RAM-limit)> "

read PID
if [ "$PID" == "0" ]
then
  echo ""
else
  echo Orion-LD: process ID $PID
  prlimit --pid $PID --as=$brokerMaxMem  # 6 gigabytes, by default
  echo "Limited the RAM usage of the broker to $brokerMaxMem bytes"
fi
echo
echo


echo "1.5. Launching the test case $testCase"
echo "============================================================="
echo
echo
$testCase $requests $threads $entities $attributes
r=$?



if [ "$r" != "0" ]
then
    echo "The test case failed with error code $r"
else
    echo "The test case seems to have worked just fine :)"
fi

exit $r
