#!/bin/sh

docker --tlsverify=false run -d --name mongoOrion mongo:2.6.10 
