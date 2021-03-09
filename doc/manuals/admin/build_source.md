# Building from sources

Orion Context Broker reference distribution is CentOS 8.x. This doesn't mean that the broker cannot be built in other distributions (actually, it can). This section also includes indications on how to build in other distributions, just in the case it may help people that don't use CentOS. However, note that the only "officially supported" procedure is the one for CentOS 8.x; the others are provided "as is" and can get obsolete from time to time.

## CentOS 8.x (officially supported)

The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.66
* libmicrohttpd: 0.9.70 (from source)
* libcurl: 7.61.1
* openssl: 1.1.1g
* libuuid: 2.32.1
* Mongo C driver: 1.17.4 (from source)
* rapidjson: 1.1.0 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)

The basic procedure is as follows (assuming you don't run commands as root, we use sudo for those
commands that require root privilege):

* Install the needed building tools (compiler, etc.).

        sudo yum install make cmake gcc-c++

* Install the required libraries (except what needs to be taken from source, described in following steps).

        sudo yum install boost-devel libcurl-devel gnutls-devel libgcrypt-devel openssl-devel libuuid-devel cyrus-sasl-devel

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

* Get the code (alternatively you can download it using a zipped version or a different URL pattern, e.g `git clone git@github.com:telefonicaid/fiware-orion.git`):

        sudo yum install git
        git clone https://github.com/telefonicaid/fiware-orion

* Build the source:

        cd fiware-orion
        make

* (Optional but highly recommended) run unit test and functional tests. More on this on [its specific section below](#testing-coverage-and-rpm).

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

### Testing, coverage and RPM

The Orion Context Broker comes with a suite of unit, valgrind and end-to-end tests that you can also run, following the following procedure (optional but highly recommended):

* Install Google Test/Mock from sources (there are RPM packages for this, but they do not work with the current CMakeLists.txt configuration). Previously the URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but Google removed that package in late August 2016 and it is no longer working.

        sudo yum install python2
        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sed -i 's/env python/env python2/' gtest/scripts/fuse_gtest_files.py  # little hack to make installation to work on CentOS 8
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

In the case of the aarch64 architecture, install perl-Digest-MD5 and libxslt using yum, and run `./configure` with `--build=arm-linux` option.

* Install MongoDB (tests rely on mongod running in localhost). Check [the official MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-red-hat/) for details. Recommended version is 4.4.

* Run unit test

        make unit_test

* Install aditional required tools for functional and valgrind tests:

        sudo yum install curl nc valgrind bc
        sudo pip2 install virtualenv

In the case of the aarch64 architecture, additionally install python-devel and libffi-devel using yum. It is needed when building pyOpenSSL.

* Prepare the environment for test harness. Basically, you have to install the `accumulator-server.py` script and in a path under your control, `~/bin` is the recommended one. Alternatively, you can install them in a system directory such as `/usr/bin` but it could collide with an RPM installation, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see `scripts/testEnv.sh` file) and create a virtualenv environment with the required Python packages.

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        virtualenv /opt/ft_env --python=/usr/bin/python2
        . /opt/ft_env/bin/activate
        pip install Flask==1.0.2 pyOpenSSL==19.0.0

* Run test harness in this environment (it takes some time, please be patient).

        make functional_test INSTALL_DIR=~

* Once passed all the functional tests, you can run the valgrind tests (this will take longer than the functional tests, arm yourself with a lot of patience):

        make valgrind INSTALL_DIR=~

You can generate coverage reports for the Orion Context Broker using the following procedure (optional):

* Install the lcov tool

        # Download .rpm file from http://downloads.sourceforge.net/ltp/lcov-1.14-1.noarch.rpm
        sudo yum install lcov-1.14-1.noarch.rpm

* Do first a successful pass for unit_test and functional_test, to check that everything is ok (see above)

* Run coverage

        make coverage INSTALL_DIR=~

You can generate the RPM for the source code (optional):

* Install the required tools

        sudo yum install rpm-build

* Generate the RPM

        make rpm

* The generated RPMs are placed in directory `~/rpmbuild/RPMS/x86_64`.

## Ubuntu 18.04 LTS

**FIXME:** this section needs to be reviewed taking into account
the new procedure to install the mongo C driver.

This instruction is how to build the Orion Context Broker for the x86_64 or the aarch64 architecture on Ubuntu 18.04 LTS.
And it includes the instruction to build MongoDB 3.6 that the Orion depends on.
The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.65.1
* libmicrohttpd: 0.9.70 (from source)
* libcurl: 7.58.0
* openssl: 1.0.2n
* libuuid: 2.31.1
* Mongo Driver: legacy-1.1.2 (from source)
* rapidjson: 1.1.0 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)
* MongoDB: 3.6.17 (from source)

