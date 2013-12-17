# ------------------------------------------------------------------------------
#
# harnessInit - 
#
if [ "$CONTEXTBROKER_TESTENV_SOURCED" != "YES" ]
then
  echo "scripts/testEnv.sh not sourced - I do it for you"
  source ../../scripts/testEnv.sh
fi



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
    echo 'db.dropDatabase()' | mongo ${BROKER_DATABASE_NAME_AUX} --quiet
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
    CB_START_CMD="contextBroker -harakiri -port ${BROKER_PORT} -pidpath ${BROKER_PID_FILE} -db ${BROKER_DATABASE_NAME} -t $traceLevels"
  elif [ "$role" == "CM" ]
  then
    port=$CM_PORT
    CB_START_CMD="contextBroker -logDir /tmp/configManager -ngsi9 -harakiri -port ${CM_PORT} -pidpath ${BROKER_PID_FILE_AUX} -db ${BROKER_DATABASE_NAME_AUX} -fwdPort ${BROKER_PORT} -t $traceLevels"
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

  localBrokerStop CB
  localBrokerStart CB $traceLevels
}
