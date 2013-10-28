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

# This suite does two kinds of tests:
#
# 1. "Pure" valgrind tests, which live in this directory with the .vtest extension
# 2. Test harness (that has their own independent evolution), running in a
#    "valgrind-ized mode"
#
# A selector parameter allows to run either group 1, group 2 or both (default is both)
#



# -----------------------------------------------------------------------------
#
# CB_TEST_PORT is used by cbTest.sh as default port
#
export CB_TEST_PORT=9999



# -----------------------------------------------------------------------------
#
# usage
#
function usage()
{
  echo $0 "[-u (usage)] [-v (verbose)] [-filter (test filter)] [-dryrun (don't execute any tests)] [-leakTest (test a memory leak)] <pure|harness>"
  exit $1
}



# -----------------------------------------------------------------------------
#
# vMsg
#
function vMsg()
{
  if [ "$verbose" = "on" ]
  then
    echo $*
  fi
}



# -----------------------------------------------------------------------------
#
# Parsing parameters
#
verbose=off
mode=all
filter=""
dryrun=off
leakTest=off

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];        then usage 0;
  elif [ "$1" == "-v" ];        then verbose=on;
  elif [ "$1" == "-leakTest" ]; then leakTest=on;
  elif [ "$1" == "-filter" ];   then TEST_FILTER=$2; shift;
  elif [ "$1" == "-dryrun" ];   then dryrun=on;
  elif [ "$1" == "pure" ];      then mode=pure;
  elif [ "$1" == "harness" ];   then mode=harness;
  else
    echo $0: bad parameter/option: "'"${1}"'";
    usage 1
  fi
  shift
done

if [ "$dryrun" == "on" ]
then
  okString="DryRun"
else
  okString="OK"
fi

vMsg
vMsg "----- Command line options -----"
vMsg "mode: $mode"
vMsg "filter: $TEST_FILTER"
vMsg "dryrun: $dryrun"
vMsg "leakTest: $leakTest"
vMsg




# -----------------------------------------------------------------------------
#
# add - 
#
# We need this function because the regular '+' can not process correctly
# numbers with ',', as the ones generated in the valgrind report (e.g. "1,223")
#
function add()
{
  typeset -i num
  typeset -i sum
  sum=0

  for i in $*
  do
    num=$(echo $i | sed 's/,//g')
    sum=$sum+$num
  done

  echo $sum
}



# -----------------------------------------------------------------------------
#
# Start broker
#
function brokerStart()
{
    # Starting contextBroker in valgrind with a clean database
    killall contextBroker 2> /dev/null
    echo 'db.dropDatabase()' | mongo valgrindtest --quiet > /dev/null
    valgrind contextBroker -port ${CB_TEST_PORT} -db valgrindtest -harakiri -t0-255 > ${NAME}.out 2>&1 &
    valgrindPid=$!

    # Awaiting valgrind to start contextBroker (sleep 10)
    typeset -i loopNo
    typeset -i loops
    loopNo=0
    loops=10

    vMsg
    while [ $loopNo -lt $loops ]
    do
      nc -z localhost ${CB_TEST_PORT}
      if [ "$?" == "0" ]
      then
        vMsg The orion context broker has started, listening on port $CB_TEST_PORT
        sleep 1
        break;
      fi
      vMsg Awaiting valgrind to fully start the orion context broker '('$loopNo')' ...
      sleep 1
      loopNo=$loopNo+1
    done

    # Check CB started fine
    curl -s localhost:${CB_TEST_PORT}/version | grep version > /dev/null
    result=$?
}

# -----------------------------------------------------------------------------
#
# Stop broker
#
function brokerStop()
{
  # Sending REST exit to contextBroker
  vMsg Sending REST exit to contextBroker on port $CB_TEST_PORT
  curl -s localhost:${CB_TEST_PORT}/exit/harakiri >> ${NAME}.stop.out

  vMsg Waiting for valgrind to terminate - PID: $valgrindPid
  wait $valgrindPid
}



