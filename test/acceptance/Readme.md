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
  * Create a virtual environment somewhere, e.g. in ~/venv (virtualenv ~/venv) (optional)
  * Activate the virtual environment (source ~/venv/bin/activate) (optional)
- Both if you are using a virtual environment or not:
  * Change to the test/acceptance folder of the project.
  * Set the environment variable (Windows and Linux) GIT_SSL_NO_VERIFY=true
  * Install the requirements for the acceptance tests in the virtual environment
     ```
     pip install -r requirements.txt --allow-all-external
     ```

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

- Git issue 713
- Git issue 714
- Git issue 719
- Git issue 322
- Git issue 715
- Git issue 716

### properties.json

Config the environment to execute the test.
Its needed:

- Orion compiled and running (From code or RPM, its independent)


### tags

There is a tag for each feature file

