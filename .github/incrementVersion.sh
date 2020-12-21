#!/bin/bash

version=$LATEST_RELEASE

a=( ${version//./ } )
((a[1]++))
a[2]=0
echo "${a[0]}.${a[1]}.${a[2]}"
version=$(echo "${a[0]}.${a[1]}.${a[2]}")
echo "::set-output name=version::${version}"

