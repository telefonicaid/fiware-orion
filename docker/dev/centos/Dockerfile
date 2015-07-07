FROM centos:centos6

MAINTAINER FIWARE Orion Context Broker Team. Telefónica I+D

WORKDIR /root

# Install dependencies for building
RUN \
	yum -y install git make cmake gcc-c++ scons libmicrohttpd-devel boost-devel libcurl-devel tar wget bzip2

# Install MongoDB C++ Driver
RUN \
    curl -kOL https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.2.tar.gz && \
    tar xfz legacy-1.0.2.tar.gz && \
    cd mongo-cxx-driver-legacy-1.0.2 && \
    scons install --prefix=/usr/local && \
    cd .. && \
    rm legacy-1.0.2.tar.gz && \
    rm -rf mongo-cxx-driver-legacy-1.0.2

# Testing tools
RUN \
    wget http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 && \
    tar xfvj gmock-1.5.0.tar.bz2 && \
    cd gmock-1.5.0 && \
    ./configure && \
    make && \
    make install && \
    ldconfig && \
    cd .. && \
    rm gmock-1.5.0.tar.bz2 && \
    rm -rf gmock-1.5.0

