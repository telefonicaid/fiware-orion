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
As mentioned, Orion-LD runs on a variety of Linux platforms.
Ubuntu 18.04 LTS is the officially supported distribution, but Orion-LD potentially works on any modern Linux distribution.

To install Orion-LD from source code, please follow the instructions for your selected distribution.

Installation guides for different distributions:
* [Ubuntu 18.04.3 LTS](installation-guide-ubuntu-18.04.3.md) - the Official Distribution
* [CentOS 7](installation-guide-centos7.md)
* [Debian 9](installation-guide-debian9.md)


### Sanity check

Now that you have Orion-LD compiled and installed, and the MongoDB server running, the broker should work.
To make sure all is OK, let's start it and run a few simple commands against it.

First, make sure the MongoDB server really is running:

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
I'd propose to start with the [Getting Started with Orion-LD Guide](getting-started-guide.md)
 
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
Now please continue to learn about Orion-LD with the (Getting Started Guide)[getting-started-guide.md].

If you feel like running tests, to **really** make sure your Orion-LD Context Broker is working correctly,
please follow the instructions in your respective Installation Guide for Functional Tests:
* [Ubuntu 18.04.3](installation-guide-functional-tests-ubuntu18.04.3.md)
* [CentOS 7](installation-guide-functional-tests-centos7.md)
* [Debian 9](installation-guide-functional-tests-debian9.md)


And/Or: run the Unit Tests (there are no specific Unit Tests for the NGSi-LD part, only older orion stuff), how to do it is described [here](installation-guide-for-unit-tests.md).
