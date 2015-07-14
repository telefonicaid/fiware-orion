FROM centos:centos6
MAINTAINER FIWARE Orion Context Broker Team. Telefónica I+D

# Setup FIWARE repo
RUN echo -e "[testbed-fiware]\n\
name=Fiware Repository\n\
baseurl=http://repositories.testbed.fiware.org/repo/rpm/x86_64/\n\
gpgcheck=0\n\
enabled=1\n" > /etc/yum.repos.d/testbed-fi-ware.repo

# Install Orion
RUN yum -y install contextBroker

EXPOSE 1026
ENTRYPOINT ["/usr/bin/contextBroker", "-fg", "-t 0-255"]