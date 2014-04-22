#!/bin/bash

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
