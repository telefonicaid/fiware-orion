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

#
# libmicrohttpd and the two callbacks for:
#   * Request Received
#     1. START:   Headers+URI Params
#     2. READING: Paylad Body Chunks
#     3. DONE:    Entire Request Received
#
#   * Request Completed
#     4. The response has been sent
#
# As an extra detail, Orion cannot return a response (which is done via MHD, of course)
# until the ENTIRE REQUEST HAS BEEN RECEIVED.
# So, errors found in "START" and "READING" must be stored and not sent until we're in "DONE".
#
#
# Actions taken in each of these 4 steps:
# ------------------------------------------------------------------------------------------------------
#
# 1. Request Received: START
# ------------------------------------------------------------------------------------------------------------------
#   * ciP = new(ConnectionInfo)  (Big malloc - but Orion depends on it, especially MongoBackend - Orion-LD some day will not need it)
#   * orionldStateInit()
#   * verbGet() - checks for invalid verb
#   * serviceLookup(VERB, URL)
#   * Retrieve/Check all URI Params
#   * Retrieve/Check all HTTP Headers
#
# 2. Request Received: READING
# ------------------------------------------------------------------------------------------------------------------
#   * Allocate memory for the payload body buffer (only first time and only if > 32k)
#   * Copy the payload body chunk into its place in the payload body buffer
#
# 3. Request Received: DONE
# ------------------------------------------------------------------------------------------------------------------
#   * Check that all present URI params are supported by the service (the service was looked up in "START")
#   * If 'tenant' given, make sure it exists (not for all requests - not for creation of entities, subscriptions, and registrations)
#   * Check for empty payload for POST/PATCH/PUT
#   * PARSE the Payload Body + numerous checks
#   * Check for valid Content-Type and Accept HTTP headers (+ @context in payload body vs HTTP header)
#   * Treat inline context (parse + add to context cache)
#   * CALL THE SERVICE ROUTINE
#   * Add GEO-index if necessary
#   * Add tenant if necessary (to the brokers 'tenant cache')
#   * If reponse payload body (GET or Any Error) - serialize the KjNode tree info JSON text
#   * RESPOND to the caller
#   * Call the corresponding TRoE function (if turned on) - to populate the TRoE tables
#   * Cleanup
#
# 4. Request Completed
# ------------------------------------------------------------------------------------------------------------------
#   * This is where I will send the notifications, once Orion-LD starts notifying without the help of mongoBackend
#   * More cleanup
#   * Performance, Statistics and Metrics (if turned on)
#
# EXAMPLE RUN:
# kz@xps:context.Orion-LD-6> performanceTest.sh
#
# 0001/1/0: 0000_ngsild/ngsild_performance_POST_entities_100.test ........................................................  08 seconds
# POST /entities:
#   TPUT: Before Service .000174 seconds (average of 100 runs)
#   TPUT: During Service .002187 seconds (average of 100 runs)
#   TPUT: Awaiting Mongo .001531 seconds (average of 100 runs)
#   TPUT: After Service  .000171 seconds (average of 100 runs)
#   TPUT: Entire request .002532 seconds (average of 100 runs)
#   TPUT: Entire request 394 requests per second
#   => "During Service" - "Awaiting Mongo"  == (2187 - 1531) us == 656 microseconds
#
# 0001/1/0: 0000_ngsild/ngsild_performance_GET_entity_out_of_100.test ....................................................  15 seconds
# GET /entities/{EID} among 100:
#   TPUT: Before Service .000106 seconds (average of 100 runs)
#   TPUT: During Service .000447 seconds (average of 100 runs)
#   TPUT: Awaiting Mongo .000419 seconds (average of 100 runs)
#   TPUT: After Service  .000222 seconds (average of 100 runs)
#   TPUT: Entire request .000776 seconds (average of 100 runs)
#   TPUT: Entire request 1288 requests per second
#   => "During Service" - "Awaiting Mongo"  == (447 - 419) us == 28 microseconds
#
# 0001/1/0: 0000_ngsild/ngsild_performance_GET_entity_out_of_10.test .....................................................  09 seconds
# GET /entities/{EID} among 10:
#   TPUT: Before Service .000113 seconds (average of 100 runs)
#   TPUT: During Service .000402 seconds (average of 100 runs)
#   TPUT: Awaiting Mongo .000372 seconds (average of 100 runs)
#   TPUT: After Service  .000230 seconds (average of 100 runs)
#   TPUT: Entire request .000746 seconds (average of 100 runs)
#   TPUT: Entire request 1340 requests per second
#   => "During Service" - "Awaiting Mongo"  == (402 - 372) us == 30 microseconds
#
# 0001/1/0: 0000_ngsild/ngsild_performance_GET_entities_out_of_100.test ..................................................  15 seconds
# GET /entities?type=X among 100:
#   TPUT: Before Service .000110 seconds (average of 100 runs)
#   TPUT: During Service .000629 seconds (average of 100 runs)
#   TPUT: Awaiting Mongo .000611 seconds (average of 100 runs) - this is not just mongo - it's the entire mongoBackend
#   TPUT: After Service  .000187 seconds (average of 100 runs)
#   TPUT: Entire request .000927 seconds (average of 100 runs)
#   TPUT: Entire request 1078 requests per second
#   => "During Service" - "Awaiting Mongo"  == (629 - 611) us == 18 microseconds (can't use as comparison as ALL of mongoBackend is subtracted)
#
# 0001/1/0: 0000_ngsild/ngsild_performance_GET_entities_out_of_10.test ...................................................  10 seconds
# GET /entities?type=X among 10:
#   TPUT: Before Service .000119 seconds (average of 100 runs)
#   TPUT: During Service .000606 seconds (average of 100 runs)
#   TPUT: Awaiting Mongo .000587 seconds (average of 100 runs) - this is not just	mongo -	it's the entire	mongoBackend
#   TPUT: After Service  .000208 seconds (average of 100 runs)
#   TPUT: Entire request .000933 seconds (average of 100 runs)
#   TPUT: Entire request 1071 requests per second
#   => "During Service" - "Awaiting Mongo"  == (606 - 587) us == 19 microseconds (can't use as comparison as ALL of mongoBackend is subtracted)
#
#
#
# SUMMARY:
# ================================================================
# POST /entities:        394 reqs/s
# GET /entities/{EID}:  1325 reqs/s
# GET /entities:        1075 reqs/s
#
# GET /entities/{EID} is 30% faster than GET /entities
# GET /entities/{EID} is >300% faster than POST /entities
# GET /entities       is 250% faster than POST /entities
#


