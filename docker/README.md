
# How to use this Dockerfile

You can build a docker image based on this Dockerfile. This image will contain only an Orion Context Broker instance, exposing port `1026`. This requires that you have [docker](https://docs.docker.com/installation/) installed on your machine.

If you just want to have an Orion Context Broker running as fast as possible jump to section [The Fastest Way](#the_fastest_way).

If you want to know what and how we are doing things with the container step by step you can  go ahead and read the [Build](#build_the_image) and [Run](#run_the_container) sections.

## The Fastest Way

A Docker Compose file is provided for convenience. You must install [Docker Compose](https://docs.docker.com/compose/install/) for this method to work.

Simply navigate to the docker directory of the fiware-orion code (if you have downloaded it) and run

	docker-compose up

If you haven't or you don't want to download the whole thing, you can download the file called `docker-compose.yaml` in a directory of your choice and run the aforementioned command. It will work just the same.

## Build the image

This is an alternative approach than the one presented in section [The Fastest Way](#the_fastest_way). You do not need to go through these steps if you have used docker-compose.

You only need to do this once in your system:

	docker build -t orion .

The parameter `-t orion` gives the image a name. This name could be anything, or even include an organization like `-t org/fiware-orion`. This name is later used to run the container based on the image.

If you want to know more about images and the building process you can find it in [Docker's documentation](https://docs.docker.com/userguide/dockerimages/).
    
## Run the container

The following line will run the container exposing port `1026`, give it a name -in this case `orion1`, link it to a mongodb docker, and present a bash prompt.

	  docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 orion -dbhost mongodb

As a result of this command, there is a context broker listening on port 1026 on localhost. Try to see if it works now with

	curl localhost:1026/version

A few points to consider:

* The name `orion1` can be anything and doesn't have to be related to the name given to the docker image in the previous section.
* `--link mongodb:mongodb` assumes there is a docker container running a MongoDB image in your system, whose name is `mongodb`. In case you need one type `docker run --name mongodb -d mongo`.
* In `-p 1026:1026` the first value represents the port to listen in on localhost. If you wanted to run a second context broker on your machine you should change this value to something else, for example `-p 1027:1026`.
* Anything after the name of the container image (in this case `orion`) is interpreted as a parameter for the Orion Context Broker. In this case we are telling the broker where the MongoDB host is, represented by the name of our other MongoDB container. Take a look at the [documentation](../.../.../doc/admin/cli.md) for other command-line options.

