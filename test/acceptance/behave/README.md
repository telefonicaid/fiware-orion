# Context Broker Acceptance Tests

Folder for acceptance tests of context broker NGSI v2.In this framework we are using Behave-python (http://pythonhosted.org/behave).

## How to Run the Acceptance Tests

### Prerequisites:

- Python 2.6 or newer
- pip installed (http://docs.python-guide.org/en/latest/starting/install/linux/)
- virtualenv installed (pip install virtualenv) (optional).
Note: We recommend the use of virtualenv, because is an isolated working copy of Python which allows you to work on a specific project without worry of affecting other projects.

##### Environment preparation:

- If you are going to use a virtual environment (optional):
  * Create a virtual environment somewhere, e.g. in ~/venv (virtualenv ~/venv) (optional)
  * Activate the virtual environment (source ~/venv/bin/activate) (optional)
- Both if you are using a virtual environment or not:
  * Change to the test/acceptance/behave folder of the project.
  * Install the requirements for the acceptance tests in the virtual environment
     ```
     pip install -r requirements.txt --allow-all-external
     ```

#### Requirements to fabric
    ```
     yum install gcc python-devel
    ```

#### Folders/Files Structure

    components/:                         folder with several sub-folders with features and steps
        --> common_steps/:               folder with common steps used in several features
              -->  *.py                  files with common steps associated to previous scenarios
        --> functionality/               sub-folder to save all features in each functionality area
              --> environment.py:        may define code to run before and after certain events during your testing.
              --> *.feature              files used to define scenarios
              --> steps/                 folder of steps associated to features in the functionality
                   -->  *.py             files with steps associated to previous scenarios
    logs/:                               folder to log (it is created if does not exist)
    results/:                            folder to save junit reports (it is created if does not exist)
    settings/:                           folder to store different configurations (this place is configurable in configuration.py) (optional)
    tools/:                              internal libraries
    behave.ini:                          configuration files for behave are called either ”.behaverc” or “behave.ini”
    configurations.json:                 initial configuration, before of execution
    properties.json.base:                reference file with parameters (properties) used in tests (after is copied to properties.json)
    README.md:                           this file, a brief explication about this framework to test
    requirement.txt:                     external library, necessary install before to execute test (see Test execution section)

### Tests execution:

- Change to the test/acceptance/behave folder of the project if not already on it.
- We recommend to create `settings` folder in  behave root directory if it does not exists and store all configurations to referenced by `properties.json` files.
  The settings folder path could be changed in the `configuration.json`.
  This file initially will overwrite properties.py in each feature.
- modify in `configuration.json`:
       * JENKINS: determine whether you are in jenkins or not.
       * PATH_TO_SETTING_FOLDER: folder where are the different configurations
- `properties.py` will be update automatically from settings folder (see configuration.json)
- Run behave (see available params with the -h option).
```
    Some examples:
       behave .../example.feature                      -- run only one feature
       behave .../example.feature -t test              -- run scenarios tagged with "test" in a feature
       behave .../example.feature -t=-test             -- run all scenarios except tagged with "skip" in a feature
```

### Properties.json

 Properties used by Context Broker tests
- context_broker_env
    * CB_PROTOCOL: web protocol used (http by default)
    * CB_HOST: context broker host (localhost by default)
    * CB_PORT: sth port (8666 by default)
    * CB_VERSION: context broker version (x.y.z by default)
    * CB_VERIFY_VERSION: determine whether the version is verified or not (True or False).
    * CB_FABRIC_USER: user used to connect by Fabric
    * CB_FABRIC_PASS: password used to connect by Fabric, if use password, cert_file must be None.
    * CB_FABRIC_CERT: cert_file used to connect by Fabric, if use cert file, password must be None.
    * CB_FABRIC_RETRY: Number of times Fabric will attempt to connect when connecting to a new server
    * CB_LOG_FILE: log file used by context broker
    * CB_LOG_OWNER: owner and group log used by context broker
    * CB_LOG_MOD: mod file used by context broker
    * CB_EXTRA_OPS:  Use the following variable if you need extra ops

 Properties used by mongo verifications
- mongo_env
    * MONGO_HOST: IP address or host of mongo Server.
    * MONGO_PORT: port where mongo is listening.
    * MONGO_USER: user valid in the mongo server.
    * MONGO_PASS: password to user above.
    * MONGO_VERSION: mongo version installed.
    * MONGO_VERIFY_VERSION: determine whether the version is verified or not (True or False).
    * MONGO_DATABASE: mongo database (sth by default).
    * MONGO_RETRIES: number of retries for data verification.
    * MONGO_DELAY_TO_RETRY: time to delay in each retry.


### BackgroundFeature

In certain cases, could be useful define a background by feature instead of `Background` that is by scenario.
Recommend append `BackgroundFeature` in Feature description and define the steps with `Environment Setup:` prefix.
Example:
```
Feature: feature name...
  As a behave user
  I would like to execute once several steps in the feature
  So ....

  BackgroundFeature:
    Environment Setup: update config file
    Environment Setup: restart service
    Environment Setup: verify if the service is installed successfully
```

### Tests Suites Coverage:

- base.

### Tags

You can to use multiples tags in each scenario, possibles tags used:

    - happy_path, skip, errors_40x, only_develop, ISSUE_XXX, BUG_XXX, etc

and to filter scenarios by these tags: see Tests execution section.



