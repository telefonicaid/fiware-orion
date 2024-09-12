# How to use Orion Context Broker on Raspberry Pi.

You can run Orion Context Broker very easily using docker on Raspberry Pi. 

The [Raspberry Pi](https://www.raspberrypi.org/) is a low cost, credit-card sized computer.
It is an ARM based device and requires binaries compiled for the ARM architecture. 
To build and run the docker image of Orion, the 64 bit Linux and docker for the ARM architecture are installed on Raspberry Pi.

## Prerequisites

### Hardware

The target devices are Raspberry Pi 3 and 4 which support the 64 bit ARM architecture (aarch64).

### Linux OS

To use Raspberry Pi OS Bookworm 12 is better. You can get the OS image and find the install instruction
[here](https://www.raspberrypi.com/software/).

### Docker

You install Docker and Docker compose plugin on Raspberry Pi OS. The details to install Docker are
[here](https://docs.docker.com/engine/install/raspberry-pi-os/).

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
    command: -dbURI mongodb://mongo

  mongo:
    image: mongo:6.0
    command: --nojournal
```

To start up Orion, you run `docker-compose up -d`. Run the `curl localhost:1026/version` command
to check if Orion has started up.
