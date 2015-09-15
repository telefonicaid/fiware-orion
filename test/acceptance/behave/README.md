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
  * Remember to unset the virtual environment when you're done testing, if it has been previously activated (deactivate) (optional)
- You may need to set `export GIT_SSL_NO_VERIFY=true` environment variable in your machine
- Both if you are using a virtual environment or not:
  * Change to the test/acceptance/behave folder of the project.
  * Install the requirements for the acceptance tests in the virtual environment
```
     pip install -r requirements.txt --allow-all-external
```

#### Requirements to fabric (http://www.fabfile.org/)
```
     yum install gcc python-devel
```
   Fabric is a Python (2.5-2.7) library and command-line tool for streamlining the use of SSH for application deployment or systems administration tasks.

#### Folders/Files Structure

    components/:                         folder with several sub-folders with features and steps
        --> common_steps/:               folder with common steps used in several features
              -->  *.py                  files with common steps associated to previous scenarios
        --> functionality/               sub-folder to save all features in each functionality area
              --> environment.py:        may define code to run before and after certain events during your testing. (recommend use generic in root path)
              --> *.feature              files used to define scenarios
              --> steps/                 folder of steps associated to features in the functionality
                   -->  *.py             files with steps associated to previous scenarios
    logs/:                               folder to log (it is created if does not exist)
    results/:                            folder to save junit reports (it is created if does not exist)
    settings/:                           folder to store different configurations (this place is configurable in configuration.json) (optional)
    tools/:                              internal libraries
    behave.ini:                          configuration files for behave are called either ”.behaverc” or “behave.ini”
    logging.ini                          log configuration with several loggers (environment, steps and utils)
    configuration.json:                  initial configuration, before of execution
    environment.py                       (generic environment) may define code to run before and after certain events during your testing
    properties.json.base:                reference file with parameters (properties) used in tests (after is copied to properties.json)
    README.md:                           this file, a brief explication about this framework to test
    requirement.txt:                     external library, necessary install before to execute test (see Test execution section)

   Note:
```
       The “environment.py” file (optional), if present, must be in the same directory that contains the “steps” directory 
       (not in the “steps” directory). We recommend use a generic environment.py and import it in the environment.py 
       file in the same directory that contains the “steps” directory.
```
### Tests execution:

- Change to the test/acceptance/behave folder of the project if not already on it.
- You may need to set `export GIT_SSL_NO_VERIFY=true` environment variable in your machine
- You should add "--upgrade" (if you have a previous installed version of this library)
- We recommend to create `settings` folder in  behave root directory if it does not exists and store all configurations to referenced by `properties.json` files.
  The settings folder path could be changed in the `configuration.json`.
  This file initially will overwrite properties.json in each feature.
- modify in `configuration.json`:
       * JENKINS: determine whether you are in jenkins or not.
       * PATH_TO_SETTING_FOLDER: folder where are the different configurations
- `properties.json` will be update automatically from settings folder (see configuration.json)
- Run behave (see available params with the -h option).
```
    Some examples:
       behave .../example.feature                      -- run only one feature
       behave .../example.feature -t test              -- run scenarios tagged with "test" in a feature
       behave .../example.feature -t=-skip             -- run all scenarios except tagged with "skip" in a feature
       behave .../example.feature -t ~@skip            -- run all scenarios except tagged with "skip" in a feature
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
Recommend append `BackgroundFeature` in Feature description and define the steps with `Setup:` or `Check:` prefix.
Example:
```
Feature: feature name...
  As a behave user
  I would like to execute once several steps in the feature
  So ....

  BackgroundFeature:
    Setup: update config file
    Setup: restart service
    Check: verify if the service is installed successfully
```

### Tests Suites Coverage (features):

  - entities
    * general_operations - 16 testcases
    * create_entities - 535 testcases
    * list_all_entities - 200 testcases
    * list_an_entity_by_id - 209 testcases
    * list_an_attribute_by_id - 198 testcases
    * update_append_attribute_by_id - 729 testcases
    * update_only_by_id (pending)
    * replace_attributes_by_id (pending)
    * remove_entity (pending)

  -  alarms (pending)


### Hints:
  - If we need " char, use \' and it will be replaced (`mappping_quotes` method in `helpers_utils.py` library) (limitation in behave and lettuce).
  - If value is "max length allowed", per example, it is a generated random value with max length allowed and characters allowed.
  - "attr_name", "attr_value", "attr_type", "meta_name", "meta_type" and "meta_value" could be generated with random values.
      The number after "=" is the number of chars
        ex: | attr_name | random=10 |
  - If entities number is "1", the entity id is without consecutive, ex: `entity_id=room`
    Else entities number is major than "1" the entities number are prefix plus consecutive, ex:
        `entity_id=room_0, entity_id=room_1, ..., entity_id=room_N`
  - If attribute number is "1", the attribute name is without consecutive, ex: `attributes_name=temperature`
    Else attributes number is major than "1" the attributes name are prefix plus consecutive, ex:
        `attributes_name=temperature_0, attributes_name=temperature_1, ..., temperature_N`
  - If would like a wrong query parameter name, use `qp_` prefix       
        


### Tags

You can to use multiples tags in each scenario, possibles tags used:

    - happy_path, skip, errors_40x, only_develop, ISSUE_XXX, BUG_XXX, etc

and to filter scenarios by these tags: see Tests execution section.
