# Context Broker Acceptance Tests

Folder for acceptance tests of Orion Context Broker NGSIv2, implemented using [Behave-python](http://pythonhosted.org/behave).

## Quick way

Installing and setting up the environment from (almost :) the scratch in your local machine (running tests on Orion
running in an external machine is advanced stuff, look to BB_FABRIC_USER, CB_FABRIC_PASS or CB_FABRIC_CERT and CB_FABRIC_SUDO
parameters).

* Ensure you have the following requirements in your system:
  * Python 2.7. If you need to keep different version of Python interpreter at the same time, you could have a look
    to [SCL](https://www.softwarecollections.org).
  * Python pip 1.4.1 or higher
  * virtualenv 1.10.1 or higher
  * Requeriments for the fabric Python module (installed in a next step), typically installed using: `yum install gcc python-devel`. [Fabric](http://www.fabfile.org) is a library and command-line tool for streamlining the use of SSH for application deployment or systems administration tasks.
* Creating a virtual env (e.g. named orion_bh). Actually this step is optional (if you know what a virtual env is, then you would know which 
  steps to skip in order to avoid using it ;)

```
virtualenv orion_bh
```

* Activate virtual env (you can later run `deactivate` to exit out of the virtual env)

```
source orion_bh/bin/activate
```

* Install requirements. Assuming that you are at the test/acceptance/behave directory, run:

```
pip install -r requirements.txt --allow-all-external
```

* Set properties.json

```
cp properties.json.base properties.json
vi properties.json
# Ensure that CB_LOG_FILE and CB_PID_FILE are ok in your environment
# Ensure that CB_EXTRA_OPS
```

* Check that test run ok. Note that you need a contextBroker built in DEBUG mode in your system PATH, e.g:

```
behave components/ngsiv2/api_entry_point/retrieve_api_resource.feature
```

* You are done!

*Note*: you may need to set `export GIT_SSL_NO_VERIFY=true` environment variable in your machine.

## Upgrading

Use the same pip command done for installation, but adding `--upgrade` to the command line, i.e:

```
pip install --upgrade -r requirements.txt --allow-all-external
```

## Folders/Files Structure

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
    properties.json:                     initially does not exists. This has parameters necessary to execute the tests (see [properties.json.base](https://github.com/telefonicaid/fiware-orion/tree/develop/test/acceptance/behave#propertiesjsonbase))
    README.md:                           this file, a brief explication about this framework to test
    requirement.txt:                     external library, necessary install before to execute test (see [Quick way](https://github.com/telefonicaid/fiware-orion/tree/develop/test/acceptance/behave#quick-way) section)
    behave_all.py                        execute all features in a given directory and its subdirectories

*Note*: The `environment.py` file (optional), if present, must be in the same directory that contains the steps/ directory 
(not inside the steps/ directory itself). We recommend use a generic environment.py and import it in the environment.py 
file in the same directory that contains the “steps” directory.

## Executing Tests:

- Change to the test/acceptance/behave folder of the project if not already on it.
- We recommend to create `settings` folder in  behave root directory if it does not exists and store all configurations to `properties.json` and `configuration.json` files.
    - we can use several configurations, create a file (with ssh commands) to each configuration (this file can to have whatever name and extension). This option is valid with `UPDATE_PROPERTIES_JSON` equal to `true`
      each file will be composed of lines with ssh commands, that will be executed.
         Example of configuration file into `PATH_TO_SETTING_FOLDER`:
         ```
            # replace values in configuration.json
            sed -i "s/\"CB_RUNNING_MODE\".*/\"CB_RUNNING_MODE\": \"RPM\"/" configuration.json
            
            # copy properties.json.base to properties.json
            cp -R properties.json.base properties.json
            
            # replace values in properties.py
            sed -i "s/\"CB_HOST\".*/\"CB_HOST\":\"localhost\",/" properties.json
            sed -i "s/\"CB_VERSION\".*/\"CB_VERSION\":\"0.24.0\",/" properties.json
            sed -i "s/\"CB_VERIFY_VERSION\".*/\"CB_VERIFY_VERSION\":\"true\",/" properties.json
            sed -i "s/\"CB_FABRIC_USER\".*/\"CB_FABRIC_USER\":\"username\",/" properties.json
            sed -i "s/\"CB_FABRIC_PASS\".*/\"CB_FABRIC_PASS\":\"password\",/" properties.json
            sed -i "s/\"MONGO_HOST\".*/\"MONGO_HOST\":\"mongo_host\",/" properties.json
            sed -i "s/\"MONGO_VERSION\".*/\"MONGO_VERSION\":\"2.6.10\",/" properties.json
            sed -i "s/\"MONGO_VERIFY_VERSION\".*/\"MONGO_VERIFY_VERSION\":\"true\",/" properties.json
         ```
    - Other option is `UPDATE_PROPERTIES_JSON` equal to `false` but in this case `properties.json` will be create manually from `properties.json.base` in root path (used in Jenkins)     
  The settings folder path could be changed in `PATH_TO_SETTING_FOLDER` in the `configuration.json` file. 
- modify in `configuration.json`:
       * PATH_TO_SETTING_FOLDER: folder where are the different configurations
       * UPDATE_PROPERTIES_JSON: determine whether you are create/update `properties.json` automatically or manually (used to jenkins).
           - true: read external file (the file is mandatory) in `PATH_TO_SETTING_FOLDER` (with ssh commands) and execute these automatically (used to create `properties.json`) 
           - false: it does nothing and the creation of `properties.json` will be by user in jenkins console.
       * CB_RUNNING_MODE: is used to determine how is compiled ContextBroker.
           ContextBroker will be compiled and installed previously by user, `http://fiware-orion.readthedocs.org/en/develop/admin/build_source/index.html`.        
           - RPM: CB is installed as RPM, so service tooling will be used to start and stop
           - CLI: plain CB command line interface will be used to start contextBroker, the contextBroker binary (compiled in DEBUG mode) 
             must be available in the system PATH.   
- `properties.json` (MANDATORY) will be create/update in `root path` automatically from settings folder (see `configuration.json` file) or manually.
- Run behave (see available params with the -h option).
```
    Some examples:
       behave component/<path>/example.feature                      -- run only one feature
       behave component/<path>/example.feature -t test              -- run scenarios tagged with "test" in a feature
       behave component/<path>/example.feature -t=-skip             -- run all scenarios except tagged with "skip" in a feature
       behave component/<path>/example.feature -t ~@skip            -- run all scenarios except tagged with "skip" in a feature
       
    It is possible to execute only one row in a Scenario Outline (Examples), first is necessary to define a placeholder in the tag into the feature file, example:      
       @test.row<row.id> (http://pythonhosted.org/behave/new_and_noteworthy_v1.2.5.html#tags-may-contain-placeholders)
    After:
       behave component/<path>/example.feature -t @test.row1.10       -- run only the row 10 with this tag     
```


## Scripts

 - **behave_all.py**:  This script execute all .feature files in a given directory and its subdirectories.     
   ```
      usage:                                                                                   
         python behave_all.py [-u] [-only] [-path==directory_path] [-tags==tags_list] [-skip==folders_list] 
                                                                                               
      parameters:                                                                              
         -u: show the usage [optional].                                                       
         -path: find .feature files in the directory and subdirectory (default: .) [optional]. 
         -tags: use tags defined (default: nothing) [optional].                                
         -skip: list of directories skipped, separated by comma (default: steps) [optional].  
         -only: The features are not executed only returns the command line. [optional].
                                                                                               
      example:                                                                                 
          python  behave_all.py  -path==components/ngsiv2  -tags==-t=-skip  -skip==types,entities  -only       
    ```                                                                                           
    Note: If the folder has not a file with .feature extension, this folder is skipped automatically.                                   


## Properties.json.base

 Properties used by Context Broker tests
- context_broker_env
    * CB_PROTOCOL: web protocol used (http by default)
    * CB_HOST: context broker host (localhost by default)
    * CB_PORT: sth port (1026 by default)
    * CB_VERSION: context broker version (x.y.z by default)
    * CB_VERIFY_VERSION: determine whether the version is verified or not (True or False).
    * CB_FABRIC_USER: user used to connect by Fabric
    * CB_FABRIC_PASS: password used to connect by Fabric, if use password, cert_file must be None.
    * CB_FABRIC_CERT: cert_file used to connect by Fabric, if use cert file, password must be None.
    * CB_FABRIC_RETRY: Number of times Fabric will attempt to connect when connecting to a new server
    * CB_FABRIC_SUDO: determine whether with superuser privileges or not (True | False)
    * CB_LOG_FILE: log file used by context broker
    * CB_PID_FILE: where to store the pid for context broker
    * CB_LOG_OWNER: owner and group log used by context broker
    * CB_LOG_MOD: mod file used by context broker
    * CB_EXTRA_OPS:  Use the following variable if you need extra ops
    * CB_RETRIES: number of retries for verification of starting the context broker
    * CB_DELAY_TO_RETRY: time in seconds to delay in each retry.

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
    * MONGO_DELAY_TO_RETRY: time in seconds to delay in each retry.


## Actions pre-defined in Feature Descriptions (Pre and/or Post Actions)

In certain cases, could be useful to define actions before or/and after of the feature or/and each scenario. This help to read all pre or post actions into the feature. 

Recommend append labels defined (`Actions Before the Feature`, `Actions Before each Scenario`, `Actions After each Scenario`,`Actions After the Feature`)
into de feature description, these labels are Optional. And define steps with `Setup:` or `Check:` prefix (must be `:` in the step prefix). See `environment.py` in root path.

Example:
```
Feature: feature name...
  As a behave user
  I would like to execute once several steps in the feature
  So ....

   Actions Before the Feature:
      Setup: update config file
      Setup: start service
      Check: verify if the service is installed successfully
  
   Actions Before each Scenario:
      Setup: reinit the configuration
  
   Actions After each Scenario:
      Setup: delete database

   Actions After the Feature:
       Setup: stop service
```


## Summary of Features and Scenarios

Finally, after each execution is displayed a summary (Optional) with all features executed and its scenarios status. See `environment.py` in root path.
To activate/deactivate this summary, modify `SHOW_SUMMARY` variable (boolean).

Example:
```
                    SUMMARY:
-------------------------------------------------
  - components/ngsiv2/attributes/get_attribute_data.feature >> passed: 141, failed: 0, skipped: 77 and total: 218 with duration: 27.799 seconds.
  - components/ngsiv2/attributes/update_attribute_data.feature >> passed: 260, failed: 0, skipped: 347 and total: 607 with duration: 53.619 seconds.
  - components/ngsiv2/entities/create_entity.feature >> passed: 444, failed: 0, skipped: 136 and total: 580 with duration: 50.480 seconds.
  - components/ngsiv2/entities/list_entities.feature >> passed: 123, failed: 0, skipped: 77 and total: 200 with duration: 28.104 seconds.
  - components/ngsiv2/entities/remove_entity.feature >> passed: 55, failed: 0, skipped: 10 and total: 65 with duration: 6.725 seconds.
  - components/ngsiv2/entities/replace_all_entity_attributes.feature >> passed: 245, failed: 0, skipped: 315 and total: 560 with duration: 46.318 seconds.
  - components/ngsiv2/entities/retrieve_entity.feature >> passed: 132, failed: 0, skipped: 77 and total: 209 with duration: 22.966 seconds.
  - components/ngsiv2/entities/update_existing_entity_attributes.feature >> passed: 298, failed: 0, skipped: 356 and total: 654 with duration: 51.908 seconds.
  - components/ngsiv2/entities/update_or_append_entity_attributes.feature >> passed: 384, failed: 0, skipped: 382 and total: 766 with duration: 64.614 seconds.
  - components/ngsiv2/api_entry_point/retrieve_api_resource.feature >> passed: 20, failed: 0, skipped: 0 and total: 20 with duration: 0.127 seconds.
-------------------------------------------------
10 features passed, 0 failed, 0 skipped
2102 scenarios passed, 0 failed, 1777 skipped
11072 steps passed, 0 failed, 9618 skipped, 0 undefined
Took 5m52.659s
```


## Logs

The log is stored in `logs` folder (if this folder does not exist it is created) and is called `behave.log` see `logging.ini`.


## Tests Suites Coverage (features):

|       FEATURE/REFERENCE                     |  TEST CASES  | METHOD  |            URL                                       |  PAYLOAD  | QUERIES PARAMS |
|:--------------------------------------------|:------------:|--------:|:-----------------------------------------------------|:---------:|:--------------:|      
|**api_entry_point**                                                                                                                                       |
|  retrieve_api_resource                      |     19       | GET     | /version  /statistics  cache/statistics    /v2       | No        | No             |
|                                                                                                                                                          |
|**entities folder**                                                                                                                                       |
| list_entities                               |    510       | GET     | /v2/entities/                                        | No        | Yes            |
| create_entity                               |    693       | POST    | /v2/entities/                                        | Yes       | Yes            |    
|                                                                                                                                                          |
| retrieve_entity                             |    220       | GET     | /v2/entities/`<entity_id>`                           | No        | Yes            |
| update_or_append_entity_attributes          |    823       | POST    | /v2/entities/`<entity_id>`                           | Yes       | Yes            |  
| update_existing_entity_attributes           |    642       | PATCH   | /v2/entities/`<entity_id>`                           | Yes       | Yes            |
| replace_all_entity_attributes               |    586       | PUT     | /v2/entities/`<entity_id>`                           | Yes       | Yes            |  
| remove_entity                               |     92       | DELETE  | /v2/entities/`<entity_id>`                           | No        | Yes            |
|                                                                                                                                                          |
|**attributes folder**                                                                                                                                     |
| get_attribute_data                          |    244       | GET     | /v2/entities/`<entity_id>`/attrs/`<attr_name>`       | No        | Yes            |   
| update_attribute_data                       |    610       | PUT     | /v2/entities/`<entity_id>`/attrs/`<attr_name>`       | Yes       | Yes            |
| remove_a_single_attribute                   |    121       | DELETE  | /v2/entities/`<entity_id>`/attrs/`<attr_name>`       | No        | Yes            |
|                                                                                                                                                          |
|**attributes_value folder**                                                                                                                               |
| get_attribute_value                         |    190       | GET     | /v2/entities/`<entity_id>`/attrs/`<attr_name>`/value | No        | Yes            |  
| update_attribute_value                      |    337       | PUT     | /v2/entities/`<entity_id>`/attrs/`<attr_name>`/value | Yes       | Yes            |
|                                                                                                                                                          |
|**types folder**                                                                                                                                          |
| retrieve_entity_types                       |     68       | GET     | /v2/types/                                           | No        | Yes            |   
| retrieve_an_entity_type                     |  (pending)   | GET     | /v2/types/`<entity_type>`                            | No        | No             |   
|                                                                                                                                                          |
|**subscriptions folder**                                                                                                                                  |
| retrieve_subscriptions                      |  (pending)   | GET     | /v2/subscriptions                                    | No        | Yes            |   
| create_a_new_subscription                   |  (pending)   | POST    | /v2/subscriptions                                    | Yes       | No             |   
|                                                                                                                                                          |
| retrieve_subscription                       |  (pending)   | GET     | /v2/subscriptions/`<subscription_id>`                | No        | No             |   
| update_subscription                         |  (pending)   | PATCH   | /v2/subscriptions/`<subscription_id>`                | Yes       | No             |   
| delete_subscription                         |  (pending)   | DELETE  | /v2/subscriptions/`<subscription_id>`                | No        | No             |   
|                                                                                                                                                          |
|**registration folder**                                                                                                                                   |
| retrieve_registrations                      |  (pending)   | GET     | /v2/registrations                                    | No        | Yes            |   
| create_a_new_context_provider_registration  |  (pending)   | POST    | /v2/registrations                                    | Yes       | No             |   
|                                                                                                                                                          |
| retrieve_context_provider_registration      |  (pending)   | GET     | /v2/registrations/`<registration_id>`                | No        | No             |   
| update_context_provider_registration        |  (pending)   | PATCH   | /v2/registrations/`<registration_id>`                | Yes       | No             |   
| delete_context_provider_registration        |  (pending)   | DELETE  | /v2/registrations/`<registration_id>`                | No        | No             |   
|                                                                                                                                                          |
|**alarms folder**                            |  (pending)   |                                                                                             |
|                                                                                                                                                          |
|**cli_parameters folder**                    |  (pending)   |                                                                                             |



## Hints:
  Generals
  - If we need " char, use \' and it will be replaced (`mappping_quotes` method in `helpers_utils.py` library) (limitation in behave and lettuce).
  - If value is "max length allowed", per example, it is a generated random value with max length allowed and characters allowed.
  - If would like a query parameter name in POST, PUT or PATCH requests, use `qp_` prefix into `properties to entities` step   
  - the `-harakiri` option is used to kill contextBroker (must be compiled in DEBUG mode)
  - It is possible to use the same value of the previous request in another request using this string: 
       `the same value of the previous request`.
  - If we wanted an empty payload in a second request, use:
      ```
          | parameter          |
          | without_properties |
      ```
  
  Entities_
  - "attr_name", "attr_value", "attr_type", "meta_name", "meta_type" and "meta_value" could be generated with random values.
      The number after "=" is the number of chars
        ex: | attr_name | random=10 |
  - If entity id prefix is true, the entity id is value plus a suffix (consecutive), ex:
        `entity_id=room_0, entity_id=room_1, ..., entity_id=room_N`
  - If entity type prefix is true, the entity type is value plus a suffix (consecutive), ex:
        `entity_type=room_0, entity_type=room_1, ..., entity_type=room_N`        
  - The prefixes function (id or type) are used if entities_number is greater than 1.
  - If attributes number is equal to "1", the attribute name has not suffix, ex: `attributes_name=temperature`
    else attributes number is major than "1" the attributes name are value plus a suffix (consecutive), ex:
        `attributes_name=temperature_0, attributes_name=temperature_1, ..., temperature_N`
  - If metadatas number is equal to "1", the attribute name has not suffix, ex: `metadatas_name=alarm`
    else metadatas number is major than "1" the metadatas name are value plus a suffix (consecutive), ex:
        `metadatas_name=alarm_0, alarm_1, ..., alarm_N`
  - If we wanted attributes in keyValues mode in create or update request, use `keyValues` else use `normalized ` value. ex:
      ```
          create entity group with "3" entities in "keyValues" mode
          or
          create entity group with "3" entities in "normalized" mode
      ```
  - If we need to use No-String values (boolean, null, json object, etc) use raw step appropriate.Ex:
       ```
       create an entity in raw and "normalized" modes
       ```
  Logs:     
  - If don´t want verify the value of something trace in log, use `ignored` as value.
     ```
      | trace | value   |
      | time  | ignored |
     ```
  Subscriptions:   
  - If `subject_entities_number` is major than "1" will have N entities object using `subject_entities_prefix` to differentiate.
  - If `condition_attributes_number` is equal "1", the attribute name has not suffix, ex: `"attributes": ["temperature"]`
    else `condition_attributes_number` is major than "1" the attributes name are value plus a suffix (consecutive), ex:
       `"attributes": ["temperature", "temperature_0", "temperature_1", ..., "temperature_N"]`
  - If `notification_attributes_number` is equal "1", the attribute name has not suffix, ex: `"attributes": ["temperature"]`
    else `notification_attributes_number` is major than "1" the attributes name are value plus a suffix (consecutive), ex:
       `"attributes": ["temperature", "temperature_0", "temperature_1", ..., "temperature_N"]`
  - It is possible to use the same value of the previous request in another request using this string:
       `the same value of the previous request`.
  - "type", "id", "attributes" could be random values.nThe number after "=" is the number of chars, ex:
       `| attributes_name | random=10 |`
  - If would you like that `subject` field is missing, use `subject_type` equals to `without subject field`
  - If would you like that `entities` field is missing, use `subject_type` equals to `without entities field`
  - If would you like that `conditions` field is missing, use `condition_attributes` equals to `without condition field`
  - If would you like that `conditions attributes` field is empty, use `condition_attributes` equals to `array is empty`
  - If would you like that `conditions expression` field is empty, use `condition_expression` equals to `object is empty`
  - If would you like that `notification` field is missing, use `notification_callback` equals to `without notification field`
  - If would you like that `notification attributes` field is empty, use `notification_attributes` equals to `array is empty`
  - In expression value have multiples expressions uses `&` as separator, and in each operation use `>>>` as separator between the key and the value,
     ex:
         `| condition_expression | q>>>temperature>40&georel>>>near&geometry>>>point&coords>>>40.6391 |`

## Tags

You can to use multiples tags in each scenario, possibles tags used:

    - happy_path, skip, errors_40x, only_develop, too_slow, ISSUE_XXX, BUG_XXX, etc

and to filter scenarios by these tags: see [Executing Tests](https://github.com/telefonicaid/fiware-orion/tree/develop/test/acceptance/behave#executing-tests) section.
