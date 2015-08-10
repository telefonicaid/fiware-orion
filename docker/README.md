
# How to use this Dockerfile

You can build a docker image based on this Dockerfile. This image will contain only an Orion Context Broker instance, exposing port `1026`. This requires that you have [docker](https://docs.docker.com/installation/) installed on your machine.

If you just want to have an Orion Context Broker running as quickly as possible jump to section *The Fastest Way*.

If you want to know what is behind the scenes of our container you can go ahead and read the build and run sections.

## The Fastest Way

Docker Compose allows you to link an Orion Context Broker container to a MongoDB container in a few minutes. You must install [Docker Compose](https://docs.docker.com/compose/install/) for this method to work.

### Run a container from an image you just built

If you have downloaded the [Orion's code](https://github.com/telefonicaid/fiware-orion/) simply navigate to the docker directory and run

	sudo docker-compose up

You are using this [docker-compose.yml](docker-compose.yml) file. This will instruct to build a new image for Orion and run it linking it to a MongoDB container.

### Run a container pulling an image from the cloud (recommended)

If you do not have or want to download the Orion repository, you can create a file called `docker-compose.yml` in a directory of your choice and fill it with the following content

	orion:
	  image: fiware/orion
	  links:
	    - mongo
	  ports:
	    - "1026:1026"
	  command: -dbhost mongo

	mongo:
	  image: mongo:2.6

Then run

	sudo docker-compose up

This way is equivalent to the previous one, except that it pulls the image from the Docker Registry instead of building your own. Keep in mind though that everything is run locally. 

> **Warning**
> Everything you do with Orion Context Broker when dockerized is non-persistent. *You will lose all your data* if you turn off the MongoDB container. This happens with either this method or the manual one.
> If you want to prevent this from happening take a look at [this link](https://registry.hub.docker.com/_/mongo/) in section *Where to Store Data* of the MongoDB docker documentation. In it you will find instructions and ideas on how to make your MongoDB data persistent.

## Build the image

This is an alternative approach to the one presented in the previous section. You do not need to go through these steps if you have used docker-compose. The end result will be the same, but this way you have a bit more of control of what's happening.

You only need to do this once in your system:

	sudo docker build -t orion .

> **Note**
> If you do not want to have to use `sudo` in this or in the next section follow [these instructions](http://askubuntu.com/questions/477551/how-can-i-use-docker-without-sudo).


The parameter `-t orion` gives the image a name. This name could be anything, or even include an organization like `-t org/fiware-orion`. This name is later used to run the container based on the image.

If you want to know more about images and the building process you can find it in [Docker's documentation](https://docs.docker.com/userguide/dockerimages/).
    
### Run the container

The following line will run the container exposing port `1026`, give it a name -in this case `orion1`, link it to a MongoDB docker, and present a bash prompt. This uses the image built in the previous section.

	  sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 orion -dbhost mongodb

If you did not build the image yourself and want to use the one on Docker Hub use the following command:

	  sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 fiware/orion -dbhost mongodb

> **Note**
> Keep in mind that if you use this last command you get access to the tags and specific versions of Orion. For example, you may use `fiware/orion:0.22` instead of `fiware/orion` in the above command if you need that particular version. If you do not specify a version you are pulling from `latest` by default.

As a result of this command, there is a context broker listening on port 1026 on localhost. Try to see if it works now with

	curl localhost:1026/version



A few points to consider:

* The name `orion1` can be anything and doesn't have to be related to the name given to the docker image in the previous section.
* `--link mongodb:mongodb` assumes there is a docker container running a MongoDB image in your system, whose name is `mongodb`. In case you need one type `sudo docker run --name mongodb -d mongo:2.6`.
* In `-p 1026:1026` the first value represents the port to listen in on localhost. If you wanted to run a second context broker on your machine you should change this value to something else, for example `-p 1027:1026`.
* Anything after the name of the container image (in this case `orion`) is interpreted as a parameter for the Orion Context Broker. In this case we are telling the broker where the MongoDB host is, represented by the name of our other MongoDB container. Take a look at the [documentation](https://github.com/telefonicaid/fiware-orion) for other command-line options.

