# How to use Orion Context Broker on Raspberry Pi.

You can run Orion Context Broker very easily using docker on Raspberry Pi. 

The [Raspberry Pi](https://www.raspberrypi.org/) is a low cost, credit-card sized computer.
It is an ARM based device and requires binaries compiled for the ARM architecture. 
To build and run the docker image of Orion, the 64 bit Linux and docker for the ARM architecture are installed on Raspberry Pi.
If you want to build and run Orion directly on an operating system installed on a Raspberry Pi, See the
[documentation](../doc/manuals/admin/build_source.md#ubuntu-1804-lts) on how to do this.

## Prerequisites

### Hardware

The target devices are Raspberry Pi 3 and 4 which support the 64 bit ARM architecture (aarch64).

### Linux OS

As of now, there are not many options to use the 64 bit Linux on Raspberry Pi. 
To use Ubuntu 20.04 LTS is better. You can get the OS image and find the install instruction
[here](https://ubuntu.com/download/raspberry-pi).

### Docker

You can install Docker on Ubuntu by following the commands as shown:

```
sudo cp -p /etc/apt/sources.list{,.bak}
sudo apt-get update
sudo apt-get install -y \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg-agent \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
   "deb [arch=arm64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get install -y docker-ce
```

The details to install Docker are [here](https://docs.docker.com/install/linux/docker-ce/ubuntu/).

### Docker compose

The Docker Compose binary for aarch64 is not provided. It is necessary to build it from its source code.
You can install the docker compose version 1.27.4 by running the commands as shown:

```
git clone -b 1.27.4 https://github.com/docker/compose.git
cd compose/
sudo ./script/build/linux
sudo cp dist/docker-compose-Linux-aarch64 /usr/local/bin/docker-compose
```

## How to build Orion

To build the docker images of Orion, Clone the Orion repository and run the `docker build` command.

```
git clone https://github.com/telefonicaid/fiware-orion.git
cd fiware-orion/docker
docker build -t orion .
```

## How to run Orion

Create the `docker-compose.yml` file which has the contents as shown:

```
version: "3"

services:
  orion:
    image: orion:latest
    ports:
      - "1026:1026"
    depends_on:
      - mongo
    command: -dbhost mongo

  mongo:
    image: mongo:4.4
    command: --nojournal
```

To start up Orion, you run `docker-compose up -d`. Run the `curl localhost:1026/version` command
to check if Orion has started up.
