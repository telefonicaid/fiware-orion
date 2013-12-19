# FIXME P1 - This doesn't work if the test is in a subdirectory such as xmlParse
# ------------------------------------------------------------------------------
#
# harnessInit - 
#
# if [ "$CONTEXTBROKER_TESTENV_SOURCED" != "YES" ]
# then
#   source ../../scripts/testEnv.sh
# fi



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

  if [ "$role" == "CB" ]
  then
    echo 'db.dropDatabase()' | mongo ${BROKER_DATABASE_NAME} --quiet
  elif [ "$role" == "CM" ]
  then
    echo 'db.dropDatabase()' | mongo ${BROKER_DATABASE_AUX_NAME} --quiet
  fi
}



# ------------------------------------------------------------------------------
#
# localBrokerStart
#
function localBrokerStart()
{
  role=$1
  traceLevels=$2


  if [ "$role" == "CB" ]
  then
    port=$BROKER_PORT
    CB_START_CMD="contextBroker -harakiri -port ${BROKER_PORT} -pidpath ${BROKER_PID_FILE}     -db ${BROKER_DATABASE_NAME}     -t $traceLevels"
  elif [ "$role" == "CM" ]
  then
    mkdir -p /tmp/configManager
    port=$CM_PORT
    CB_START_CMD="contextBroker -harakiri -port ${CM_PORT}     -pidpath ${BROKER_PID_FILE_AUX} -db ${BROKER_DATABASE_AUX_NAME} -t $traceLevels -fwdPort ${BROKER_PORT} -logDir /tmp/configManager -ngsi9 "
  fi

  if [ "$VALGRIND" == "" ]; then
    $CB_START_CMD
    # Wait some time so that the contextBroker is able to do its initial steps (reset database, start HTTP server, etc.)
    sleep 1
  else
    valgrind $CB_START_CMD > ${TEST_BASENAME}.valgrind.out 2>&1 &
    # Waiting for valgrind to start (sleep 10)
    sleep 10s
  fi

  # Test to see whether we have a broker running on $port. If not raise an error
  running_broker=$(ps -fe | grep contextBroker | grep $port | wc -l)
  if [ $running_broker -ne 1 ]; then
    echo "Unable to start contextBroker"
    exit 1
  fi
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
    port=$BROKER_PORT
  else
    port=CM_PORT
  fi

  # Test to see if we have a broker running on $port if so kill it!
  running_broker=$(ps -fe | grep contextBroker | grep $port | wc -l)
  if [ $running_broker -ne 0 ]; then
    kill $(ps -fe | grep contextBroker | grep $port | awk '{print $2}')
    # Wait some time so the broker can finish properly
    sleep 1
    running_broker=$(ps -fe | grep contextBroker | grep $port | wc -l)
    if [ $running_broker -ne 0 ]; then
      # If the broker refuses to stop politely, kill the process by brute force
      kill -9 $(ps -fe | grep contextBroker | grep $port | awk '{print $2}')
      sleep 1
      running_broker=$(ps -fe | grep contextBroker | grep $port | wc -l)
      if [ $running_broker -ne 0 ]; then
        echo "Existing contextBroker is inmortal, can not be killed!"
        exit 1
      fi
    fi
  fi
}



# ------------------------------------------------------------------------------
#
# brokerStart
#
function brokerStart()
{
  role=$1
  traceLevels=$2

  if [ "$role" == "" ]
  then
    echo "No role given as first parameter for brokerStart"
    return
  fi

  if [ "$traceLevels" == "" ]
  then
    traceLevels=0-255
  fi

  localBrokerStop $role
  localBrokerStart $role $traceLevels
}



# ------------------------------------------------------------------------------
#
# brokerStop - 
#
function brokerStop
{
  role=$1

  if [ "$role" == "CB" ]
  then
    pidFile=$BROKER_PID_FILE
    port=$BROKER_PORT
  elif [ "$role" == "CM" ]
  then
    pidFile=$BROKER_PID_FILE_AUX
    port=$CM_PORT
  fi

  if [ "$VALGRIND" == "" ]; then
    kill $(cat $pidFile)
    rm /tmp/orion_${port}.pid
  else
    curl localhost:${port}/exit/harakiri >> ${TEST_BASENAME}.valgrind.stop.out
    # Waiting for valgrind to terminate (sleep 10)
    sleep 10
  fi
}



