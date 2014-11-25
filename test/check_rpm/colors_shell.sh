#!/bin/bash

# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

##########################
###  COLORS FOR LOGS   ###
##########################

## LOG FUNCTIONS
_message(){
   _error_color="\\033[37m\\033[1;31m" # BG light grey, FG red
   _stage_color="\\033[37m\\033[1;33m" # FG yellow
   _ok_color="\\033[37m\\033[1;32m" # FG light green
   _user_color="\\033[37m\\033[1;30m" # FG Dark Grey
   _default_log_color_pre="\\033[0m\\033[1;34m"
   _default_log_color_suffix="\\033[0m"
   isError=$(echo "$1"|grep -i "\\[ERROR\\]")
   isStage=$(echo "$1"|grep -i "\\[INFO\\]")
   isOk=$(echo "$1"|grep -i "\\[OK\\]")
   isUser=$(echo "$1"|grep -i "\\[USER\\]")
   isDash=`echo -en|grep "\-en"`
   outLog="/dev/stdout"
   if ! [ -z "$isError" ]; then 
      messageColor=${_error_color} && outLog="/dev/stderr" 
   else
      if ! [ -z "$isStage" ]; then 
         messageColor=${_stage_color}
      elif ! [ -z "$isOk" ]; then
         messageColor=${_ok_color}
      elif ! [ -z "$isUser" ]; then
         messageColor=${_user_color}
      else      
         messageColor=${_default_log_color_pre}
      fi
   fi 
   [ -z "$isDash" ] && echo -en "${messageColor}$1${_default_log_color_suffix}\n" >$outLog|| echo "${messageColor}$1${_default_log_color_suffix}\n" >$outLog
}

##
# Logs with colors into Console and, if $2 has value, 
# without them into log file
# Input:
# *$1: Message
# $2: Log file
##
_log(){
   _message "[`date '+%X'`] $1" 
   #
   if [[ -f "$2" ]]; then
     echo "[`date '+%X'`] $1" >> "./$2"
   fi
}

_logError(){
  _log "[ERROR] $1" $2
}

_logStage(){
  _log "[INFO] $1" $2
}

_logOk(){
  _log "[OK] $1" $2
}

_logUser(){
  _log "[USER] $1" $2
}
