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
# Author: Fernando Lopez
#
function Ubuntu20.04() {
    echo -e "\n\nInstalling \033[1;34mOrion-LD\033[0m from source code\n"

    pwd=$(pwd)
    filename=$pwd"/errors.log"

    echo -n -e "    ⏳ Adding apt repository and update apt..."
    sudo add-apt-repository -y ppa:timescale/timescaledb-ppa >/dev/null 2>$filename
    sudo apt -y update >/dev/null 2>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting installation of \033[1;31maptitude\033[0m..."
    sudo apt -y install aptitude >/dev/null 2>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting installation of \033[1;31mTools needed for compilation and testing\033[0m..."
    sudo aptitude -y install build-essential cmake scons curl >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting installation of \033[1;31mLibraries\033[0m..."
    sudo aptitude -y install libssl-dev gnutls-dev libcurl4-gnutls-dev libsasl2-dev \
                      libgcrypt-dev uuid-dev libboost1.67-dev libboost-regex1.67-dev libboost-thread1.67-dev \
                      libboost-filesystem1.67-dev libz-dev libpq-dev timescaledb-postgresql-12 >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mlibmongoclient-dev\033[0m..."
    sudo aptitude -y install libmongoclient-dev >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    GROUP=$(id | sed 's/(/ /g' | sed 's/)/ /g' | awk '{print $4}')

    echo -n -e "    ⏳ Awaiting installation of \033[1;31mMongoC driver\033[0m..."
    sudo mkdir /opt/mongoc >/dev/null 2>>$filename
    sudo chown $USER:$GROUP mongoc >/dev/null 2>>$filename
    cd /opt/mongoc >/dev/null 2>>$filename
    wget https://github.com/mongodb/mongo-c-driver/releases/download/1.22.0/mongo-c-driver-1.22.0.tar.gz >/dev/null 2>>$filename
    tar xzf mongo-c-driver-1.22.0.tar.gz >/dev/null 2>>$filename
    cd mongo-c-driver-1.22.0 >/dev/null 2>>$filename
    mkdir cmake-build >/dev/null 2>>$filename
    cd cmake-build >/dev/null 2>>$filename
    cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. >/dev/null 2>>$filename
    cmake --build . >/dev/null 2>>$filename
    sudo cmake --build . --target install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mlibmicrohttpd\033[0m..."
    sudo mkdir /opt/libmicrohttpd >/dev/null 2>>$filename
    sudo chown $USER:$GROUP /opt/libmicrohttpd >/dev/null 2>>$filename
    cd /opt/libmicrohttpd >/dev/null 2>>$filename
    wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.75.tar.gz >/dev/null 2>>$filename
    tar xvf libmicrohttpd-0.9.75.tar.gz >/dev/null 2>>$filename
    cd libmicrohttpd-0.9.75 >/dev/null 2>>$filename
    ./configure --disable-messages --disable-postprocessor --disable-dauth >/dev/null 2>>$filename
    make >/dev/null 2>>$filename
    sudo make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mrapidjson\033[0m..."
    sudo mkdir /opt/rapidjson >/dev/null 2>>$filename
    sudo chown $USER:$GROUP /opt/rapidjson >/dev/null 2>>$filename
    cd /opt/rapidjson >/dev/null 2>>$filename
    wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz >/dev/null 2>>$filename
    tar xfvz v1.0.2.tar.gz >/dev/null 2>>$filename
    sudo mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mkbase\033[0m..."
    cd ~/git >/dev/null 2>>$filename
    git clone https://gitlab.com/kzangeli/kbase.git >/dev/null 2>>$filename
    cd kbase >/dev/null 2>>$filename
    git checkout release/0.8 >/dev/null 2>>$filename
    make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mklog\033[0m..."
    cd ~/git >/dev/null 2>>$filename
    git clone https://gitlab.com/kzangeli/klog.git >/dev/null 2>>$filename
    cd klog >/dev/null 2>>$filename
    git checkout release/0.8 >/dev/null 2>>$filename
    make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mkalloc\033[0m..."
    cd ~/git >/dev/null 2>>$filename
    git clone https://gitlab.com/kzangeli/kalloc.git >/dev/null 2>>$filename
    cd kalloc >/dev/null 2>>$filename
    git checkout release/0.8 >/dev/null 2>>$filename
    make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mkjson\033[0m..."
    cd ~/git >/dev/null 2>>$filename
    git clone https://gitlab.com/kzangeli/kjson.git >/dev/null 2>>$filename
    cd kjson >/dev/null 2>>$filename
    git checkout release/0.8.1 >/dev/null 2>>$filename
    make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mkhash\033[0m..."
    cd ~/git >/dev/null 2>>$filename
    git clone https://gitlab.com/kzangeli/khash.git >/dev/null 2>>$filename
    cd khash >/dev/null 2>>$filename
    git checkout release/0.8 >/dev/null 2>>$filename
    make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting download, build and installation of \033[1;31mMQTT: Eclipse Paho\033[0m..."
    sudo aptitude -y install doxygen >/dev/null 2>>$filename
    sudo aptitude -y install graphviz >/dev/null 2>>$filename
    sudo rm -f /usr/local/lib/libpaho* >/dev/null 2>>$filename
    cd ~/git >/dev/null 2>>$filename
    git clone https://github.com/eclipse/paho.mqtt.c.git >/dev/null 2>>$filename
    cd paho.mqtt.c >/dev/null 2>>$filename
    git fetch -a >/dev/null 2>>$filename
    git checkout tags/v1.3.1 >/dev/null 2>>$filename
    make html >/dev/null 2>>$filename
    make >/dev/null 2>>$filename
    sudo make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting installation of the python library for MQTT: \033[1;31mpaho-mqtt\033[0m..."
    sudo aptitude -y install python3-pip >/dev/null 2>>$filename
    pip3 install paho-mqtt >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Awaiting installation and enabling of \033[1;31mEclipse mosquito\033[0m..."
    sudo aptitude -y install mosquitto >/dev/null 2>>$filename
    sudo systemctl start mosquitto >/dev/null 2>>$filename
    sudo systemctl enable mosquitto >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Cloning \033[1;31mOrion-LD\033[0m..."
    cd ~/git >/dev/null 2>>$filename
    git clone https://github.com/FIWARE/context.Orion-LD.git >/dev/null 2>>$filename
    cd context.Orion-LD >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Compiling \033[1;31mOrion-LD\033[0m..."
    sudo touch /usr/bin/orionld >/dev/null 2>>$filename
    sudo chown $USER:$GROUP /usr/bin/orionld >/dev/null 2>>$filename
    sudo touch /etc/init.d/orionld >/dev/null 2>>$filename
    sudo chown $USER:$GROUP /etc/init.d/orionld >/dev/null 2>>$filename
    sudo touch /etc/default/orionld >/dev/null 2>>$filename
    sudo chown $USER:$GROUP /etc/default/orionld >/dev/null 2>>$filename
    make install >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -e "\nInstalling \033[1;34mMongoDB Server\033[0m:"

    echo -n -e "    ⏳ Import the MongoDB public GPG Key...  \033[1;32m"
    sudo aptitude -y install gnupg >/dev/null 2>>$filename
    wget -qO - https://www.mongodb.org/static/pgp/server-4.4.asc | sudo apt-key add -
    # Should respond with "OK"

    echo -n -e "\033[0m    ⏳ Create the list file: /etc/apt/sources.list.d/mongodb-org-4.0.list ..."
    echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/4.4 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.4.list >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Reload local package database..."
    sudo aptitude -y update >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Install the MongoDB packages..."
    sudo aptitude -y install mongodb-org >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Start the mongodb daemon..."
    sudo systemctl start mongod.service >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -n -e "    ⏳ Ensure that MongoDB will start following a system reboot..."
    sudo systemctl enable mongod.service >/dev/null 2>>$filename
    echo -e "  \033[1;32mdone\033[0m"

    echo -e "\nFinish ;)\n\n"
}

function check_linux_version {
    distributor=$(lsb_release -a 2>/dev/null | grep Distributor | awk '{print $3}')

    release=$(lsb_release -a 2>/dev/null | grep Release | awk '{print $2}')

    version=$distributor$release
    echo $version
}

version=$(check_linux_version)
eval $version
