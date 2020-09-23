# Orion-LD Installation Guide for Ubuntu 20.04.1

In order to write this guide, Ubuntu 20.04.01 LTS (Desktop image) was downloaded from [here](https://releases.ubuntu.com/20.04/), and installed as a virtual machine under VMWare.

## Disclaimer
Running Orion-LD in Ubuntu 20.04 is experimental. 18.04 is the official distibution.
While the Orion-LD development team has checked that it is possible to compile and run Orion-LD under Ubuntu 20.04,
it has yet not been thoroughly tested and its use is not recommended until all tests have been performed.
Once we are satisfied with the test results, this disclaimer will be removed and 20.04 will push out 18.04 as
official distribution.

## Installation of dependency packages

To be installed via package manager:
* boost (plenty of libraries)
* libssl-dev
* libcurl4
* libsasl2
* libgcrypt
* libuuid
* libz-dev

Now, a whole bunch of packages are to be installed. Personally I prefer *aptitude* over *apt-get*, so the first thing I do is to install *aptitude*:

```bash
sudo apt-get install aptitude
```

Tools needed for compilation and testing:

```bash
sudo aptitude install build-essential cmake scons curl
```

Libraries that aren't built from source code:

```bash
sudo aptitude install libssl-dev gnutls-dev libcurl4-gnutls-dev libsasl2-dev \
                      libgcrypt-dev uuid-dev libboost1.67-dev libboost-regex1.67-dev libboost-thread1.67-dev \
                      libboost-filesystem1.67-dev libz-dev

```

## Download and build dependency libraries from source code
Some libraries are built from source code and those sources must be downloaded and compiled.
* Mongo Driver:   legacy-1.1.2
* libmicrohttpd:  0.9.48
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

And, as `git` will be used, we might as well install it right away:

```bash
sudo aptitude install git
```

### Mongo Driver
As Orion-LD is based on Orion, and Orion uses the old Legacy C++ driver of the mongo client library, Orion-LD also uses that old library.
Plans are to migrate, at least all the NGSI-LD requests to the newest C driver of the mongo client, but that work has still not commenced.

To download, build and install:

```bash
sudo aptitude install libmongoclient-dev
```

After this, you should have the library *libmongoclient.a* under `/usr/local/lib/` and the header directory *mongo* under `/usr/local/include/`.

### libmicrohttpd

*libmicrohttpd* is the library that takes care of incoming connections and http/https.
We use an older version of it, soon to be repaced by the latest release.
This is how you install libmicrohttpd from source code:

```bash
sudo mkdir /opt/libmicrohttpd
sudo chown $USER:$GROUP /opt/libmicrohttpd
cd /opt/libmicrohttpd
wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz
tar xvf libmicrohttpd-0.9.48.tar.gz
cd libmicrohttpd-0.9.48
./configure --disable-messages --disable-postprocessor --disable-dauth
make
sudo make install
```

### rapidjson

*rapidjson* is the JSON parser used by the NGSI APIv2 implementation.
AS Orion-LD includes NGSI APIv2 as well, we need this library.
Like libmicrohttpd, we use an older version of the library.
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
make install
```

### MQTT (Paho & Mosquitto)

*MQTT* is a machine-to-machine (M2M)/"Internet of Things" connectivity protocol. It was designed as an extremely lightweight publish/subscribe messaging transport. It is useful for connections with remote locations where a small code footprint is required and/or network bandwidth is at a premium. Source: https://mqtt.org

To download, build and install:

#### Eclipse Paho

The *Eclipse Paho* project provides open-source client implementations of MQTT and MQTT-SN messaging protocols aimed at new, existing, and emerging applications for the Internet of Things (IoT). Source: https://www.eclipse.org/paho

```bash
sudo aptitude install doxygen
sudo aptitude install graphviz
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
sudo aptitude install python3-paho-mqtt
```
#### Eclipse Mosquitto

*Eclipse Mosquitto* is an open source (EPL/EDL licensed) message broker that implements the MQTT protocol versions 5.0, 3.1.1 and 3.1. Mosquitto is lightweight and is suitable for use on all devices from low power single board computers to full servers. Source: https://mosquitto.org

```bash
sudo aptitude install mosquitto
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
```

At the end of `make install`, the makefile wants to copy the executable (orionld) to /usr/bin, and more files under /usr.
Unless we do something, this will fail, as privileges are needed to create/modify files in system directories.
What we do is to create the files by hand, using `sudo` and then set oueselves as owner of the files.
For this yoiu need to know your user and group ids.
Your user id you already have in the env var `USER`. Your GROUP you have to look up.
Normally the group id is the same as the user id (but, you can be in more than one group).
See your group using the command `id`:

```bash
$ id
uid=1000(kz) gid=1000(kz) groups=1000(kz),4(adm),24(cdrom),27(sudo),30(dip),46(plugdev),120(lpadmin),131(lxd),132(sambashare)
```
As you can see in this example, my USER is 'kz' and my group is also 'kz', only, I'm in a few more groups as well.
The first one is the one we'll use: `groups=1000(kz)`.
To use it we'll create an env var:
```bash
export GROUP=kz  # *
```
(*): Please don't blindly use 'kz' - use the group that `id` gave you!

After that, we create and change owner of three files:
```bash
sudo touch /usr/bin/orionld
sudo chown $USER:$GROUP /usr/bin/orionld
sudo touch /etc/init.d/orionld
sudo chown $USER:$GROUP /etc/init.d/orionld
sudo touch /etc/default/orionld
sudo chown $USER:$GROUP /etc/default/orionld
```
And finally we can compile the broker:
```bash
make install
```

You now have *orionld*, the NGSI-LD Context Broker compiled, installed and ready to work :)

Except, of course, you need to install the MongoDB server as well.
So far, we have only installed the mongo client library, so that *orionld* can speak to the MongoDB server.

## Install the MongoDB server
If using a docker image, the MongoDB server comes as part of the docker, but if docker is not used, then the MongoDB server must be installed.
For this, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/).
The version 4.4 is recommended for Ununtu 20.04, as older versions of mongodb don't seem to be supported bu mongodb 4.4 (there are no
installation instructions in the mongodb website for Ubuntu 20.04 and mongodb 4.0, nor 4.2 - only 4.4).

This is what the MongoDB documentation tells us to do to install MongoDB server 4.4 under Ubuntu 20.04:

```bash
# Import the MongoDB public GPG Key
sudo aptitude install gnupg
wget -qO - https://www.mongodb.org/static/pgp/server-4.4.asc | sudo apt-key add -
# Should respond with "OK"

# Create the list file /etc/apt/sources.list.d/mongodb-org-4.0.list
echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/4.4 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.4.list

# Reload local package database
sudo aptitude update

# Install the MongoDB packages
sudo aptitude install -y mongodb-org

# Start the mongodb daemon
sudo systemctl start mongod.service

# Ensure that MongoDB will start following a system reboot
sudo systemctl enable mongod.service
```

For more detail on the MongoDB installation process, or if something goes wrong, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/)
