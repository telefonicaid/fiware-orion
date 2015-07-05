#!/bin/sh

source $HOME/.bashrc
docker --tlsverify=false run -p 1026:1026 --name orionDev --ipv6 --link mongoOrion:mongoOrion -v /Users/jmcf/work/develop/fiware-orion:/root/src -t -i orion-dev:latest /bin/bash
