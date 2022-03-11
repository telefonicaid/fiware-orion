# Building from sources

Orion Context Broker reference distribution is Debian 11. This doesn't mean that the broker cannot be built in other distributions (actually, it can). This section also includes indications on how to build in other distributions, just in the case it may help people that don't use Debian. However, note that the only "officially supported" procedure is the one for Debian 11; the others are provided "as is" and can get obsolete from time to time.

## Debian 11 (officially supported)

The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.74
* libmicrohttpd: 0.9.70 (from source)
* libcurl: 7.74.0
* openssl: 1.1.1k
* libuuid: 2.36.1
* libmosquitto: 2.0.12 (from source)
* Mongo C driver: 1.17.4 (from source)
* rapidjson: 1.1.0 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)

The basic procedure is as follows (assuming you don't run commands as root, we use sudo for those
commands that require root privilege):

* Install the needed building tools (compiler, etc.).

        sudo apt-get install make cmake g++

* Install the required libraries (except what needs to be taken from source, described in following steps).

        sudo apt-get install libssl-dev libcurl4-openssl-dev libboost-dev libboost-regex-dev libboost-filesystem-dev libboost-thread-dev uuid-dev libgnutls28-dev libsasl2-dev libgcrypt-dev

* Install the Mongo Driver from source.

        wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.4/mongo-c-driver-1.17.4.tar.gz
        tar xfvz mongo-c-driver-1.17.4.tar.gz
        cd mongo-c-driver-1.17.4
        mkdir cmake-build
        cd cmake-build
        cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
        make
        sudo make install

* Install rapidjson from sources:

        wget https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz
        tar xfvz v1.1.0.tar.gz
        sudo mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include

* Install libmicrohttpd from sources (the `./configure` command below shows the recommended build configuration to get minimum library footprint, but if you are an advanced user, you can configure as you prefer)

        wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz
        tar xvf libmicrohttpd-0.9.70.tar.gz
        cd libmicrohttpd-0.9.70
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* Install mosquitto from sources (appart from changing WITH_CJSON, WITH_STATIC_LIBRARIES and WITH_SHARED_LIBRARIES settings, config.mk file under mosquitto-2.0.12/ can be modified to fine tune the build)

        wget http://mosquitto.org/files/source/mosquitto-2.0.12.tar.gz
        tar xvf mosquitto-2.0.12.tar.gz
        cd mosquitto-2.0.12
        sed -i 's/WITH_CJSON:=yes/WITH_CJSON:=no/g' config.mk
        sed -i 's/WITH_STATIC_LIBRARIES:=no/WITH_STATIC_LIBRARIES:=yes/g' config.mk
        sed -i 's/WITH_SHARED_LIBRARIES:=yes/WITH_SHARED_LIBRARIES:=no/g' config.mk
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # Update /etc/ld.so.cache with the new library files in /usr/local/lib

* Get the code (alternatively you can download it using a zipped version or a different URL pattern, e.g `git clone git@github.com:telefonicaid/fiware-orion.git`):

        sudo apt-get install git
        git clone https://github.com/telefonicaid/fiware-orion

* Build the source:

        cd fiware-orion
        make

* (Optional but highly recommended) run unit test and functional tests. More on this on [its specific section below](#testing-coverage-and-rpm).

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

### Testing and coverage

The Orion Context Broker comes with a suite of unit, valgrind and end-to-end tests that you can also run, following the following procedure (optional but highly recommended):

* Install Google Test/Mock from sources. Previously the URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but Google removed that package in late August 2016 and it is no longer working.

        sudo apt-get install python2
        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        sed -i 's/env python/env python2/' gtest/scripts/fuse_gtest_files.py  # little hack to make installation to work on Debian 11
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

In the case of the aarch64 architecture, install libxslt using apt-get, and run `./configure` with `--build=arm-linux` option.

* Install MongoDB (tests rely on mongod running in localhost). Check [the official MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-red-hat/) for details. Recommended version is 4.4 (it may work with previous versions, but we don't recommend it).

* Run unit test

        make unit_test

* Install additional required tools for functional and valgrind tests:

        sudo apt-get install curl nc valgrind bc python3 python3-pip
        sudo pip3 install virtualenv

* Prepare the environment for test harness. Basically, you have to install the `accumulator-server.py` script and in a path under your control, `~/bin` is the recommended one. Alternatively, you can install them in a system directory such as `/usr/bin` but it could collide with an other programs, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see `scripts/testEnv.sh` file) and create a virtualenv environment with the required Python packages.

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        virtualenv /opt/ft_env --python=/usr/bin/python3
        . /opt/ft_env/bin/activate
        pip install Flask==2.0.2 paho-mqtt==1.6.1

* Run test harness in this environment (it takes some time, please be patient).

        make functional_test INSTALL_DIR=~

* Once passed all the functional tests, you can run the valgrind tests (this will take longer than the functional tests, arm yourself with a lot of patience):

        make valgrind INSTALL_DIR=~

You can generate coverage reports for the Orion Context Broker using the following procedure (optional):

* Install the lcov tool

        sudo apt-get install lcov

* Do first a successful pass for unit_test and functional_test, to check that everything is ok (see above)

* Run coverage

        make coverage INSTALL_DIR=~

## Ubuntu 20.04 LTS

This instruction is how to build the Orion Context Broker for the x86_64 or the aarch64 architecture on Ubuntu 20.04 LTS.
And it includes the instruction to build MongoDB 4.4 that the Orion depends on.
The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.71.0
* libmicrohttpd: 0.9.70 (from source)
* libcurl: 7.68.0
* openssl: 1.1.1f
* libuuid: 2.34-0.1
* libmosquitto: 2.0.12 (from source)
* Mongo C driver: 1.17.4 (from source)
* rapidjson: 1.1.0 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)

