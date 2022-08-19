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
# iot_support at tid dot es

#
# FIXME - Capture Ctrl-C and send an /exit/harakiri to the broker
# FIXME - Get a log line in /tmp/contextBroker.log to assure an operation went well ("OK").
#         See logCheck for this. The calls to logCheck is commented until this is resolved.
#
#



# -----------------------------------------------------------------------------
#
# vMsg
#
function vMsg()
{
  if [ "$verbose" = "on" ]
  then
    echo $1
  fi
}



# -----------------------------------------------------------------------------
#
# usage 
#
function usage()
{
  echo $0 "[-u (usage)] [-U (extended usage)] [-v (verbose)] [-fast (fast run)] [-loops <loops (10 by default)>] [-cb (don't start the broker)] [-acc (don't start the accumulator)]"

  if [ "$1" != "continue" ]
  then
    exit
  fi
}



# -----------------------------------------------------------------------------
#
# Usage 
#
function Usage()
{
  usage "continue"
  echo '--------------------------------------------------'
  echo "  -u:     print usage line and exit"
  echo "  -U:     print this extended usage message and exit"
  echo "  -v:     verbose mode"
  echo "  -fast:  Make each loop invoke each part-test less times, thus making the test quicker"
  echo "  -loops: Number of loops - default number of loops is 10"
  echo "  -cb:    do not start the contextBroker, it is already started (normally with gdb or valgrind, or if more than one instance of heavyTest is used)"
  echo "  -acc:   do not start the accumulator, it is already started (normally used when more than one instance of heavyTest is used)"
  echo
  echo "The 'heavyTest' script is intended to exercise the contextBroker in order to find memory leakage,"
  echo "or to check the stability of the broker."
  echo
  echo "In one loop of 'heavyTest', all the URLs that the broker accepts are exercised in three cycles - "
  echo "  - first once,"
  echo "  - then 99 times, and finally"
  echo '  - many times (200-900 - depending on the service).'
  echo
  echo "The third exercise-cycle if skipped if the option '-fast' is issued when starting heavyTest."
  echo
  echo 'A FULL loop (without the "-fast" option) takes around 30 minutes and for a quick round (typically: valgrind test),'
  echo "the following commands would be used:"
  echo
  echo "% valgrind contextBroker -t255 > VALGRIND.out 2>&1"
  echo "% heavyTest -cb -fast"
  echo
  echo "For valgrind to terminate and printout its conclusions the contextBroker process needs to terminate."
  echo "The contextBroker process (when compiled in DEBUG mode) supports a REST command to die gracefully, which"
  echo "is what we need for valgrind; the command to kill the contextBroker is:"
  echo
  echo "% curl localhost:1026/exit/harakiri"
  echo
  echo "The heavyTest script does invoke this REST request when the loops terminates, but if the loops are interrupted"
  echo "by a Ctrl-C or a 'kill' then this invocation of the REST request must be done by hand in order for valgrind to"
  echo "terminate and to print its report."
  echo

  echo 'In the report of valgrind (redirected to the file VALGRIND.out), we especifically look for the LEAK summary, e.g.:'
  echo "  ==31845== LEAK SUMMARY:"
  echo "  ==31845==    definitely lost: 0 bytes in 0 blocks"
  echo "  ==31845==    indirectly lost: 0 bytes in 0 blocks"
  echo "  ==31845==      possibly lost: 28 bytes in 1 blocks"
  echo "  ==31845==    still reachable: 18,936 bytes in 123 blocks"
  echo "  ==31845==         suppressed: 942 bytes in 10 blocks"
  echo 
  echo "The first two counters 'definitely lost' and 'indirectly lost' should be at ZERO, and in a perfect world, 'possibly lost' also."
  echo "If this is not the case ('definitely' and 'indirectly'), view VALGRIND.out in your favorite editor and search these strings to"
  echo "see a backtrace of when the lost memory was allocated. E.g.:"
  echo
  echo '==1919== 28 bytes in 1 blocks are possibly lost in loss record 36 of 98'
  echo '==1919==    at 0x4C2C7A7: operator new(unsigned long) (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)'
  echo '==1919==    by 0x62BF108: std::string::_Rep::_S_create(unsigned long, unsigned long, ...) (in /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.17)'
  echo '==1919==    by 0x62C0AE4: char* std::string::_S_construct<char const*>(...) (in /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.17)'
  echo '==1919==    by 0x62C0BC2: std::basic_string<char, std::char_traits<char>, ... (in /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.17)'
  echo '==1919==    by 0x5E2CF7: exitTreat(ConnectionInfo*, int, std::vector<std::string, std::allocator<std::string> >, ParseData*) (exitTreat.cpp:74)'
  echo '==1919==    by 0x6218DA: restService(ConnectionInfo*, RestService*) (RestService.cpp:158)'
  echo '==1919==    by 0x61C66E: connectionTreat(void*, MHD_Connection*, char const*, char const*, char const*, char const*, unsigned long*, void**) (rest.cpp:419)'
  echo '==1919==    by 0x6AE1D8: call_connection_handler (in /usr/bin/contextBroker)'
  echo '==1919==    by 0x6AE6DF: MHD_connection_handle_idle (in /usr/bin/contextBroker)'
  echo '==1919==    by 0x6B11F8: MHD_handle_connection (in /usr/bin/contextBroker)'
  echo '==1919==    by 0x547BF8D: start_thread (pthread_create.c:311)'
  echo '==1919==    by 0x6B1AE1C: clone (clone.S:113)'
  echo
  echo "This stack trace tells us that we have possibly lost 28 bytes, allocated by 'new' (top stack frame), via the function exitTreat (file exitTreat.cpp, line 74),"
  echo 'called by the function restService (file RestService.cpp, line 158)'
  echo "So, had this been a 'definitely lost', we'd have to see how to gracefully free this allocated memory in order to prevent memory leakage."
  echo '----------------------------------------------------------------------------------------------------'
  echo
  echo "If we encounter a problem with the contextBroker dying for us, we'd start the test with gdb:"
  echo
  echo '% gdb --args contextBroker -fg'
  echo '% heavyTest -cb -fast'
  echo
  echo '--------------------------------------------------'

  exit 1
}


