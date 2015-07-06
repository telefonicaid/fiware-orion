# Develop Orion Context Broker Ubuntu-Latest
FROM ubuntu:latest

MAINTAINER FIWARE Orion Context Broker Team. Telefónica I+D

RUN \
    apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    curl \
    git \
    libboost-dev \
    libboost-regex-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libcurl4-gnutls-dev \
    libmicrohttpd-dev \
    libssl-dev libcrypto++-dev \
    libxml2-utils \
    make \
    mongodb-clients \
    netcat \
    scons  \
    wget && \
    cd /root && \
    wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.2.tar.gz && \
    tar xfvz legacy-1.0.2.tar.gz && \
    cd mongo-cxx-driver-legacy-1.0.2 && \
    scons &&  scons install --prefix=/usr/local && \
    cd .. && \
    rm -R mongo-cxx-driver-legacy-1.0.2 && \
    wget http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 && \
    tar xfvj gmock-1.5.0.tar.bz2 && \
    cd gmock-1.5.0 && \
    ./configure && \
    make && make install && \
    cd .. && \
    rm -R gmock-1.5.0 && \
    wget https://bootstrap.pypa.io/get-pip.py && \
    python get-pip.py && \
    pip install Flask && \
    echo cd /root/src >> /root/.bashrc

EXPOSE 1026