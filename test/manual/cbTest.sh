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

# -----------------------------------------------------------------------------
#
# CONTENT_TYPE
#
# Default value is application/xml, but this is overridden by
# exporting the env var CONTENT_TYPE
#
export OUT_FORMAT="Accept: application/xml"
export IN_FORMAT="Content-Type: application/xml"



# -----------------------------------------------------------------------------
#
# verboseMsg - 
#
function verboseMsg()
{
  if [ "$verbose" == "on" ] || [ "$verbose" == "1" ]
  then
    echo $*
  fi
}



# -----------------------------------------------------------------------------
#
# usage - 
#
function usage()
{
  echo usage: $0 '[-host <host>] [-port <p>] [-v (verbose)] [-lint (use xmllint)] [-cop <convenience operation>] [-X <method>] [--tenant <tenant>] [--httpTenant <tenant>] [--params <URI parameters>] [-enId <entityId>] [-enType <entityType>] [-attr (attributeName)] [-id <reg/sub id>] [-json (in and out-format in JSON)] [-informat (in-format)] [-outformat (out-format)] [-table <table to debug>] [--https] <operation> <data file>'

  verbose=1
  verboseMsg "Operations:"
  verboseMsg "  rcr:    registerContext"
  verboseMsg "  rcrs:   registerContextResponse"
  verboseMsg "  dcar:   discoverContextAvailability"

  verboseMsg "  qcr:    queryContext"
  verboseMsg "  scr:    subscribeContext"
  verboseMsg "  ucsr:   updateContextSubscription"
  verboseMsg "  uncr:   unsubscribeContext"
  verboseMsg "  upcr:   updateContext"
  verboseMsg "  ncr:    notifyContext"
  verboseMsg "  conv:   convenience operation"
  verboseMsg "  debug:  debug lists"

  verboseMsg ""
  verboseMsg "Convenience Operations:"
  verboseMsg "  ce:     contextEntities/ENTITY_ID"
  verboseMsg "  cea:    contextEntities/ENTITY_ID/attributes"
  verboseMsg "  ceaa:   contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME"
  verboseMsg "  acet:   v1/contextEntities/type/ENTITY_TYPE/id/ENTITY_ID"
  exit 1
}



# -----------------------------------------------------------------------------
#
# Option/Parameter variables
#
verbose=off
host=localhost
port=${CB_TEST_PORT:-1026}
convOp=""
method=POST
operation=""
dataFile=""
id=""
entityId=""
entityType=""
attributeName=""
table=""
protocol='http'
cert=""
tenant=""
httpTenant=""
uriParams=""


# -----------------------------------------------------------------------------
#
# Parsing command line options
#
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];           then usage;
  elif [ "$1" == "-v" ];           then verbose=on; CURL_VERBOSE='-vvvvv'
  elif [ "$1" == "-lint" ];        then useXmlLint=1;
  elif [ "$1" == "--https" ];      then protocol='https'; cert='--cacert ../../security/localhost.pem';
  elif [ "$1" == "--tenant" ];     then tenant=$2;                      shift;
  elif [ "$1" == "--params" ];     then uriParams=$2;                   shift;
  elif [ "$1" == "--httpTenant" ]; then httpTenant=$2;                  shift;
  elif [ "$1" == "-host" ];        then host=$2;                        shift;
  elif [ "$1" == "-port" ];        then port=$2;                        shift;
  elif [ "$1" == "-cop" ];         then convOp=$2;                      shift;
  elif [ "$1" == "-X" ];           then method=$2;                      shift;
  elif [ "$1" == "-enId" ];        then entityId=$2;                    shift;
  elif [ "$1" == "-enType" ];      then entityType=$2;                  shift;
  elif [ "$1" == "-attr" ];        then attributeName=$2;               shift;
  elif [ "$1" == "-id" ];          then id=$2;                          shift;
  elif [ "$1" == "-json" ];        then IN_FORMAT="Content-Type: application/json"; OUT_FORMAT="Accept: application/json";
  elif [ "$1" == "-informat" ];    then IN_FORMAT="Content-Type: $2";   shift;
  elif [ "$1" == "-outformat" ];   then OUT_FORMAT="Accept: $2";        shift;
  elif [ "$1" == "-table" ];       then table=$2;                       shift;
  else
    if   [ "$operation" == "" ];       then operation=$1;
    elif [ "$dataFile"  == "" ];       then dataFile=$1;
    else
      echo "unknown option/parameter: " $1
      exit 1
    fi
  fi

  shift
done



# -----------------------------------------------------------------------------
#
# Debugging command line options
#
verboseMsg verbose = $verbose
verboseMsg host = $host
verboseMsg port = $port
verboseMsg convOp = $convOp
verboseMsg method = $method
verboseMsg entityId = $entityId
verboseMsg entityType = $entityType
verboseMsg attributeName = $attributeName
verboseMsg operation = $operation
verboseMsg dataFile = $dataFile
verboseMsg id = $id
verboseMsg table = $table



# -----------------------------------------------------------------------------
#
# Checking parameters
#
verboseMsg "Checking parameters"

if [ "$operation" = "" ]
then
  echo "$0: Error: no operation"
  echo
  usage
fi

if [ "$dataFile" = "" ] && [ "$method" == "POST" ]
then
  echo "$0: Error: no data file for POST operation"
  echo
  usage
fi
verboseMsg "parameters checked"