# -----------------------------------------------------------------------------
#
# Params
#
typeset -i loops

loops=10
verbose=off
brokerStart=yes
accStart=yes
fast=off

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];       then usage;
  elif [ "$1" == "-U" ];       then Usage;
  elif [ "$1" == "-v" ];       then verbose=on;
  elif [ "$1" == "-loops" ];   then loops=$2;           shift;
  elif [ "$1" == "-cb" ];      then brokerStart=no;
  elif [ "$1" == "-acc" ];     then accStart=no;
  elif [ "$1" == "-fast" ];    then fast=on;
  fi

  shift
done



# -----------------------------------------------------------------------------
#
# logCheck - 
#
function logCheck()
{
  noOf=$1
  pattern=$2

  lines=$(grep "$pattern" /tmp/contextBroker.log | wc -l)

  # echo checking we have $noOf lines of \' $pattern \' in /tmp/contextBroker.log
  if [ "$lines" != "$noOf" ]
  then
    echo " (ERROR)"
    echo "Found $lines instances of '" $pattern "'. Expected " $noOf
    exit 1
  fi
}



# -----------------------------------------------------------------------------
#
# register
#
function register()
{
  noOf=$1
  concept=$2
  rss[$rssIx]=$(ps -p $pid -o rss=)
  echo " (RSS: "${rss[$rssIx]}" kb)" 
  rssText[$rssIx]="A total of $noOf $concept DONE"
  rssIx=$rssIx+1
}



# -----------------------------------------------------------------------------
#
# report
#
function report()
{
  endDate=$(date)

  echo
  echo '------------------------------------------------'
  echo
  echo "Test started: $startDate"
  echo "Test ended:   $endDate"
  echo
  echo "Resident RAM:"

  typeset -i ix
  ix=0
  while [ $ix -lt $rssIx ]
  do
    echo ${rss[$ix]}: ${rssText[$ix]}
    ix=$ix+1
  done  
}



# -----------------------------------------------------------------------------
#
# Cleanup
#
PATH=../manual:$PATH  # Adding cbTest.sh
date > /tmp/superTestLog
startDate=$(date)
echo Start date: $startDate

