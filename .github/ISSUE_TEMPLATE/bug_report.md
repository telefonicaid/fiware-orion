---
name: Bug report
about: Create a report to help us improve
title: ''
labels: 'bug'
assignees: ''

---

**Bug description**
A clear and concise description of what the bug is. Please do not forget to add:

- Orion version: X.Y.Z (this can be got with `GET /version` in the API port or using `contextBroker --version`)
- MongoDB version X.Y
- Env variables or CLI parameters
- Operating System or Docker Image used

**How to reproduce it**
Steps to reproduce the behavior (as an example)
1. Create an entity this way:
    ```BASH
    curl -iX POST 'http://localhost:1026/v2/entities' \
    -H 'Content-Type: application/json' \
    -d '
    {
        "id": "Bug:007",
        "type": "bug",
        "status": {
            "type": "Text",
            "value": "fixed"
        },
        "created": {
            "value": "2021-12-01T9:35:00Z",
            "type": "DateTime"
        }  
    }'
    ```
2. See error in the request response
3. Check the log '....'

It is extremely useful to add `curl` request examples for each step to reproduce the problem (as in the above example, 
the `curl` to do the `POST /v2/entities` in step 1)

> Note that information requested above can contains sensitive information (e.g. public IP addresses, api keys/tokens, 
> passwords, etc.). Please change it before opening the issue 

**Expected behavior**
A clear and concise description of what you expected to happen.

**Additional information**

Add any other information about the problem here, such as screenshots, logs or outputs, response code, errors, a MongoDB dump, network, etc.
configuration etc.
