## Overview
Travis is enabled in this repository so each pull request is checked before being allowed to merge.
The system is based on fiware/orion-ci:rpm7  which is built from master branch each time a new PR lands in master,
providing a clean environment with all build dependencies onboard. The Dockerfile used to build this docker is available
in the ci/rpm7 directory.

The Travis checks are divided into stages, which are described in "Supported tests" section.

## Supported tests
Current version of CI supports:
* file compliance check
* payload check
* style check
* unit test
* function test

File compliance, payload and style checks are combined in one 'compliance' test.

## Changes in tests
Special function named _fix_tests is created in build.sh to reduce the time that is spent on function tests because of travis time limits.
This tests changed:
* 3000_allow_creation_transient_entities
