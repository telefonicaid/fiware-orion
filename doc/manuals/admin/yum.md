## Using Yum repositories

This document describes the guidelines of using FIWARE Yum repository to install Orion Context Broker. Provided configuration corresponds to x86_64 architecture and CentOS/RHEL 7 OS.

There are two available repositories:

* Nightly, for nightly packages.
* Release, for release packages.

You can read about differences between packages [here](install.md#installation) 

You can manually add a config for repositories, or download it from the FIWARE public repository.
Keep in mind, if you use both repositories together on the same server, nightly packages will always be ahead of release.
  
Use this configuration for release repository:
```
[fiware-release]
name=FIWARE release repository
baseurl=https://nexus.lab.fiware.org/repository/el/7/x86_64/release
enabled=1
gpgcheck=0
priority=1

```
or download it from [FIWARE public repository](https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo)
```
sudo wget -P /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo
```
Use this configuration for nightly repository:
```
[fiware-nightly]
name=FIWARE nightly repository
baseurl=https://nexus.lab.fiware.org/repository/el/7/x86_64/nightly
enabled=1
gpgcheck=0
priority=1

```
or download it from [FIWARE public repository](https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo)
```
sudo wget -P /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo

```

Next you can simply install ContextBroker
```
sudo yum install contextBroker 
```
