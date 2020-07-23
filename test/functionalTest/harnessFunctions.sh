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

# ------------------------------------------------------------------------------
#
# harnessInit - 
#

if [ "$CONTEXTBROKER_TESTENV_SOURCED" != "YES" ]
then
  echo
  echo '---------------------------------------------------'
  echo "Test Environment missing - please source testEnv.sh"
  echo '---------------------------------------------------'
  echo
  exit 1
fi

export CONTEXTBROKER_HARNESS_FUNCTIONS_SOURCED="YES"

if [ "$ORION_FT_DEBUG" == "1" ]
then
  _debug='on'
else
  _debug='off'
fi



# ------------------------------------------------------------------------------
#
# harnessExit - 
#
function harnessExit()
{
  rm -f headers.out
}



# ------------------------------------------------------------------------------
#
# dbInit - 
#
function dbInit()
{
  role=$1  
  tenant=$2
  
  host="${CB_DATABASE_HOST}"
  if [ "$host" == "" ]
  then
    host="localhost"
  fi
  
  port="${CB_DATABASE_PORT}"
  if [ "$port" == "" ]
  then
    port="27017"
  fi
  
  logMsg initializing database for $role

  if [ "$role" == "CB" ]
  then
    baseDbName=${CB_DB_NAME}
  elif [ "$role" == "CP1" ]
  then
    baseDbName=${CP1_DB_NAME}
  elif [ "$role" == "CP2" ]
  then
    baseDbName=${CP2_DB_NAME}
  elif [ "$role" == "CP3" ]
  then
    baseDbName=${CP3_DB_NAME}
  elif [ "$role" == "CP4" ]
  then
    baseDbName=${CP4_DB_NAME}
  elif [ "$role" == "CP5" ]
  then
    baseDbName=${CP5_DB_NAME}
  else
    baseDbName=$1
  fi

  # If a tenant was provided, then we build the tenant DB, e.g. orion-ftest1
  if [ $# == 2 ]
  then 
    db=$baseDbName-$tenant
  else
    db=$baseDbName
  fi

  logMsg "database to drop: <$db>" 
  echo 'db.dropDatabase()' | mongo mongodb://$host:$port/$db --quiet
}



# ------------------------------------------------------------------------------
#
# dbDrop - 
#
function dbDrop()
{
  if [ "$CB_DB_DROP" != "No" ]
  then
    # FIXME: Not sure if $@ should be better...
    dbInit $*
  fi
}


# -----------------------------------------------------------------------------
#
# dbList
#
function dbList
{
  name="$1"

  host="${CB_DATABASE_HOST}"
  if [ "$host" == "" ]
  then
    host="localhost"
  fi

  port="${CB_DATABASE_PORT}"
  if [ "$port" == "" ]
  then
    port="27017"
  fi

  if [ "$name" != "" ]
  then
    echo show dbs | mongo mongodb://$host:$port --quiet | grep "$name" | awk '{ print $1 }'
  else
    echo show dbs | mongo mongodb://$host:$port --quiet | awk '{ print $1 }'
  fi
}



# ------------------------------------------------------------------------------
#
# dbResetAll - 
#
function dbResetAll()
{
  host="${CB_DATABASE_HOST}"
  if [ "$host" == "" ]
  then
    host="localhost"
  fi
  
  port="${CB_DATABASE_PORT}"
  if [ "$port" == "" ]
  then
    port="27017"
  fi
  
  all=$(echo show dbs | mongo mongodb://$host:$port --quiet | grep ftest | awk '{ print $1 }')
  for db in $all
  do
    dbDrop $db
  done
}



# ------------------------------------------------------------------------------
#
# brokerStopAwait
#
function brokerStopAwait
{
  port=$1

  typeset -i loopNo
  typeset -i loops
  loopNo=0
  loops=50

  logMsg "Awaiting broker to stop (broker on port $port)"
  while [ $loopNo -lt $loops ]
  do
    logMsg "Calling nc: nc -w 2 localhost $port"
    nc -zv localhost $port &>/dev/null </dev/null
    r=$? # 0 == OK
    logMsg "nc returned $r"
    if [ "$r" != "0" ]
    then
      logMsg The orion context broker on port $port has stopped
      sleep 1
      break;
    fi

    logMsg Awaiting orion context broker to fully stop '('$loopNo')' ...
    sleep .2
    loopNo=$loopNo+1
  done

  sleep .5

  # Make sure that is CB is NOT running
  curl -s localhost:${port}/version | grep version > /dev/null
  result=$?
  if [ "$result" == "0" ]
  then
    result=1  # ERROR - the broker is still running
  else
    result=0
  fi
}



# ------------------------------------------------------------------------------
#
# brokerStartAwait
#
# For some really strange reason, all functests fail under valgrind when executed by Jenkins
# without the line:
#
#   echo "Broker started after $loopNo checks" >> /tmp/brokerStartCounter
#
# It also works with echo to /dev/null, but using a file we get some more information.
#
# As we only append (>>) to /tmp/brokerStartCounter in this file, /tmp/brokerStartCounter
# is reset in testHarness.sh, using the command "date > tmp/brokerStartCounter".
#
# The reason we send information to a file is that in case the 100 loop is not enough in the future,
# we will have that information in the file /tmp/brokerStartCounter 
#
function brokerStartAwait
{
  result=0

  # If BROKER_AWAIT_SLEEP_TIME is set, then we just sleep instead of using 'nc'
  if [ "$BROKER_AWAIT_SLEEP_TIME" != "" ]
  then
    sleep $BROKER_AWAIT_SLEEP_TIME
    return
  fi

  port=$1

  typeset -i loopNo
  typeset -i loops
  loopNo=0
  loops=100

  while [ $loopNo -lt $loops ]
  do
    nc -zv localhost $port &>/dev/null </dev/null
    if [ "$?" == "0" ]
    then
      logMsg The orion context broker has started, listening on port $port
      echo "Broker started after $loopNo checks" >> /tmp/brokerStartCounter
      break;
    fi

    logMsg Awaiting valgrind to fully start the orion context broker '('$loopNo')' ...
    sleep .2
    loopNo=$loopNo+1
  done

  if [ $loopNo == 100 ]
  then
    echo "The context broker didn't start!"
    result=1
    return
  fi

  sleep .5
}



# ------------------------------------------------------------------------------
#
# localBrokerStart
#
function localBrokerStart()
{
  role=$1
  traceLevels=$2
  ipVersion=$3

  POOL_SIZE=${POOL_SIZE:-10}

  shift
  shift
  shift

  if [ "$traceLevels" != "" ]
  then
    extraParams="-logLevel DEBUG "$*
  else
    extraParams=$*
  fi

  IPvOption=""

  #
  # In case a previous broker died in a bad manner and the PID file has not been removed
  #
  \rm -f /tmp/orion_*.pid

  dbHost="${CB_DATABASE_HOST}"
  if [ "$dbHost" == "" ]
  then
    dbHost="localhost"
  fi
  
  dbPort="${CB_DATABASE_PORT}"
  if [ "$dbPort" == "" ]
  then
    dbPort="27017"
  fi

  logMsg starting broker for role $role

  if [ "$ipVersion" == "IPV4" ]
  then
    IPvOption="-ipv4" 
  elif [ "$ipVersion" == "IPV6" ]
  then
    IPvOption="-ipv6"
  fi

  CB_START_CMD_PREFIX="$BROKER -harakiri"
  if [ "$role" == "CB" ]
  then
    port=$CB_PORT
    CB_START_CMD="$CB_START_CMD_PREFIX -port $CB_PORT  -pidpath $CB_PID_FILE  -dbhost $dbHost:$dbPort -db $CB_DB_NAME -dbPoolSize $POOL_SIZE -t $traceLevels $IPvOption $extraParams"
  elif [ "$role" == "CP1" ]
  then
    mkdir -p $CP1_LOG_DIR
    port=$CP1_PORT
    CB_START_CMD="$CB_START_CMD_PREFIX -port $CP1_PORT -pidpath $CP1_PID_FILE -dbhost $dbHost:$dbPort -db $CP1_DB_NAME -dbPoolSize $POOL_SIZE -t $traceLevels $IPvOption -logDir $CP1_LOG_DIR $extraParams"
  elif [ "$role" == "CP2" ]
  then
    mkdir -p $CP2_LOG_DIR
    port=$CP2_PORT
    CB_START_CMD="$CB_START_CMD_PREFIX -port $CP2_PORT -pidpath $CP2_PID_FILE -dbhost $dbHost:$dbPort -db $CP2_DB_NAME -dbPoolSize $POOL_SIZE -t $traceLevels $IPvOption -logDir $CP2_LOG_DIR $extraParams"
  elif [ "$role" == "CP3" ]
  then
    mkdir -p $CP3_LOG_DIR
    port=$CP3_PORT
    CB_START_CMD="$CB_START_CMD_PREFIX -port $CP3_PORT -pidpath $CP3_PID_FILE -dbhost $dbHost:$dbPort -db $CP3_DB_NAME -dbPoolSize $POOL_SIZE -t $traceLevels $IPvOption -logDir $CP3_LOG_DIR $extraParams"
  elif [ "$role" == "CP4" ]
  then
    mkdir -p $CP4_LOG_DIR
    port=$CP4_PORT
    CB_START_CMD="$CB_START_CMD_PREFIX -port $CP4_PORT -pidpath $CP4_PID_FILE -dbhost $dbHost:$dbPort -db $CP4_DB_NAME -dbPoolSize $POOL_SIZE -t $traceLevels $IPvOption -logDir $CP4_LOG_DIR $extraParams"
  elif [ "$role" == "CP5" ]
  then
    mkdir -p $CP5_LOG_DIR
    port=$CP5_PORT
    CB_START_CMD="$CB_START_CMD_PREFIX -port $CP5_PORT -pidpath $CP5_PID_FILE -dbhost $dbHost:$dbPort -db $CP5_DB_NAME -dbPoolSize $POOL_SIZE -t $traceLevels $IPvOption -logDir $CP5_LOG_DIR $extraParams"
  fi

  # In case the PID file still exists ... (happens after crashes)
  \rm -f /tmp/orion_${port}.pid

  #  
  # Start broker under valgrind if VALGRIND set to 1 and if it's the 'main' broker
  # 
  # This is IMPORTANT
  # In test cases involving more than **one** broker - valgrind is run only for the CB, not CP1 etc.
  # Having valgrind run for *every broker* destroys the result of the main broker (the 'CB' broker).
  # The 'other' brokers mess up the output file from valgrind, i.e. the result of the main broker 
  # is destroyed.
  # So, now ONLY the broker started as 'CB' is started under VALGRIND
  # 
  # [ A *number* of old leaks were discovered when this modification was made. ]
  #
  if [ "$VALGRIND" == "" ] || [ "$port" != "$CB_PORT" ]
  then
    if [ "$verbose" == "on" ]
    then
      echo Starting broker: $CB_START_CMD
    fi
    $CB_START_CMD
    brokerPid=$!
    if [ "$verbose" == "on" ]
    then
      echo Broker PID: $brokerPid
    fi
  else
    #
    # Important: the -v flag must be present so that the text "X errors in context Y of Z" is present in the output
    #
    valgrind -v --leak-check=full --track-origins=yes --trace-children=yes $CB_START_CMD > /tmp/valgrind.out 2>&1 &
  fi

  # Waiting for broker/valgrind to start
  brokerStartAwait $port
  if [ "$result" != 0 ]
  then
    echo "Unable to start $BROKER as $role"
    exit 1
  else
    if [ "$verbose" == "on" ]
    then
      echo "$BROKER is running as $role"
    fi
  fi

  # Test to see whether we have a broker running on $port. If not raise an error
  brokerPidLines=$(ps -fe | grep $BROKER | grep $port | wc -l)
  if [ $brokerPidLines != 1 ]
  then
    echo "Unable to start $BROKER as $role"
    exit 1
  fi

  ps=$(ps aux | grep $BROKER)
  logMsg $ps

  # Sometimes (especially when using remote DB) CB needs some time to connect DB and
  # the test execution needs to a guard time. The righ value for CB_WAIT_AFTER_START
  # needs to be find empirically (it seems that when DB is localhost this time is
  # not needed at all)
  #
  # This guard could be removed once issue #2123 gets completed
  if [ "$CB_WAIT_AFTER_START" != "" ]; then
    sleep $CB_WAIT_AFTER_START
  fi

  logMsg Started $BROKER as $role
}



# ------------------------------------------------------------------------------
#
# localBrokerStop
#
function localBrokerStop
{
  role=$1

  if [ "$role" == "CB" ]
  then
    port=$CB_PORT
  elif [ "$role" == "CP1" ]
  then
    port=$CP1_PORT
  elif [ "$role" == "CP2" ]
  then
    port=$CP2_PORT
  elif [ "$role" == "CP3" ]
  then
    port=$CP3_PORT
  elif [ "$role" == "CP4" ]
  then
    port=$CP4_PORT
  elif [ "$role" == "CP5" ]
  then
    port=$CP5_PORT
  fi

  # Test to see if we have a broker running on $port if so kill it!
  brokerPidLines=$(ps -fe | grep $BROKER | grep $port | wc -l)
  if [ $brokerPidLines != 0 ]
  then
    kill $(ps -fe | grep $BROKER | grep $port | awk '{print $2}') 2> /dev/null

    # Wait some time so the broker can finish properly
    sleep .5
    brokerPidLines=$(ps -fe | grep $BROKER | grep $port | wc -l)

    if [ $brokerPidLines != 0 ]
    then
      # If the broker refuses to stop politely, kill the process by brute force
      kill -9 $(ps -fe | grep $BROKER | grep $port | awk '{print $2}') 2> /dev/null
      sleep .5
      brokerPidLines=$(ps -fe | grep $BROKER | grep $port | wc -l)
      if [ $brokerPidLines != 0 ]
      then
        echo "Existing $BROKER is immortal, can not be killed!"
        exit 1
      fi
    fi
  fi

  \rm -f /tmp/orion_*.pid
}



# ------------------------------------------------------------------------------
#
# brokerStart
#
function brokerStart()
{
  role=$1
  traceLevels=$2
  ipVersion=$3

  if [ "$CB_EXTERNAL_BROKER" == "ON" ]
  then
    return
  fi

  shift
  shift
  shift

  notificationModeGiven=FALSE

  # Check for --noCache and --cache options in 'extraParams'
  xParams=""
  while [ "$#" != 0 ]
  do
    if   [ "$1" == "--noCache" ];            then noCache=ON;
    elif [ "$1" == "--cache" ];              then noCache=OFF;
    elif [ "$1" == "-noCache" ];             then noCache=ON;
    elif [ "$1" == "-cache" ];               then noCache=OFF;
    elif [ "$1" == "-notificationMode" ] || [ "$1" == "--notificationMode" ]
    then
      notificationModeGiven=TRUE
      xParams="$xParams $1 $2"
      shift
    else
      xParams=$xParams' '$1
    fi
    shift
  done

  if [ "$noCache" != "OFF" ]
  then
    if [ "$CB_NO_CACHE" == "ON" ] || [ "$noCache"  == "ON" ]
    then
      xParams=$xParams' -noCache'
    fi
  fi

  # Not given notificationMode but not forbidden, use default
  if [ "$notificationModeGiven" == "FALSE" ]  &&  [ "$CB_THREADPOOL" == "ON" ]
  then
    xParams=$xParams' -notificationMode threadpool:200:20'
  fi

  if [ "$role" == "" ]
  then
    echo "No role given as first parameter for brokerStart"
    return
  fi

  if [ "$traceLevels" == "" ]
  then
    traceLevels=0-255
  fi

  if [ "$ipVersion" == "" ]
  then
    ipVersion=BOTH
  fi


  localBrokerStop $role
  localBrokerStart $role $traceLevels $ipVersion $xParams
}



# ------------------------------------------------------------------------------
#
# brokerStop - 
#
function brokerStop
{
  logMsg "In brokerStop"
  if [ "$1" == "-v" ]
  then
    _verbose=1
    shift
  fi

  role=$1
  if [ "$role" == "" ]
  then
    role=CB
  fi
 
  logMsg "Stopping broker $1"

  if [ "$role" == "CB" ]
  then
    pidFile=$CB_PID_FILE
    port=$CB_PORT
  elif [ "$role" == "CP1" ]
  then
    pidFile=$CP1_PID_FILE
    port=$CP1_PORT
  elif [ "$role" == "CP2" ]
  then
    pidFile=$CP2_PID_FILE
    port=$CP2_PORT
  elif [ "$role" == "CP3" ]
  then
    pidFile=$CP3_PID_FILE
    port=$CP3_PORT
  elif [ "$role" == "CP4" ]
  then
    pidFile=$CP4_PID_FILE
    port=$CP4_PORT
  elif [ "$role" == "CP5" ]
  then
    pidFile=$CP5_PID_FILE
    port=$CP5_PORT
  fi

  if [ "$VALGRIND" == "" ]
  then
    logMsg "killing with PID from pidFile"
    kill $(cat $pidFile 2> /dev/null) 2> /dev/null
    logMsg "should be dead"
    rm -f /tmp/orion_${port}.pid 2> /dev/null
  else
    logMsg "killing broker by sending /exit/harakiri request"
    curl localhost:${port}/exit/harakiri 2> /dev/null >> ${TEST_BASENAME}.valgrind.stop.out
    # Waiting for valgrind to terminate (sleep a max of 10)
    logMsg "waiting broker to stop"
    brokerStopAwait $port
    logMsg "broker seems to have stopped"
  fi
}



# ------------------------------------------------------------------------------
#
# accumulatorStop - 
#
function accumulatorStop()
{
  port=$1

  # If port is missing, we use the default LISTENER_PORT
  if [ -z "$port" ]
  then
    port=${LISTENER_PORT}
  fi

  pid=$(cat /tmp/accumulator.$port.pid 2> /dev/null)
  if [ "$pid" != "" ]
  then
    kill -15 $pid 2> /dev/null
    sleep .1
    kill -2 $pid 2> /dev/null
    sleep .1
    kill -9 $pid 2> /dev/null
    rm -f /tmp/accumulator.$port.pid
  fi
}


# ------------------------------------------------------------------------------
#
# accumulatorStart - 
#
function accumulatorStart()
{
  # FIXME P6: note that due to the way argument processing work, the arguments have to be
  # in a fixed order in the .test, i.e.: --pretty-print, --https, --key, --cert

  if [ "$1" = "--pretty-print" ]
  then
    pretty="$1"
    shift
  fi

  if [ "$1" = "--https" ]
  then
    https="$1"
    shift
  fi

  if [ "$1" = "--key" ]
  then
    key="$1 $2"
    shift
    shift
  fi

  if [ "$1" = "--cert" ]
  then
    cert="$1 $2"
    shift
    shift
  fi

  bindIp=$1
  port=$2

  # If port is missing, we use the default LISTENER_PORT
  if [ -z "$port" ]
  then
    port=$LISTENER_PORT
  fi

  if [ -z "$bindIp" ]
  then
    bindIp='localhost'
  fi

  accumulatorStop $port

  $REPO_HOME/scripts/accumulator-server.py --port $port --url /notify --host $bindIp $pretty $https $key $cert > /tmp/accumulator_${port}_stdout 2> /tmp/accumulator_${port}_stderr &
  echo accumulator running as PID $$

  # Wait until accumulator has started or we have waited a given maximum time
  port_not_ok=1
  typeset -i time
  time=0

  until [ $port_not_ok -eq 0 ]
  do
   if [ "$time" -eq "$MAXIMUM_WAIT" ]
   then
      echo "Unable to start listening application after waiting ${MAXIMUM_WAIT}"
      exit 1
   fi 
   sleep 1

   time=$time+1
   nc -zv $bindIp $port &>/dev/null </dev/null
   port_not_ok=$?
  done
}



# ------------------------------------------------------------------------------
#
# mqttTestClientStart -
#
function mqttTestClientStart()
{
  logMsg Starting MQTT notification client
  logMsg Parameters for MQTT notification client: $*

  rm -f /tmp/mqttTestClient.log
  rm -f /tmp/mqttTestClient.dump

  $REPO_HOME/scripts/mqttTestClient.py $*   &
  sleep 0.2
  ps aux | grep mqttTestClient > /tmp/kz
  logMsg Started MQTT notification client
}



# ------------------------------------------------------------------------------
#
# mqttTestClientStop -
#
function mqttTestClientStop()
{
  topic=$1
  $REPO_HOME/scripts/mqttSend.py --topic "$topic" --payload exit
}



# ------------------------------------------------------------------------------
#
# mqttTestClientDump -
#
function mqttTestClientDump()
{
  sleep 0.2
  topic=$1
  $REPO_HOME/scripts/mqttSend.py --topic "$topic" --payload dump
  cat /tmp/mqttTestClient.dump
}



# ------------------------------------------------------------------------------
#
# accumulatorDump
#
function accumulatorDump()
{
  valgrindSleep 2

  if [ "$1" == "IPV6" ]
  then
    url="[::1]:${LISTENER_PORT}/dump"
    g_flag="-g"
  else
    url="localhost:${LISTENER_PORT}/dump"
  fi

  if [ "$2" == "HTTPS" ]
  then
    schema="https://"
    k_flag="-k"
  else
    schema="http://"
  fi

  curl $k_flag $g_flag $schema$url -s -S 2> /dev/null
}


# ------------------------------------------------------------------------------
#
# accumulator2Dump
#
function accumulator2Dump()
{
  valgrindSleep 2

  if [ "$1" == "IPV6" ]
  then
    curl -g [::1]:${LISTENER2_PORT}/dump -s -S 2> /dev/null
  else
    curl localhost:${LISTENER2_PORT}/dump -s -S 2> /dev/null
  fi
}


# ------------------------------------------------------------------------------
#
# accumulator3Dump
#
function accumulator3Dump()
{
  valgrindSleep 2

  if [ "$1" == "IPV6" ]
  then
    curl -g [::1]:${LISTENER3_PORT}/dump -s -S 2> /dev/null
  else
    curl localhost:${LISTENER3_PORT}/dump -s -S 2> /dev/null
  fi
}


# ------------------------------------------------------------------------------
#
# accumulatorCount
#
function accumulatorCount()
{
  valgrindSleep 2

  if [ "$1" == "IPV6" ]
  then
    curl -g [::1]:${LISTENER_PORT}/number -s -S 2> /dev/null
  else
    curl localhost:${LISTENER_PORT}/number -s -S 2> /dev/null
  fi
}


# ------------------------------------------------------------------------------
#
# accumulator2Count
#
function accumulator2Count()
{
  valgrindSleep 2

  if [ "$1" == "IPV6" ]
  then
    curl -g [::1]:${LISTENER2_PORT}/number -s -S 2> /dev/null
  else
    curl localhost:${LISTENER2_PORT}/number -s -S 2> /dev/null
  fi
}


# ------------------------------------------------------------------------------
#
# accumulator3Count
#
function accumulator3Count()
{
  valgrindSleep 2

  if [ "$1" == "IPV6" ]
  then
    curl -g [::1]:${LISTENER3_PORT}/number -s -S 2> /dev/null
  else
    curl localhost:${LISTENER3_PORT}/number -s -S 2> /dev/null
  fi
}


# ------------------------------------------------------------------------------
#
# accumulatorReset - 
#
function accumulatorReset()
{
  curl localhost:${LISTENER_PORT}/reset -s -S -X POST
}


# ------------------------------------------------------------------------------
#
# valgrindSleep
#
function valgrindSleep()
{
  if [ "$FUNC_TEST_RUNNING_UNDER_VALGRIND" == "true" ]
  then
    sleep $1
  fi
}



# ------------------------------------------------------------------------------
#
# mongoCmd - 
#
# This functions is needed due to some problems with jenkins that seems to avoid
# the usage of 'mongo --quiet ...' directly. Thus, we need to use mongo without
# --quiet, but we need to get rid of some preamble lines about mongo version and
# connection information and a final 'bye' line
#
function mongoCmd()
{
  host="${CB_DATABASE_HOST}"
  if [ "$host" == "" ]
  then
    host="localhost"
  fi

  port="${CB_DATABASE_PORT}"
  if [ "$port" == "" ]
  then
    port="27017"
  fi

  db=$1
  cmd=$2
  echo $cmd | mongo mongodb://$host:$port/$db | tail -n 2 | head -n 1
}



# ------------------------------------------------------------------------------
#
# mongoCmd2 - like mongoCmd but showing all the output, not just the last line.
#             Meant to be used in conjunction with 'grep'
#
function mongoCmd2()
{
  host="${CB_DATABASE_HOST}"
  if [ "$host" == "" ]
  then
    host="localhost"
  fi

  port="${CB_DATABASE_PORT}"
  if [ "$port" == "" ]
  then
    port="27017"
  fi

  db=$1
  cmd=$2
  echo $cmd | mongo mongodb://$host:$port/$db | grep -v "Implicit session: session" | sed 's/?gssapiServiceName=mongodb//'
}



# ------------------------------------------------------------------------------
#
# dbInsertEntity -
#
# Insert a crafted entity with an "inflated" attribute. The database is passed
# as first argument and the id of the entity as second. The size of the "inflated"
# attribute is the third argument x 10.
#
function dbInsertEntity()
{
  db=$1
  entId=$2
  size=$3

  # This is a JavaScript code that generates a string variable whose length is the
  # third argument multiplicated by 10
  jsCode="s = \"\"; for (var i = 0; i < $size ; i++) { s += \"0123456789\"; }"

  ent="entity = \"$entId\""

  doc='doc = {
    "_id": {
        "id": entity,
        "type": "T"
    },
    "attrNames": [ "A" ],
    "attrs": {
        "A": {
            "type": "TA",
            "value": s,
            "creDate" : 1389376081,
            "modDate" : 1389376081
        },
    },
    "creDate": 1389376081,
    "modDate": 1389376081
  }'

  cmd='db.entities.insert(doc)'

  host="${CB_DATABASE_HOST}"
  if [ "$host" == "" ]
  then
    host="localhost"
  fi

  port="${CB_DATABASE_PORT}"
  if [ "$port" == "" ]
  then
    port="27017"
  fi

  echo "$jsCode ; $ent ; $doc ; $cmd" | mongo mongodb://$host:$port/$db
}


# ------------------------------------------------------------------------------
#
# orionCurl
#
# Options:
#   -X            <HTTP method>    (default: according to curl. GET if no payload, POST if with payload)
#   --host        <host>           (default: localhost)
#   --port        <port>           (default: $CB_PORT)
#   --url         <URL>            (default: empty string)
#   --urlParams   <params>         (URI parameters 'in' URL-string)
#   --payload     <payload>        (default: NO PAYLOAD. Possible values: [filename | "${string}"])
#   --in          (input payload)  (default: xml => application/xml, If 'json': application/json)
#   --out         (output payload  (default: xml => application/xml, If 'json': application/json)
#   --json        (in/out JSON)    (if --in/out is used AFTER --json, it overrides) 
#   --tenant      <tenant>         (tenant in HTTP header)
#   --servicePath <path>           (Service Path in HTTP header)
#   --xauthToken  <token>          (X-Auth token value)
#   --origin      <origin>         (Origin in HTTP header)
#   --correlator  <correlatorId>   (default: no correlator ID)
#   --noPayloadCheck               (don't check the payload)
#   --payloadCheck <format>        (force specific treatment of payload)
#   --linkHeaderFix                (replace hostname and port in Link HTTP header with "IP:PORT")
#   --header      <HTTP header>    (more headers)
#   -H            <HTTP header>    (more headers)
#   --verbose                      (verbose output)
#   -v                             (verbose output)
#   --debug                        (debug mode - output to /tmp/orionFuncTestDebug.log)
#
# Any parameters are sent as is to 'curl'
# 
function orionCurl()
{
  payloadCheckFormat='json'

  logMsg
  logMsg $(date)
  logMsg orionCurl $*

  _host='localhost'
  _port=$CB_PORT
  _url=''
  _method=''
  _payload=''
  _servicePath=''
  _xtra=''
  _headers=''
  _noPayloadCheck='off'
  _forcedNoPayloadCheck='off'
  _tenant=''
  _origin=''
  _correlator=''
  _inFormat=''
  _outFormat='--header "Accept: application/json"'
  _in='';
  _out='';
  _urlParams=''
  _xauthToken=''
  _payloadCheck=''
  _linkHeaderFix=''

  #
  # Parsing parameters
  #
  while [ "$#" != 0 ]
  do
    if   [ "$1" == "--host" ]; then            _host=$2; shift;
    elif [ "$1" == "--port" ]; then            _port=$2; shift;
    elif [ "$1" == "--url" ]; then             _url=$2; shift;
    elif [ "$1" == "--urlParams" ]; then       _urlParams=$2; shift;
    elif [ "$1" == "-X" ]; then                _method="-X $2"; shift;
    elif [ "$1" == "--payload" ]; then         _payload=$2; shift;
    elif [ "$1" == "--noPayloadCheck" ]; then  _noPayloadCheck='on'; _forcedNoPayloadCheck='on'
    elif [ "$1" == "--payloadCheck" ]; then    _payloadCheck=$2; _noPayloadCheck='off'; shift;
    elif [ "$1" == "--servicePath" ]; then     _servicePath='--header "Fiware-ServicePath: '${2}'"'; shift;
    elif [ "$1" == "--tenant" ]; then          _tenant='--header "Fiware-Service: '${2}'"'; shift;
    elif [ "$1" == "--origin" ]; then          _origin='--header "Origin: '${2}'"'; shift;
    elif [ "$1" == "--correlator" ]; then      _correlator='--header "Fiware-Correlator: '${2}'"'; shift;
    elif [ "$1" == "-H" ]; then                _headers=${_headers}" --header \"$2\""; shift;
    elif [ "$1" == "--header" ]; then          _headers=${_headers}" --header \"$2\""; shift;
    elif [ "$1" == "--in" ]; then              _in="$2"; shift;
    elif [ "$1" == "--out" ]; then             _out="$2"; shift;
    elif [ "$1" == "--xauthToken" ]; then      _xauthToken='--header "X-Auth-Token: '${2}'"'; shift;
    elif [ "$1" == "--linkHeaderFix" ]; then   _linkHeaderFix='on'
    elif [ "$1" == "-v" ]; then                _verbose=on;
    elif [ "$1" == "--verbose" ]; then         _verbose=on;
    elif [ "$1" == "--debug" ]; then           _debug=on;
    elif [ "$1" == "-d" ]; then                _debug=on;
    else                                       _xtra="$_xtra $1";
    fi
    shift
  done

  #
  # Remove the old HTTP header file
  #
  \rm -f /tmp/httpHeaders2.out /tmp/httpHeaders.out

  if [ "$_payload" != "" ]
  then
    if [ "$_in" == "jsonld" ]
    then
      _inFormat='--header "Content-Type: application/ld+json"'
    else
      _inFormat='--header "Content-Type: application/json"'
    fi
  fi

  #
  # Check the parameters
  #

  # 1. Do we have a URL present?
  if [ "$_url" == "" ]
  then
    echo "No URL";
    return 1;
  fi

  # 2. Payload can be either data or a path to a file - if data, a file is created with the data as contents
  _PAYLOAD=''
  if [ "$_payload" != "" ]
  then
    if [ -f "$_payload" ]
    then
      _PAYLOAD="-d @$_payload"
    else
      echo $_payload > /tmp/orionFuncTestPayload
      _PAYLOAD="-d @/tmp/orionFuncTestPayload"
    fi
  fi

  # 3. Fix for 'Content-Type' and 'Accept' short names 'xml' and 'json'
  if   [ "$_in"   == "application/xml"     ]; then _in='xml';     fi
  if   [ "$_in"   == "application/json"    ]; then _in='json';    fi
  if   [ "$_in"   == "application/ld+json" ]; then _in='jsonld';  fi
  if   [ "$_out"  == "application/xml"     ]; then _out='xml';    fi
  if   [ "$_out"  == "application/json"    ]; then _out='json';   fi
  if   [ "$_out"  == "application/ld+json" ]; then _out='jsonld'; fi
  if   [ "$_out"  == "text/plain"          ]; then _out='text';   fi

  if   [ "$_in"  == "xml"    ];  then _inFormat='--header "Content-Type: application/xml"'
  elif [ "$_in"  == "json"   ];  then _inFormat='--header "Content-Type: application/json"'
  elif [ "$_in"  == "jsonld" ];  then _inFormat='--header "Content-Type: application/ld+json"'
  elif [ "$_in"  == "text"   ];  then _inFormat='--header "Content-Type: text/plain"'
  elif [ "$_in"  != ""       ];  then _inFormat='--header "Content-Type: '${_in}'"'
  fi

  # Note that payloadCheckFormat is also json in the case of --in xml, as the CB also returns error in JSON in this case
  if   [ "$_out" == "xml" ];   then _outFormat='--header "Accept: application/xml"'; payloadCheckFormat='json'
  elif [ "$_out" == "json" ];  then _outFormat='--header "Accept: application/json"'; payloadCheckFormat='json'
  elif [ "$_out" == "text" ];  then _outFormat='--header "Accept: text/plain"'; _noPayloadCheck='on'
  elif [ "$_out" == "any" ];   then _outFormat='--header "Accept: */*"'; _noPayloadCheck='on'
  elif [ "$_out" == "EMPTY" ]; then _outFormat='--header "Accept:"'
  elif [ "$_out" != "" ];      then _outFormat='--header "Accept: '${_out}'"'; _noPayloadCheck='off'
  fi

  if [ "$_payloadCheck" != "" ]
  then
    payloadCheckFormat=$_payloadCheck
    _noPayloadCheck='off'
  fi

  if [ "$_forcedNoPayloadCheck" == 'on' ]
  then
    _noPayloadCheck='on'
  fi

  logMsg $_in: $_in
  logMsg _out: $_out
  logMsg _outFormat: $_outFormat
  logMsg _inFormat: $_inFormat
  logMsg payloadCheckFormat: $payloadCheckFormat
  logMsg _noPayloadCheck: $_noPayloadCheck


  #
  # Assemble the command
  #
  command='curl "'$_host':'${_port}${_url}'"'

  if [ "$_urlParams"   != "" ]; then  command=${command}'?'${_urlParams};   fi
  if [ "$_PAYLOAD"     != "" ]; then  command=${command}' '${_PAYLOAD};     fi
  if [ "$_method"      != "" ]; then  command=${command}' '${_method};      fi
  if [ "$_tenant"      != "" ]; then  command=${command}' '${_tenant};      fi
  if [ "$_servicePath" != "" ]; then  command=${command}' '${_servicePath}; fi
  if [ "$_inFormat"    != "" ]; then  command=${command}' '${_inFormat};    fi
  if [ "$_outFormat"   != "" ]; then  command=${command}' '${_outFormat};   fi
  if [ "$_origin"      != "" ]; then  command=${command}' '${_origin};      fi
  if [ "$_correlator"  != "" ]; then  command=${command}' '${_correlator};  fi
  if [ "$_xauthToken"  != "" ]; then  command=${command}' '${_xauthToken};  fi
  if [ "$_headers"     != "" ]; then  command=${command}' '${_headers};     fi
  if [ "$_xtra"        != "" ]; then  command=${command}' '${_xtra};        fi

  command=${command}' --header "Expect:"'
  command=${command}' -s -S --dump-header /tmp/httpHeaders.out'
  logMsg command: $command


  #
  # Execute the command
  #
  logMsg Executing the curl-command
  eval $command > /tmp/orionCurl.response 2> /dev/null
  _response=$(cat /tmp/orionCurl.response)

  if [ ! -f /tmp/httpHeaders.out ]
  then
    echo "Broker seems to have died ..."
  else
    cat /tmp/httpHeaders.out  | sed 's///g' > /tmp/httpHeaders.noCtrlM
    mv /tmp/httpHeaders.noCtrlM /tmp/httpHeaders.out

    _responseHeaders=$(cat /tmp/httpHeaders.out)

    #
    # Substitute the IP of the host where the test is running for the string IP
    # Why?
    # Ran into troubles matching the Link HTTP Header using Python REGEX:
    #
    # Link: <http://REGEX(.*):9999/ngsi-ld/ex/v1/contexts/urn:ngsi-ld:T:12:13:14>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
    #
    # Some of the chars <"<; probably fucks REGEX up.
    # I tried everything, like replacing these chars for something else, ('<' => '-LT-', etc) etc but nothing worked.
    # Removing the hostname removed the need for a REGEX, so ...
    #
    if [ "$_linkHeaderFix" == "on" ]
    then
      hostName=$(hostname)
      sed "s/$hostName/IP/" /tmp/httpHeaders.out > /tmp/httpHeaders2.out
      sed "s/$CB_PORT/PORT/" /tmp/httpHeaders2.out > /tmp/httpHeaders.out
    fi

    #
    # Remove "Connection: Keep-Alive" and "Connection: close" headers
    #
    sed '/Connection: Keep-Alive/d' /tmp/httpHeaders.out  > /tmp/httpHeaders2.out
    sed '/Connection: close/d'      /tmp/httpHeaders2.out > /tmp/httpHeaders.out
    sed '/Connection: Close/d'      /tmp/httpHeaders.out
  fi

  #
  # Print and beautify response body, if any - and if option --noPayloadCheck hasn't been set
  #
  if [ "$_noPayloadCheck" == "on" ]
  then
    cat /tmp/orionCurl.response
    echo
    rm -f /tmp/orionCurl.response
  else
    if [ "$_response" != "" ]
    then
      logMsg buffer to $payloadCheckFormat beautify: $_response

      if [ "$payloadCheckFormat" == json ] || [ "$payloadCheckFormat" == "" ]
      then
        logMsg Running python tool for $_response
        #
        # We need to apply pretty-print on _response. Otherwise positional processing used in .test
        # (e.g. to get SUB_ID typically grep and awk are used) will break
        #
        _response=$(echo $_response | python -mjson.tool)
        echo "$_response"
      else
        logMsg Unknown payloadCheckFormat
      fi
    fi
  fi
}



# -----------------------------------------------------------------------------
#
# dateDiff - calculate diff in seconds between two date strings
#
function dateDiff()
{
  typeset -i t1
  typeset -i t2
  typeset -i diff

  t1=$(date -d"$1" +%s)
  t2=$(date -d"$2" +%s)

  diff=$t1-$t2

  if [ $diff -lt 0 ]; then diff=-$diff; fi

  echo $diff
}



# -----------------------------------------------------------------------------
#
# eqTimestamp - make sure two timestamps are equal
#
function eqTimestamp()
{
  name=$1
  ts1="$2"
  ts2="$3"
  if [ "$ts1" != "$ts2" ]
  then
    echo ERROR: Timestamp $name has changed: \"$ts1\", \"$ts2\"
  else
    echo OK: Timestamp $name has not changed
  fi
}



# -----------------------------------------------------------------------------
#
# neqTimestamp - make sure two timestamps differ
#
function neqTimestamp()
{
  name=$1
  ts1="$2"
  ts2="$3"
  if [ "$ts1" == "$ts2" ]
  then
    echo ERROR: Timestamp $name has not changed: \"$ts1\"
  else
    echo OK: Timestamp $name has changed
  fi
}



export -f dbInit
export -f dbList
export -f dbDrop
export -f dbResetAll
export -f brokerStart
export -f localBrokerStop
export -f localBrokerStart
export -f brokerStop
export -f accumulatorStart
export -f accumulatorStop
export -f accumulatorDump
export -f accumulator2Dump
export -f accumulator3Dump
export -f accumulatorCount
export -f accumulator2Count
export -f accumulator3Count
export -f accumulatorReset
export -f orionCurl
export -f dbInsertEntity
export -f mongoCmd
export -f mongoCmd2
export -f valgrindSleep
export -f brokerStartAwait
export -f brokerStopAwait
export -f dateDiff
export -f mqttTestClientStart
export -f mqttTestClientStop
export -f mqttTestClientDump
export -f eqTimestamp
export -f neqTimestamp
