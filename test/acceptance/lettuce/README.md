# Orion Acceptance Tests

Folder for acceptance tests of Orion.

## How to Run the Acceptance Tests

### Prerequisites:

- Python 2.7
- pip installed (http://docs.python-guide.org/en/latest/starting/install/linux/)
- virtualenv installed (pip install virtualenv) (optional).
Note: We recommend the use of virtualenv, because is an isolated working copy of Python which allows you to work on a specific project without worry of affecting other projects.

##### Environment preparation:

- If you are going to use a virtual environment (optional):
  * Create a virtual environment somewhere, e.g. in ~/venv (`virtualenv ~/venv`) (optional)
  * Activate the virtual environment (`source ~/venv/bin/activate`) (optional)
- Both if you are using a virtual environment or not:
  * Change to the test/acceptance folder of the project.
  * Set the environment variable (Windows and Linux) `export GIT_SSL_NO_VERIFY=true`
  * Install the requirements for the acceptance tests in the virtual environment
     ```
     pip install -r requirements.txt --allow-all-external
     ```
- Orion compiled and running

### Tests execution:

- Change to the test/acceptance folder of the project if not already on it.
- Fill properties.json with the environment configuration (see in the properties section).
- Run lettuce (see available params with the -h option).

```
Some examples:
   lettuce .                                   -- run all features
   lettuce --tag=issue-713                     -- run issue-713 feature
```

### Tests Coverage (features):

- ISSUE-322 Service support for registries (NGSI9)
- ISSUE-713 CPr-propagate Fiware-Service header in forwarded requests
- ISSUE-714 CPr-propagate Fiware-Servicepath header in forwarded requests
- ISSUE-715 CPr-propagate X-Auth-Token header in forwarded requests
- ISSUE-716 ContextProvicer forwarding for attributes not existing in existing entities
- ISSUE-719 ServicePath for registrations (NGSI9) (not recursive)
- Delete entity
- Subscribe context

### properties.json

Copy the "properties_base.json" config file and config the environment to execute the test.
Its needed:
```
{
  "environment": {
    "name": "orion",
    "logs_path": "logs" # Path where the logs will be stored
    "log_level": "ERROR" # Level of the logs inside the tests ('CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG')
  },
  "context_broker": {
    "host": "", # Ip where Orion is running
    "port": "" # Port where Orion is listening
  },
  "mock": {
    "port": "", # Port where the mock will be listening
    "bind_ip": "" # Address from mock will accept requests
  },
  "mongo": {
    "host": "", # Ip where the mongoDB is running
    "port": "" # Port where MongoDB is listening
  },
  "deploy_data": {
    "host": "", # Host where the context broker is deployed
    "ssh_port": "", # Ssh port of the host where the CB is deployed (if localhost or 127.0.0.1 is set in host, this is not used)
    "user": "", # Ssh user of the host where the CB is deployed (if localhost or 127.0.0.1 is set in host, this is not used)
    "password": "" # Ssh password of the host where the CB is deployed (if localhost or 127.0.0.1 is set in host, this is not used)
    "bin_path": "" # Path where the bin file of CB is located.
    "log_path": "", # Path where the Context Broker log will be stored. If its empty, the path will be in /tmp/acceptance
    "pid_file": "" # Path where the Context Broker pid file will be stored. If its empty, the path will be in /tmp/acceptance/contextBorker.pid
  }
}
```


### tags

There is a tag for each issue in github:

```
delete_entity
issue-322
issue-713
issue-714
issue-715
issue-716
issue-719
issue-755
issue_755
subscribe_context
```

