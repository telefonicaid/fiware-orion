## Overview
Travis is enabled in this repository so each pull request is checked before being allowed to merge.
The system is based on `fiware/orion-ci:deps-centos` which is built from master branch each time a new PR lands in master,
providing a clean environment with all build dependencies onboard. The Dockerfile used to build this docker is available
in the `ci/` directory.

Note that `fiware/orion-ci:deps-centos` is *not* rebuilt due to changes in the PR branch under test. Thus, if you are developing
a functionality that requires a new library or base system you need to do *first* a PR adding such library or base system
to `ci/build.sh` and/or `ci/build-centos.sh` and/or `Dockerfile`. Once that PR gets merged into master and `fiware/orion-ci:deps-centos` gets rebuild 
(checking progress in Dockerhub at: https://hub.docker.com/r/fiware/orion-ci/builds) your PR branch with the new 
functionality is ready to be tested with travis. 

The Travis checks are divided into stages, which are described in "Supported tests" section.

## Supported tests
Current version of CI supports:
* file compliance check
* payload check
* style check
* unit test
* function test

File compliance, payload and style checks are combined in one 'compliance' test.