vMsg "Preparing processes"



# -----------------------------------------------------------------------------
#
# Start broker
#
if [ "$brokerStart" == "yes" ]
then
  vMsg killing old contextBroker
  killall contextBroker

  vMsg starting contextBroker
  contextBroker -t255 > /tmp/superTestLog 2>&1
  echo "contextBroker started - log file in /tmp/contextBroker.log"
  sleep 1
else
  vMsg using external accumulator
fi

pid=$(ps aux | grep 'contextBroker -t255' | grep -v grep | grep -v gdb | awk '{ print $2 }')
if [ "$pid" == "" ]
then
  echo "Sorry, cannot find the contextBroker process"
  exit 1
fi



# -----------------------------------------------------------------------------
#
# Start the accumulator?
#
accPid=0

if [ "$accStart" == "yes" ]
then
  # Kill the accumulator, if running
  accPid=$(curl localhost:6666/pid 2>> /tmp/superTestLog)
  echo accumulator pid: $accPid >> /tmp/superTestLog
  if [ "$accPid" != "" ]
  then
    vMsg killing old accumulator
    kill $accPid
  fi

  vMsg "starting the accumulator"
  ../../scripts/accumulator-server.py --port 6666 --url /test/sub > /tmp/accumulator-server.log 2>&1 &
  echo "accumulator started - log file in /tmp/accumulator-server.log"
  sleep 1
  vMsg "accumulator running"
else
  vMsg using external accumulator
fi

accPid=$(ps aux | grep 'accumulator-server.py --port 6666' | grep -v grep | awk '{ print $2 }')
if [ "$accPid" == "" ]
then
  echo "Sorry, cannot find the accumulator process"
  exit 1
fi



# -----------------------------------------------------------------------------
#
# Prepare measurements
#
typeset -i matches
matches=0

typeset -i rssIx
rssIx=0
echo -n "Starting"
register 1 "Initial part"



# -----------------------------------------------------------------------------
#
# repeat
#
function repeat()
{
  typeset -i noOfTimes
  typeset -i t

  noOfTimes=$1
  op=$2
  file=$3
  t=0

  while [ $t -lt $noOfTimes ]
  do
    cbTest.sh $op $file > /dev/null 2>&1
    t=$t+1
    echo done test $t of $noOfTimes
  done
}



# -----------------------------------------------------------------------------
#
# partTest
#
function partTest()
{
  typeset -i noOf
  typeset -i matches
  typeset -i initialMatches

  noOf=$1
  op=$2
  file=$3
  concept=$4
  logPattern=$5
  okPattern=$6

  if [ $fast == on ] && [ $noOf -gt 100 ]
  then
    return
  fi

  initialMatches=$(grep "Treating service $logPattern" /tmp/contextBroker.log | wc -l)
  okInitialMatches=$(grep "$okPattern" /tmp/contextBroker.log | wc -l)

  echo -n $(date +%k:%M:%S) " - loop "${loop}" of "${loops}" - making $noOf $concept"
  repeat $noOf $op $file >> /tmp/superTestLog

  matches=$initialMatches+$noOf
  # logCheck $matches "Treating service $logPattern"

  if [ "$6" != "" ] && [ "$6" != "NO PATTERN" ]
  then
    matches=$okInitialMatches+$noOf
    # logCheck $matches "$okPattern"
  fi

  register $noOf "$concept"
}



#
# Capture Ctrl-C
#
function ctrlC()
{
  echo "Caught signal"
  exit 0
}

trap '{ report; exit 11;  }' INT


typeset -i loop
loop=1

