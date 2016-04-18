### Feature Files Structure

An important advantage of BDD is that it is performed using a natural language, like Gherkin.

In Orion context broker `.feature`-files are composed of the following parts:

1.	Context Broker License
2.  It is possible to use **tags** to identify each scenario, ex: `@happy_path`. If a scenario detects a bug, a tag is appended (`@ISSUE_XXXX` or `@BUG_XXXX`) and perhaps another tag (called `@skip`) to skip the scenario if you so wish.
3.	**Feature**: name and description of the Suite in question (which may be considered a Test Suite)
    
    Actions executed before or after a feature and each scenario:  
    >**Actions Before the feature:**   
    *modify the file `properties.json` from a template file, with all necessary configuration used in the tests*.
    ```
      Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
    ```  
    *modify the contextBroker configuration file remotely, using the configuration stored in `properties.json`* 
    ```
    Setup: update contextBroker config file
    ```
    *start the ContextBroker remotely, per CLI mode or RPM mode*
    ```
    Setup: start ContextBroker
    ```
    *verify if Context Broker is running, else retry the verification until it is*
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
    
    **Scenario:** contains a use case with its steps written in a language that everyone can understand (using keywords as: **Given**, **And**, **When**, **Then**, **But**). The use case can have parameters and tables with variable values. Ex:
    > In this step, an entity group with three entities is created in normalized mode (not keyValues) and the “type” will be used as prefix so that the entities are different, other properties are entered in a previous step.
    ```
        And create entity group with "3" entities in "normalized" mode
         | entity | prefix |
         | type   | true   |
    ```
    
    **Scenario Outline:** Sometimes a scenario should be run with a number of variables that give a set of known states, all executions with the same actions (steps). Then the scenario is repeated as many times as lines found in  **Examples** (see below) by replacing the value of each column by its variable in the step (<service>). Ex:   
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
    
**Example of complete scenario:**
    
    Steps:
    - **Given** a definition of headers 
    - **And**   "defining" properties to subscriptions
    - **When**  create a new subscription 
    - **Then**  verify that a "Created" http code is received
    - **And**   verify headers in the response
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
    
 **Test flow:**

 1. create necessary configuration for the tests (properties.json)
 2. modify configuration file of CB, remotely
 3. start application remotely
 4. define and create previous steps for each scenario
 5. execute the test case for the scenario
 6. verify the necessary data in the response, db, log, etc
 7. remove everything previously created in step 4
 8. stop application remotely
 9. generate a report for the executed test (passed, failed and skipped) for each feature
