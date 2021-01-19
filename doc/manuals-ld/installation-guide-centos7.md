# Orion-LD Installation Guide for CentOS 7

In order to write this guide, CentOS 7 (Desktop image) was downloaded from [here](http://isoredirect.centos.org/centos/8/isos/x86_64/CentOS-8-x86_64-1905-dvd1.iso), and installed as a virtual machine under VMWare.

*Obs.: If you don't have internet access execute this command:*
```bash
dhclient
```

## Installation of dependency packages

To be installed via package manager:

* boost: 1.53
* libmicrohttpd: 0.9.72 (from source)
* libcurl: 7.29.0
* openssl: 1.0.2k
* libuuid: 2.23.2
* Mongo Driver: legacy-1.1.2 (from source)
* rapidjson: 1.0.2 (from source)

Tools needed for compilation and testing:

Install OKay repository that offers an alternate to complement missing packages from CENTOS and EPEL. Source (https://okay.network/blog-news/rpm-repositories-for-centos-6-and-7.html)

```bash
sudo yum install http://repo.okay.com.mx/centos/7/x86_64/release/okay-release-1-1.noarch.rpm
```

```bash
sudo yum install make cmake gcc-c++ scons curl wget
```

Libraries that aren't built from source code:

```bash
sudo yum install boost-devel libcurl-devel gnutls-devel libgcrypt-devel openssl-devel libuuid-devel cyrus-sasl-devel libicu libicu-devel

```

## Download and build dependency libraries from source code
Some libraries are built from source code and those sources must be downloaded and compiled.

* Mongo Driver:   legacy-1.1.2
* libmicrohttpd:  0.9.72
* rapidjson:      1.0.2
* kbase:          0.5
* klog:           0.5
* kalloc:         0.5
* kjson:          0.5
* khash:          0.5
* gtest:          1.5 (needed for unit testing only)
* gmock:          1.5 (needed for unit testing only)

For those libraries that are cloned repositories, I myself keep all repositories in a directory I call *git* directly under my home directory: `~/git`.
This guide follows that example, so, let's start by creating the directory for repositories:

```bash
mkdir ~/git
```

And, as `git` will be used, we might as well install it:

```bash
sudo yum install git
```

### Mongo Driver
As Orion-LD is based on Orion, and Orion uses the old Legacy C++ driver of the mongo client library, Orion-LD also uses that old library.
Plans are to migrate, at least all the NGSI-LD requests to the newest C driver of the mongo client, but that work has still not commenced.

To download, build and install:

```bash
sudo mkdir /opt/mongoclient
sudo chown $USER:$GROUP /opt/mongoclient  # (1)
cd /opt/mongoclient
wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
tar xfvz legacy-1.1.2.tar.gz
cd mongo-cxx-driver-legacy-1.1.2
scons --disable-warnings-as-errors --ssl --use-sasl-client
sudo scons install --prefix=/usr/local --disable-warnings-as-errors --ssl --use-sasl-client
```
(1) To make you the owner of a file, you need to state your username and group.
    The env var **USER** already exists, but if you want to cut 'n paste this "sudo chown" command, you'll need to create the env var **GROUP**, to reflect your group. In my case, I do this:
```bash
export GROUP=kz
```

After this, you should have the library *libmongoclient.a* under `/usr/local/lib/` and the header directory *mongo* under `/usr/local/include/`.

### libmicrohttpd

*libmicrohttpd* is the library that takes care of incoming connections and http/https.
This is how you install libmicrohttpd from source code:

```bash
sudo mkdir /opt/libmicrohttpd
sudo chown $USER:$GROUP /opt/libmicrohttpd
cd /opt/libmicrohttpd
wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.72.tar.gz
tar xvf libmicrohttpd-0.9.72.tar.gz
cd libmicrohttpd-0.9.72
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
git checkout release/0.5
make install
```

### klog

*klog* is a library for logging, used by the rest of the "K-libs".
To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/klog.git
cd klog
git checkout release/0.5
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
git checkout release/0.5
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
git checkout release/0.5
make install
```

### khash

*khash* is a library that provides a hash table implementation. This hash table is used for the Context Cache of Orion-LD.

To download, build and install:
```bash
cd ~/git
git clone https://gitlab.com/kzangeli/khash.git
cd khash
git checkout release/0.5
vi makefile
# Edit makefile and add/write that content at CFLAGS after "-g"
CFLAGS= -g -std=c99
make install
```

### MQTT (Paho & Mosquitto)

*MQTT* is a machine-to-machine (M2M)/"Internet of Things" connectivity protocol. It was designed as an extremely lightweight publish/subscribe messaging transport. It is useful for connections with remote locations where a small code footprint is required and/or network bandwidth is at a premium. Source: https://mqtt.org

To download, build and install:

#### Eclipse Paho

The *Eclipse Paho* project provides open-source client implementations of MQTT and MQTT-SN messaging protocols aimed at new, existing, and emerging applications for the Internet of Things (IoT). Source: https://www.eclipse.org/paho

```bash
sudo yum -y install doxygen
sudo yum -y install graphviz
sudo rm -f /usr/local/lib/libpaho*
cd ~/git
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
git fetch -a
git checkout tags/v1.3.1
make html
make
sudo make install

# Python library

# If you don't have pip installed:
sudo yum update
sudo yum install python-pip

sudo pip install paho-mqtt
```
#### Eclipse Mosquitto

*Eclipse Mosquitto* is an open source (EPL/EDL licensed) message broker that implements the MQTT protocol versions 5.0, 3.1.1 and 3.1. Mosquitto is lightweight and is suitable for use on all devices from low power single board computers to full servers. Source: https://mosquitto.org

```bash
sudo yum -y install epel-release
sudo yum -y install mosquitto
sudo systemctl start mosquitto

# If you wish to enable `mosquitto` to have it start automatically on system reboot:
# [ If you prefer to use another MQTT broker, that's fine too. But, bear in mind that only mosquitto has been tested ]
sudo systemctl enable mosquitto
```


## Source code of Orion-LD

Now that we have all the dependencies installed, it's time to clone the Orion-LD repository:

```bash
cd ~/git
git clone https://github.com/FIWARE/context.Orion-LD.git
cd context.Orion-LD
make install
```

At the end of `make install`, the makefile wants to copy the executable (orionld) to /usr/bin, and more files under /usr.
As the compilation hasn't been (and shouldn't be) run as root (sudo), these copies will fail.
So, you have two options here:

&nbsp;1.  Create the files by hand, using `sudo` and then set yourself as owner of the files:
```bash
sudo touch /usr/bin/orionld
sudo chown $USER:$GROUP /usr/bin/orionld
sudo touch /etc/init.d/orionld
sudo chown $USER:$GROUP /etc/init.d/orionld
sudo touch /etc/default/orionld
sudo chown $USER:$GROUP /etc/default/orionld
```

&nbsp;2.  Run `sudo make install` and let the files be owned by root.

Personally I prefer option 1. I really dislike to use `sudo`.

You now have *orionld*, the NGSI-LD Context Broker compiled, installed and ready to work!

Except, of course, you need to install the MongoDB server as well.
So far, we have only installed the mongo client library, so that *orionld* can speak to the MongoDB server.

## Install the MongoDB server
If using a docker image, the MongoDB server comes as part of the docker, but if docker is not used, then the MongoDB server must be installed.
For this, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-red-hat/).
The version 4.0 is recommended, but both older and newer should work just fine.

This is what the MongoDB documentation tells us to do to install MongoDB server 4.0 under CentOS 7:

```bash
# Create a .repo file for yum, the package management utility for CentOS:
sudo vi /etc/yum.repos.d/mongodb-org.repo

# Copy & paste that content into mongodb-org.repo file:
[mongodb-org-3.4]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/$releasever/mongodb-org/4.0/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-4.0.asc

# Verify that the MongoDB repository exists
yum repolist
# Output
. . .
repo id                          repo name
base/7/x86_64                    CentOS-7 - Base
extras/7/x86_64                  CentOS-7 - Extras
mongodb-org-${VERSION}/7/x86_64  MongoDB Repository
updates/7/x86_64                 CentOS-7 - Updates
. . .

# Install the MongoDB packages
sudo yum install mongodb-org

# Start the MongoDB service with the systemctl utility:
sudo systemctl start mongod
```
*Mongo Server for CentOS 7, tutorial from: [Digital Ocean: How To Install MongoDB on CentOS 7](https://www.digitalocean.com/community/tutorials/how-to-install-mongodb-on-centos-7)


For more detail on the MongoDB installation process, or if something goes wrong, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/)
