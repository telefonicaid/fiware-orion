# Building from sources

Orion Context Broker reference distribution is CentOS 7.x. This doesn't mean that the broker cannot be built in other distributions (actually, it can). This section also includes indications on how to build in other distributions, just in the case it may help people that don't use CentOS. However, note that the only "officially supported" procedure is the one for CentOS 7.x; the others are provided "as is" and can get obsolete from time to time.

## CentOS 7.x (officially supported)

The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.53
* libmicrohttpd: 0.9.48 (from source)
* libcurl: 7.29.0
* openssl: 1.0.2k
* libuuid: 2.23.2
* Mongo Driver: legacy-1.1.2 (from source)
* rapidjson: 1.0.2 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)

The basic procedure is as follows (assuming you don't run commands as root, we use sudo for those
commands that require root privilege):

* Install the needed building tools (compiler, etc.).

        sudo yum install make cmake gcc-c++ scons

* Install the required libraries (except what needs to be taken from source, described in following steps).

        sudo yum install boost-devel libcurl-devel gnutls-devel libgcrypt-devel openssl-devel libuuid-devel

* Install the Mongo Driver from source. The following procedure corresponds with default installation, if you want to include SASL and SSL support use [this alternative procedure](#building-mongodb-driver-with-sasl-and-ssl-support) instead.

        wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
        tar xfvz legacy-1.1.2.tar.gz
        cd mongo-cxx-driver-legacy-1.1.2
        scons                                         # The build/linux2/normal/libmongoclient.a library is generated as outcome
        sudo scons install --prefix=/usr/local        # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib

* Install rapidjson from sources:

        wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz
        tar xfvz v1.0.2.tar.gz
        sudo mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include

* Install libmicrohttpd from sources (the `./configure` command below shows the recommended build configuration to get minimum library footprint, but if you are an advanced user, you can configure as you prefer)

        wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz
        tar xvf libmicrohttpd-0.9.48.tar.gz
        cd libmicrohttpd-0.9.48
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* Install Google Test/Mock from sources (there are RPM packages for this, but they do not work with the current CMakeLists.txt configuration). Previously the URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but Google removed that package in late August 2016 and it is no longer working.

        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* Get the code (alternatively you can download it using a zipped version or a different URL pattern, e.g `git clone git@github.com:telefonicaid/fiware-orion.git`):

        sudo yum install git
        git clone https://github.com/telefonicaid/fiware-orion

* Build the source:

        cd fiware-orion
        make

* (Optional but highly recommended) run unit test. Firstly, you have to install MongoDB as the unit and functional tests
rely on mongod running in localhost. Check [the official MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-red-hat/)
for details. Recommended version is 3.4 (although 3.2 should also work fine).

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

The Orion Context Broker comes with a suite of functional, valgrind and end-to-end tests that you can also run, following the following procedure (optional):

* Install the required tools:

        sudo yum install python python-flask pyOpenSSL curl nc mongodb-org-shell valgrind bc

* Prepare the environment for test harness. Basically, you have to install the `accumulator-server.py` script and in a path under your control, `~/bin` is the recommended one. Alternatively, you can install them in a system directory such as `/usr/bin` but it could collide with an RPM installation, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see `scripts/testEnv.sh` file).

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh

* Run test harness (it takes some time, please be patient).

        make functional_test INSTALL_DIR=~

* Once passed all the functional tests, you can run the valgrind tests (this will take longer than the functional tests, arm yourself with a lot of patience):

        make valgrind

You can generate coverage reports for the Orion Context Broker using the following procedure (optional):

* Install the lcov tool

        sudo yum install lcov

* Do first a successful pass for unit_test and functional_test, to check that everything is ok (see above)

* Run coverage

        make coverage INSTALL_DIR=~

You can generate the RPM for the source code (optional):

* Install the required tools

        sudo yum install rpm-build

* Generate the RPM

        make rpm

* The generated RPMs are placed in directory `~/rpmbuild/RPMS/x86_64`.

### Building MongoDB driver with SASL and SSL support

The procedure is as follows:

```
wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
tar xfvz legacy-1.1.2.tar.gz cd mongo-cxx-driver-legacy-1.1.2
yum install cyrus-sasl-devel
scons --use-sasl-client --ssl                                   # The build/linux2/normal/libmongoclient.a library is generated as outcome
sudo scons install --prefix=/usr/local --use-sasl-client --ssl  # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib
```
