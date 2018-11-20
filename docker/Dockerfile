FROM debian:stretch-slim

ARG USER
ARG PASSWORD
ARG REV

ADD build.sh /usr/bin/build

RUN build ${USER} ${PASSWORD} ${REV}

ENTRYPOINT ["orionld","-fg", "-multiservice", "-ngsiv1Autocast" ]

EXPOSE 1026
