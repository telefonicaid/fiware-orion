# Installing Orion

* [Introduction](#introduction)
* [Requirements](#requirements)
* [Installation](#installation)
    * [Using yum (recommended)](#using-yum-recommended)
    * [Using RPM file](#using-rpm-file)
* [Upgrading from a previous version](#upgrading-from-a-previous-version)
    * [Upgrading MongoDB version](#upgrading-mongodb-version)
    * [Migrating the data stored in DB](#migrating-the-data-stored-in-db)
    * [Standard upgrade](#standard-upgrade)

## Introduction

The recommended procedure is to install using RPM packages in CentOS 7.x. If you are interested in
building from sources, check [this document](build_source.md).

## Requirements

* System resources: see [these recommendations](diagnosis.md#resource-availability)
* Operating system: CentOS/RedHat. The reference operating system is CentOS 7.4.1708
  but it should work also in any later CentOS/RedHat 7.x version.
* Database: MongoDB is required to run either in the same host where Orion Context Broker is to be installed or in a different host accessible through the network. The recommended MongoDB versions
  are 2.6/3.0/3.2/3.4. It is not recommended using MongoDB 2.4.x., as some [geolocated queries](../user/geolocation.md) may not work.
    * In the case of using MongoDB 3.0/3.2/3.4 with its new authentication mechanism (SCRAM_SHA1) you may need to compile from sources using special switches for the MongoDB driver.
      See [this issue](https://github.com/telefonicaid/fiware-orion/issues/1061) for details.
* RPM dependencies (some of these packages could not be in the official CentOS/RedHat repository but in EPEL, in which case you have to configure EPEL repositories, see <http://fedoraproject.org/wiki/EPEL>):
    * The contextBroker package (mandatory) depends on the following packages: libstdc++, boost-thread, boost-filesystem, gnutls, libgcrypt, libcurl, openssl, logrotate and libuuid

## Installation

There are two available packages:

* Nightly, which are built from master branch every night.
* Release, "official" release RPMs. The usual release period is 1-2 months.

Depending on their type (nightly or release), packages have different versioning:

* Release packages: contextBroker-X.Y.Z-1
* Nightly packages: contextBroker-X.Y.Z-yyyy.mm.dd

Keep in mind, the version of nightly packages will always be ahead of release, and we advise to use nightly packages only for testing purposes.

From this point, if you want to use the nightly builds, remember that it has a different system of versions.

### Using yum (recommended)

Configure the FIWARE yum repository as described [here](yum.md). Then you can install doing (as root):

```
yum install contextBroker
```

Sometimes the above commands fails due to yum cache. In that case, run
`yum clean all` and try again.

### Using RPM file

Download the package directly from the [FIWARE Yum repository](https://nexus.lab.fiware.org/service/rest/repository/browse/el/7/x86_64/) (both types of packages are provided).

Next, install the package using the rpm command (as root):

```
rpm -i contextBroker-X.Y.Z-1.x86_64.rpm
```

## Upgrading from a previous version

Upgrade procedure depends on whether the *upgrade path* (i.e. from the installed Orion version to the target one to upgrade) crosses a version number that requires:

* Upgrading MongoDB version
* Migrating the data stored in DB (due to a change in the DB data model).

### Upgrading MongoDB version

You only need to pay attention to this if your upgrade path crosses 0.11.0 or 0.22.0. Otherwise, you can skip this section.

* Orion versions previous to 0.11.0 recommend MongoDB 2.2
* Orion version from 0.11.0 to 0.20.0 recommend MongoDB 2.4. Check [the 2.4 upgrade procedure in the oficial MongoDB documentation.](http://docs.mongodb.org/master/release-notes/2.4-upgrade/)
* Orion version from 0.21.0 on recommend MongoDB 2.6/3.0/3.2/3.4. check [the 2.6 upgrade procedure](http://docs.mongodb.org/master/release-notes/2.6-upgrade/),
  [the 3.0 upgrade procedure](http://docs.mongodb.org/master/release-notes/3.0-upgrade/), [the 3.2 upgrade procedure](http://docs.mongodb.org/master/release-notes/3.2-upgrade/)
  or [the 3.4 upgrade procedure](https://docs.mongodb.com/master/release-notes/3.4/) in the oficial MongoDB documentation.

### Migrating the data stored in DB

You only need to pay attention to this if your upgrade path crosses 0.14.1, 0.19.0, 0.21.0 or 1.3.0. Otherwise, you can skip this section. You can also skip this section if your DB are not valuable (e.g. debug/testing environments) and you can flush your DB before upgrading.

* [Upgrading to 0.14.1 and beyond from a pre-0.14.1 version](upgrading_crossing_0-14-1.md)
* [Upgrading to 0.19.0 and beyond from a pre-0.19.0 version](upgrading_crossing_0-19-0.md)
* [Upgrading to 0.21.0 and beyond from a pre-0.21.0 version](upgrading_crossing_0-21-0.md)
* [Upgrading to 1.3.0 and beyond from a pre-1.3.0 version](upgrading_crossing_1-3-0.md)
* [Upgrading to 1.5.0 and beyond from a pre-1.5.0 version](upgrading_crossing_1-5-0.md)

If your upgrade cover several segments (e.g. you are using 0.13.0 and
want to upgrade to 0.19.0, so both "upgrading to 0.14.1 and beyond from
a pre-0.14.1 version" and "upgrading to 0.19.0 and beyond from a
pre-0.19.0 version" applies to the case) you need to execute the
segments in sequence (the common part are done only one time, i.e. stop
CB, remove package, install package, start CB). In the case of doubt,
please [ask using StackOverflow](http://stackoverflow.com/questions/ask)
(remember to include the "fiware-orion" tag in your questions).

### Standard upgrade

If you are using yum, then you can upgrade doing (as root):

```
yum install contextBroker
```

Sometimes the above commands fails due to yum cache. In that case, run
`yum clean all` and try again.

If you are upgrading using the RPM file, then first download the new package from the [FIWARE yum repository](https://nexus.lab.fiware.org/service/rest/repository/browse/el/7/x86_64/) (both types of packages are provided).

Then upgrade the package using the rpm command (as root):

```
rpm -U contextBroker-X.Y.Z-1.x86_64.rpm
```
