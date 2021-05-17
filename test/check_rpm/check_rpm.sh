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

#############################################
###   Script for check the contextBroker RPM   ###
#############################################

# The ${WORKSPACE} is set by the user of this script (e.g. Jenkins)
if [[ -z ${WORKSPACE} ]]; then
   echo "WORKSPACE variable should be defined with the root path to the fiware-orion git repository"
   exit 1
fi

# Define the color log
source ${WORKSPACE}/test/check_rpm/colors_shell.sh

_logStage "######## Executing the Check RPM Stage ... ########"
echo ""

_logStage "######## Checking MongoDB... ########"
_log "#### Check the MongoDB process... ####"
# Check the if the mongod is installed/running
_log "#### Checking the mongodb status... #####"
if [ "`sudo service mongod status | grep 'forked process' | awk -F 'forked process: ' '{print $2}'`" == "" ]; then
	_logError ".............. Mongo is NOT running .............."
	exit 1
else
	_logOk ".............. Mongod is running .............." 
fi
echo ""

## Install RPM of contextBroker
cd ${WORKSPACE}/rpm/RPMS/x86_64

_logStage "######## Installing RPM contextBroker ... ########"
# Install ONLY the contextBroker RPM (no the tests RPM)
ls contextBroker*.rpm | grep -v contextBroker-fiware | xargs sudo rpm -Uvh
if [[ $? -eq 0 ]]; then
	_logOk ".............. contextBroker RPM installed .............."
else
	_logError ".............. contextBroker RPM NOT installed .............." 
	exit 1
fi
echo ""

## Check the if the user contextBroker is created
_logStage "######## Checking the user configuration... ########"
if [ "`cat /etc/passwd | grep orion`" == "" ]; then
	_logError ".............. User orion is NOT created .............."
	exit 1
else
	_logOk ".............. User orion is created .............." 
fi
echo ""

## Check if exist the contextBroker directory in the home directory of the contextBroker
_logStage "######## Check contextBroker directories... ########"
if [[ ! -d /usr/share/contextBroker/ ]]; then
	_logError ".............. directories are NOT created well .............."
	exit 1
else
	_logOk ".............. directories are created well .............."
fi 
echo ""

## Start the contextBroker service
_logStage "######## Starting the contextBroker service... ########"
sudo service contextBroker start
if [[ $? -eq 0 ]]; then
	_logOk ".............. contextBroker service is started .............."
else
	_logError ".............. contextBroker RPM NOT started .............." 
	exit 1
fi
echo ""

## Check if the contextBroker service is running
_logStage "######## Checking the contextBroker service ########"
if [ "`sudo /etc/init.d/contextBroker status | grep pid`" == "" ]; then
	_logError ".............. contextBroker service is NOT running .............."
	exit 1
else
	_logOk ".............. contextBroker service is running .............." 
fi
echo ""

## Check if the contextBroker is listen the specific ports
## (We use contextBroke as we have discovered that in CentOS 8 netstat may cut the name in the display)
_logStage "######## Checking the contextBroker service ########"
if [ "`sudo netstat -putan | grep contextBroke`" == "" ]; then
	_logError ".............. contextBroker is not LISTEN .............."
	exit 1
else
	_logOk ".............. contextBroker is LISTEN the correct ports! .............." 
fi

echo ""

## Check END-TO-END for the CCB service
_logStage "######## Starting the END-TO-END Check ########"
curl -i localhost:1026/version
echo ""
if [[ $? -eq 0 ]]; then
    _logOk ".............. The END-TO-END Check of the contextBroker is OK .............."
else
    _logError ".............. The END-TO-END Check failed .............." 
    exit 1
fi
echo ""

# Stop the contextBroker service 
_logStage "######## Stopping the contextBroker service ########"
sudo service contextBroker stop 
if [[ $? -eq 0 ]]; then
	_logOk ".............. contextBroker service is stopped .............."
else
	_logError ".............. contextBroker RPM NOT stopped .............." 
	exit 1
fi
echo ""

## Uninstall the contextBroker RPM
_logStage "######## Uninstall the contextBroker RPM ########"
sudo rpm -e contextBroker contextBroker-tests &> /dev/null
result=$?
if [[ $result -ne 0 ]]; then 
	_logError ".............. Uninstall failed .............."
	exit 1
else
	_logOk ".............. Uninstall completed! .............."
fi
echo ""

_logStage "######## Check if the directories are cleaned... ########"
if [[ ! -d /usr/share/contextBroker/ ]]; then
	_logOk ".............. All clean .............."
else
	_logError ".............. FAIL the directories are NOT cleaned .............."
	exit 1
fi 
echo ""

_logStage "############ Check RPM Stage completed! ############"
