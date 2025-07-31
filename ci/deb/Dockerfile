FROM debian:12.10-slim

RUN apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -yq openjdk-17-jre-headless

ADD build.sh /opt/bin/
ADD build-dep.sh /opt/bin/
ADD makefile /opt/archive/
ADD fuse_gtest_files.py.patch /opt/archive/

RUN ln -s /opt/bin/build.sh /usr/local/bin/build \
&& /opt/bin/build-dep.sh

WORKDIR /opt/
ENV PATH="${PATH}:/opt/kafka/bin"

CMD ["/usr/local/bin/build", "-h"]
