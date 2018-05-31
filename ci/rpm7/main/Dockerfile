FROM fiware/orion-ci:rpm7-env

ADD build.sh /opt/bin/
ADD makefile /opt/archive/

RUN  ln -s /opt/bin/build.sh /usr/local/bin/build

WORKDIR /opt/

CMD ["/usr/local/bin/build", "-h"]
