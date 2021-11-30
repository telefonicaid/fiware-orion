---
name: Bug report
about: Create a report to help us improve
title: ''
labels: 'bug'
assignees: ''

---

**Bug description**
A clear and concise description of what the bug is. Please do not forget to add:
- Orion version: X.Y.Z
- MongoDB version X.Y
- Env variables or CLI parameters
- Operating System or Docker Image used

**How to reproduce it**
Steps to reproduce the behavior (as an example)
1. Send `GET /version`
2. Create a subscription `POST /v2/subscriptions ...`
3. Check the log '....'
4. See error in the request response

It is extremely useful to add `curl` request examples for each step steps to reproduce the problem.

**Expected behavior**
A clear and concise description of what you expected to happen.

**Additional information**
Add any other information about the problem here like screenshots, logs or outputs, response code, errors, a MongoDB dump, network 
configuration etc.