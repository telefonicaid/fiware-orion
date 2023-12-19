## How to run functional tests

### Prerequisites

* The `contextBroker` binary is in execution path
* MongoDB database is up and running
* accumulator-server.py is correctly installed and available in the execution path (see specific section below)

### How to install accumulator-server.py

We recommend to use Python [virtualenv](https://virtualenv.pypa.io/en/latest) to install the required dependencies to avoid any potential conflict with operating system Python installation. So, first create your virtual env (named for instance `ft_env`):

```
pip install virtualenv  # if you don't have virtualenv itself previously installed
virtualenv --python=/usr/bin/python3 /path/to/ft_env
```

Then activate the virtual env:

```
. /path/to/ft_env/bin/activate
```

Next install accumulator-server.py depencencies:

```
pip install Flask==2.0.2
pip install Werkzeug==2.0.2
pip install paho-mqtt==1.6.1
pip install amqtt==0.11.0b1  # Not actually an accumulator-server.py dependency, but needed by some tests
```

Next, install the accumulator-server.py script itself:

```
make install_scripts  # add INSTALL_DIR=... if you need to install in a specific place
```

and check that you have it in the path:

```
accumulator-server.py -u
```

**IMPORTANT:** remember to activate the virtual env (`. /path/to/ft_env/bin/activate`) before running functional tests

#### Alternative installation using docker

You can use the following Dockerfile to build the accumulator:

```
FROM debian:stable
RUN apt-get update -y
RUN apt-get install -y python3 python3-venv python3-pip

# Create a virtual environment
RUN python3 -m venv /venv
ENV PATH="/venv/bin:$PATH"

# Install required packages within the virtual environment
RUN pip install Flask==2.0.2 Werkzeug==2.0.2 paho-mqtt==1.6.1

COPY . /app
WORKDIR /app
ENTRYPOINT [ "python3", "./accumulator-server.py"]
CMD ["--port", "1028", "--url", "/accumulate", "--host", "0.0.0.0", "-v"]
```

**Important note**: copy the accumulator-server.py to the same directory in which Dockerfile is, before running your `docker build` command.

Once build (let's say with name 'accum') you can run it with:

```
docker run -p 0.0.0.0:1028:1028/tcp accum
```

Note the `-p` arguments, so the accumulator listening port in the container gets mapped to the one in your host.

### Run functional tests

The easiest way is running just:

```
cd test/functionalTest
./testHarness.sh
```

In case you only want to run a single file or folder, you can also add the path to the file or folder like this:

```
./testHarness.sh cases/3949_upsert_with_wrong_geojson/
```

If you want to set the number of retries of the test you can use the env var `CB_MAX_TRIES` (e.g. you only want to run the test once when you know it is going 
to fail but it is useful to see the output)

Another useful env var is `CB_DIFF_TOOL`, that allows to set a tool to view diff of failing tests (e.g. [meld](https://meldmerge.org/))

As an example of the usage of both env vars, the following line:

```
CB_MAX_TRIES=1 CB_DIFF_TOOL=meld ./testHarness.sh cases/3949_upsert_with_wrong_geojson/
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

## Miscelanea

Useful command to remove trailing whitespace in all .test files (it makes comparison less noisy in the case of failing tests):

```
find cases/ -name *.test -exec sed -i 's/[[:space:]]*$//' {} \;
```
