#!/bin/bash

set -e

secret=$1
type=$2

if [ "$type" == "release" ]
then
  type="1"
fi

docker run -e BROKER_RELEASE=$type -v $(pwd):/opt/orion --workdir=/opt/orion fiware/orion-ci:rpm8 make rpm

for file in "$(pwd)/rpm/RPMS/x86_64"/*
do
  filename=$(basename $file)
  if [ "$type" == "nightly" ]
  then
      # to make the name of the .rpm independent of the version name (i.e. contextBroker-nightly.x86_64.rpm),
      # avoiding obsolete packages to remain in the yum version when version number steps
      filename=$(echo $filename | sed 's/[0-9]+.[0-9]+.[0-9]+_next-//g')
  fi
  echo "Uploading $filename"
  curl -v -f -u telefonica-github:$secret --upload-file $file https://nexus.lab.fiware.org/repository/el/8/x86_64/$type/$filename
done