# -----------------------------------------------------------------------------
#
# Process valgrind result
#
# It uses as argument the .out file to process
#
function processResult()
{

    # Get info from valgrind
    startLine1=$(grep -n "LEAK SUMMARY" $1 | awk -F: '{ print $1 }' | head -1)
    startLine2=$(grep -n "LEAK SUMMARY" $1 | awk -F: '{ print $1 }' | tail -1)

    headEndLine1=$startLine1+7

    head -$headEndLine1 $1 | tail -8 > valgrind.leakSummary

    definitelyLost1=$(grep 'definitely lost' valgrind.leakSummary | awk '{ print $4 }')
    indirectlyLost1=$(grep 'indirectly lost' valgrind.leakSummary | awk '{ print $4 }')
    possiblyLost1=$(grep 'possibly lost' valgrind.leakSummary | awk '{ print $4 }')
    stillReachable1=$(grep 'still reachable' valgrind.leakSummary | awk '{ print $4 }')

    headEndLine2=$startLine2+7

    head -$headEndLine2 $1 | tail -8 > valgrind.leakSummary2

    definitelyLost2=$(grep 'definitely lost' valgrind.leakSummary2 | awk '{ print $4 }')
    indirectlyLost2=$(grep 'indirectly lost' valgrind.leakSummary2 | awk '{ print $4 }')
    possiblyLost2=$(grep 'possibly lost' valgrind.leakSummary2 | awk '{ print $4 }')
    stillReachable2=$(grep 'still reachable' valgrind.leakSummary2 | awk '{ print $4 }')

    \rm valgrind.leakSummary valgrind.leakSummary2

    lost=$(add $definitelyLost1 $indirectlyLost1 $definitelyLost2 $indirectlyLost2)

}

# -----------------------------------------------------------------------------
#
# Printing related functions
#
function printTestLinePrefix()
{
  if [ $testNo -lt 10 ]
  then
    testNoString=" Test 0"${testNo}"/"${noOfTests}": "
  else
    testNoString="Test "${testNo}"/"${noOfTests}": "
  fi
}

function printNotImplementedString()
{
  echo " NOT IMPLEMENTED - skipped"
  echo                                    >> /tmp/valgrindTestSuiteLog
  echo "$1 not implemented - skipped"     >> /tmp/valgrindTestSuiteLog
  echo                                    >> /tmp/valgrindTestSuiteLog
}

function printImplementedString()
{
  echo '# ---------------------------------------------' >> /tmp/valgrindTestSuiteLog
  echo '# '                                              >> /tmp/valgrindTestSuiteLog
  echo "# $1"                                            >> /tmp/valgrindTestSuiteLog
  echo '# '                                              >> /tmp/valgrindTestSuiteLog
}



# -----------------------------------------------------------------------------
#
# setNumberOfTests
#
function setNumberOfTests()
{
  noOfTests=0

  if [ "$runPure" -eq "1" ]
  then
    for vtest in $(ls $TEST_FILTER)
    do
      noOfTests=$noOfTests+1
    done
  fi

  if [ "$runHarness" -eq "1" ]
  then
    for file in $(find ../testharness -name \*.test)
    do
      noOfTests=$noOfTests+1
    done
  fi

  if [ "$leakTest" == "on" ]
  then
    noOfTests=$noOfTests+1
  fi

  vMsg "Number Of Tests: " $noOfTests
}



# -----------------------------------------------------------------------------
#
# Failed tests
#
function failedTest()
{
  if [ "$3" != "0" ]
  then
    echo " FAILED (lost: $3). Check $1 for clues"
  fi

  testFailures=$testFailures+1
  failedTests[$testFailures]=$2
  failedTests[$testFailures+1]=$okString
}


# -----------------------------------------------------------------------------
#
# Main program
#

# Port tests
nc -z localhost ${CB_TEST_PORT} 
if [ "$?" == "0" ]
then
   # Sucessful nc means that port CB_TEST_PORT is used, thus exit
   echo "Port $CB_TEST_PORT is in use. Aborting"
   exit 1
fi


vMsg "Running valgrind test suite"
if [ "$leakTest" == "on" ]
then
  runPure=0
  runHarness=0
  vMsg "Selecting only the leak test"
elif [ "$mode" == "pure" ]
then
  runPure=1
  runHarness=0
  vMsg "Selecting only pure valgrind tests"
