## Overview
Travis integration is prepared to check pull request with different tests. It uses docker images as clean environment with all build dependencies onboard. 
Such an approach reduces time because it uses pull command instead of compiling dependencies and does not require "clean" operations. Depending on 
current Orion test logic, unit and functional testing should be executed separately, so travis build is divided into stages. 'Optional' test
does not depend on 3rd party tools and does not use binaries of Orion, so it can be executed directly. 

## Supported tests
For now it supports:
* unit test
* function test
* style check
* payload check
* file compliance check

Style, payload and file compliance are combined in one 'optional' test.

## Prerequisites
These variables should be defined:
* enable_unit - to enable unit testing
* enable_functional - to enable functional testing
* enable_optional - to enable optional testing

These variables should be 'true' to enable testing.

## TODO
This CI should be extended to prepare Orion binaries and Docker images.
