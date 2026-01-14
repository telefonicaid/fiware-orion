## Overview
GitHub Actions is enabled in this repository so each pull request is checked before being allowed to merge.
The system is based on `telefonicaiot/fiware-orion:ci` which is built from master branch each time a new PR lands in master,
providing a clean environment with all build dependencies onboard. The Dockerfile used to build this docker is available
in the `ci/deb` directory.

Note that `telefonicaiot/fiware-orion:ci` is *not* rebuilt due to changes in the PR branch under test. Thus, if you are developing
a functionality that requires a new library or base system you need to do *first* a PR adding such library or base system
to `ci/deb/build-dep.sh` and/or `Dockerfile`. Once that PR gets merged into master and `telefonicaiot/fiware-orion:ci` gets rebuild
(checking progress in Dockerhub at: https://hub.docker.com/r/telefonicaiot/fiware-orion/builds) your PR branch with the new 
functionality is ready to be tested with GitHub Actions.

The GitHub Actions checks are divided into stages, which are described in "Supported tests" section.

## Supported tests
Current version of CI supports:
* file compliance check
* payload check
* style check
* unit test
* function test

File compliance, payload and style checks are combined in one 'compliance' test.

# How to run all test locally

You can run the functional tests with:

```
docker compose -f ci/deb/docker-compose-ci.yml -f ci/deb/docker-compose-ci.functional.yml --project-directory . up
```

That would set up all the needed services (MongoDB, MQTT broker, Kafka broker, etc.) with the `docker-compose-ci.yml` and run the tests with `docker-compose-ci.functional.yml` (which uses the `telefonicaiot/fiware-orion:ci` for that).

Alternatively you can run the valgrind tests with:

```
docker compose -f ci/deb/docker-compose-ci.yml -f ci/deb/docker-compose-ci.valgrind.yml --project-directory . up
```

For alternative ways of using the image, read next section.

# How to use the image locally

Sometime you need to run the CI image locally (for instance, to debug problems found in GitHub Action jobs). In that case,
the following cheatsheet can be useful:

First, you have to set up required services running in a terminal (stop local instances of the service before or you will get conflicts)

```
cd /path/to/fiware-orion
docker compose -f ci/deb/docker-compose-ci.yml --project-directory . up
```

When you end your testing you can stop the services in the terminal typically with Ctrl+C.

Next, to download the image (alternativelly you can [build it locally](#how-to-build-the-image-locally))

```
docker pull telefonicaiot/fiware-orion:ci
```

To run the image in the same way that GitHub Actions does, for instance:

```
cd /path/to/fiware-orion
docker run --network host --rm -e CB_NO_CACHE=ON -e FT_FROM_IX=1001 -v $(pwd):/opt/fiware-orion telefonicaiot/fiware-orion:ci build -miqts functional
```

To run the image using an interactive bash on it

```
cd /path/to/fiware-orion
docker run --network host -ti -v $(pwd):/opt/fiware-orion telefonicaiot/fiware-orion:ci bash
```

Once have a bash shell, you can do the same execution:

```
root@debian:/opt# CB_NO_CACHE=ON FT_FROM_IX=1001 build -miqts functional
```

Alternatively, you can run the `testHarness.sh` script directly (for instance, to execute a single test):

```
root@debian:/opt# . /opt/ft_env/bin/activate
(ft_env) root@debian:/opt# cd /opt/fiware-orion/test/functionalTest/
(ft_env) root@debian:/opt/fiware-orion/test/functionalTest# ./testHarness.sh cases/3541_subscription_max_fails_limit/mqtt_subscription_without_maxfailslimit_and_failscounter.test
```

**NOTE:** the above procedure makes that Orion is compiled using root user. It is advisable to do a recursive owner
change after ending your debug session. Something like this:

```
sudo chown -R fermin:fermin /path/to/fiware-orion
```

**NOTE2:** the `build` script will make changes in your `makefile` file. Do a `git checkout makefile` after your debug session
to recover it.

# How to build the image locally

In some case, we may want to build the image locally instead of pull the one at Dockerhub. The following commands could help.

First, it's a good idea to remove any existing `telefonicaiot/fiware-orion:ci` image downloaed from Dockerhub:

```
docker rmi <id of the image>
```

Next, to build the container:

```
cd /path/to/fiware-orion/ci/deb
docker build -t telefonicaiot/fiware-orion:ci .
```
