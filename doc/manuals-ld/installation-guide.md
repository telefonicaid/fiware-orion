# Installing Orion-LD

## Introduction
Orion-LD is an NGSI-LD publish/subscribe broker (or: Context Broker).

For installation of Orion-LD, alpha version 0.0.1, you can:
* use a prebuilt docker image, or
* compile the broker from source code

For now, Orion-LD only runs under Linux.
No effort has been made so far to make Orion-LD run under any other platform.
The obvious choice would be MacOS, and if anybody volunteers to investigate on this, it would be a very welcome contribution.

## Supported Linux Distributions
Orion-LD NGSI-LD Context Broker reference distribution is Ubuntu 18.04.
While Orion-LD works just fine also in CentOS, Debian, etc, or, other versions of Ubuntu, the inly officially supported distribution is Ubuntu 18.04.

## Installation

### Docker Images

Docker images for Ubuntu and CentOS are produced for each Pull Request merged into the `develop` branch.
To create and run the Orion-LD Docker image, it is necessary to have [Docker](https://www.docker.com/).
and [Docker Compose](https://docs.docker.com/compose) installed. A sample `docker-compose.yml` can be found below:

```yaml
version: "3.5"
services:
  # Orion is the context broker
  orion:
    image: fiware/orion-ld
    hostname: orion
    container_name: fiware-orion
    depends_on:
      - mongo-db
     expose:
      - "1026"
    ports:
      - "1026:1026" 
    command: -dbhost mongo-db -logLevel DEBUG
    healthcheck:
      test: curl --fail -s http://orion:1026/version || exit 1

  # Databases
  mongo-db:
    image: mongo:3.6
    hostname: mongo-db
    container_name: db-mongo
    expose:
      - "27017"
    ports:
      - "27017:27017" 
    command: --nojournal
    volumes:
      - mongo-db:/data
      
volumes:
  mongo-db: ~
```

We also provide Centos and Debian based Dockerfiles to facilitate the building of your own images. 
They can be found in the following [location](https://github.com/FIWARE/context.Orion-LD/tree/develop/docker) 
along  with the documentation describing how to use it.

A public image is also available on [Docker Hub](https://hub.docker.com/r/fiware/orion-ld/) 

### Building from source code
First thing to do is to install the operating system.
In order to write this guide, Ubuntu 18.04.3 LTS (Desktop image) was downloaded from [here](http://releases.ubuntu.com/18.04/), and installed as a virtual machine under VMWare.

#### Install dependencies packages

To be installed via package manager:
* boost (plenty of libraries)
* libssl1.0
* libcurl4
* libsasl2
* libgcrypt
* libuuid

Now, a whole bunch of packages are to be installed. Personally I prefer *aptitude* over *apt-get*, so the first thing I do is to install *aptitude*:

```bash
sudo apt-get install aptitude
```

Tools needed for compilation:

```bash
sudo aptitude install build-essential cmake scons
```

Libraries that aren't built from source code:

```bash
sudo aptitude install libssl1.0-dev gnutls-dev libcurl4-gnutls-dev libsasl2-dev \
    libgcrypt-dev uuid-dev libboost-dev libboost-regex-dev libboost-thread-dev \
    libboost-filesystem-dev
```

#### Download and build dependency libraries from source code
Some libraries are built from source code and those sources must be downloaded and compiled.
* Mongo Driver:   legacy-1.1.2
* libmicrohttpd:  0.9.48
* rapidjson:      1.0.2
* kbase:          0.2
* klog:           0.2
* kalloc:         0.2
* kjson:          0.2
* khash:          0.2
* gtest:          1.5 (needed for unit testing only)
* gmock:          1.5 (needed for unit testing only)

For those libraries that are cloned repositories, I myself keep all repositories in a directory I call *git* directly under my home directory: `~/git`.
This guide follows that example, so, let's start by creating the directory for repositories:

```bash
mkdir ~/git
```

And, as `git` will be used, we might as well install it:

```bash
sudo aptitude install git
```

##### Mongo Driver
As Orion-LD is based on Orion, and Orion uses the old Legacy C++ driver of the mongo client library, Orion-LD also uses that old library.
Plans are to migrate, at least all the NGSI-LD requests to the newest C driver of the mongo client, but that work has still not commenced.

To download, build and install:

```bash
sudo mkdir /opt/mongoclient
sudo chown $USER:$GROUP /opt/mongoclient  (*)
cd /opt/mongoclient
wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
tar xfvz legacy-1.1.2.tar.gz
cd mongo-cxx-driver-legacy-1.1.2
scons --disable-warnings-as-errors --ssl --use-sasl-client
sudo scons install --prefix=/usr/local --disable-warnings-as-errors --ssl --use-sasl-client
```

> To make you the owner of a file, you need to state your username and group.
>      The env var USER already exists, but if you want to cut 'n paste this "sudo chown" command, you'll 
> need to create the `env var GROUP`, to reflect your group. In my case, I do the following:
>```
>export GROUP=kz
>```
>
>After this, you should have the library *libmongoclient.a* under `/usr/local/lib/` and the header 
>directory *mongo* under `/usr/local/include/`.

##### libmicrohttpd
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

##### rapidjson

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

##### kbase

*kbase* is a collection of basic functionality, like string handling, that is used by the rest of the "K-libs".
To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/kbase.git
cd kbase
git checkout release/0.2
make install
```

##### klog
*klog* is a library for logging, used by the rest of the "K-libs".
To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/klog.git
cd klog
git checkout release/0.2
make install
```


##### kalloc

*kalloc* is a library that provides faster allocation by avoiding calls to `malloc`.
The library allocates *big* buffers by calling `malloc` and then gives out portions of this big allocated buffer.
The portions cannot be freed, only the *big* buffers allocated via `malloc` and that is done when the kalloc instance dies.
For a context broker, that treats every request in a separate thread, this is ideal from a performance point of view.

To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/kalloc.git
cd kalloc
git checkout release/0.2
make install
```

##### kjson  - ToDo: Move kjson/kjLog.h to klog

*kjson* is a JSON parser that builds a simple-to-use KjNode tree from the textual JSON input.
It is very easy to use (linked lists) and many times faster than rapidjson, which APIv2 uses.
The new implementation for NGSI-LD uses `kjson` instead of rapidjson.

To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/kjson.git
cd kjson
git checkout release/0.2
make install
```

##### khash

*khash* is a library that provides a hash table implementation. This hash table is used for the Context Cache of Orion-LD.

To download, build and install:

```bash
cd ~/git
git clone https://gitlab.com/kzangeli/khash.git
cd khash
git checkout release/0.2
make install
```

#### Source code of Orion-LD

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

#### Install the MongoDB server

If using a docker image, the MongoDB server comes as part of the docker, but if docker is not used, then the MongoDB server must be installed.
For this, preser refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/).
The version 4.0 is recommended, but both older and newer should work just fine.

This is an what the MongoDB documentation tells us to do to install MongoDB server 4.0 under Ubuntu 18.04:

```bash
# Import the MongoDB public GPG Key
wget -qO - https://www.mongodb.org/static/pgp/server-4.0.asc | sudo apt-key add -
# Should respon with "OK"

# Create the list file /etc/apt/sources.list.d/mongodb-org-4.2.list
echo "deb [ arch=amd64 ] https://repo.mongodb.org/apt/ubuntu bionic/mongodb-org/4.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.0.list

# Reload local package database
sudo apt-get update

# Install the MongoDB packages
sudo apt-get install -y mongodb-org
```

For more detail on the installation process, or if something goes wrong, please refer to the [MongoDB documentation](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/)

### Sanity check

Now that you have everything installed the broker should work.
To make sure, let's start it and run a few simple commands against it.
First, make sure the MongoDB server is running:

```bash
ps aux | grep mongodb
```

Example output if MongDB *is* running:

```text
mongodb   27265  7.5  3.9 1115980 79720 ?       Ssl  11:34   0:00 /usr/bin/mongod --config /etc/mongod.conf
```

If not running (no /usr/bin/mongod process found in the previous command), please start it:

```bash
sudo service mongod start
```

You might want to look in the MongoDB documentation on how to make it start automatically on boot.

Let's start the broker in foreground, in a separate window:

```bash
orionld -fg
```

In another window, we send curl commands to the broker.
First we need to install curl though :):

```bash
sudo aptitude install curl
curl localhost:1026/ngsi-ld/ex/v1/version
```

You should see a response similar to this:

```bash
kz@ubuntu:~$ curl localhost:1026/ngsi-ld/ex/v1/version
```

```json
{
  "@context": "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
  "branch": "develop",
  "kbase version": "0.2",
  "kalloc version": "0.2",
  "kjson version": "0.2",
  "boost version": "1_65_1",
  "microhttpd version": "0.9.48-0",
  "openssl version": "OpenSSL 1.0.2n  7 Dec 2017",
  "mongo version": "1.1.2",
  "rapidjson version": "1.0.2",
  "libcurl version": "7.58.0",
  "libuuid version": "UNKNOWN",
  "Next File Descriptor": 18
}
```
You will also probably notice traces output in the broker's terminal.

Now let's create an entity:

```bash
  curl localhost:1026/ngsi-ld/v1/entities -d '{ "id": "urn:entities:E1", "type": "T", "P1": { "type": "Property", "value": 12 }}' -H 'Content-Type: application/json' --dump-header /tmp/httpHeaders.out
```
*curl* only prints the payload data of the response on the screen, and as an Entity Creation _has no payload data_, you will see nothing (unless there is a nerror).
You can check that the curl command worked by issuing the following command (right after the curl command):

```bash
echo $?
```

*$?* is an environment variable that stores the return code of the last executed shell command.
Zero means OK, anything else means an error.

There is no payload data in the response, but, can see the HTTP headers of the response, as the option `--dump-header /tmp/httpHeaders.out` was used:

```bash
cat /tmp/httpHeaders.out
```
Expected HTTP headers:

```text
HTTP/1.1 201 Created
Connection: Keep-Alive
Content-Length: 0
Link: <https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
Location: /ngsi-ld/v1/entities/urn:entities:E2
Date: Tue, 19 Nov 2019 18:56:01 GMT
```

As may be extrapolated, there is no payload data in the response - only HTTP headers. That's why `--dump-header /tmp/httpHeaders.out` was added in the curl request andf the second command shows the context of the file.
An entity has been created and we can see it in mongo, like this:

```sql
  echo 'db.entities.findOne()' | mongo mongodb://localhost:27017/orion
```
Expected response:

```text
kz@ubuntu:~$ echo 'db.entities.findOne()' | mongo mongodb://localhost:27017/orion
MongoDB shell version v4.0.13
connecting to: mongodb://localhost:27017/orion?gssapiServiceName=mongodb
Implicit session: session { "id" : UUID("9ece1fb7-df0d-41a3-8316-042425351dd8") }
MongoDB server version: 4.0.13
{
	"_id" : {
		"id" : "urn:entities:E1",
		"type" : "https://uri.etsi.org/ngsi-ld/default-context/T",
		"servicePath" : "/"
	},
	"attrNames" : [
		"https://uri.etsi.org/ngsi-ld/default-context/P1"
	],
	"attrs" : {
		"https://uri=etsi=org/ngsi-ld/default-context/P1" : {
			"type" : "Property",
			"creDate" : 1574189638,
			"modDate" : 1574189638,
			"value" : 12,
			"mdNames" : [ ]
		}
	},
	"creDate" : 1574189638,
	"modDate" : 1574189638,
	"lastCorrelator" : ""
}
bye
```

If you see something similar to this, then congratulations!!! everything works! 

If you look closer at the data in mongo, you will see that the entity type and the name of the attribute are longer exactly what you "asked for".
Actually it is - this is the way it should be, it's an essential part of NGSI-LD-
In a few words, the context has been used to expand the entity type and the attribute name.
How the context works is fully explained in documents and tutorials on NGSI-LD.
I'd propose to start with the [Getting Started with Orion-LD Guide](doc/manuals-ld/getting-started-guide.md)
 
Now, before we end, let's ask _the broker_ to give us the entity, instead of just peeking inside the database:

```bash
curl 'localhost:1026/ngsi-ld/v1/entities?type=T&prettyPrint=yes&space=2'
```

The output should be exactly like this:

```json
[
  {
    "id": "urn:entities:E1",
    "type": "T",
    "P1": {
      "type": "Property",
      "value": 12
    }
  }
]
```
As you can see, the entity type (T) and attribute name (P1) are no longer long names (they are in the database).
This is because the context has been used to compact the values.

What? No context was used in these two operations?

Actually, that is not entirely true. The *Core Context* was used, and no *user context* was supplied.
The *Core Context* is the default context and if no context is specified, then this Core Context is used.

Actually, even if the user specifies a context, the broker will still use the Core Context, and any item
present in the Core Context will override and item (with the same key/name) of the user provided context.

The context that was used to compose the response is returned in the HTTP *Link* header.
If you ask curl for the HTTP headers, doing the GET operation like this:

```bash
curl 'localhost:1026/ngsi-ld/v1/entities?type=T&prettyPrint=yes&space=2' --dump-header /tmp/httpHeaders.out
```
and then view the HTTP headers:

```bash
cat --dump-header /tmp/httpHeaders.out
```
you'll see this:

```text
HTTP/1.1 200 OK
Connection: Keep-Alive
Content-Length: 120
Content-Type: application/json
Link: <https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld>; rel="http://www.w3.org/ns/json-ld#context"; type="application/ld+json"
Date: Wed, 20 Nov 2019 11:49:04 GMT
```

*https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld* - that's the Core Context!

If you instead ask the broker to return the context inside the payload data (which is done by setting the *Accept* header to *application/ld+json*):

```bash
curl 'localhost:1026/ngsi-ld/v1/entities?type=T&prettyPrint=yes&space=2' -H "Accept: application/ld+json"
```

then the response would be like this:

```json
[
  {
    "@context": "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
    "id": "urn:entities:E1",
    "type": "T",
    "P1": {
      "type": "Property",
      "value": 12
    }
  }
]
```
and no HTTP Link header would be present.

That's all for the installation guide.
Now please continue to learn about Orion-LD with the (Getting Started Guide)[doc/manuals-ld/getting-started-guide.md].
