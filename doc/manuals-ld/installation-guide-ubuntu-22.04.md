# Orion-LD Installation Guide for Ubuntu 22.04.1

In order to write this guide, Ubuntu 22.04.01 LTS (Desktop image) was downloaded from [here](https://releases.ubuntu.com/22.04/), and installed as a virtual machine under VMWare.

## Warning
MongoDB still doesn't support installation on Ubuntu 22.04.
Scroll down to the end (last 10 lines) and read more about it.
Hint: if you really want to install on Ubuntu 22.04, there's a workaround for it.

## Disclaimer
Running Orion-LD in Ubuntu 22.04 is experimental. 18.04 is the official distribution.
While the Orion-LD development team has checked that it is possible to compile and run Orion-LD under Ubuntu 22.04,
it has yet not been thoroughly tested and its use is not recommended until all tests have been performed.
Once we are satisfied with the test results, this disclaimer will be removed and 22.04 will push out 18.04 as
official distribution.

## Installation of dependency packages

A whole bunch of packages are to be installed. Personally I prefer *aptitude* over *apt-get*, so the first thing I do is to install *aptitude*:
```bash
sudo apt-get install -y aptitude
```
If you want to be able to cut 'n paste from this guide, then please do the same - install aptitude.

### Tools for compilation and testing:
```bash
sudo aptitude install -y build-essential scons curl cmake
```

### Dependency libraries that aren't built from source code:
```bash
sudo aptitude install -y libssl-dev gnutls-dev libcurl4-gnutls-dev libsasl2-dev \
                         libgcrypt-dev uuid-dev libboost1.67-dev libboost-regex1.67-dev libboost-thread1.67-dev \
                         libboost-filesystem1.67-dev libz-dev libmongoclient-dev
```

## Download and build dependency libraries from source code
Some libraries are built from source code and those sources must be downloaded and compiled.
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

### k-libs

A number of generic libraries are being implemented while implementing Orion-LD.
Well, the job started when I implemented my JSON parser - kjson - back in ... 2018?
As Orion-LD makes use of these libraries and sometimes stuff is added (seldom changed), they "live beside" the broker

* kbase      a collection of basic functionality, like string handling, that is used by the rest of the "K-libs".
* klog       logging - used by the rest of the "K-libs"
* kalloc     a library that provides faster allocation by avoiding calls to `malloc`.
             The library allocates *big* buffers by calling `malloc` and then gives out portions of this big allocated buffer.
             The portions cannot be freed, only the *big* buffers allocated via `malloc` and that is done when the kalloc instance dies.
             For a context broker, that treats every request in a separate thread, this is ideal from a performance point of view.
* kjson      a JSON parser that builds a simple-to-use KjNode tree from the textual JSON input.
             It is very easy to use (linked lists) and it is A LOT faster than rapidjson, which APIv2 uses.
             The new NGSI-LD requests uses `kjson` instead of rapidjson.
* khash      a library that provides a hash table implementation. This hash table is used for the Context Cache of Orion-LD.


```bash
cd ~/git
for k in kbase klog kalloc kjson khash
do
  git clone https://gitlab.com/kzangeli/$k.git
done

for k in kbase klog kalloc khash
do
  cd $k
  git checkout release/0.8
  make install
  cd ..
done

cd kjson
git checkout release/0.8.2
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
git checkout tags/v1.3.10
make html
make
sudo make install

#### Python paho-mqtt library
sudo aptitude install -y python3-pip
pip3 install paho-mqtt
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
sed 's/build_test cmake -v/build_test cmake/' autolib/build.sh > XXX
mv XXX autolib/build.sh
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
curl -fsSL https://www.postgresql.org/media/keys/ACCC4CF8.asc|sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/postgresql.gpg
echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | sudo tee  /etc/apt/sources.list.d/pgdg.list

sudo apt update
sudo apt -y install postgresql-12 postgresql-client-12
sudo apt install -y postgis postgresql-12-postgis-3
sudo apt-get install -y postgresql-12-postgis-3-scripts
```


#### Add timescale db and postgis
```bash
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 55EE6BF7698E3D58D72C0DD9ECB3980CC59E610B
sudo tee /etc/apt/sources.list.d/timescale-ubuntu-timescaledb-ppa-jammy.list<<EOF
deb https://ppa.launchpadcontent.net/timescale/timescaledb-ppa/ubuntu/ focal main
EOF
sudo apt install timescaledb-postgresql-12

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
sudo systemctl restart postgresql@12-main.service
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

Ubuntu 22.04 is still more or less new and the good people from MongoDB are still to provide installation instructions.
Here's the issue in [MongoDB's Jira](https://jira.mongodb.org/browse/SERVER-62300).

If you really want to run on 22.04, this guide from [cloudbooklet](https://www.cloudbooklet.com/how-to-install-mongodb-on-ubuntu-22-04/)
gives you info on how to make it work.
They install MongoDB 5.0, which probably works just fine with Orion-LD.
However, if you want to go safe, install 4.4 instead - 5.x is untested.

The official position of the Orion-LD team is:

  "Wait until MongoDB releases instructions on how to install MongoDB on Ubuntu 22.04"
