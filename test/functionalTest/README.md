## How to run functional tests

### Prerequisites

* The `contextBroker` binary is in execution path
* MongoDB database is up and running
* accumulator-server.py is correctly installed and available in the execution path (see specific section below)

### How to install accumulator-server.py

We recommend to use Python [virtualenv](https://virtualenv.pypa.io/en/latest) to install the required dependencies to avoid any potential conflict with operating system Python installation. So, first create your virtual env (named for instance `ft_env`):

```
pip install virtualenv  # if you doesn't have virtualenv itself previously installed
virtualenv /path/to/ft_env
```

Then activate the virtual env:

```
. /path/to/ft_env/bin/activate
```

Next install accumulator-server.py depencencies:

```
pip install Flask==1.0.2
pip install pyOpenSSL==19.0.0
```

Next, install the accumulator-server.py script itself:

```
make install_scripts  # add INSTALL_DIR=... if you need to install in an specific place
```

and check that you have it in the path:

```
accumulator-server.py -u
```

**IMPORTANT:** remember to activate the virtual env (`. /path/to/ft_env/bin/activate`) before running functional tests

### Run functional tests

The easiest way is running just:

```
cd test/functionalTest
./testHarness.sh
```

## Known issues

### Decimal numbers rounding

You may find failures likes this one when you run the tests:

```
-----  String filters for compound values of attributes: string match  -----
(qfilters_and_compounds_deeper.test) output not as expected
VALIDATION ERROR: input line:
                           "f": 3.1400000000000001,
does not match ref line:
                           "f": 3.14,
```

This is not an actual problem with the test or Orion, but a rounding problem in the version of the json module that 
comes with Python 2.6 (the testHarness.sh program typically uses `python -mjson.tool` to beautify responses).

The solution is easy: don't use Python 2.6. The recommended version is Python 2.7. Note that CentOS 6 comes with Python 2.6 at
system level, but you can use [virtualenv](https://virtualenv.pypa.io/en/stable/) to use Python 2.7 in an easy way.
