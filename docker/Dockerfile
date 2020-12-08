FROM fiware/orion-ld-base AS build-stage

ARG PATH_TO_SRC='opt/orion/'
ARG DEBUG='false'

RUN cp -a /usr/local/lib/. /usr/lib/x86_64-linux-gnu/
RUN rm -r /usr/local/lib/

COPY . ${PATH_TO_SRC}

WORKDIR ${PATH_TO_SRC}

RUN  if [ "$DEBUG" = 'true' ] ; then make debug install ; else make release install ; fi

RUN strip /usr/bin/orionld

FROM debian:stretch-slim

RUN mkdir -p /var/{log,run}/orionld

COPY --from=build-stage /lib/x86_64-linux-gnu/ /lib/x86_64-linux-gnu/
COPY --from=build-stage /usr/lib/x86_64-linux-gnu/ /usr/lib/x86_64-linux-gnu/
COPY --from=build-stage /usr/bin/orionld /usr/bin/orionld

ENTRYPOINT ["orionld", "-fg", "-multiservice", "-ngsiv1Autocast"]

EXPOSE 1026