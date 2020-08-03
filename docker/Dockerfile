# Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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

ARG  IMAGE_TAG=centos7.7.1908
FROM centos:${IMAGE_TAG}

ARG GITHUB_ACCOUNT=telefonicaid
ARG GITHUB_REPOSITORY=fiware-orion

ARG GIT_NAME
ARG GIT_REV_ORION
ARG CLEAN_DEV_TOOLS

ENV ORION_USER orion
ENV GIT_NAME ${GIT_NAME:-telefonicaid}
ENV GIT_REV_ORION ${GIT_REV_ORION:-master}
ENV CLEAN_DEV_TOOLS ${CLEAN_DEV_TOOLS:-1}

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

WORKDIR /opt

RUN \
    adduser --comment "${ORION_USER}" ${ORION_USER} && \
    # Install dependencies
    yum -y install epel-release && \
    yum -y install \
      boost-devel \
      bzip2 \
      cmake \
      gnutls-devel \
      libgcrypt-devel \
      libcurl-devel \
      openssl-devel \
      libuuid-devel \
      make \
      nc \
      git \
      gcc-c++ \
      scons \
      tar \
      which \
      cyrus-sasl-devel && \
    # Install libmicrohttpd from source
    cd /opt && \
    curl -kOL http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz && \
    tar xvf libmicrohttpd-0.9.70.tar.gz && \
    cd libmicrohttpd-0.9.70 && \
    ./configure --disable-messages --disable-postprocessor --disable-dauth && \
    make && \
    make install && \
    ldconfig && \
    # Install mongodb driver from source
    cd /opt && \
    curl -kOL https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz && \
    tar xfz legacy-1.1.2.tar.gz && \
    cd mongo-cxx-driver-legacy-1.1.2 && \
    scons --use-sasl-client --ssl && \
    scons install --prefix=/usr/local --use-sasl-client --ssl && \
    # Install rapidjson from source
    cd /opt && \
    curl -kOL https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz && \
    tar xfz v1.1.0.tar.gz && \
    mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include && \
    # Install orion from source
    cd /opt && \
    git clone https://github.com/${GIT_NAME}/fiware-orion && \
    cd fiware-orion && \
    git checkout ${GIT_REV_ORION} && \
    make && \
    make install && \
    # reduce size of installed binaries
    strip /usr/bin/contextBroker && \
    # create needed log and run paths
    mkdir -p /var/log/contextBroker && \
    mkdir -p /var/run/contextBroker && \
    chown ${ORION_USER} /var/log/contextBroker && \
    chown ${ORION_USER} /var/run/contextBroker && \
    cd /opt && \
    if [ ${CLEAN_DEV_TOOLS} -eq 0 ] ; then yum clean all && exit 0 ; fi && \
    # cleanup sources, dev tools, locales and yum cache to reduce the final image size
    rm -rf /opt/libmicrohttpd-0.9.70.tar.gz \
           /usr/local/include/microhttpd.h \
           /usr/local/lib/libmicrohttpd.* \
           /opt/libmicrohttpd-0.9.70 \
           /opt/legacy-1.1.2.tar.gz \
           /opt/mongo-cxx-driver-legacy-1.1.2 \
           /usr/local/include/mongo \
           /usr/local/lib/libmongoclient.a \
           /opt/rapidjson-1.1.0 \
           /opt/v1.1.0.tar.gz \
           /usr/local/include/rapidjson \
           /opt/fiware-orion \
           # We don't need to manage Linux account passwords requisites: lenght, mays/mins, etc.
           # This cannot be removed using yum as yum uses hard dependencies and doing so will 
           # uninstall essential packages
           /usr/share/cracklib \
           # We don't need glibc locale data. This cannot be removed using yum as yum uses hard 
           # dependencies and doing so will uninstall essential packages
           /usr/share/i18n /usr/{lib,lib64}/gconv \
           && \
    yum -y erase git perl* rsync \
        cmake libarchive \
        gcc-c++ cloog-ppl cpp gcc glibc-devel glibc-headers \
        kernel-headers libgomp libstdc++-devel mpfr ppl \
        scons boost-devel libcurl-devel gnutls-devel libgcrypt-devel \
        clang llvm llvm-libs \
        CUnit-devel CUnit \
        autoconf automake m4 libidn-devel \
        boost-wave boost-serialization boost-python \
        boost-iostreams boost boost-date-time \
        boost-test boost-graph boost-signals \
        boost-program-options boost-math \
        openssh openssh-clients libedit hwdata dbus-glib fipscheck* *devel sysvinit-tools \
        && \
    # Erase without dependencies of the document formatting system (man). This cannot be removed using yum 
    # as yum uses hard dependencies and doing so will uninstall essential packages
    rpm -qa groff | xargs -r rpm -e --nodeps && \
    # Clean yum data
    yum clean all && rm -rf /var/lib/yum/yumdb && rm -rf /var/lib/yum/history && \
    # Rebuild rpm data files
    rpm -vv --rebuilddb && \
    # Delete unused locales. Only preserve en_US and the locale aliases
    find /usr/share/locale -mindepth 1 -maxdepth 1 ! -name 'en_US' ! -name 'locale.alias' | xargs -r rm -r && \
    bash -c 'localedef --list-archive | grep -v -e "en_US" | xargs localedef --delete-from-archive' && \
    # We use cp instead of mv as to refresh locale changes for ssh connections. We use /bin/cp instead of 
    # cp to avoid any alias substitution, which in some cases has been problematic
    /bin/cp -f /usr/lib/locale/locale-archive /usr/lib/locale/locale-archive.tmpl && \
    build-locale-archive && \
    # Don't need old log files inside docker images
    rm -f /var/log/*log

WORKDIR /
ENTRYPOINT ["/usr/bin/contextBroker","-fg", "-multiservice", "-ngsiv1Autocast" ]
EXPOSE 1026

LABEL "maintainer"="FIWARE IoTAgent Team. Telefónica I+D"
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



