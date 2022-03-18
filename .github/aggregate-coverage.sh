#!/bin/bash

set -e

cd test-coverage

covFilesList=""

for FILENAME in *.info; do
  covFilesList+="-a "+ FILENAME;
done

lcov ${covFilesList} -o ../lcov.info