# ------------------------------------------------------------------------------
#
# accumulatorStop - 
#
function accumulatorStop()
{
  kill $(curl localhost:${LISTENER_PORT}/pid -s -S)
  sleep 1

  running_app=$(ps -fe | grep accumulator-server | grep ${LISTENER_PORT} | wc -l)

  if [ $running_app -ne 0 ]
  then
    kill $(ps -fe | grep accumulator-server | grep ${LISTENER_PORT} | awk '{print $2}')
    # Wait some time so the accumulator can finish properly
    sleep 1
    running_app=$(ps -fe | grep accumulator-server | grep ${LISTENER_PORT} | wc -l)
    if [ $running_app -ne 0 ]
    then
      # If the accumulator refuses to stop politely, kill the process by brute force
      kill -9 $(ps -fe | grep accumulator-server | grep ${LISTENER_PORT} | awk '{print $2}')
      sleep 1
      running_app=$(ps -fe | grep accumulator-server | grep ${LISTENER_PORT} | wc -l)

      if [ $running_app -ne 0 ]
      then
        echo "Existing accumulator-server.py is inmortal, can not be killed!"
        exit 1
      fi
    fi
  fi
}



# ------------------------------------------------------------------------------
#
# accumulatorStart - 
#
function accumulatorStart()
{
  accumulatorStop

  accumulator-server.py ${LISTENER_PORT} /notify &
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
   nc -z localhost ${LISTENER_PORT} 
   port_not_ok=$?
  done
}



# ------------------------------------------------------------------------------
#
# print_xml_with_headers - 
#
function print_xml_with_headers()
{
  cat headers.out
  echo $response | xmllint --format -
  rm headers.out
}



# ------------------------------------------------------------------------------
#
# print_json_with_headers - 
#
function print_json_with_headers()
{
  cat headers.out
  echo "${response}" | python -mjson.tool
  rm headers.out
}



# ------------------------------------------------------------------------------
#
# curlit - 
#
function curlit()
{
  url=$1
  payload=$2
  contenttype=$3
  accept=$4
  extraoptions=$5
  
  params="-s -S --dump-header headers.out "
  
  response=$(echo ${payload} | (curl localhost:${BROKER_PORT}${url} ${params} --header "${contenttype}" --header "${accept}" $extraoptions -d @- ))
  
#   echo "\$(echo ${payload} | (curl localhost:${BROKER_PORT}${url} ${params} --header \"${contenttype}\" --header \"${accept}\" $extraoptions -d @- ))"
}



# ------------------------------------------------------------------------------
#
# curlxml - 
#
function curlxml()
{
  url=$1
  payload=$2
  contenttype=$3
  accept=$4
  extraoptions=$5
  
  params="-s -S --dump-header headers.out "
  
  if [ "$contenttype" == "" ]
  then
    contenttype="Content-Type: application/xml"
  fi
  
  if [ "$accept" == "" ]
  then
    response=$(echo ${payload} | (curl localhost:${BROKER_PORT}${url} ${params} --header "${contenttype}" $extraoptions -d @- ))
  else
    response=$(echo ${payload} | (curl localhost:${BROKER_PORT}${url} ${params} --header "${contenttype}" --header "${accept}" $extraoptions -d @- ))
  fi
  
  print_xml_with_headers
}



# ------------------------------------------------------------------------------
#
# curljson - 
#
function curljson()
{
  url=$1
  payload=$2
  extraoptions=$3
  
  curlit "${url}" "${payload}" "Content-Type: application/json" "Accept: application/json" $extraoptions
  
  print_json_with_headers
}



# ------------------------------------------------------------------------------
#
# curlxmlCM - 
#
function curlxmlCM()
{
  url=$1
  payload=$2
  contenttype=$3
  accept=$4
  extraoptions=$5
  
  params="-s -S --dump-header headers.out "
  
  if [ "$contenttype" == "" ]
  then
    contenttype="Content-Type: application/xml"
  fi
  
  if [ "$accept" == "" ]
  then
    response=$(echo ${payload} | (curl localhost:${CM_PORT}${url} ${params} --header "${contenttype}" $extraoptions -d @- ))
  else
    response=$(echo ${payload} | (curl localhost:${CM_PORT}${url} ${params} --header "${contenttype}" --header "${accept}" $extraoptions -d @- ))
  fi
  
  print_xml_with_headers
}



# ------------------------------------------------------------------------------
#
# curlNoPayload - 
#
function curlNoPayload()
{
  url=$1
  extraoptions=$2
  contenttype=$3
  accept=$4
   
  params="-s -S --dump-header headers.out "
  
  response=$(curl localhost:${BROKER_PORT}${url} ${params} $extraoptions --header "${contenttype}" --header "${accept}")
    
  print_xml_with_headers
}



# ------------------------------------------------------------------------------
#
# curlxmlNoPayload - 
#
function curlxmlNoPayload()
{
  url=$1
  extraoptions=$2
  
  curlNoPayload $url $extraoptions "Content-Type: application/xml" "Accept: application/xml"
}



# ------------------------------------------------------------------------------
#
# curljsonNoPayload - 
#
function curljsonNoPayload()
{
  url=$1
  extraoptions=$2
  
  curlNoPayload $url $extraoptions "Content-Type: application/json" "Accept: application/json"
}