# -----------------------------------------------------------------------------
#
# If convenience operation, the 'op' parameter can be conv/cea etc
#
copPart=$(echo $operation | awk -F/ '{ print $2 }')
if [ "$copPart" != "" ]
then
  convOp=$copPart
  operation=$(echo $operation | awk -F/ '{ print $1 }')

  echo "Operation '"${operation}"'"
  echo "Convenience operation '"${convOp}"'"
fi



# -----------------------------------------------------------------------------
#
# Fixing parameters
#
if [ "$IN_FORMAT" = "Content-Type: json" ]
then
  IN_FORMAT="Content-Type: application/json"
fi

if [ "$OUT_FORMAT" = "Accept: json" ]
then
  OUT_FORMAT="Accept: application/json"
fi



# -----------------------------------------------------------------------------
#
# Getting dataFile in a variable and substituting:
#  REGISTRATIONID
#  SUBSCRIPTIONID
#
verboseMsg "dataFile='"$dataFile"'"
if [ "$dataFile" != "" ]
then
  data=$(cat $dataFile)
  data=$(echo $data | sed "s/REGISTRATIONID/$id/")
  data=$(echo $data | sed "s/SUBSCRIPTIONID/$id/")
  data=$(echo $data | sed "s/REGISTRATION_ID/$id/")
  data=$(echo $data | sed "s/SUBSCRIPTION_ID/$id/")
else
  data=""
fi
verboseMsg "data='"$data"'"



# -----------------------------------------------------------------------------
#
# operation shortname mapping
#
declare -A op
op['rcr']='NGSI9/registerContext'
op['rcrs']='NGSI9/registerContextResponse'
op['dcar']='NGSI9/discoverContextAvailability'

op['qcr']='NGSI10/queryContext'
op['scr']='NGSI10/subscribeContext'
op['ucsr']='NGSI10/updateContextSubscription'
op['uncr']='NGSI10/unsubscribeContext'
op['upcr']='NGSI10/updateContext'
op['ncr']='NGSI10/notifyContext'

op['conv']="Not used - the convenience operation URL is used instead ..."
op['debug']="debug/$table"



# -----------------------------------------------------------------------------
#
# convenience operation shortname mapping
#
declare -A cop
cop['ce']='NGSI9/contextEntities/ENTITY_ID'
cop['cea']='NGSI9/contextEntities/ENTITY_ID/attributes'
cop['ceaa']='NGSI9/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME'
cop['ce10']='NGSI10/contextEntities/ENTITY_ID'
cop['cea10']='NGSI10/contextEntities/ENTITY_ID/attributes'
cop['ceaa10']='NGSI10/contextEntities/ENTITY_ID/attributes/ATTRIBUTE_NAME'
cop['acet']='v1/contextEntities/type/ENTITY_TYPE/id/ENTITY_ID'


# -----------------------------------------------------------------------------
#
# Checking for valid operation
#
if [ "${op[$operation]}" = "" ]
then
  echo "$0: Error: bad operation: '" $operation "'"
  exit 1
fi
verboseMsg "operation: '"$operation"'"
verboseMsg "op[operation]: '"${op[$operation]}"'"



# -----------------------------------------------------------------------------
#
# Checking for valid convenience operation
#
if [ "$operation" == "conv" ]
then
  if [ "${cop[$convOp]}" == "" ]
  then
    echo "$0: Error: bad convenience operation: '" $convOp "'"
    exit 1
  fi
  verboseMsg "convenience operation: '"${cop[$convOp]}"'"
fi



# -----------------------------------------------------------------------------
#
# Creating the url
#
verboseMsg Creating curl command

if [ "$operation" == "conv" ]
then
  echo Convenience $protocol operation: $convOp
  if [ "$tenant" != "" ]
  then
    url=${protocol}"://$host:$port/$tenant/${cop[$convOp]}"
  else
    url=${protocol}"://$host:$port/${cop[$convOp]}"
  fi
else
  if [ "$tenant" != "" ]
  then
    url=${protocol}"://$host:$port/$tenant/${op[$operation]}"
  else
    url=${protocol}"://$host:$port/${op[$operation]}"
  fi
fi
verboseMsg "URL: '"$url"'"



# -----------------------------------------------------------------------------

# Substitution in URL
#
if [ "$entityId" != "" ]
then
  url=$(echo $url | sed "s/ENTITY_ID/$entityId/g")
fi

if [ "$entityType" != "" ]
then
  url=$(echo $url | sed "s/ENTITY_TYPE/$entityType/g")
fi

if [ "$attributeName" != "" ]
then
  url=$(echo $url | sed "s/ATTRIBUTE_NAME/$attributeName/g")
fi

if [ "$uriParams" != "" ]
then
  url=${url}"?$uriParams"
fi

echo CURL: curl -3 $url --header "$IN_FORMAT" --header "$OUT_FORMAT" --header "fiware-service: $httpTenant" $CURL_VERBOSE -X $method $cert
# -----------------------------------------------------------------------------
#
# curl
#
# Always call curl with '-d "$data"' ?
#
if [ "$useXmlLint" == 1 ] 
then
  (curl -3 $url --include --header "$IN_FORMAT" --header "$OUT_FORMAT" --header "fiware-service: $httpTenant" $CURL_VERBOSE -X $method $cert -d @- | xmllint --format -) << EOF
$(echo $data)
EOF
else
  (curl -3 $url --include --header "$IN_FORMAT" --header "$OUT_FORMAT" --header "fiware-service: $httpTenant" $CURL_VERBOSE -X $method $cert -d @-) << EOF
$(echo $data)
EOF
fi
