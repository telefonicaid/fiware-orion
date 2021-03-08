
# How to use Orion Context Broker with Docker

You can run Orion Context Broker very easily using docker. There are several ways to accomplish this. These are (in order of complexity):

- _"Have everything automatically done for me"_. See Section **1. The Fastest Way** (recommended).
- _"Only run a Context Broker, I'll take care of where I put my database"_. See Section **2. Run one container**.
- _"Let me see how this docker thing works from the inside"_ or _"I want to customize my Orion Docker file"_ : See Section **3. Build a docker image**.

These are alternative ways to do the same thing, you do not need to do all three of them.

And you can also run Orion on Raspberry Pi. See the [documentation](./raspberry_pi.md) on how to do this.

You do need to have docker in your machine. See the [documentation](https://docs.docker.com/installation/) on how to do this.


----
## 1. The Fastest Way

Docker Compose allows you to link an Orion Context Broker container to a MongoDB container in a few minutes. This method requires that you install [Docker Compose](https://docs.docker.com/compose/install/).

Consider this method if you want to try Orion Context Broker and do not want to bother about databases or you don't care about losing data.

Follow these steps:

1. Create a directory on your system on which to work (for example, `~/fiware`).
2. Create a new file called `docker-compose.yml` inside your directory with the following contents:
	
		mongo:
		  image: mongo:4.2
		  command: --nojournal
		orion:
		  image: fiware/orion
		  links:
		    - mongo
		  ports:
		    - "1026:1026"
		  command: -dbhost mongo

3. Using the command-line and within the directory you created type: `sudo docker-compose up`.

> Regarding --nojournal it is not recommened for production, but it speeds up mongo container start up and avoids some race conditions problems if Orion container is faster and doesn't find the DB up and ready.

After a few seconds you should have your Context Broker running and listening on port `1026`.

Check that everything works with

	curl localhost:1026/version

What you have done with this method is download images for [Orion Context Broker](https://hub.docker.com/r/fiware/orion/) and [MongoDB](https://hub.docker.com/_/mongo/) from the public repository of images called [Docker Hub](https://hub.docker.com/). Then you have created two containers based on both images.

If you want to stop the scenario you have to press Control+C on the terminal where docker-compose is running. Note that you will lose any data that was being used in Orion using this method.

----
## 2. Run one container

This method will launch a container running Orion Context Broker, but it is up to you to provide a MongoDB instance. This MongoDB instance may be running on localhost, other host on your network, another container, or anywhere you have network access to.

> TIP: If you are trying these methods or run them more than once and come across an error saying that the container already exists you can delete it with `docker rm orion1`. If you have to stop it first do `docker stop orion1`.

Keep in mind that if you use these commands you get access to the tags and specific versions of Orion. For example, you may use `fiware/orion:0.22` instead of `fiware/orion` in the following commands if you need that particular version. If you do not specify a version you are pulling from `latest` by default.

### 2A. MongoDB is on localhost

To do this run this command

	sudo docker run -d --name orion1 -p 1026:1026 fiware/orion

Check that everything works with

	curl localhost:1026/version

### 2B. MongoDB runs on another docker container
In case you want to run MongoDB on another container you can launch it like this

	sudo docker run --name mongodb -d mongo:3.4

And then run Orion with this command

	sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 fiware/orion -dbhost mongodb

Check that everything works with

	curl localhost:1026/version

This method is functionally equivalent as the one described in section 1, but doing the steps manually instead of with a docker-compose file. You equally lose your data as soon as you turn off your MongoDB container.

### 2C. MongoDB runs on a different host

If you want to connect to a different MongoDB instance do the following command **instead of** the previous one

	sudo docker run -d --name orion1 -p 1026:1026 fiware/orion -dbhost <MongoDB Host>

Check that everything works with

	curl localhost:1026/version
----
## 3. Build a docker image

Building an image gives more control on what is happening within the Orion Context Broker container. Only use this method if you are familiar with building docker images or really need to change how this image is built. For most purposes you probably don't need to build an image, only deploy a container based on one already built for you (which is covered in sections 1 and 2).

Steps:

1. Download [Orion's source code](https://github.com/telefonicaid/fiware-orion/) from Github (`git clone https://github.com/telefonicaid/fiware-orion/`)
2. `cd fiware-orion/docker`
3. Modify the Dockerfile to your liking
4. Run Orion...
	* Using an automated scenario with docker-compose and building your new image: `sudo docker-compose up`. You may also modify the provided `docker-compose.yml` file if you need so.
	* Manually, running MongoDB on another container: 
        	1. `sudo docker run --name mongodb -d mongo:3.4`
		2. `sudo docker build -t orion .`
		3. `sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 orion -dbhost mongodb`.
	* Manually, specifying where to find your MongoDB host:
		1. `sudo docker build -t orion .`
		2. `sudo docker run -d --name orion1 -p 1026:1026 orion -dbhost <MongoDB Host>`.

Check that everything works with

	curl localhost:1026/version

The parameter `-t orion` in the `docker build` command gives the image a name. This name could be anything, or even include an organization like `-t org/fiware-orion`. This name is later used to run the container based on the image.

The parameter `--build-arg` in the `docker build` can be set build-time variables. 

| ARG             | Description                                                         | Example                         |
| --------------- | ------------------------------------------------------------------- | ------------------------------- |
| IMAGE_TAG       | Specify a tag of the base image.                                    | --build-arg IMAGE_TAG=centos7   |
| GIT_NAME        | Specify a username of GitHub repository.                            | --build-arg GIT_NAME=fiware-ges |
| GIT_REV_ORION   | Specify the Orion version you want to build.                        | --build-arg GIT_REV_ORION=2.3.0 |
| CLEAN_DEV_TOOLS | Specify whether the development tools clear. It is remained when 0. | --build-arg CLEAN_DEV_TOOLS=0   |

If you want to know more about images and the building process you can find it in [Docker's documentation](https://docs.docker.com/userguide/dockerimages/).

## 4. Other info

Things to keep in mind while working with docker containers and Orion Context Broker.

### 4.1 Data persistence
Everything you do with Orion Context Broker when dockerized is non-persistent. *You will lose all your data* if you turn off the MongoDB container. This will happen with either method presented in this README.

If you want to prevent this from happening take a look at [this link](https://registry.hub.docker.com/_/mongo/) in section *Where to Store Data* of the MongoDB docker documentation. In it you will find instructions and ideas on how to make your MongoDB data persistent.

#### Set-up appropriate Mongo-DB Database Indexes

Subscription, registration and entity details are retrieved from a database.  Without supplying the `fiware-service` header, 
the default name of the database is `orion`. Database access can be optimized by creating appropriate indices.

For example: 

```console
docker exec  db-mongo mongo --eval '
	conn = new Mongo();db.createCollection("orion");
	db = conn.getDB("orion");
	db.createCollection("entities");
	db.entities.createIndex({"_id.servicePath": 1, "_id.id": 1, "_id.type": 1}, {unique: true});
	db.entities.createIndex({"_id.type": 1}); 
	db.entities.createIndex({"_id.id": 1});' > /dev/null
```

if the `fiware-service` header is being used, the name of the database will vary. Alter the `conn.getDB()` statement 
above if an alternative database is being used. Additional database indexes may be required depending upon your use case.
Further information on [performance tuning](https://fiware-orion.readthedocs.io/en/master/admin/perf_tuning/index.html#database-indexes)
and [database administration](https://fiware-orion.readthedocs.io/en/master/admin/database_admin/index.html) can
be found within the Orion documentation.

### 4.2 Using `sudo`

If you do not want to have to use `sudo` follow [these instructions](http://askubuntu.com/questions/477551/how-can-i-use-docker-without-sudo).

### 4.3 Listen on different ports

In `-p 1026:1026` the first value represents the port to listen on localhost. If you want to run a second context broker
on your machine you should change this value to something else, for example `-p 1027:1026`.

### 4.4 Extra parameters for Orion

Anything after the name of the container image (`orion` if you are building, or `fiware/orion` if you are pulling from the repository) is interpreted as a parameter for the Orion Context Broker. In this case we are telling the broker where the MongoDB host is, represented by the name of our other MongoDB container. Take a look at the [documentation](https://github.com/telefonicaid/fiware-orion) for other command-line options.

Orion will be running on [multi-tenant](https://fiware-orion.readthedocs.io/en/master/user/multitenancy/index.html) mode.
   
