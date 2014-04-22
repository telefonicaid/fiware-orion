#!/bin/bash

#############################################
###   Script for check the contextBroker RPM   ###
#############################################

# Define the color log
source /home/vagrant/workspace/fiware-orion/test/check_rpm/colors_shell.sh
source ${WORKSPACE}/scripts/colors_shell.sh

_logStage "######## Executing the Check RPM Stage ... ########"
echo ""

_logStage "######## Checking MongoDB... ########"
_log "#### Check the MongoDB process... ####"
# Check the if the mongod is installed/running
_log "#### Checking the mongodb status... #####"
if [ "`sudo /etc/init.d/mongod status | grep pid`" == "" ]; then
	_logError ".............. Mongo is NOT running .............."
	exit 1
else
	_logOk ".............. Mongod is running .............." 
fi
echo ""

## Install RPM of contextBroker
cd /home/vagrant/workspace/fiware-orion/rpm/RPMS/x86_64
# cd ${WORKSPACE}/rpm/RPMS/x86_64

_logStage "######## Installing RPM contextBroker ... ########"
# Install ONLY the contextBroker RPM (no the tests RPM)
ls contextBroker*.rpm | grep -v test | xargs sudo rpm -Uvh
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
if [[ ! -d /opt/contextBroker/ ]]; then
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
_logStage "######## Checking the contextBroker service ########"
if [ "`sudo netstat -putan | grep contextBroker`" == "" ]; then
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
sudo rpm -e contextBroker &> /dev/null
result=$?
if [[ $result -ne 0 ]]; then 
	_logError ".............. Uninstall failed .............."
	exit 1
else
	_logOk ".............. Uninstall completed! .............."
fi
echo ""

_logStage "######## Check if the directories are cleaned... ########"
if [[ ! -d /opt/contextBroker/ ]]; then
	_logOk ".............. All clean .............."
else
	_logError ".............. FAIL the directories are NOT cleaned .............."
	exit 1
fi 
echo ""

echo "############ Check RPM Stage completed! ############"