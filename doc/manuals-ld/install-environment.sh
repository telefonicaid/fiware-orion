function Ubuntu20.04() {
    echo -n -e "⏳ Waiting installation \033[1;31maptitude\033[0m..."
    sudo apt -y install aptitude >/dev/null 2>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation \033[1;31mTools needed for compilation and testing\033[0m..."
    sudo aptitude -y install build-essential cmake scons curl >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation \033[1;31mLibraries\033[0m..."
    sudo aptitude -y install libssl-dev gnutls-dev libcurl4-gnutls-dev libsasl2-dev \
                      libgcrypt-dev uuid-dev libboost1.67-dev libboost-regex1.67-dev libboost-thread1.67-dev \
                      libboost-filesystem1.67-dev libz-dev >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mlibmongoclient-dev\033[0m..."
    sudo aptitude -y install libmongoclient-dev >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    GROUP=$(id | sed 's/(/ /g' | sed 's/)/ /g' | awk '{print $4}')
    echo $GROUP

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mlibmicrohttpd\033[0m..."
    sudo mkdir /opt/libmicrohttpd >/dev/null 2>>errors.log
    sudo chown $USER:$GROUP /opt/libmicrohttpd >/dev/null 2>>errors.log
    cd /opt/libmicrohttpd >/dev/null 2>>errors.log
    wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.72.tar.gz >/dev/null 2>>errors.log
    tar xvf libmicrohttpd-0.9.72.tar.gz >/dev/null 2>>errors.log
    cd libmicrohttpd-0.9.72 >/dev/null 2>>errors.log
    ./configure --disable-messages --disable-postprocessor --disable-dauth >/dev/null 2>>errors.log
    make >/dev/null 2>>errors.log
    sudo make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mrapidjson\033[0m..."
    sudo mkdir /opt/rapidjson >/dev/null 2>>errors.log
    sudo chown $USER:$GROUP /opt/rapidjson >/dev/null 2>>errors.log
    cd /opt/rapidjson >/dev/null 2>>errors.log
    wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz >/dev/null 2>>errors.log
    tar xfvz v1.0.2.tar.gz >/dev/null 2>>errors.log
    sudo mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mkbase\033[0m..."
    cd ~/git >/dev/null 2>>errors.log
    git clone https://gitlab.com/kzangeli/kbase.git >/dev/null 2>>errors.log
    cd kbase >/dev/null 2>>errors.log
    git checkout release/0.5 >/dev/null 2>>errors.log
    make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mklog\033[0m..."
    cd ~/git >/dev/null 2>>errors.log
    git clone https://gitlab.com/kzangeli/klog.git >/dev/null 2>>errors.log
    cd klog >/dev/null 2>>errors.log
    git checkout release/0.5 >/dev/null 2>>errors.log
    make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mkalloc\033[0m..."
    cd ~/git >/dev/null 2>>errors.log
    git clone https://gitlab.com/kzangeli/kalloc.git >/dev/null 2>>errors.log
    cd kalloc >/dev/null 2>>errors.log
    git checkout release/0.5 >/dev/null 2>>errors.log
    make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mkjson\033[0m..."
    cd ~/git >/dev/null 2>>errors.log
    git clone https://gitlab.com/kzangeli/kjson.git >/dev/null 2>>errors.log
    cd kjson >/dev/null 2>>errors.log
    git checkout release/0.5 >/dev/null 2>>errors.log
    make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mkhash\033[0m..."
    cd ~/git >/dev/null 2>>errors.log
    git clone https://gitlab.com/kzangeli/khash.git >/dev/null 2>>errors.log
    cd khash >/dev/null 2>>errors.log
    git checkout release/0.5 >/dev/null 2>>errors.log
    make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation download, build and install \033[1;31mMQTT: Eclipse Paho\033[0m..."
    sudo aptitude -y install doxygen >/dev/null 2>>errors.log
    sudo aptitude -y install graphviz >/dev/null 2>>errors.log
    sudo rm -f /usr/local/lib/libpaho* >/dev/null 2>>errors.log
    cd ~/git >/dev/null 2>>errors.log
    git clone https://github.com/eclipse/paho.mqtt.c.git >/dev/null 2>>errors.log
    cd paho.mqtt.c >/dev/null 2>>errors.log
    git fetch -a >/dev/null 2>>errors.log
    git checkout tags/v1.3.1 >/dev/null 2>>errors.log
    make html >/dev/null 2>>errors.log
    make >/dev/null 2>>errors.log
    sudo make install >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation the python library for MQTT: \033[1;31mpaho-mqtt\033[0m..."
    sudo aptitude -y install python3-pip >/dev/null 2>>errors.log
    pip3 install paho-mqtt >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Waiting installation and enabling of \033[1;31mEclipse mosquito\033[0m..."
    sudo aptitude -y install mosquitto >/dev/null 2>>errors.log
    sudo systemctl start mosquitto >/dev/null 2>>errors.log
    sudo systemctl enable mosquitto >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"

    echo -n -e "⏳ Cloning \033[1;31mOrion-LD\033[0m and compiling..."
    cd ~/git >/dev/null 2>>errors.log
    git clone https://github.com/FIWARE/context.Orion-LD.git >/dev/null 2>>errors.log
    cd context.Orion-LD >/dev/null 2>>errors.log
    echo -e "    \033[1;32mdone\033[0m\n"
}

function check_linux_version {
    distributor=$(lsb_release -a 2>/dev/null | grep Distributor | awk '{print $3}')

    release=$(lsb_release -a 2>/dev/null | grep Release | awk '{print $2}')

    version=$distributor$release
    echo $version
}

version=$(check_linux_version)
eval $version
