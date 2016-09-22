FROM centos:centos6.6

MAINTAINER FIWARE Orion Context Broker Team. Telefónica I+D

ENV ORION_USER orion
ENV GIT_REV_ORION 1.2.2
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
      gnutls-devel \
      libgcrypt-devel \
      libcurl-devel \
      libuuid-devel \
      make \
      nc \
      git \
      gcc-c++ \
      scons \
      tar \
      which && \
    cd /opt && \
    # Install libmicrohttpd from source
    curl -kOL http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz && \
    tar xvf libmicrohttpd-0.9.48.tar.gz && \
    cd libmicrohttpd-0.9.48 && \
    ./configure --disable-messages --disable-postprocessor --disable-dauth && \
    make && \
    make install && \
    ldconfig && \
    # Install mongodb driver from source
    curl -kOL https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.7.tar.gz && \
    tar xfz legacy-1.0.7.tar.gz && \
    cd mongo-cxx-driver-legacy-1.0.7 && \
    scons install --prefix=/usr/local && \
    # Install rapidjson from source
    curl -kOL https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz && \
    tar xfz v1.0.2.tar.gz && \
    mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include && \
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
    rm -rf /opt/libmicrohttpd-0.9.48.tar.gz \
           /usr/local/include/microhttpd.h \
           /usr/local/lib/libmicrohttpd.* \
           /opt/libmicrohttpd-0.9.48 \
           /opt/legacy-1.0.7.tar.gz \
           /opt/mongo-cxx-driver-legacy-1.0.7 \
           /usr/local/include/mongo \
           /usr/local/lib/libmongoclient.a \
           /opt/rapidjson-1.0.2 \
           /opt/v1.0.2.tar.gz \
           /usr/local/include/rapidjson \
           /opt/fiware-orion \
           && \
    yum -y erase git perl rsync \
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
