# Installation Guide for the Orion-LD Functional Test Suite on Ubuntu 20.04

The functional test suite for Orion-LD, found in the Orion-LD repository, under `test/functionalTests`, needs some 
installation in order to work.

First, there is a python script (`scripts/accumulator-server.py`) that is used as the receptor of the notifications 
during the functional tests.

This python script (that needs python 3.8) needs a few packages to be installed, therefore it is needed to check the 
current version of python. Nevertheless, Ubuntu 20.04 and other versions of Debian Linux ship with Python 3 
pre-installed. To make sure that our versions are up-to-date, update your local package index:

```bash
sudo apt update
```

Then upgrade the packages installed on your system to ensure you have the last versions:

```bash
sudo apt -y upgrade
```

The flag '-y' inform that we accept all the packages to be installed. Once this operation ends, we can check which 
version of python 3 is installed in the system:

```bash
python3 -V
Python 3.8.10
```

> Note: If you want to use python and not python3, create the corresponding link with the following command:
> ```bash
> sudo ln -s /usr/bin/python2 /usr/bin/python
> ```

The next step is the installation of the python virtualenv:

```bash
sudo apt -y install python3-virtualenv
```

Now we can create our virtual python environment executing:

```bash
virtualenv -p python3 .venv
```

This operation will create the .venv environment that we use to execute our scripts. Active the environment with the 
following command:

```bash
. .venv/bin/activate

Finally, install the corresponding requirements files executing:

```bash
pip install -r scripts/requirements.txt
```

That should be all for the accumulator python script.

The test script (`test/functionalTest/testHarness.sh`) needs to *find* the accumulator, to be able to start it:
```bash
cd <orion-ld repository base directory>
export PATH=$PATH:$PWD/scripts
which accumulator-server.py
```

The output of the `which` command should be:

```text
~/git/context.Orion-LD/scripts/accumulator-server.py
```

Also, the test script uses `nc` to verify that the broker has started, and `bc` for simple calculations.
Install both of them:
```bash
sudo aptitude install netcat
sudo aptitude install bc
```

Test it by launching:
```bash
test/functionalTest/testHarness.sh
```

There are over 1500 test cases (each with a number of steps), so, it will take a while.
Orion-LD has inherited the functional test suite from `orion` and added some 450 test cases only for NGSI-LD.
If you want to run only the NGSi-LD test cases, run the suite with the `-ld`, or/and the `-troe` option:

```bash
test/functionalTest/testHarness.sh -ld
test/functionalTest/testHarness.sh -troe
```

There are lots of command line options for the test suite; use the `-u` option to see all of them.

In case some functional test case fails, normally due to installation error, and you're not really interested in that 
case (might be for IPv6 and you aren't interested in IPv6), you can disable test cases.

Functional test cases can be disabled by exporting an environment variable called `CB_SKIP_FUNC_TESTS`.
For example, to disable the test case 'direct_https_notifications.test' (you don't want https notification, so ...), 
do this:

```bash
export CB_SKIP_FUNC_TESTS=0706_direct_https_notifications/direct_https_notifications.test
```

Note that not only the name of the test case file, but also the directory where it resides is part of the "identifier".
This is so because different functional test case directories can have test case files with the same name.

FYI: after following myself the instructions in the installation guides, the following functional tests failed for me:

* 0706_direct_https_notifications/direct_https_notifications.test
* 0706_direct_https_notifications/direct_https_notifications_no_accept_selfsigned.test

Hmmm, something about https seems to be missing in the instructions ...

Looking closer at the errors, it seems clear that it is the accumulator script that is having problems (the accumulator 
log-file says: **AttributeError: 'Context' object has no attribute 'wrap_socket'**).
I will have to look into this, but for now, I simply do this:

```bash
export CB_SKIP_FUNC_TESTS="0706_direct_https_notifications/direct_https_notifications.test 0706_direct_https_notifications/direct_https_notifications_no_accept_selfsigned.test"
```

Remember to put this environment variable in your startup files (e.g. `~/.bash_profile`), so that you don't lose it.
Also, the directory added to PATH should be saved in your startup files.