date
while [ $loop -le $loops ]
do

  # -----------------------------------------------------------------------------
  #
  # NGSI-9
  #
  partTest 1   rcr          ngsi9.registerContextRequest.ok.valid.xml                        "NGSI9 Registration"                                "/NGSI9/registerContext"  "/NGSI9/registerContext OK (registrationId:"
  partTest 99  rcr          ngsi9.registerContextRequest.ok.valid.xml                        "NGSI9 Registrations"                               "/NGSI9/registerContext"  "/NGSI9/registerContext OK (registrationId:"
  partTest 400 rcr          ngsi9.registerContextRequest.ok.valid.xml                        "NGSI9 Registrations"                               "/NGSI9/registerContext"  "/NGSI9/registerContext OK (registrationId:"
  
  partTest 1   dcar         ngsi9.discoverContextAvailabilityRequest.ok.xml                  "NGSI9 Discovery"                                   "/NGSI9/discoverContextAvailability" "NO PATTERN"
  partTest 99  dcar         ngsi9.discoverContextAvailabilityRequest.ok.xml                  "NGSI9 Discoveries"                                 "/NGSI9/discoverContextAvailability" "NO PATTERN"
  partTest 400 dcar         ngsi9.discoverContextAvailabilityRequest.ok.xml                  "NGSI9 Discoveries"                                 "/NGSI9/discoverContextAvailability" "NO PATTERN" 
  
  
  # -----------------------------------------------------------------------------
  #
  # NGSI-10
  #
  partTest 1   qcr          ngsi10.queryContextRequest.ok.valid.xml                "NGSI10 Query"                                       "/NGSI10/queryContext" "NO PATTERN"
  partTest 99  qcr          ngsi10.queryContextRequest.ok.valid.xml                "NGSI10 Queries"                                     "/NGSI10/queryContext" "NO PATTERN"
  partTest 400 qcr          ngsi10.queryContextRequest.ok.valid.xml                "NGSI10 Queries"                                     "/NGSI10/queryContext" "NO PATTERN"
  
  partTest 1   scr          ngsi10.subscribeContextRequest.ok.valid.xml            "NGSI10 Subscription"                                "/NGSI10/subscribeContext" "NO PATTERN"
  partTest 99  scr          ngsi10.subscribeContextRequest.ok.valid.xml            "NGSI10 Subscriptions"                               "/NGSI10/subscribeContext" "NO PATTERN"
  partTest 400 scr          ngsi10.subscribeContextRequest.ok.valid.xml            "NGSI10 Subscriptions"                               "/NGSI10/subscribeContext" "NO PATTERN"
  
  partTest 1   ucsr         ngsi10.updateContextSubscriptionRequest.ok.valid.xml   "NGSI10 UpdateSubscription"                          "/NGSI10/updateContextSubscription" "NO PATTERN"
  partTest 99  ucsr         ngsi10.updateContextSubscriptionRequest.ok.valid.xml   "NGSI10 UpdateSubscriptions"                         "/NGSI10/updateContextSubscription" "NO PATTERN"
  partTest 400 ucsr         ngsi10.updateContextSubscriptionRequest.ok.valid.xml   "NGSI10 UpdateSubscriptions"                         "/NGSI10/updateContextSubscription" "NO PATTERN"
  
  partTest 1   uncr         ngsi10.unsubscribeContextRequest.ok.valid.xml          "NGSI10 Unsubscribe"                                 "/NGSI10/unsubscribeContext" "NO PATTERN"
  partTest 99  uncr         ngsi10.unsubscribeContextRequest.ok.valid.xml          "NGSI10 Unsubscribe"                                 "/NGSI10/unsubscribeContext" "NO PATTERN"
  partTest 400 uncr         ngsi10.unsubscribeContextRequest.ok.valid.xml          "NGSI10 Unsubscribe"                                 "/NGSI10/unsubscribeContext" "NO PATTERN"
 
  partTest 1   upcr         ngsi10.updateContextRequest.ok.valid.xml               "NGSI10 Update"                                      "/NGSI10/updateContext" "NO PATTERN"
  partTest 99  upcr         ngsi10.updateContextRequest.ok.valid.xml               "NGSI10 Updates"                                     "/NGSI10/updateContext" "NO PATTERN"
  partTest 400 upcr         ngsi10.updateContextRequest.ok.valid.xml               "NGSI10 Updates"                                     "/NGSI10/updateContext" "NO PATTERN"
  
  partTest 1   ncr          ngsi10.notifyContextRequest.ok.valid.xml               "NGSI10 Notification"                                "/NGSI10/notifyContext" "NO PATTERN"
  partTest 99  ncr          ngsi10.notifyContextRequest.ok.valid.xml               "NGSI10 Notifications"                               "/NGSI10/notifyContext" "NO PATTERN"
  partTest 400 ncr          ngsi10.notifyContextRequest.ok.valid.xml               "NGSI10 Notifications"                               "/NGSI10/notifyContext" "NO PATTERN"
  
  
  
  # -----------------------------------------------------------------------------
  #
  # NGSI-9 Convenience operations
  #
  partTest 1   conv/ce      ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntity Convenience"             "/NGSI9/contextEntities/ENTITY_ID" "NO PATTERN"
  partTest 99  conv/ce      ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntity Convenience"             "/NGSI9/contextEntities/ENTITY_ID" "NO PATTERN"
  partTest 200 conv/ce      ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntity Convenience"             "/NGSI9/contextEntities/ENTITY_ID" "NO PATTERN"
  
  partTest 1   conv/cea     ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntityAttributes Convenience"   "/NGSI9/contextEntities/ENTITY_ID/attributes" "NO PATTERN"
  partTest 99  conv/cea     ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntityAttributes Convenience"   "/NGSI9/contextEntities/ENTITY_ID/attributes" "NO PATTERN"
  partTest 200 conv/cea     ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntityAttributes Convenience"   "/NGSI9/contextEntities/ENTITY_ID/attributes" "NO PATTERN"
  
  partTest 1   conv/ceaa    ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntityAttribute Convenience"    "/NGSI9/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME" "NO PATTERN"
  partTest 99  conv/ceaa    ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntityAttribute Convenience"    "/NGSI9/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME" "NO PATTERN"
  partTest 200 conv/ceaa    ngsi9.registerProviderRequest.ok.postponed.xml    "NGSI9 ContextEntityAttribute Convenience"    "/NGSI9/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME" "NO PATTERN"
  
  
  
  # -----------------------------------------------------------------------------
  #
  # NGSI-10 Convenience operations
  #
  partTest 1   conv/ce10    ngsi10.appendContextElementRequest.ok.postponed.xml   "NGSI10 ContextEntity Convenience"            "/NGSI10/contextEntities/ENTITY_ID" "NO PATTERN"
  partTest 99  conv/ce10    ngsi10.appendContextElementRequest.ok.postponed.xml   "NGSI10 ContextEntity Convenience"            "/NGSI10/contextEntities/ENTITY_ID" "NO PATTERN"
  partTest 200 conv/ce10    ngsi10.appendContextElementRequest.ok.postponed.xml   "NGSI10 ContextEntity Convenience"            "/NGSI10/contextEntities/ENTITY_ID" "NO PATTERN"

  partTest 1   conv/cea10   ngsi10.appendContextElementRequest.ok.postponed.xml   "NGSI10 ContextEntityAttributes Convenience"  "/NGSI10/contextEntities/ENTITY_ID/attributes" "NO PATTERN"
  partTest 99  conv/cea10   ngsi10.appendContextElementRequest.ok.postponed.xml   "NGSI10 ContextEntityAttributes Convenience"  "/NGSI10/contextEntities/ENTITY_ID/attributes" "NO PATTERN"
  partTest 200 conv/cea10   ngsi10.appendContextElementRequest.ok.postponed.xml   "NGSI10 ContextEntityAttributes Convenience"  "/NGSI10/contextEntities/ENTITY_ID/attributes" "NO PATTERN"

  partTest 1   conv/ceaa10  ngsi10.updateContextAttributeRequest.ok.postponed.xml "NGSI10 ContextEntityAttribute Convenience"   "/NGSI10/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME" "NO PATTERN"
  partTest 99  conv/ceaa10  ngsi10.updateContextAttributeRequest.ok.postponed.xml "NGSI10 ContextEntityAttribute Convenience"   "/NGSI10/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME" "NO PATTERN"
  partTest 200 conv/ceaa10  ngsi10.updateContextAttributeRequest.ok.postponed.xml "NGSI10 ContextEntityAttribute Convenience"   "/NGSI10/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME" "NO PATTERN"
  
  loop=$loop+1
done



# -----------------------------------------------------------------------------
#
# Terminate the test
#
report
curl localhost:1026/exit/harakiri
kill -9 $accPid
exit 0
