# How to use Orion Context Broker on Raspberry Pi.

You can run Orion Context Broker very easily using docker on Raspberry Pi. 

The [Raspberry Pi](https://www.raspberrypi.org/) is a low cost, credit-card sized computer.
It is an ARM based device and requires binaries compiled for the ARM architecture. 
To build and run the docker image of Orion, the 64 bit Linux and docker for the ARM architecture are installed on Raspberry Pi.

## Prerequisites

### Hardware

The target hardwares are Raspberry Pi 3 and 4 which support the 64bit ARM architecture (aarch64).

### Linux OS

As of now, there are not many options to use the 64 bit Linux on Raspberry Pi. 
To use Ubuntu 18.04 LTS or 19.10 is better. You can get the OS image and find the install instruction 
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

As the docker binary for Ubuntu 19.10 (Eoan) is not provided, it is necessary to install the docker binary
for Ubuntu 18.04 LTS (Bionic). You should replace `$(lsb_release -cs)` with `bionic` in `add-apt-repository` command.

The details to install Docker are [here](https://docs.docker.com/install/linux/docker-ce/ubuntu/).

### Dokcer compose

The Docker Compose binary for aarch64 is not provided. It is necessary to build it from its source code.
You can install the docker compose versin 1.25.1 by running the commands as shown:

```
git clone -b 1.25.1 https://github.com/docker/compose.git
cd compose/
sed -i -e "43i'setuptools<45.0.0'," setup.py
sudo ./script/build/linux
sudo cp dist/docker-compose-Linux-aarch64 /usr/local/bin/docker-compose
```

Note: A patch to avoid a build error is added by using the sed command.

## How to build Orion

To build the docker images of Orion, Clone the Orion repository and run the `docker build` command.

```
git clone https://github.com/telefonicaid/fiware-orion.git
cd fiware-orion/docker
docker build --build-arg=centos7 -t orion .
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
    image: mongo:3.6
    command: --nojournal
```

To start up Orion, you run `docker-compose up -d`. Run the `curl localhost:1026/version` command
to check if Orion has started up.
