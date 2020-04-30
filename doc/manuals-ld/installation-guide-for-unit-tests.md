# Installation Guide for the Orion Unit Test Suite

Unit tests in Orion-LD are implemented with `gtest` and `gmock` from Google, so those two libraries need to be
installed. The version 1.5 is used and it must be downloaded and compiled.

```bash
cd /opt
sudo mkdir gmock
sudo chown $USER:$GROUP gmock
cd gmock
wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
tar xfvj gmock-1.5.0.tar.bz2
cd gmock-1.5.0
./configure
make
sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
```

After this, the broker must be compiled, as the unit test is an executable that links to the broker code:

```bash
cd ~/git/context.Orion-LD
make unit_test
```
The make target `unit_test` both compiles the unit test executable and executes it.
If you want to execute it by hand, without recompilation:

```bash
./BUILD_UNITTEST/test/unittests/unitTest
```

That executes the entire unit test suite.

Especially interesting might be to run the suite inside `gdb`. In case of a crash inside the test suite, that's the way to go:

```bash
# gdb should be installed by default, but if not (if on Ubuntu/Debian):
sudo aptitude install gdb

gdb ./BUILD_UNITTEST/test/unittests/unitTest
```

Normally, what you'd want to do would be to execute a single test case, as the normal reason to run the unit test suite by hand is that some test case fails.
Or, that you are implementing a new unit test case and want to run only that new case.

Test cases have names based on 'family' and 'detail', e.g. `RestService.payloadParse`.

To run only the test case `RestService.payloadParse`:

```bash
./BUILD_UNITTEST/test/unittests/unitTest --gtest_filter=RestService.payloadParse
```

To run an "entire family" of test cases, e,g, the "RestService Family":

```bash
./BUILD_UNITTEST/test/unittests/unitTest --gtest_filter=RestService.*
```

The web is full of information about gtest/gmock, so if you need to know more, ask my friend Google. He knows!
