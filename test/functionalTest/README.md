## How to run functional tests

TBD

### Configure Python Virtual Environment

In order to execute the python script in a controlled environment, we recommend executing the following steps.

### Requirements

The following software must be installed:

- Python 3.8
- pip
- virtualenv

### Installation

The recommended installation method is using a virtualenv. Actually, the installation 
process is only about the installation of the corresponding python dependencies.

1. Clone this repository.
3. Create the virtualenv: `virtualenv -ppython3.8 .env`
4. Activate the python environment: `source ./.env/bin/activate`
5. Install the requirements: `pip install -r requirements.txt

Now, when you execute python, you will be using the python script located in the `.env/bin` folder. If you want
to deactivate the current python environment just execute `deactivate` in the command line.

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

The solution is easy: don't use Python 2.6. The recommended version is Python 2.7. Note that CentOS 6 comes with 
Python 2.6 at system level, but you can use [virtualenv](https://virtualenv.pypa.io/en/stable/) to use Python 2.7 
easily.
