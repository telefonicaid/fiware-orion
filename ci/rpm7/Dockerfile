FROM centos:centos7.9.2009

ADD build.sh /opt/bin/
ADD build-dep.sh /opt/bin/
ADD makefile /opt/archive/

RUN ln -s /opt/bin/build.sh /usr/local/bin/build \
&& /opt/bin/build-dep.sh

WORKDIR /opt/

CMD ["/usr/local/bin/build", "-h"]
