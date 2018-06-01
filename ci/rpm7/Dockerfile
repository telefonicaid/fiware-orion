FROM centos:centos7

ADD mongodb.repo /etc/yum.repos.d/
ADD build.sh /opt/bin/
ADD makefile /opt/archive/

RUN ln -s /opt/bin/build.sh /usr/local/bin/build

RUN yum -y install epel-release \
&&  yum -y install \
  bc \
  boost-devel \
  bzip2 \
  cmake \
  yum \
  gcc-c++ \
  git \
  gnutls-devel \
  lcov \
  libcurl-devel \
  libgcrypt-devel \
  libuuid-devel \
  make \
  mongodb-org \
  mongodb-org-shell \
  nc \
  openssl-devel \
  pyOpenSSL \
  python \
  python-flask \
  psmisc \
  rpm-build \
  scons \
  tar \
  valgrind \
  which

RUN curl -L https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz | tar xzC /opt/ \
  &&  cd /opt/mongo-cxx-driver-legacy-1.1.2 \
  &&  scons --disable-warnings-as-errors \
  &&  scons install --disable-warnings-as-errors --prefix=/usr/local \
  &&  rm -Rf /opt/mongo-cxx-driver-legacy-1.1.2

RUN curl -L https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz | tar xzC /opt/ \
  &&  mv /opt/rapidjson-1.0.2/include/rapidjson/ /usr/local/include \
  &&  rm -Rf /opt/rapidjson-1.0.2

RUN curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz | tar xzC /opt/ \
  &&  cd /opt/libmicrohttpd-0.9.48  \
  &&  ./configure --disable-messages --disable-postprocessor --disable-dauth  \
  &&  make \
  &&  make install \
  &&  ldconfig \
  &&  rm -Rf /opt/libmicrohttpd-0.9.48

RUN curl -L ftp://repositories.lab.fiware.org/gmock-1.5.0.tar.bz2 | tar xjC /opt/ \
  &&  cd /opt/gmock-1.5.0 \
  &&  ./configure \
  &&  make \
  &&  make install \
  &&  ldconfig \
  &&  rm -Rf /opt/gmock-1.5.0

WORKDIR /opt/

CMD ["/usr/local/bin/build", "-h"]
