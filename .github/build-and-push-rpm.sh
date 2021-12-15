#!/bin/bash

set -e

docker run -e BROKER_RELEASE=nightly -v $(pwd):/opt/orion --workdir=/opt/orion fiware/orion-ci:rpm8 make rpm

for file in "$(pwd)/rpm/RPMS/x86_64"/*
do
  filename=$(basename $file)
  echo "Processing $filename file..."
  curl -v -f -u telefonica-github:$1 --upload-file $file https://nexus.lab.fiware.org/repository/el/8/x86_64/nightly/$filename
done