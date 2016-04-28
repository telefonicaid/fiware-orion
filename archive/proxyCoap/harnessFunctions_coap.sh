# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
# In order to "recover" the following functions, they should be moved back
# to harnessFunctions.sh file (and enabled with export -f at the end of that file)

# ------------------------------------------------------------------------------
#
# proxyCoapStart
#
function proxyCoapStart()
{
  extraParams=$*

  proxyCoap $extraParams -cbPort $CB_PORT

  # Test to see whether we have a proxy running. If not raise an error
  running_proxyCoap=$(ps -fe | grep ' proxyCoap' | grep "cbPort $CB_PORT" | wc -l)
  if [ "$running_proxyCoap" == "" ]
  then
    echo "Unable to start proxyCoap"
    exit 1
  fi
}



# ------------------------------------------------------------------------------
#
# proxyCoapStop
#
function proxyCoapStop
{
  port=$COAP_PORT

  # Test to see if we have a proxy running if so kill it!
  running_proxyCoap=$(ps -fe | grep proxyCoap | wc -l)
  if [ $running_proxyCoap -ne 1 ]; then
    kill $(ps -fe | grep proxyCoap | awk '{print $2}') 2> /dev/null
    # Wait some time so the proxy can finish properly
    sleep 1
    running_proxyCoap=$(ps -fe | grep proxyCoap | wc -l)
    if [ $running_proxyCoap -ne 1 ]; then
      # If the proxy refuses to stop politely, kill the process by brute force
      kill -9 $(ps -fe | grep proxyCoap | awk '{print $2}') 2> /dev/null
      sleep 1
      running_proxyCoap=$(ps -fe | grep proxyCoap | wc -l)
      if [ $running_proxyCoap -ne 1 ]; then
        echo "Existing proxyCoap is immortal, can not be killed!"
        exit 1
      fi
    fi
  fi
}


