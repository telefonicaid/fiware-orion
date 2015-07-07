# How to use this Dockerfile

This image is intended for **development** of Orion Context Broker. This will create an environment on which Orion can compile and be run based on CentOS 6.

If you are only interested in **running** Orion Context Broker in a container this image is not what you are looking for. 

## Build the image

You only need to do this once.

    docker build -t orion-dev:centos6 .
    
## Run the container

This assumes that there is another docker running with name `mongodb`. If you don't, type this:

    docker run -d --name mongodb mongo

The following line will run the container exposing port 1026, give it a name -in this case `orionDev`-, link it to the mongodb docker just created, and present a bash prompt. Keep in mind that /path/to/orion/source is the path to your Orion Context Broker code in the host, not the container. 

    docker run -p 1026:1026 --name orionDev -v /path/to/orion/source:/root/src --link mongodb:mongodb -t -i orion-dev:centos6 /bin/bash

## Compile Orion

Now you are in the docker container running a shell. Before you compile you must disable proxyCoap (not supported in this docker image):

    sed -i '/proxyCoap/s/^/#/' /root/src/CMakeLists.txt    

Now you can compile Orion Context Broker:

    cd /root/src
    make
    make install

## Run Orion

To run orion you can do it interactively:

    contextBroker -fg -t 0-255 --dbhost mongodb

Or on the background
    
    contextBroker -t 0-255 --dbhost mongodb

Take a look a the [documentation](../.../.../doc/admin/cli.md) for all command-line options.

