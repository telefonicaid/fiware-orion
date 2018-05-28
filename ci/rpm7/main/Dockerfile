FROM fiware/orion-ci:rpm7-env

ADD build /usr/local/bin/
ADD makefile /opt/archive/

WORKDIR /opt/
RUN chmod +x /usr/local/bin/build

CMD ["/usr/local/bin/build", "-h"]
