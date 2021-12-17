#!/bin/bash

set -e

secret=$1
type=$2

docker run -e BROKER_RELEASE=$type -v $(pwd):/opt/orion --workdir=/opt/orion fiware/orion-ci:rpm8 make rpm

for file in "$(pwd)/rpm/RPMS/x86_64"/*
do
  filename=$(basename $file)
  echo "Uploading $filename"
  curl -v -f -u telefonica-github:$secret --upload-file $file https://nexus.lab.fiware.org/repository/el/8/x86_64/$type/$filename
done