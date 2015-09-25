FROM centos:centos6.6

MAINTAINER FIWARE Orion Context Broker Team. Telefónica I+D

ENV ORION_USER orion
ENV GIT_REV_ORION develop
ENV CLEAN_DEV_TOOLS 1

RUN adduser --comment "${ORION_USER}" ${ORION_USER}

WORKDIR /opt

RUN \
    # Install dependencies
    yum -y install epel-release && \
    yum -y install \
      boost-devel \
      bzip2 \
      cmake \
      epel-release \
      libcurl-devel \
      libmicrohttpd-devel \
      make \
      nc \
      git \
      gcc-c++ \
      scons \
      tar \
      which && \
    # Install mongodb driver from source
    curl -kOL https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.2.tar.gz && \
    tar xfz legacy-1.0.2.tar.gz && \
    cd mongo-cxx-driver-legacy-1.0.2 && \
    scons install --prefix=/usr/local && \
    cd /opt && \
    # Install rapidjson from source
    curl -kOL https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz && \
    tar xfz v1.0.2.tar.gz && \
    mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include && \
    # Install Google Test/Mock from source
    curl -kOL http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 && \
    tar xfj gmock-1.5.0.tar.bz2 && \
    cd gmock-1.5.0 && \
    ./configure && \
    make && \
    make install && \
    ldconfig && \
    cd /opt && \
    # Install orion from source
    git clone https://github.com/telefonicaid/fiware-orion && \
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
    rm -rf /opt/legacy-1.0.2.tar.gz \
           /opt/mongo-cxx-driver-legacy-1.0.2 \
           /usr/local/include/mongo \
           /usr/local/lib/libmongoclient.a \
           /opt/rapidjson-1.0.2 \
           /opt/v1.0.2.tar.gz \
           /usr/local/include/rapidjson \
           /opt/gmock-1.5.0.tar.bz2 \
           /opt/gmock-1.5.0 \
           /usr/local/bin/gmock* \
           /usr/local/bin/gtest* \
           /usr/local/include/gmock \
           /usr/local/include/gtest \
           /usr/local/lib/libgmock* \
           /usr/local/lib/libgtest* \
           /usr/local/share/aclocal/gtest* \
           /opt/fiware-orion \
           && \
    yum -y erase git perl rsync \
        cmake libarchive \
        gcc-c++ cloog-ppl cpp gcc glibc-devel glibc-headers \
        kernel-headers libgomp libstdc++-devel mpfr ppl \
        scons libmicrohttpd-devel boost-devel libcurl-devel \
        clang llvm llvm-libs \
        CUnit-devel CUnit \
        autoconf automake m4 libidn-devel \
        boost-wave boost-serialization boost-python \
        boost-iostreams boost boost-date-time \
        boost-test boost-graph boost-signals \
        boost-program-options boost-math \
        openssh openssh-clients libedit \
        && \
    yum clean all && \
    bash -c 'find /usr/share/locale -maxdepth 1 -mindepth 1 -type d | grep -v -e "en_US" | xargs rm -rfv' && \
    bash -c 'localedef --list-archive | grep -v -e "en_US" | xargs localedef --delete-from-archive' && \
    mv -f /usr/lib/locale/locale-archive /usr/lib/locale/locale-archive.tmpl && \
    build-locale-archive

WORKDIR /
ENTRYPOINT ["/usr/bin/contextBroker","-fg", "-multiservice"]
EXPOSE 1026
