# Building from sources

Orion Context Broker reference distribution is Debian 12. This doesn't mean that the broker cannot be built in other distributions (actually, it can). This section may include indications on how to build in other distributions, just in the case it may help people that don't use Debian. However, note that the only "officially supported" procedure is the one for Debian 12.

You can also have a look to [3.1 Building in not official distributions](../../../docker/README.md#31-building-in-not-official-distributions) section in the Docker documentation to check how to build Docker containers images in distributions other than the official one.

*NOTE:* the build process described in this document does not include the cjexl library, as it is considered optional from the point of view of the basic building process.

## Debian 12 (officially supported)

The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.74
* libmicrohttpd: 1.0.1 (from source)
* libcurl: 7.88.1
* openssl: 3.0.14
* libuuid: 2.38.1
* libmosquitto: 2.0.20 (from source)
* Mongo C driver: 1.29.0 (from source)
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

        wget https://github.com/mongodb/mongo-c-driver/releases/download/1.29.0/mongo-c-driver-1.29.0.tar.gz
        tar xfvz mongo-c-driver-1.29.0.tar.gz
        cd mongo-c-driver-1.29.0
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

        wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-1.0.1.tar.gz
        tar xvf libmicrohttpd-1.0.1.tar.gz
        cd libmicrohttpd-1.0.1
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* Install mosquitto from sources (appart from changing WITH_CJSON, WITH_STATIC_LIBRARIES and WITH_SHARED_LIBRARIES settings, config.mk file under mosquitto-2.0.20/ can be modified to fine tune the build)

        wget https://mosquitto.org/files/source/mosquitto-2.0.20.tar.gz
        tar xvf mosquitto-2.0.20.tar.gz
        cd mosquitto-2.0.20
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

* (Optional but highly recommended) run unit test and functional tests. More on this on [its specific section below](#testing-and-coverage).

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

### Testing and coverage

The Orion Context Broker comes with a suite of unit, valgrind and end-to-end tests that you can also run, following the following procedure (optional but highly recommended):

* Install Google Test/Mock from sources. Previously the URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but Google removed that package in late August 2016 and it is no longer working.

        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        # Adjust /path/to/fiware-orion in the next line accordingly to where you local copy of fiware-orion repo is in your system
        patch -p1 gtest/scripts/fuse_gtest_files.py < /path/to/fiware-orion/ci/deb/fuse_gtest_files.py.patch
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

In the case of the aarch64 architecture, install libxslt using apt-get, and run `./configure` with `--build=arm-linux` option.

* Install MongoDB (tests rely on mongod running in localhost). Check [the official MongoDB documentation](https://www.mongodb.com/docs/manual/tutorial/install-mongodb-on-debian/) for details. Recommended version is 6.0 (it may work with previous versions, but we don't recommend it).

* Run unit test

        make unit_test

* Install additional required tools for functional and valgrind tests:

        sudo apt-get install curl netcat-traditional valgrind bc python3 python3-pip mosquitto

* Prepare the environment for test harness. Basically, you have to install the `accumulator-server.py` script and in a path under your control, `~/bin` is the recommended one. Alternatively, you can install them in a system directory such as `/usr/bin` but it could collide with an other programs, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see `scripts/testEnv.sh` file) and create a virtualenv environment with the required Python packages.

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        python3 -m venv /opt/ft_env   # or 'virtualenv /opt/ft_env --python=/usr/bin/python3' in some systems
        . /opt/ft_env/bin/activate
        pip install Flask==2.0.2 Werkzeug==2.0.2 paho-mqtt==1.6.1 amqtt==0.11.0b1

* Run test harness in this environment (it takes some time, please be patient).

        make functional_test INSTALL_DIR=~

* Once passed all the functional tests, you can run the valgrind tests (this will take longer than the functional tests, arm yourself with a lot of patience):

        make valgrind INSTALL_DIR=~

You can generate coverage reports for the Orion Context Broker using the following procedure (optional):

* Install the lcov tool

        sudo apt-get install lcov xsltproc

* Do first a successful pass for unit_test and functional_test, to check that everything is ok (see above)

* Run coverage

        make coverage INSTALL_DIR=~

*NOTE*: Functional tests relying in debug traces are expected to fail under coverage execution (e.g. notification_different_sizes or not_posix_regex_idpattern.test). This is due to the LM_T macros used by the debug traces are disabled in the coverage code build, as they add "noise" in condition coverage. This way coverage reports are more useful.