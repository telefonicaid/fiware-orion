## Using YUM repositories

There are 2 available packages:
- nightly 
- release
 
You can manually add config for yum repo or download it from FIWARE public repository.

Use this config for nightly packages:
```
[fiware-nightly]
name=FIWARE nightly repository
baseurl=http://nexus.lab.fiware.org/repository/el/$releasever/$basearch/nightly
enabled=1
gpgcheck=0
priority=1

```
or download it from [FIWARE public repository](http://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo)
```
curl -o /etc/yum.repos.d/fiware-nightly.repo http://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo

```

Use this config for release packages:
```
[fiware-release]
name=FIWARE release repository
baseurl=http://nexus.lab.fiware.org/repository/el/$releasever/$basearch/release
enabled=1
gpgcheck=0
priority=1

```
or download it from [FIWARE public repository](http://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo)
```
curl -o /etc/yum.repos.d/fiware-release.repo http://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo
```

Keep in mind, configs from FIWARE public repository have https connection and this can cause problems, to avoid them see [SSL configuration](#SSL)

## SSL configuration
If you want to use https versions of yum repositories, you have to add [GeoTrust](http://nexus.lab.fiware.org/repository/raw/public/ca/geotrust.pem) root certificates to your system.

Download and add certificate
```
curl -o /etc/pki/ca-trust/source/anchors/geotrust.pem http://nexus.lab.fiware.org/repository/raw/public/ca/geotrust.pem 
update-ca-trust
```