The basic procedure is as follows (assuming you don't run commands as root, we use sudo for those
commands that require root privilege):

* Install the needed building tools (compiler, etc.).

        sudo apt install build-essential cmake

* Install the required libraries (except what needs to be taken from source, described in following steps).

        sudo apt install libboost-dev libboost-regex-dev libboost-thread-dev libboost-filesystem-dev \
                         libcurl4-gnutls-dev gnutls-dev libgcrypt-dev libssl-dev uuid-dev libsasl2-dev

* Install the Mongo Driver from source.

        wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.4/mongo-c-driver-1.17.4.tar.gz 
        tar xfvz mongo-c-driver-1.17.4.tar.gz 
        cd mongo-c-driver-1.17.4 
        mkdir cmake-build 
        cd cmake-build 
        cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. 
        make 
        make install

* Install rapidjson from sources:

        wget https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz
        tar xfvz v1.1.0.tar.gz
        sudo mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include

* Install libmicrohttpd from sources (the `./configure` command below shows the recommended build configuration to get minimum library footprint, but if you are an advanced user, you can configure as you prefer)

        wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz
        tar xvf libmicrohttpd-0.9.70.tar.gz
        cd libmicrohttpd-0.9.70
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* Install mosquitto from sources (appart from changing WITH_CJSON, WITH_STATIC_LIBRARIES and WITH_SHARED_LIBRARIES settings, config.mk file under mosquitto-2.0.12/ can be modified to fine tune the build)

        wget http://mosquitto.org/files/source/mosquitto-2.0.12.tar.gz
        tar xvf mosquitto-2.0.12.tar.gz
        cd mosquitto-2.0.12
        sed -i 's/WITH_CJSON:=yes/WITH_CJSON:=no/g' config.mk
        sed -i 's/WITH_STATIC_LIBRARIES:=no/WITH_STATIC_LIBRARIES:=yes/g' config.mk
        sed -i 's/WITH_SHARED_LIBRARIES:=yes/WITH_SHARED_LIBRARIES:=no/g' config.mk
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # Update /etc/ld.so.cache with the new library files in /usr/local/lib

* Get the code (alternatively you can download it using a zipped version or a different URL pattern, e.g `git clone git@github.com:telefonicaid/fiware-orion.git`):

        sudo apt install git
        git clone https://github.com/telefonicaid/fiware-orion

* Build the source:

        cd fiware-orion
        make

* (Optional but highly recommended) run unit test and functional tests. More on this on [its specific section below](#testing-and-coverage).

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

### Testing and coverage

The Orion Context Broker comes with a suite of functional, valgrind and end-to-end tests that you can also run, following the following procedure (optional):

* Install Google Test/Mock from sources. Previously the URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but Google removed that package in late August 2016 and it is no longer working.

        apt install python-is-python2 xsltproc
        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

In the case of the aarch64 architecture, run `./configure` with `--build=arm-linux` option.

* Install MongoDB (tests rely on mongod running in localhost). Check [the official MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/) for details. Recommended version is 4.4 (it may work with previous versions, but we don't recommend it).

* Run unit test:

        make unit_test

* Install additional required tools for functional and valgrind tests:

        curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py 
        sudo python get-pip.py 
        sudo apt install netcat valgrind bc 
        sudo pip install --upgrade pip 
        pip install virtualenv

* Prepare the environment for test harness. Basically, you have to install the `accumulator-server.py` script and in a path under your control, `~/bin` is the recommended one. Alternatively, you can install them in a system directory such as `/usr/bin` but it could collide with other programs, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see `scripts/testEnv.sh` file) and create a virtualenv environment to use Flask version 1.0.2 instead of default Flask in Ubuntu. Run test harness in this environment.

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        virtualenv /opt/ft_env
        . /opt/ft_env/bin/activate
        pip install Flask==2.0.2 paho-mqtt==1.6.1

* Run test harness (it takes some time, please be patient). Before starting test by make command, apply the following patch to avoid test failing.

        sed -i -e "s/Peer certificate cannot be authenticated[^\"]*/SSL peer certificate or SSH remote key was not OK/" /opt/fiware-orion/test/functionalTest/cases/0706_direct_https_notifications/direct_https_notifications_no_accept_selfsigned.test
        make functional_test INSTALL_DIR=~

* Once passed all the functional tests, you can run the valgrind tests (this will take longer than the functional tests, arm yourself with a lot of patience):

        make valgrind

You can generate coverage reports for the Orion Context Broker using the following procedure (optional):

* Install the lcov tool

        sudo apt install lcov

* Do first a successful pass for unit_test and functional_test, to check that everything is ok (see above)

* Run coverage

        make coverage INSTALL_DIR=~

* Setup for running Orion as system service after running tests. Run the `curl localhost:1026/version` command to check that the setup has been successful:

        sudo mkdir /etc/sysconfig
        sudo cp /opt/fiware-orion/etc/config/contextBroker /etc/sysconfig/
        sudo touch /var/log/contextBroker/contextBroker.log
        sudo chown orion /var/log/contextBroker/contextBroker.log
        sudo cp /opt/fiware-orion/rpm/SOURCES/etc/logrotate.d/logrotate-contextBroker-daily /etc/logrotate.d/
        sudo cp /opt/fiware-orion/rpm/SOURCES/etc/sysconfig/logrotate-contextBroker-size /etc/sysconfig/
        sudo cp /opt/fiware-orion/rpm/SOURCES/etc/cron.d/cron-logrotate-contextBroker-size /etc/cron.d/
        sudo systemctl daemon-reload
        sudo systemctl start contextBroker.service 
