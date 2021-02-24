#FIXME: by the moment some tokens use 'rpm7' but we will fix that at some moment...
FROM centos:centos8.3.2011

ADD mongodb.repo /etc/yum.repos.d/
ADD build.sh /opt/bin/
ADD build-dep.sh /opt/bin/
ADD makefile /opt/archive/

RUN ln -s /opt/bin/build.sh /usr/local/bin/build \
&& /opt/bin/build-dep.sh

WORKDIR /opt/

CMD ["/usr/local/bin/build", "-h"]
