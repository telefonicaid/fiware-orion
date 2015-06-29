# Building from sources

Orion Context Broker reference distribution is CentOS 6.x. This doesn't mean that the broker cannot be built in other distributions (actually, it can). This section also includes indications on how to build in other distributions, just in the case it may help people that don't use CentOS. However, note that the only "officially supported" procedure is the one for CentOS 6.x; the others are provided "as is" and can get obsolete from time to time.

**Note:** the build includes both contextBroker binary and proxyCoap. If you are not interested in proxyCoap at all, you can disable it just commenting out the following line in the CMakeList.txt file:


    ADD_SUBDIRECTORY(src/app/proxyCoap)

and removing the following files:

    test/functionalTest/cases/coap_basic.test
    test/functionalTest/cases/coap_command_line_options.test
    test/functionalTest/cases/coap_version.test


In that case, you can also ignore all the steps in the building process marked as "(Optional proxyCoap)"

## CentOS 6.x. (officially supported)

The Orion Context Broker uses the following libraries as build dependencies:

* boost: 1.41 (the one that comes in EPEL6 repository)
* libmicrohttpd: 0.9.22 (the one that comes in EPEL6 repository)
* libcurl: 7.19.7
* Mongo Driver: legacy-1.0.2 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)
* cantcoap (for proxyCoap)

We assume that EPEL6 repository is configured in yum, given that many RPM packages are installed from there
(check the procedure at http://fedoraproject.org/wiki/EPEL#How_can_I_use_these_extra_packages.3F):

The basic procedure is as follows (assuming you don't run commands as root, we use sudo for those
commands that require root privilege):

* Install the needed building tools (compiler, etc.).

        sudo yum install make cmake gcc-c++ scons

* Install the required libraries (except the mongo driver and gmock, described in following steps).

        sudo yum install libmicrohttpd-devel boost-devel libcurl-devel

* Install the Mongo Driver from source:

        wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.2.tar.gz
        tar xfvz legacy-1.0.2.tar.gz
        cd mongo-cxx-driver-legacy-1.0.2
        scons                                         # The build/linux2/normal/libmongoclient.a library is generated as outcome
        sudo scons install --prefix=/usr/local        # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib

* Install Google Test/Mock from sources (there are RPM pacakges for this, but they don't seem to be working with the current CMakeLists.txt configuration)

        wget http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* (Optional proxyCoap) Install cantcoap (with dependencies). Note that we are using a particular snapshot of the code (corresponding to around July 21st, 2014) given that cantcoap repository doesn't provide any releasing mechanism.


        sudo yum install clang CUnit-devel

        git clone https://github.com/staropram/cantcoap
        cd cantcoap
        git checkout 749e22376664dd3adae17492090e58882d3b28a7
        make
        sudo cp cantcoap.h /usr/local/include
        sudo cp dbg.h /usr/local/include
        sudo cp nethelper.h /usr/local/include
        sudo cp libcantcoap.a /usr/local/lib

* Get the code (alternatively you can download it using a zipped version or a different URL pattern, e.g `git clone git@github.com:telefonicaid/fiware-orion.git`):

        sudo yum install git
        git clone https://github.com/telefonicaid/fiware-orion

* Build the source:

        cd fiware-orion
        make

* (Optional but highly recommended) run unit test. Firstly, you have to install MongoDB (as the unit tests rely on mongod running in localhost).

        sudo yum install mongodb-server
        sudo yum  update pcre            # otherwise, mongod crashes in CentOS 6.3
        sudo /etc/init.d/mongod start
        sudo /etc/init.d/mongod status   # to check that mongod is actually running
        make unit_test

* Install the binary. You can use INSTALL_DIR to set the installation prefix path (default is /usr), thus the broker is installed in `$INSTALL_DIR/bin` directory.

        sudo make install INSTALL_DIR=/usr

* Check that everything is ok, invoking the broker version message:

        contextBroker --version

The Orion Context Broker comes with a suite of valgrind and end-to-end tests that you can also run, following the following procedure (optional):

* Install the required tools:

        sudo yum install python python-flask python-jinja2 curl libxml2 nc mongodb valgrind libxslt

* (Optional proxyCoap) Install COAP client (an example application included in the libcoap sources).

        wget http://sourceforge.net/projects/libcoap/files/coap-18/libcoap-4.1.1.tar.gz/download
        mv download libcoap-4.1.1.tar.gz
        tar xvzf libcoap-4.1.1.tar.gz
        cd libcoap-4.1.1
        ./configure
        make
        sudo cp examples/coap-client /usr/local/bin

* Run valgrind tests (it takes some time, please be patient):

        make valgrind

* Prepare the environment for test harness. Basically, you have to install the accumulator-server.py script and in a path under your control, ~/bin is the recommended one. Alternatively, you can install them in a system directory such as /usr/bin but it could collide with an RPM installation, thus it is not recommended. In addition, you have to set several environment variables used by the harness script (see scripts/testEnv.sh file).

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh

* Run test harness (it takes some time, arm yourself with patience).

        make functional_test INSTALL_DIR=~

You can generate coverage reports for the Orion Context Broker using the following procedure (optional):

* Install the required tools. You need lcov 1.10 - the one that comes with CentOS 6.3 (lcov 1.9) is not valid.

        sudo rpm -Uhv http://downloads.sourceforge.net/ltp/lcov-1.10-1.noarch.rpm

* Do first a successful pass for unit_test and functional_test, to check that everything is ok (see above)

* Run coverage

        make coverage INSTALL_DIR=~

You can generate the RPM for the source code (optional):

* Install the required tools

        sudo yum install rpm-build

* Generate the RPM

        make rpm

* The generated RPMs are placed in directory `~/rpmbuild/RPMS/x86_64`.

## Others

If you have build orion in a system different from CentOS 6.x, don't hesitate to tell us and contribute to expand this section. Probably the best way if doing a pull request to modify this file with the new information. Thanks!

### Debian 7

The packages are basically the same described for RedHat/CentOS above, except that we need to install packages using apt-get instead of yum.

Install Google Test and Google Mock version 1.5 directly from sources.

The version of lcov that comes with Debian 7.0 (1.9) has a bug (see https://bugs.launchpad.net/ubuntu/+source/lcov/+bug/1163758). Install lcov 1.10 from sources.
