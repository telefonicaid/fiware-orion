#!/bin/bash
# Copyright 2022 Telefonica Investigacion y Desarrollo, S.A.U
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
# Author: Fernando Lopez, Ken Zangelin
#


#
# FIXME
# - There is no error checking!
#   The script should exit with a descriptive error message if anything goes wrong, e.g. Orion-LD doesn't compile.
#   Right now, the script checks nothing and just "goes on whatever happens"
#   It's easy enough to implement a single function with inputs $! and a descriptice string, that exits "if $! != 0"
#   Then call that function from ... many places
#
# - The script needs a new name, e.g. orionldCompilationEnvInstall.sh
#
# - The script needs to be moved to the "scripts" directory
#
# - We need to add documentation in the top level installation guide on the existance of this script,
#   And, once we're 100% sure it works (and Ubuntu 18.04, 22.04, deb 9?, centos 8?  etc are supported),
#   the manual installation docs can be removed - they will no longer be useful and we don't want to maintain them!
#
# - One more problem - `lsb_release` ... I think that's only for Ubuntu ... we need something to detect *any*^distro
#                                        See comment on the function "linux_version".
#
# - Also, we need to make sure we support the distro before we blindly eval a function that may not exist
#


# -----------------------------------------------------------------------------
#
# sudo check - to install packages etc you need to be able to run sudo
#
sudo ls > /dev/null 2> /dev/null
if [ $? != 0 ]
then
    echo "Unable to run as root (using sudo) - can't install - please fix and try again"
    exit 1
fi



# -----------------------------------------------------------------------------
#
# Check for ~/git
#
# This entire script relies on the existence of a directory at ~/git, where to
# clone a number of repos from github/gitlab.
#
# Right now, an error is printed on stdout abd this script exits.
# An alternative way would be to silently create the directory and continue ...
#
if [ ! -d ~/git ]
then
  echo "No directory ~/git found - please create it and try again"
  exit 1
fi



# -----------------------------------------------------------------------------
#
# errorFile -
#
logFile=/tmp/orionldBuildEnvInstallation.log
errorFile=/tmp/orionldBuildEnvInstallation.err
date > $logFile
date > $errorFile



# -----------------------------------------------------------------------------
#
# GROUP -
#
GROUP=$(id | sed 's/(/ /g' | sed 's/)/ /g' | awk '{print $4}')




# -----------------------------------------------------------------------------
#
# linux_version -
#
# This function probably only works for Ubuntu ...
# Use instead "scripts/build/osDistro.sh" to get the info.
#
function linux_version
{
  distributor=$(lsb_release -a 2> $logFile | grep Distributor | awk '{ print $3 }')
  release=$(lsb_release -a 2> $logFile | grep Release | awk '{ print $2 }')

  version=$distributor$release
  echo $version
}



# -----------------------------------------------------------------------------
#
# Intro -
#
function Intro()
{
  distro="$1"

  echo -e "\n\nInstalling \033[1;34mOrion-LD\033[0m from source code for $distro\n"
}



# -----------------------------------------------------------------------------
#
# Header -
#
function Header()
{
  pre="$1"
  what="$2"
  echo -e "\n  "$pre" \033[1;34m"$what"\033[0m:"
}



# -----------------------------------------------------------------------------
#
# actionStart -
#
function actionStart()
{
  action="$1"
  target="$2"

  if [ "$target" == "" ]
  then
    echo -n -e "    ⏳ "$action" ... "
  else
    echo -n -e "    ⏳ "$action"\033[1;31m "$target"\033[0m ... "
  fi
}



# -----------------------------------------------------------------------------
#
# errorCheck -
#
function errorCheck()
{
  typeset -i errorLines
  cat $errorFile | grep -v "WARNING" > ${errorFile}.check
  errorLines=$(wc -l ${errorFile}.check | awk '{ print $1 }')
  \rm -f ${errorFile}.check

  if [ $errorLines -gt 3 ]
  then
    Header "Installation Error"
    echo "  Error for $action: $target"
    cat $errorFile
    exit 1
  fi
}



# -----------------------------------------------------------------------------
#
# actionEnd -
#
function actionEnd()
{
  echo -e "  \033[1;32mDone!\033[0m"
  errorCheck
}



# -----------------------------------------------------------------------------
#
# mongocDriver -
#
function mongocDriver()
{
  sudo mkdir -p /opt/mongoc
  sudo chown $USER:$GROUP mongoc
  cd /opt/mongoc

  wget https://github.com/mongodb/mongo-c-driver/releases/download/1.22.0/mongo-c-driver-1.22.0.tar.gz
  tar xzf mongo-c-driver-1.22.0.tar.gz

  cd mongo-c-driver-1.22.0
  mkdir -p cmake-build

  cd cmake-build
  cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
  cmake --build .
  sudo cmake --build . --target install
}



# -----------------------------------------------------------------------------
#
# libmicrohttpd -
#
function libmicrohttpd()
{
  sudo mkdir -p /opt/libmicrohttpd
  sudo chown $USER:$GROUP /opt/libmicrohttpd
  cd /opt/libmicrohttpd
  wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.75.tar.gz
  tar xvf libmicrohttpd-0.9.75.tar.gz
  cd libmicrohttpd-0.9.75
  ./configure --disable-messages --disable-postprocessor --disable-dauth
  make
  sudo make install
}