# ------------------------------------------------------------------------------
#
# coapCurl
#
# Options:
#   -X            <HTTP method>    (default: according to curl. GET if no payload, POST if with payload)
#   --host        <host>           (default: localhost)
#   --port        <port>           (default: $COAP_PORT)
#   --url         <URL>            (default: empty string)
#   --payload     <payload>        (default: NO PAYLOAD. Possible values: [filename | "${string}"])
#   --in          (input payload)  (default: xml => application/xml, If 'json': application/json)
#   --out         (output payload  (default: xml => application/xml, If 'json': application/json)
#   --json        (in/out JSON)    (if --in/out is used AFTER --json, it overrides)
#   --tenant      <tenant>         (tenant in HTTP header)
#   --servicePath <path>           (Service Path in HTTP header)
#   --noPayloadCheck               (skip paylosd check filter)
#
# Any parameters are sent as is to 'curl'
#
function coapCurl()
{
  #
  # Default values
  #
  _method=""
  _host="0.0.0.0"
  _port=$COAP_PORT
  _url=""
  _payload=""
  _inFormat=""
  _outFormat=""
  _json=""
  _tenant=""
  _servicePath=""
  _xtra=''
  _noPayloadCheck='off'

  while [ "$#" != 0 ]
  do
    if   [ "$1" == "-X" ]; then                _method="$2"; shift;
    elif [ "$1" == "--host" ]; then            _host="$2"; shift;
    elif [ "$1" == "--port" ]; then            _port="$2"; shift;
    elif [ "$1" == "--url" ]; then             _url="$2"; shift;
    elif [ "$1" == "--payload" ]; then         _payload="$2"; shift;
    elif [ "$1" == "--in" ]; then              _inFormat="$2"; shift;
    elif [ "$1" == "--out" ]; then             _outFormat="$2"; shift;
    elif [ "$1" == "--json" ]; then            _inFormat=application/json; _outFormat=application/json;
    elif [ "$1" == "--tenant" ]; then          _tenant="$2"; shift;
    elif [ "$1" == "--servicePath" ]; then     _servicePath="$2"; shift;
    elif [ "$1" == "--noPayloadCheck" ]; then  _noPayloadCheck=on;
    else                                       _xtra="$_xtra $1"; shift;
    fi

    shift
  done

  #
  # Sanity check of parameters
  #
  if [ "$_url" == "" ]
  then
    echo "No URL";
    return 1;
  fi


  #
  # Fix for 'Content-Type' and 'Accept' short names 'xml' and 'json'
  #
  if [ "$_inFormat" == "xml" ];   then _inFormat=application/xml;   fi;
  if [ "$_outFormat" == "xml" ];  then _outFormat=application/xml;  fi;
  if [ "$_inFormat" == "json" ];  then _inFormat=application/json;  fi;
  if [ "$_outFormat" == "json" ]; then _outFormat=application/json; fi;



  #
  # Cleanup 'compound' variables, so that we don't inherit values from previous calls
  #
  _METHOD=''
  _URL=''
  _ACCEPT=''
  _CONTENTFORMAT=''

  if [ "$_inFormat" != "" ];  then  _CONTENTFORMAT="-t $_inFormat";  fi
  if [ "$_outFormat" != "" ]; then  _ACCEPT="-A $_outFormat";        fi
  if [ "$_method" != "" ];    then  _METHOD=' -m '$_method;          fi

  _URL="coap://$_host:$_port$_url"

  #   usage: coap-client [-A type...] [-t type] [-b [num,]size] [-B seconds] [-e text]
  #                   [-g group] [-m method] [-N] [-o file] [-P addr[:port]] [-p port]
  #                   [-s duration] [-O num,text] [-T string] [-v num] URI
  #
  #           URI can be an absolute or relative coap URI,
  #           -A type...      accepted media types as comma-separated list of
  #                           symbolic or numeric values
  #           -t type         content type for given resource for PUT/POST
  #           -b [num,]size   block size to be used in GET/PUT/POST requests
  #                           (value must be a multiple of 16 not larger than 1024)
  #                           If num is present, the request chain will start at
  #                           block num
  #           -B seconds      break operation after waiting given seconds
  #                           (default is 90)
  #           -e text         include text as payload (use percent-encoding for
  #                           non-ASCII characters)
  #           -f file         file to send with PUT/POST (use '-' for STDIN)
  #           -g group        join the given multicast group
  #           -m method       request method (get|put|post|delete), default is 'get'
  #           -N              send NON-confirmable message
  #           -o file         output received data to this file (use '-' for STDOUT)
  #           -p port         listen on specified port
  #           -s duration     subscribe for given duration [s]
  #           -v num          verbosity level (default: 3)
  #           -O num,text     add option num with contents text to request
  #           -P addr[:port]  use proxy (automatically adds Proxy-Uri option to
  #                           request)
  #           -T token        include specified token
  #
  #   examples:
  #           coap-client -m get coap://[::1]/
  #           coap-client -m get coap://[::1]/.well-known/core
  #           coap-client -m get -T cafe coap://[::1]/time
  #           echo 1000 | coap-client -m put -T cafe coap://[::1]/time -f -


  _BUILTINS='-B 1 -b 1024'


#   echo '====================================================================================='
  if [ "$_payload" != "" ]
  then
#     echo "echo "$_payload" | ./coap-client -f - $_BUILTINS $_METHOD $_ACCEPT $_CONTENTFORMAT $_URL"
    _response=$(echo "$_payload" | coap-client -f - $_BUILTINS $_METHOD $_ACCEPT $_CONTENTFORMAT $_URL)
  else
#     echo "./coap-client $_BUILTINS $_METHOD $_ACCEPT $_CONTENTFORMAT $_URL"
    _response=$(coap-client $_BUILTINS $_METHOD $_ACCEPT $_CONTENTFORMAT $_URL)
  fi
#   echo '====================================================================================='


  # Get headers
  _responseHeaders=$(echo "$_response" | head -n 1)

  # Strip headers and garbage bytes from response
  _response=$(echo "$_response" | tail -n +2 | head -c-3)


  echo $_responseHeaders
  echo

  #
  # Print and beautify response body (IF ANY)
  #
  if [ "$_noPayloadCheck" == "on" ]
  then
    echo $_response
  else
    if [ "$_response" != "" ]
    then
      if [ "$_outFormat" == application/xml ] || [ "$_outFormat" == "" ]
      then
        # FIXME P10: XML removal
        #echo $_response | xmllint --format -
        echo $_response | python -mjson.tool
      elif [ "$_outFormat" == application/json ]
      then
        vMsg "JSON check for:" $_response
        echo $_response | python -mjson.tool
      else
        # FIXME P10: XML removal
        #echo $_response | xmllint --format -
        echo $_response | python -mjson.tool
      fi
    fi
  fi
}
