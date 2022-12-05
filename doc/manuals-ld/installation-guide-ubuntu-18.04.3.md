# Orion-LD Installation Guide for Ubuntu 18.04.3

In order to write this guide, Ubuntu 18.04.3 LTS (Desktop image) was downloaded from [here](http://releases.ubuntu.com/18.04/), and installed as a virtual machine under VMWare.
As 18.04 is pretty old (4 years as of this last edit), you will be asked to upgrade to Ubuntu 20.04.
Well, it goes without saying, DON'T - this guide is for 18.04.

#### NOTE  
You'll also be asked to update a bunch of packages - do that.
Always good to be as up-to-date as possible.

## Installation of dependency packages

A whole bunch of packages are to be installed. Personally I prefer *aptitude* over *apt-get*, so the first thing I do is to install *aptitude*:
```bash
sudo apt-get install -y aptitude
```
If you want to be able to cut 'n paste from this guide, then please do the same - install aptitude.

### Tools for compilation and testing:
```bash
sudo aptitude install -y build-essential scons curl python-pip
```

### Dependency libraries that aren't built from source code:
```bash
sudo aptitude install -y libssl1.0-dev libcurl4-gnutls-dev libsasl2-dev libgnutls28-dev \
                         libgcrypt-dev uuid-dev libboost-dev libboost-regex-dev libboost-thread-dev \
                         libboost-filesystem-dev
```

## Download and build dependency libraries from source code
Some libraries are built from source code and those sources must be downloaded and compiled.
* cmake                3.14.5
* Mongo C++ Driver:    legacy-1.1.2
* Mongo C driver:      1.22.0
* libmicrohttpd:       0.9.75
* rapidjson:           1.0.2
* kbase:               0.8
* klog:                0.8
* kalloc:              0.8
* kjson:               0.8.2
* khash:               0.8
* Paho MQTT            1.3.1
* Prometheus C Client  0.1.3
* Postgres libs        12

For those libraries that are cloned repositories, I myself keep all repositories in a directory I call *git* directly under my home directory: `~/git`.
For this guide to work exactly "as is", you will need to do the same.

### USER and GROUP environment variables
In order to make yourself owner of "a bunch of" files, basically to avoid running everything as root, which is generally a bad idea,
we'll prepare an environment variable for the name of your group - your user and group ids are needed to change ownership of files.

Your user id you already have in the env var `USER`. Your GROUP you have to look up.
Normally the group id is the same as the user id (but, you can be in more than one group).
See your group using the command `id`:

```bash
$ id
uid=1000(kz) gid=1000(kz) groups=1000(kz),4(adm),24(cdrom),27(sudo),30(dip),46(plugdev),120(lpadmin),131(lxd),132(sambashare)
```
As you can see in this example, my USER is 'kz' and my group is also 'kz' - only, I'm in a few more groups as well.
The first one is the one we'll use: `gid=1000(kz)`.
The following env var is created, to enable more copy 'n paste :
```bash
export GROUP=kz
```
Please don't blindly use 'kz' - use the group that `id` gave you!

After this we have both USER and GROUP env vars for changing owners of any file - and those lines can be copy 'n pasted from this document.

Now, let's create the git directory for cloned repositories:
```bash
mkdir ~/git
```

And, as `git` will be used, we might as well install it right away:

```bash
sudo aptitude install -y git
```

### cmake
sudo mkdir /opt/cmake
sudo chown $USER:$GROUP /opt/cmake
cd /opt/cmake
wget https://cmake.org/files/v3.14/cmake-3.14.5.tar.gz
tar zxvf cmake-3.14.5.tar.gz
cd cmake-3.14.5
./bootstrap --prefix=/usr/local
make
sudo make install


### Mongo C++ Driver
As Orion-LD is based on Orion, and Orion uses the old Legacy C++ driver of the mongo client library, Orion-LD also uses that old library.
Plans are to migrate, at least all the NGSI-LD requests to the newest C driver of the mongo client, but that work has still not commenced.

To download, build and install:

```bash
sudo mkdir /opt/mongoclient
sudo chown $USER:$GROUP /opt/mongoclient
cd /opt/mongoclient
wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
tar xfvz legacy-1.1.2.tar.gz
cd mongo-cxx-driver-legacy-1.1.2
scons --disable-warnings-as-errors --ssl --use-sasl-client
sudo scons install --prefix=/usr/local --disable-warnings-as-errors --ssl --use-sasl-client
```

Footnote: _Orion (as opposed to Orion-LD) no longer uses the legacy mongodb driver._
_Orion-LD was forked from Orion's repo back in 2018, long before Orion made the move from the legacy driver to mongoc, and thus_
_Orion-LD still uses the legacy driver - something that ib being amended for the NGSI-LD API endpoints of Orion-LD._

### Mongo C driver
The idea is to leave the old mongo legacy driver in the dust and new stuff is implemented using the new C driver.
Install the driver like this:
```
sudo mkdir /opt/mongoc
sudo chown $USER:$GROUP /opt/mongoc
cd /opt/mongoc
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.22.0/mongo-c-driver-1.22.0.tar.gz
tar xzf mongo-c-driver-1.22.0.tar.gz
cd mongo-c-driver-1.22.0
mkdir cmake-build
cd cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
cmake --build .
sudo cmake --build . --target install
```

### libmicrohttpd

*libmicrohttpd* is the library that takes care of incoming connections and http/https.
This is how you install libmicrohttpd from source code:

```bash
sudo mkdir /opt/libmicrohttpd
sudo chown $USER:$GROUP /opt/libmicrohttpd
cd /opt/libmicrohttpd
wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.75.tar.gz
tar xvf libmicrohttpd-0.9.75.tar.gz
cd libmicrohttpd-0.9.75
./configure --disable-messages --disable-postprocessor --disable-dauth
make
sudo make install
```

### rapidjson

*rapidjson* is the JSON parser used by the NGSI APIv2 implementation.
AS Orion-LD includes NGSI APIv2 as well, we need this library.
We use an older version of the library.
This is	how to install it from source code:

```bash
sudo mkdir /opt/rapidjson
sudo chown $USER:$GROUP /opt/rapidjson
cd /opt/rapidjson
wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz
tar xfvz v1.0.2.tar.gz
sudo mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include
```

### kbase

*kbase* is a collection of basic functionality, like string handling, that is used by the rest of the "K-libs".
To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/kbase.git
cd kbase
git checkout release/0.8
make install
```

### klog

*klog* is a library for logging, used by the rest of the "K-libs".
To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/klog.git
cd klog
git checkout release/0.8
make install
```


### kalloc

*kalloc* is a library that provides faster allocation by avoiding calls to `malloc`.
The library allocates *big* buffers by calling `malloc` and then gives out portions of this big allocated buffer.
The portions cannot be freed, only the *big* buffers allocated via `malloc` and that is done when the kalloc instance dies.
For a context broker, that treats every request in a separate thread, this is ideal from a performance point of view.

To download, build and install:
```bash
cd ~/git
git clone https://gitlab.com/kzangeli/kalloc.git
cd kalloc
git checkout release/0.8
make install
```

### kjson

*kjson* is a JSON parser that builds a simple-to-use KjNode tree from the textual JSON input.
It is very easy to use (linked lists) and many times faster than rapidjson, which APIv2 uses.
The new implementation for NGSI-LD uses `kjson` instead of rapidjson.

To download, build and install:
```bash
cd ~/git
git clone https://gitlab.com/kzangeli/kjson.git
cd kjson
git checkout release/0.8.2
make install
```

### khash

*khash* is a library that provides a hash table implementation. This hash table is used for the Context Cache of Orion-LD.

To download, build and install:
```bash
cd ~/git
git clone https://gitlab.com/kzangeli/khash.git
cd khash
git checkout release/0.8
make install
```

### MQTT (Paho MQTT)

*MQTT* is a machine-to-machine (M2M)/"Internet of Things" connectivity protocol. It was designed as an extremely lightweight publish/subscribe messaging transport. It is useful for connections with remote locations where a small code footprint is required and/or network bandwidth is at a premium. Source: https://mqtt.org

To download, build, and install:

#### Eclipse Paho

The *Eclipse Paho* project provides open-source client implementations of MQTT and MQTT-SN messaging protocols aimed at new, existing, and emerging applications for the Internet of Things (IoT). Source: https://www.eclipse.org/paho

```bash
sudo aptitude install -y doxygen
sudo aptitude install -y graphviz
sudo rm -f /usr/local/lib/libpaho*
cd ~/git
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
git checkout tags/v1.3.1
make html
make
sudo make install

#### Python paho-mqtt library
pip install paho-mqtt
```

### Prometheus C Client Library
For Orion-LD to support Prometheus metrics, a library called prometheus-client-c is used.
[ It seems to be abandoned and some day Orion-LD might stop using it and implement native Prometheus metrics support. ]

Install prometheus-client-c like this:
```
cd ~/git
git clone https://github.com/digitalocean/prometheus-client-c.git
cd prometheus-client-c
git checkout release-0.1.3
sed 's/\&promhttp_handler,/(MHD_AccessHandlerCallback) \&promhttp_handler,/' promhttp/src/promhttp.c > XXX
mv XXX promhttp/src/promhttp.c
./auto build
```

### Postgres development libraries
```bash
sudo apt-get install -y libpq-dev
```

## Compiling Orion-LD from source code
Now that we have all the dependencies installed, it's time to clone the Orion-LD repository:

```bash
cd ~/git
git clone https://github.com/FIWARE/context.Orion-LD.git
cd context.Orion-LD
```

And, before compiling (well, during installing), we need to prepare a few files that otherwise would give access problems:
```bash
sudo touch /usr/bin/orionld
sudo chown $USER:$GROUP /usr/bin/orionld
sudo touch /etc/init.d/orionld
sudo chown $USER:$GROUP /etc/init.d/orionld
sudo touch /etc/default/orionld
sudo chown $USER:$GROUP /etc/default/orionld
```
And finally we can compile the broker, just, by default, we are on the "develop" branch.
If that is not what you want, please checkout whatever release/tag you desire.
```bash
make install
```
If you plan on developing anything for Orion-LD, you'd rather compile the debug version:
```bash
make di   # "di" is a make target that does "debug install"
```

The Prometheus lib is a shared library (well, two libraries) and in a "weird" place, so for that we will need to use LD_LIBRARY_PATH:
```
export LD_LIBRARY_PATH=~/git/prometheus-client-c/prom/build:~/git/prometheus-client-c/promhttp/build
```
[If you don't want to do this, you could just as easily copy the libs to a standard location]

With this, Orion-LD itself is ready to run, however, to actually do something, Orion-LD needs its database (MongoDB),
and Postgres as well if you plan on using the temporal feature,
If you plan on using MQTT for notifications, then you also need an MQTT broker/server.
For development on Orion-LD you need the works - as if anything is missing, the functional test suite will fail.

## Installation of 3rd Party "Enablers" - Mosquitto, Postgres, and MongoDB
### Eclipse Mosquitto
[ If you prefer to use another MQTT broker, that's fine too. But, bear in mind that only mosquitto has been tested with Orion-LD ]

*Eclipse Mosquitto* is an open source (EPL/EDL licensed) message broker that implements the MQTT protocol versions 5.0, 3.1.1 and 3.1.
Mosquitto is lightweight and is suitable for use on all devices from low power single board computers to full servers.
Source: https://mosquitto.org

```bash
sudo aptitude install -y mosquitto
sudo systemctl start mosquitto
```
If you wish to enable `mosquitto` to have it start automatically on system reboot:
```bash
sudo systemctl enable mosquitto
```

### Postgres 12
Postgres is used as database for the Temporal Evolution of entities

#### Install Postgres 12
```bash
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | sudo tee  /etc/apt/sources.list.d/pgdg.list

sudo apt update
sudo apt -y install postgresql-12 postgresql-client-12
sudo apt install -y postgis postgresql-12-postgis-3
sudo apt-get install -y postgresql-12-postgis-3-scripts
```

#### Add timescale db and postgis
```bash
sudo add-apt-repository ppa:timescale/timescaledb-ppa
sudo apt-get update
sudo apt install -y timescaledb-postgresql-12
```

#### Enable postgres
```bash
systemctl enable postgresql
```

#### Edit postgresql.conf, enabling timescaledb
```bash
sudo nano /etc/postgresql/12/main/postgresql.conf
```
Add this line at the end of the file and save it
```bash
shared_preload_libraries = 'timescaledb'
```
#### Restart Postgres
```bash
sudo /etc/init.d/postgresql restart
```

#### Create the Postgres user for Orion-LD
NOTE: As default Postgres user and password, Orion-LD uses `postgres` and `password`.
      This is probably not the username/password that you want to use, so, change those, and start the broker accordingly (CLI options `-troeUser` + `-troePwd`).
```bash
# Change user to 'postgres'
sudo su - postgres
# Enter psql interactive shell
psql
ALTER USER postgres WITH PASSWORD 'password';
\q

# Once out of the psql interactive shell - exit the session as 'postgres' user
logout
```

### MongoDB
If using a docker image, the MongoDB server comes as part of the docker, but if docker is not used, then the MongoDB server must be installed.
As a reference, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/).
The version 4.0 is recommended for Ubuntu 18.04 (by Mongo), but both older and newer should work just fine, however, MongoDB v5.x has not been tested.

This is what the MongoDB documentation tells us to do to install MongoDB server 4.0 under Ubuntu 18.04:

```bash
# Import the MongoDB public GPG Key
sudo aptitude install -y gnupg
wget -qO - https://www.mongodb.org/static/pgp/server-4.0.asc | sudo apt-key add -
# Should respond with "OK"

# Create the list file /etc/apt/sources.list.d/mongodb-org-4.0.list
echo "deb [ arch=amd64 ] https://repo.mongodb.org/apt/ubuntu bionic/mongodb-org/4.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.0.list

# Reload local package database
sudo aptitude update

# Install the MongoDB packages
sudo aptitude install -y mongodb-org

# Once the installation is done, start MongoDB, and ensure it starts on reboot
sudo systemctl start mongod
sudo systemctl enable mongod
```

For more details on the MongoDB installation process, or if something goes wrong, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/)

## Done, now what?
You now have *orionld*, the NGSI-LD Context Broker compiled, and all its 3rd party enablers installed and ready to work :)

A good idea would be to also prepare for [unit tests](installation-guide-for-unit-tests.md) and [functional tests](installation-guide-functional-tests-ubuntu18.04.3.md),
and make sure the broker is fully functional.
If you plan on developing anything for Orion-LD, then this is an absolute must!
No Pull Request should ever be sent to Orion-LD's github before all unit tests and all functional tests are working