The basic procedure is as follows (assuming you don't run commands as root, we use sudo for those
commands that require root privilege):

* Install the needed building tools (compiler, etc.).

        sudo apt install build-essential cmake scons

* Install the required libraries (except what needs to be taken from source, described in following steps).

        sudo apt install libboost-dev libboost-regex-dev libboost-thread-dev libboost-filesystem-dev \
                         libcurl4-gnutls-dev gnutls-dev libgcrypt-dev libssl1.0-dev uuid-dev libsasl2-dev

* Install the Mongo Driver from source. The Mongo Driver 1.1.2 is written in legacy code intended to compile with gcc 4.x.
So some kind of warnings are treated as errors when building with newer gcc. To avoid such errors, it is necessary to
add a `-Wno-{option name}` option to CCFLAGS.

        wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
        tar xfvz legacy-1.1.2.tar.gz
        cd mongo-cxx-driver-legacy-1.1.2
        # The build/linux2/normal/libmongoclient.a library is generated as outcome
        scons  --use-sasl-client --ssl "CCFLAGS=-Wno-nonnull-compare -Wno-noexcept-type -Wno-format-truncation"
        # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib
        sudo scons install --prefix=/usr/local --use-sasl-client --ssl "CCFLAGS=-Wno-nonnull-compare -Wno-noexcept-type -Wno-format-truncation"

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

* Install Google Test/Mock from sources (there are RPM packages for this, but they do not work with the current CMakeLists.txt configuration). Previously the URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but Google removed that package in late August 2016 and it is no longer working.

        apt install xsltproc
        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

In the case of the aarch64 architecture, run `./configure` with `--build=arm-linux` option.

* Get the code (alternatively you can download it using a zipped version or a different URL pattern, e.g `git clone git@github.com:telefonicaid/fiware-orion.git`):

        sudo apt install git
        git clone https://github.com/telefonicaid/fiware-orion

* Build the source:

        cd fiware-orion
        make

* Build MongoDB 3.6.17 from source code and install it. To run unit test, you have to build MongoDB as the unit and functional
tests rely on mongod running in localhost (The MongoDB 3.6 binary for Ubuntu 18.04 is not provided.). The instruction to build
MongoDB from source code is as the following. In the case of the aarch64 architecture, additionally add the `-march=armv8-a+crc`
option to CCFLAGS. Run the `mongo` command to check that MongoDB has been successfully installed:

        # Build MongoDB
        sudo apt install build-essential cmake scons  # Not required if you installed in previous step
        sudo apt install python python-pip            # Not required if you installed in previous step
        pip install --upgrade pip                     # Not required if you installed in previous step
        cd /opt
        git clone -b r3.6.17 --depth=1 https://github.com/mongodb/mongo.git
        cd mongo
        pip install --user -r buildscripts/requirements.txt
        python buildscripts/scons.py mongo mongod mongos \
          "CCFLAGS=-Wno-nonnull-compare -Wno-format-truncation -Wno-noexcept-type" \
          --wiredtiger=on \
          --mmapv1=on
        # Install MongoDB
        strip -s mongo*
        sudo cp -a mongo mongod mongos /usr/bin/ 
        sudo useradd -M -s /bin/false mongodb
        sudo mkdir /var/lib/mongodb /var/log/mongodb /var/run/mongodb
        sudo chown mongodb:mongodb /var/lib/mongodb /var/log/mongodb /var/run/mongodb
        sudo cp -a ./debian/mongod.conf /etc/
        sudo cp -a ./debian/mongod.service /etc/systemd/system/
        sudo systemctl start mongod

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

The Orion Context Broker comes with a suite of functional, valgrind and end-to-end tests that you can also run, following the following procedure (optional):

* Install the required tools:

        sudo apt install python python-pip curl netcat valgrind bc
        sudo pip install --upgrade pip

In the case of the aarch64 architecture, additionally install python-devel and libffi-devel using apt. It is needed when building pyOpenSSL.

* Prepare the environment for test harness. Basically, you have to install the `accumulator-server.py` script and in a path under your control, `~/bin` is the recommended one. Alternatively, you can install them in a system directory such as `/usr/bin` but it could collide with an RPM installation, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see `scripts/testEnv.sh` file) and create a virtualenv environment to use Flask version 1.0.2 instead of default Flask in Ubuntu. Run test harness in this environment.

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        pip install virtualenv
        virtualenv /opt/ft_env
        . /opt/ft_env/bin/activate
        pip install Flask==1.0.2 pyOpenSSL==19.0.0

* Run test harness (it takes some time, please be patient).

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
