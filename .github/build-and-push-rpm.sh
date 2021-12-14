#!/bin/bash

set -e

docker run -e BROKER_RELEASE=nightly -v $(pwd):/opt/orion --workdir=/opt/orion fiware/orion-ci:rpm8 make rpm

file_list=$(ls $(pwd)/rpm/RPMS/x86_64/)

for(String file : file_list.split("\\r?\\n")){
  curl -v -f -u telefonica-github:${{ secrets.NEXUS_PASSWORD }} --upload-file $(pwd)/rpm/RPMS/x86_64/${file} https://nexus.lab.fiware.org/repository/el/8/x86_64/nightly/${file}
}