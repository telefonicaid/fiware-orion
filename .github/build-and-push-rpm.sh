#!/bin/bash

set -e

docker run -e BROKER_RELEASE=nightly -v $(pwd):/opt/orion --workdir=/opt/orion fiware/orion-ci:rpm8 make rpm

for file in "$(pwd)/rpm/RPMS/x86_64"/*
do
  echo "Processing $file file..."
  curl -v -f -u telefonica-github:$1 --upload-file $(pwd)/rpm/RPMS/x86_64/$file https://nexus.lab.fiware.org/repository/el/8/x86_64/nightly/$file
done