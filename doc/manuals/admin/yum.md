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
baseurl=https://nexus.lab.fiware.org/repository/el/$releasever/$basearch/release
enabled=1
gpgcheck=0
priority=1

```
or download it from [FIWARE public repository](https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo)
```
sudo wget -d /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo
```
Use this configuration for nightly repository:
```
[fiware-nightly]
name=FIWARE nightly repository
baseurl=https://nexus.lab.fiware.org/repository/el/$releasever/$basearch/nightly
enabled=1
gpgcheck=0
priority=1

```
or download it from [FIWARE public repository](https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo)
```
sudo wget -d /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo

```
You can read about Yum variables $basearch and $releasever at [access.redhat.com](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/deployment_guide/sec-using_yum_variables)

Next you can simply install ContextBroker
```
sudo yum install contextBroker 
```
