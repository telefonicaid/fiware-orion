FROM centos:centos7

LABEL maintainer="FIWARE Orion Context Broker Team. Telefónica I+D"

ARG STAGE=deps
ARG REV=master

ENV ORION_USER=orion

WORKDIR /opt

COPY . /usr/local/bin/

RUN build.sh -s ${STAGE} -r ${REV} -c