# -----------------------------------------------------------------------------
#
# rapidjson -
#
function rapidjson
{
  sudo mkdir -p /opt/rapidjson
  sudo chown $USER:$GROUP /opt/rapidjson
  cd /opt/rapidjson
  wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz
  tar xfvz v1.0.2.tar.gz
  sudo mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include
}



# -----------------------------------------------------------------------------
#
# klibs -
#
function klibs
{
  for klib in kbase klog kalloc kjson khash
  do
    actionStart "Downloading, building, and installing" "$klib"

    cd ~/git
    git clone https://gitlab.com/kzangeli/${klib}.git >> $logFile
    cd $klib
    if [ "$klib" == "kjson" ]
    then
      git checkout release/0.8.1 >> $logFile
    else
      git checkout release/0.8 >> $logFile
    fi
      make install >> $logFile

    actionEnd
  done
}



# -----------------------------------------------------------------------------
#
# mqttEclipsePaho -
#
function mqttEclipsePaho()
{
  sudo rm -f /usr/local/lib/libpaho*
  cd ~/git
  git clone https://github.com/eclipse/paho.mqtt.c.git
  cd paho.mqtt.c
  git fetch -a
  git checkout tags/v1.3.1
  make html
  make
  sudo make install
}



# -----------------------------------------------------------------------------
#
# orionldRepoClone -
#
function orionldRepoClone
{
  cd ~/git
  git clone https://github.com/FIWARE/context.Orion-LD.git
  cd context.Orion-LD
}



# -----------------------------------------------------------------------------
#
# orionldInstall -
#
function orionldInstall()
{
  sudo touch              /usr/bin/orionld  /etc/init.d/orionld  etc/default/orionld
  sudo chown $USER:$GROUP /usr/bin/orionld  /etc/init.d/orionld  etc/default/orionld

  make install
  chmod 755 /usr/bin/orionld
}



# -----------------------------------------------------------------------------
#
# Ubuntu20.04 -
#
function Ubuntu20.04()
{
  Intro "Ubuntu 20.04"

  actionStart "Adding" "apt repository and updating apt"
  sudo add-apt-repository -y ppa:timescale/timescaledb-ppa >> $logFile 2>> $errorFile
  sudo apt -y update >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing" "aptitude"
  sudo apt -y install aptitude >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing" "Tools needed for compilation and testing"
  sudo aptitude -y install build-essential cmake scons curl >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing" "Libraries"
  sudo aptitude -y install libssl-dev gnutls-dev libcurl4-gnutls-dev libsasl2-dev \
                   libgcrypt-dev uuid-dev libboost1.67-dev libboost-regex1.67-dev libboost-thread1.67-dev \
                   libboost-filesystem1.67-dev libz-dev libpq-dev timescaledb-postgresql-12 >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing" "Mongo C++ Legacy driver"
  sudo aptitude -y install libmongoclient-dev >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Downloading, building, and installing" "Mongo C driver"
  mongocDriver >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Downloading, building, and installing" "libmicrohttpd"
  libmicrohttpd >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Downloading and installing" "rapidjson"
  rapidjson >> $logFile 2>> $errorFile
  actionEnd

  klibs 2>> $errorFile

  actionStart "Installing" "Dependencies for MQTT Eclipse Paho"
  sudo aptitude -y install doxygen >> $logFile 2>> $errorFile
  sudo aptitude -y install graphviz >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing" "MQTT Eclipse Paho"
  mqttEclipsePaho >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing python library for MQTT" "paho-mqtt"
  sudo aptitude -y install python3-pip >> $logFile 2>> $errorFile
  pip3 install paho-mqtt >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing and enabling" "Eclipse mosquitto"
  sudo aptitude -y install mosquitto >> $logFile 2>> $errorFile
  sudo systemctl start mosquitto >> $logFile 2>> $errorFile
  sudo systemctl enable mosquitto >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Cloning" "Orion-LD"
  orionldRepoClone >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Compiling and Installing" "Orion-LD"
  orionldInstall >> $logFile 2>> $errorFile
  actionEnd

  Header "Installing" "MongoDB Server"
  actionStart "Importing the MongoDB public GPG Key"
  sudo aptitude -y install gnupg >> $logFile 2>> $errorFile
  wget -qO - https://www.mongodb.org/static/pgp/server-4.4.asc | sudo apt-key add - >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Creating list file /etc/apt/sources.list.d/mongodb-org-4.0.list"
  echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/4.4 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.4.list >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Reloading local package database"
  sudo aptitude -y update >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Installing the MongoDB server"
  sudo aptitude -y install mongodb-org >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Starting the mongodb daemon"
  sudo systemctl start mongod.service >> $logFile 2>> $errorFile
  actionEnd

  actionStart "Ensuring that MongoDB will start on reboot"
  sudo systemctl enable mongod.service >> $logFile 2>> $errorFile
  actionEnd

  echo -e "\nInstallation is READY ;)\n\n"
}



version=$(linux_version)
eval $version