elif [ "$mode" == "harness" ]
then
  runPure=0
  runHarness=1
  vMsg "Selecting only harness tests"
else
  runPure=1
  runHarness=1
  vMsg "Selecting both pure valgrind and harness tests"
fi

date > /tmp/valgrindTestSuiteLog
TEST_FILTER=${TEST_FILTER:-"*.vtest"}

typeset -i noOfTests
noOfTests=0
setNumberOfTests

declare -A failedTests
typeset -i testNo
typeset -i testFailures
testNo=0;
testFailures=0


if [ "$runPure" -eq "1" ] || [ "$leakTest" == "on" ]
then
  leakTestFile=""
  if [ "$leakTest" == "on" ]
  then
    fileList="leakTest.xtest"
  else
    fileList=$(ls $TEST_FILTER)
  fi

  for vtest in $fileList
  do
    testNo=$testNo+1
    printTestLinePrefix
    echo -n $testNoString $vtest ...

    typeset -i lines
    lines=$(wc -l $vtest | awk '{ print $1 }')
    if [ $lines -lt 23 ]
    then
       printNotImplementedString $vtest
       continue
    fi
    printImplementedString $vtest

    # Executing $vtest
    NAME="./$vtest"
    typeset -i lost
    lost=0

    if [ "$dryrun" == "off" ]
    then
      brokerStart
      if [ "$result" != "0" ]
      then
        echo " context broker didn't start! check test/valgrind/$vtest.out"
        continue
      fi

      vMsg Executing $vtest
      command ./$vtest >> /tmp/valgrindTestSuiteLog 2> /tmp/valgrindTestSuiteLog.stderr    
      vTestResult=$?
      vMsg vTestResult=$vTestResult

      brokerStop

      if [ "$vTestResult" != 0 ]
      then
        echo " FAILURE! Test ended with error code $vTestResult"
        failedTest "test/valgrind/$vtest.*" $vtest 0
      else
        typeset -i headEndLine1
        typeset -i headEndLine2
        processResult ${NAME}.out
      fi
    fi

    if [ "$lost" != "0" ]
    then
      failedTest "test/valgrind/$vtest.*" $vtest $lost
    elif [ "$vTestResult" == 0 ]
    then
      echo " " $okString
    fi

    echo >> /tmp/valgrindTestSuiteLog
    echo >> /tmp/valgrindTestSuiteLog
  done
fi

if [ "$runHarness" -eq "1" ]
then
  #FIXME P2: make TEST_FILTER work for harness case

  if [ "$BROKER_PORT" == "" ]
  then
    echo "$0: please source the test script (scripts/testEnv.sh)"
    exit 1
  fi

  for file in $(find ../testharness -name \*.test | sort)
  do
    htest=$(basename $file | awk -F '.' '{print $1}')
    directory=$(dirname $file)
    testNo=$testNo+1
    printTestLinePrefix
    echo -n $testNoString $htest ...

    # In the case of harness test, we check that the test is implemented checking
    # that the word VALGRIND apears in the file. The heuristic is that if the .test
    # file includes that word, then there is conditional execution depending on if
    # valgrind is used here or not.
    grep VALGRIND $file > /dev/null 2>&1
    if [ "$?" -ne "0" ]
    then
      printNotImplementedString $htest
      continue
    fi

    printImplementedString $htest
    typeset -i lost
    lost=0

    if [ "$dryrun" == "off" ]
    then
      VALGRIND=1 ../../scripts/testHarness.sh $file > /dev/null 2>&1    

      typeset -i headEndLine1
      typeset -i headEndLine2
      processResult ${directory}/${htest}.valgrind.out
    fi

    if [ "$lost" != "0" ]
    then
      failedTest "test/testharness/$htest.valgrind.*" $htest $lost
    else
      echo " " $okString
    fi

  done
fi

if [ "${failedTests[1]}" != "" ]
then
  echo
  echo
  echo "$testFailures valgrind tests failed:"
  typeset -i ix
  ix=0

  while [ $ix -ne $testFailures ]
  do
    ix=$ix+1
    echo "  " ${failedTests[$ix]}
  done

  echo "---------------------------------------"
  exit 1
fi


echo "Great, all valgrind tests ran without any memory leakage"
exit 0