#
# New timestamps
#
# * MHD1 Start
#   * serviceLookup
# * MHD1 End
#
# * MHD2 Start of first call
# * MHD2 End of last call
#
# * MHD3 Start
#   * jsonParse
#   * Service Routine
#     * mongoBackend (if used)
#     * mongo1 (lookup)
#     * mongo2 (update)
#   * Forwarding
#     * DB Query
#     * Handling
#   * TRoE
# * MHD3 End
#
# * MHD4 Start
#   * Notifications
#   * TRoE
# * MHD4 End
#


# -----------------------------------------------------------------------------
#
# Is the broker compiled with performance tests included?
#
nm /usr/bin/orionld | grep performanceTestInluded 2> /dev/null
if [ $? != 0 ]
then
  echo
  echo "  Sorry, can't perform performance tests on the current instance of Orion-LD."
  echo "  Orion-LD hasn't been compiled for performance tests."
  echo "  Enable it by editing 'src/lib/orionld/common/performance.h' - uncomment #define REQUEST_PERFORMANCE 1."
  echo "  Recompile Orion-LD, and try again."
  echo
  exit 1
fi



# -----------------------------------------------------------------------------
#
# perfCalc -
#
function perfCalc
{
  typeset -i skip

  PATTERN="$1"
  TITLE="$2"
  doTput=$3
  skip=$4

  echo PATTERN: $PATTERN >> /tmp/kz
  echo TITLE:   $TITLE >> /tmp/kz

  SUM=0
  TITLE="$PATTERN: ........................................................................"
  TITLE=${TITLE:0:40}

  typeset -i loopNo
  typeset -i sums
  loopNo=0
  sums=0

  for F in $(orionLogView | grep "$PATTERN" | awk -F: '{ print $5 }')
  do
    if [ $loopNo -ge $skip ]
    then
      SUM=$(echo "$SUM + $F" | bc)
      sums=$sums+1
      echo counting $F >> /tmp/kz
    else
      echo Skipping $F >> /tmp/kz
    fi
    loopNo=$loopNo+1
  done

  if [ $sums != 0 ]
  then
    echo sums: $sums >> /tmp/kz
    AVG=$(echo "$SUM * 0.01" | bc)
    echo "$TITLE  $AVG seconds (average of 100 runs)"

    if [ "$doTput" == 1 ]
    then
      TPUT=$(echo "1 / $AVG" | bc)
      echo '=> ' $TPUT requests per second
    fi
  fi

  echo >> /tmp/kz
  echo >> /tmp/kz
}


date > /tmp/kz
# -----------------------------------------------------------------------------
#
# perfCalcLoop -
#
function perfCalcLoop
{
  TITLE="$1"
  skip=$2
  echo
  echo $TITLE:
  echo "------------------------------------------------------"
  echo $TITLE: >> /tmp/kz

  perfCalc "TPUT: Before Service Routine"     "Before SR"         0 $skip
  perfCalc "TPUT: During Service Routine"     "During SR"         0 $skip
  avgDuring=$AVG
  perfCalc "TPUT: After Service Routine"      "After SR"          0 $skip

  perfCalc "TPUT: Awaiting Mongo"             "In Mongo"          0 $skip
  avgAwaitingMongo=$AVG

  perfCalc "TPUT: Payload Parse"              "Payload Parse"     0 $skip
  perfCalc "TPUT: Sending Response"           "Queueing Response" 0 $skip
  perfCalc "TPUT: Rendering Response"         "Render Response"   0 0
  perfCalc "TPUT: TRoE Processing"            "TRoE Processing"   0 0
  perfCalc "TPUT: MHD Delay (send response)"  "MHD Delay"         0 $skip

  duringExceptDb=$(echo "$avgDuring - $avgAwaitingMongo" | bc)
  echo "SR excl DB: ............................  $duringExceptDb (average)"

  perfCalc "TPUT: Entire request"          "Total"            1 $skip

  echo "------------------------------------------------------"
  echo

  echo "==============================================" >> /tmp/kz
  echo >> /tmp/kz
}

ft ngsild_performance_POST_entities_100.test
perfCalcLoop "POST /entities" 0

ft ngsild_performance_GET_entity_out_of_100.test
perfCalcLoop "GET /entities/{EID} among 100" 100

ft ngsild_performance_GET_entity_out_of_10.test
perfCalcLoop "GET /entities/{EID} among 10" 10

ft ngsild_performance_GET_entities_out_of_100.test
perfCalcLoop "GET /entities?type=X among 100" 100

ft ngsild_performance_GET_entities_out_of_10.test
perfCalcLoop "GET /entities?type=X among 10" 10
