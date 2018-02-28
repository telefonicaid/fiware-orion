## Using Yum repositories

This document describes the guidelines of using FIWARE Yum repository to install Orion Context Broker. Provided configurations are corresponds to x86_64 architecture ($basearch) and CentOS/RHEL 7 OS ($releasever)

There are two available packages:
- nightly, for RPMs which are built from master branch each night (at around 2am).
- release, for "official" release RPMs. There isn't a fixed released period although it used to be around a release every 1-2 months.

Keep in mind, if you use both repositories together on the same server, nightly builds will always be ahead of release.
  
You can manually add a config for yum repositories, that provides nightly and release packages, or download it from FIWARE public repository.

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
sudo wget -d /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo
```

Next you can simply install ContextBroker
```
sudo yum install contextBroker 
```
