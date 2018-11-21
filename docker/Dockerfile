FROM debian:stretch-slim

ARG TOKEN
ARG REV

ADD build.sh /usr/bin/build

RUN build ${TOKEN} ${REV}

ENTRYPOINT ["orionld","-fg", "-multiservice", "-ngsiv1Autocast" ]

EXPOSE 1026
