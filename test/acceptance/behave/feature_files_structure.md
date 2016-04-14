### Feature Files Structure

A great advantage of BDD is that would be performed using natural language as Gherkin.

In Orion ContextBorker `.feature` files are composed of the following parts:

1.	Context Broker License
2.  It is posibble to use **tags** to identify each scenario, ex: `@happy_path`. If a scenario detects a bug, a tag is append (`@ISSUE_XXXX` or `@BUG_XXXX`) and other tag called `@skip` to skip the scenario if you desired.
3.	**Feature**: name and description of the Suite in question (which could be considered as a Test Suite)
    
    Actions executed before or after of the feature or each scenario:  
    >**Actions Before the feature:**   
    *modify the `properties.json` file from an external file, with all necessary configurations used to tests*.
    ```
      Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
    ```  
    *modify the contextBroker configuration file remotely, using the configuration  stored in `properties.json`* 
    ```
    Setup: update contextBroker config file
    ```
    *start the ContextBroker remotely, per CLI mode or RPM mode*
    ```
    Setup: start ContextBroker
    ```
    *verify if Context Broker is running, else retry the verification until it is running*
    ```
    Check: verify contextBroker is installed successfully
    ```
    *verify if mongo is running correctly as it is configured in `properties.json`*
    ```
    Check: verify mongo is installed successfully
    ```

    >**Actions After each Scenario:**   
    *remove the db used in mongoDB after each scenario*.
    ```
      Setup: delete database in mongo
    ``` 
   
    >**Actions After the Feature:**   
    *stop the Context Broker, started in `start ContextBroker` step*.
    ```
      Setup: stop ContextBroker
    ``` 
          
4.	Exist two scenario types:
    
    **Scenario:** contains an user case with its steps written in a language that understand everyone (using keywords as: **Given**, **And**, **When**, **Then**, **But**), it can has parameters and tables with variables values. Ex:
    > In this step is created an entity group with three entities in normalized mode (not keyValues) and the “type” will be used as prefix to that the entities will be different, other properties are entered in a previous step.
    ```
        And create entity group with "3" entities in "normalized" mode
         | entity | prefix |
         | type   | true   |
    ```
    
    **Scenario Outline:** Sometimes a scenario should be run with a number of variables that give a set of known states, all executions with the same actions (steps). Then the scenario is repeated as many times as lines have **Examples** (see below) by replacing the value of each column by its variable in the step (<service>). Ex:   
    > In this case the scenario is repeated 7 times, replacing Fiware-Service value per each line value in **Examples**
     ```
        Given  a definition of headers
          | parameter          | value            |
          | Fiware-Service     | <service>        |
        …
        Examples:
          | service            |
          |                    |
          | service            |
          | service_12         |
          | service_sr         |
          | SERVICE            |
          | max length allowed |
    ```
    
5.	Example of complete scenario:
    
    Steps:
    - **Given** a definition of headers 
    - **And**   "defining" properties to subscriptions
    - **When**  create a new subscription 
    - **Then**  verify that receive a "Created" http code 
    - **And**   verify headers in response
    - **And**   verify that the subscription is stored in mongo
    ```
    @happy_path
    Scenario:  create a new subscription using NGSI v2
      Given  a definition of headers
        | parameter          | value                 |
        | Fiware-Service     | test_casub_happy_path |
        | Fiware-ServicePath | /test                 |
        | Content-Type       | application/json      |
      # These properties below are used in subscriptions request
      And properties to subscriptions
        | parameter                      | value                                                              |
        | description                    | my first subscription                                              |
        | subject_type                   | room                                                               |
        | subject_idPattern              | .*                                                                 |
        | subject_entities_number        | 2                                                                  |
        | subject_entities_prefix        | type                                                               |
        | condition_attributes           | temperature                                                        |
        | condition_attributes_number    | 3                                                                  |
        | condition_expression           | q>>>temperature>40&georel>>>near&geometry>>>point&coords>>>40.6391 |
        | notification_callback          | http://localhost:1234                                              |
        | notification_attributes        | temperature                                                        |
        | notification_attributes_number | 3                                                                  |
        | notification_throttling        | 5                                                                  |
        | expires                        | 2016-04-05T14:00:00.00Z                                            |
        | status                         | active                                                             |
      When create a new subscription
      Then verify that receive a "Created" http code
      And verify headers in response
        | parameter      | value                |
        | location       | /v2/subscriptions/.* |
        | content-length | 0                    |
      And verify that the subscription is stored in mongo
    ```
    
6. Test flow

 1. create configuration necesaary to tests (properties.json)
 2. modify configuration file of CB, remotely
 3. start application remotely
 4. define and create previous steps to each scenario
 5. throw test in question by this scenario
 6. verify the necessary data in response, db, log, etc
 7. remove all create previously in step 4
 8. stop application remotely
 9. generate a report with test executed (passed, failed and skipped) by each feature