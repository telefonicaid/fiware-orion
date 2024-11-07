# Copyright 2022 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es
#

ARG  IMAGE_NAME=debian
ARG  IMAGE_TAG=12.7-slim
FROM ${IMAGE_NAME}:${IMAGE_TAG}

ARG GITHUB_ACCOUNT=telefonicaid
ARG GITHUB_REPOSITORY=fiware-orion

ARG GIT_NAME
ARG GIT_REV_ORION
ARG CLEAN_DEV_TOOLS

ENV ORION_USER ${ORION_USER:-orion}
ENV GIT_NAME ${GIT_NAME:-telefonicaid}
ENV GIT_REV_ORION ${GIT_REV_ORION:-master}
ENV CLEAN_DEV_TOOLS ${CLEAN_DEV_TOOLS:-1}

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

WORKDIR /opt

RUN --mount=type=secret,id=repo_token,dst=/run/secrets/repo_token \
    # Install security updates
    apt-get -y update && \
    apt-get -y upgrade && \
    # Install dependencies
    apt-get -y install \
      curl \
      cmake \
      libssl-dev \
      git \
      g++ \
      libcurl4-openssl-dev \
      libboost-dev \
      libboost-regex-dev \
      libboost-filesystem-dev \
      libboost-thread-dev \
      uuid-dev \
      libgnutls28-dev \
      libsasl2-dev \
      libgcrypt-dev && \
    # Install libmicrohttpd from source
    cd /opt && \
    curl -kOL https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-1.0.1.tar.gz && \
    tar xvf libmicrohttpd-1.0.1.tar.gz && \
    cd libmicrohttpd-1.0.1 && \
    ./configure --disable-messages --disable-postprocessor --disable-dauth && \
    make && \
    make install && \
    ldconfig && \
    # Install mosquitto from source
    cd /opt && \
    curl -kOL https://mosquitto.org/files/source/mosquitto-2.0.20.tar.gz && \
    tar xvf mosquitto-2.0.20.tar.gz && \
    cd mosquitto-2.0.20 && \
    sed -i 's/WITH_CJSON:=yes/WITH_CJSON:=no/g' config.mk && \
    sed -i 's/WITH_STATIC_LIBRARIES:=no/WITH_STATIC_LIBRARIES:=yes/g' config.mk && \
    sed -i 's/WITH_SHARED_LIBRARIES:=yes/WITH_SHARED_LIBRARIES:=no/g' config.mk && \
    make && \
    make install && \
    ldconfig && \
    # Install mongodb driver from source
    cd /opt && \
    curl -kOL https://github.com/mongodb/mongo-c-driver/releases/download/1.29.0/mongo-c-driver-1.29.0.tar.gz && \
    tar xfvz mongo-c-driver-1.29.0.tar.gz && \
    cd mongo-c-driver-1.29.0 && \
    mkdir cmake-build && \
    cd cmake-build && \
    # Different from ci/deb/build-dep.sh and build from source documentation, we add here also
    # the MONGOC_TEST_USE_CRYPT_SHARED=FALSE. It needs Python and in this tiny image we don't have it
    cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DMONGOC_TEST_USE_CRYPT_SHARED=FALSE ..   && \
    make && \
    make install && \
    # Install rapidjson from source
    cd /opt && \
    curl -kOL https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz && \
    tar xfz v1.1.0.tar.gz && \
    mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include && \
    # Install orion from source
    adduser ${ORION_USER} && \
    cd /opt && \
    git clone https://github.com/${GIT_NAME}/fiware-orion && \
    cd fiware-orion && \
    git checkout ${GIT_REV_ORION} && \
    bash get_cjexl.sh 0.4.0 $(cat /run/secrets/repo_token) && \
    make && \
    make install && \
    # reduce size of installed binaries
    strip /usr/bin/contextBroker && \
    # create needed run path
    mkdir -p /var/run/contextBroker && \
    chown ${ORION_USER} /var/run/contextBroker && \
    cd /opt && \
    apt-get clean && \
    if [ ${CLEAN_DEV_TOOLS} -eq 0 ] ; then exit 0 ; fi && \
    # cleanup sources, dev tools and locales to reduce the final image size
    # FIXME: this could need more tunning. Have a look to old CentOS Docerkile and try to
    # reproduce the same steps
    rm -rf /opt/libmicrohttpd-1.0.1.tar.gz \
           /usr/local/include/microhttpd.h \
           /usr/local/lib/libmicrohttpd.* \
           /opt/libmicrohttpd-1.0.1 \
           /opt/mosquitto-2.0.20.tar.gz \
           /opt/mosquitto-2.0.20 \
           /opt/mongo-c-driver-1.29.0.tar.gz \
           /opt/mongo-c-driver-1.29.0 \
           /usr/local/include/mongo \
           /usr/local/lib/libmongoclient.a \
           /usr/local/lib/libcjexl.a \
           /opt/rapidjson-1.1.0 \
           /opt/v1.1.0.tar.gz \
           /usr/local/include/rapidjson \
           /opt/fiware-orion && \
    # remove the same packages we installed at the beginning to build Orion
    apt-get -y remove --purge \
      cmake \
      libssl-dev \
      git \
      g++ \
      libcurl4-openssl-dev \
      libboost-dev \
      libboost-regex-dev \
      libboost-filesystem-dev \
      libboost-thread-dev \
      uuid-dev \
      libgnutls28-dev \
      libsasl2-dev \
      libgcrypt-dev && \
    apt-get -y autoremove --purge && \
    # The above apt removal erases some dependencies needed by contextBroker. So we reinstall it
    apt-get -y install \
        libicu72 \
        libboost-thread1.74.0 && \
    # Don't need old log files inside docker images
    rm -f /var/log/*log

WORKDIR /

# Note we disable log file as docker container will output by stdout
ENTRYPOINT ["/usr/bin/contextBroker","-fg", "-multiservice", "-ngsiv1Autocast", "-disableFileLog" ]
EXPOSE 1026

LABEL "maintainer"="Orion Team. Telefónica I+D"
LABEL "org.opencontainers.image.authors"="iot_support@tid.es"
LABEL "org.opencontainers.image.documentation"="https://fiware-orion.rtfd.io/"
LABEL "org.opencontainers.image.vendor"="Telefónica Investigación y Desarrollo, S.A.U"
LABEL "org.opencontainers.image.licenses"="AGPL-3.0-only"
LABEL "org.opencontainers.image.title"="Orion Context Broker"
LABEL "org.opencontainers.image.description"="The Orion Context Broker is an implementation of the Publish/Subscribe Context Broker GE, providing an NGSI interface"
LABEL "org.opencontainers.image.source"=https://github.com/${GITHUB_ACCOUNT}/${GITHUB_REPOSITORY}

# Create an anonymous user
RUN sed -i -r "/^(root|nobody)/!d" /etc/passwd /etc/shadow /etc/group \
    && sed -i -r 's#^(.*):[^:]*$#\1:/sbin/nologin#' /etc/passwd
USER nobody

HEALTHCHECK --interval=60s --timeout=5s --start-period=10s \
            # Note we cannot use GET /version for healthchecking as that operation is working even in the case
            # the database is down. In this sense, GET /v2/entities is better, as it touches database
            CMD curl --fail -X GET http://localhost:1026/v2/entities?limit=1 || exit 1
