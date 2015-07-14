
# How to use this Dockerfile

You can build a docker image based on this Dockerfile. This image will contain only an Orion Context Broker instance, exposing port `1026`.

## Build the image

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
