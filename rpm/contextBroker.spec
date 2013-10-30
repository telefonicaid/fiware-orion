# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# fermin at tid dot es

%define name contextBroker
%define owner orion 

# Don't byte compile python code
%global __os_install_post %(echo '%{__os_install_post}' | sed -e 's!/usr/lib[^[:space:]]*/brp-python-bytecompile[[:space:]].*$!!g')

Summary:   Orion Context Broker
Name:      %{name}
Version:   %{broker_version}
Release:   %{broker_release}
License:   AGPLv3
Group:     Applications/Engineering
Vendor:     Telefónica I+D
Packager:   Fermín Galán <fermin@tid.es>
URL:        http://catalogue.fi-ware.eu/enablers/publishsubscribe-context-broker-orion-context-broker
Source:     %{name}-%{broker_version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot
Requires:  libstdc++, boost-thread, boost-filesystem, libmicrohttpd
Buildrequires: gcc, cmake, gcc-c++, libmicrohttpd-devel, boost-devel
Requires(pre): shadow-utils

%description
The Orion Context Broker is an implementation of NGSI9 and NGSI10 interfaces. 
Using these interfaces, clients can do several operations:
* Register context producer applications, e.g. a temperature sensor within a room.
* Update context information, e.g. send updates of temperature.
* Being notified when changes on context information take place (e.g. the
  temperature has changed) or with a given frecuency (e.g. get the temperature
  each minute).
* Query context information. The Orion Context Broker stores context information
  updated from applications, so queries are resolved based on that information.

%prep
if [ -d $RPM_BUILD_ROOT/usr ]; then
   rm -rf $RPM_BUILD_ROOT
fi
%setup

%pre
getent group %{owner} >/dev/null || groupadd -r %{owner}
getent passwd %{owner} >/dev/null || useradd -r -g %{owner} -m -d /opt/orion -s /bin/bash -c 'Orion account' %{owner}
exit 0

%post
/sbin/chkconfig --add %{name}
mkdir -p /var/log/%{name}
chown -R %{owner}:%{owner} /var/log/%{name}
# Secure the configuration file to prevent un-authorized access
chown %{owner}:%{owner} /etc/sysconfig/%{name}
chmod 600 /etc/sysconfig/%{name}
cat <<EOMSG
contextBroker requires additional configuration before the service can be
started. Edit '/etc/sysconfig/%{name}' to provide the needed database
configuration.

After configuring /etc/sysconfig/%{name} execute 'chkconfig %{name} on' to
enable %{name} after a reboot.
EOMSG

%preun
/etc/init.d/%{name} stop
/sbin/chkconfig --del %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%build
#FIXME There is an open issue with "make release" malfunction. Until get fixed, we will build in debug mode
#make release DESTDIR=$RPM_BUILD_ROOT BUILD_ARCH=%{build_arch}
make debug DESTDIR=$RPM_BUILD_ROOT BUILD_ARCH=%{build_arch}

%install
#FIXME There is an open issue with "make release" malfunction. Until get fixed, we will build in debug mode
#make install DESTDIR=$RPM_BUILD_ROOT
make install_debug DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/var/%{name}
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/etc/profile.d
mkdir -p $RPM_BUILD_ROOT/usr/share/contextBroker/tests
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/contextBroker
cp -r test/testharness/*.test $RPM_BUILD_ROOT/usr/share/contextBroker/tests
cp LICENSE.txt $RPM_BUILD_ROOT/usr/share/doc/contextBroker
cp scripts/testEnv.sh scripts/testHarness.sh scripts/testDiff.py $RPM_BUILD_ROOT/usr/share/contextBroker/tests 
cp scripts/accumulator-server.py $RPM_BUILD_ROOT/usr/share/contextBroker/tests 
cp scripts/managedb/garbage-collector.py $RPM_BUILD_ROOT/usr/share/contextBroker
cp scripts/managedb/lastest-updates.py $RPM_BUILD_ROOT/usr/share/contextBroker
cp etc/init.d/contextBroker.centos $RPM_BUILD_ROOT/etc/init.d/%{name}
chmod 755 $RPM_BUILD_ROOT/etc/init.d/%{name}
mkdir -p $RPM_BUILD_ROOT/etc/sysconfig
cp etc/config/contextBroker $RPM_BUILD_ROOT/etc/sysconfig/%{name}

echo "%%defattr(-, root, root, - )" > MANIFEST
(cd %{buildroot}; find . -type f -or -type l | sed -e s/^.// -e /^$/d) >>MANIFEST

grep -v "tests" MANIFEST > MANIFEST.broker
grep "tests" MANIFEST > MANIFEST.broker-tests

%files -f MANIFEST.broker

%changelog

%package tests
Requires: %{name}, python, python-flask, nc, curl, libxml2, mongodb
Summary: Test suite for %{name}
%description tests
Test suite for %{name}

#%files tests
%files tests -f MANIFEST.broker-tests

%package -n %{name}-fiware
Requires: %{name} = %{broker_version}-%{broker_release}, %{name}-tests = %{broker_version}-%{broker_release}
Summary: FI-WARE NGSI Broker - Telefónica I+D Implementation
Version: %{fiware_version}
Release: %{fiware_release}
%description -n %{name}-fiware
The Orion Context Broker is an implementation of the NGSI9 and NGSI10 interfaces. 
Using these interfaces, clients can do several operations:
* Register context producer applications, e.g. a temperature sensor within a room.
* Update context information, e.g. send updates of temperature.
* Being notified when changes on context information take place (e.g. the
  temperature has changed) or with a given frecuency (e.g. get the temperature
  each minute).
* Query context information. The Orion Context Broker stores context information
  updated from applications, so queries are resolved based on that information.

%files -n %{name}-fiware

%changelog
* Wed Oct 30 2013 Fermin Galan <fermin@tid.es> 0.8.1-1 (FIWARE-3.2.1-1)
- First version released as open source
- ADD lastest-updates.py script 
- ADD statistics as a REST service 
- ADD compilation inforamtion in REST version message
- REMOVE -reset from contextBroker invocation (too dangerous!)
- FIX licence text on --version, to use AGPLv3
- FIX problems with discoverContextAvailability when "type" and "id" is swapped in DB (spotted in Santander hackathon)
- FIX memory leaks
- FIX libmricohttp-devel dependency not needed for runtime
- FIX crash with ONTIMEINTERVAL subscriptions (issue #9)
- Starting the work in JSON rendering, not yet finished

* Wed Oct 09 2013 Fermin Galan <fermin@tid.es> 0.8.0-1 (FIWARE-3.2.1-1)
- ADD libmicrohttpd-devel to RPM dependencies
- ADD entity creation timestamp in entities collection documents
- ADD entity and attributes modification timestamp in entities collection documents
- ADD notifications counter (count) in csubs and casubs collection documents
- ADD lastNotification timestamp in casubs collection documents
- ADD uptime to version message via REST
- Improve stability and performance
- Trace messages cleanup 

* Wed Oct 02 2013 Fermin Galan <fermin@tid.es> 0.7.3-1 (FIWARE-3.1.3-1)
- Fixed an important error in convenience function 'postIndividualContextEntityAttribute'
- Many fixes, many of them related with memory leaks and stability 
- Trace messages cleanup    
- Fix: empty Accept header right processing

* Fri Aug 30 2013 Fermin Galan <fermin@tid.es> 0.7.2-1 (FIWARE-3.1.2-1)
- FIX case insensitive HTTP processing

* Fri Aug 30 2013 Fermin Galan <fermin@tid.es> 0.7.1-1 (FIWARE-3.1.2-1)
- FIX avoid sending empty contextValue in notifyContext (breaks federation between context brokers instances)
- FIX 'Content-Type' taking into account charset 
- FIX 'Accept' taking into account list and charset

* Thu Aug 29 2013 Fermin Galan <fermin@tid.es> 0.7.0-1 (FIWARE-3.1.2-1)
- notifyContextRequest and notifyContextAvailabilityRequest support
- Proper processing of 'Content-Type' and 'Accept' headers in requests
- FIX wrong XML format in unsubscribeContextRequest operation
- FIX not-blocking notification messages when <reference> is not responding
- FIX cross-operation payload contamination
- FIX large request proper detection and response
- Improved registration and subscription ID validation

* Tue Jul 30 2013 Fermin Galan <fermin@tid.es> 0.6.0-1 (FIWARE-3.1.1-1)
- Support queryContext and discoverContextAvailability requests involving pattern entities without type
- Genaral improvement of NGSI functionality in requests involving entities or attributes without type    
- Default duration implementation for registerContext, subscribeContext and subscribeContextAvailability
- FIX adding proper Content-Type header in HTTP responses
- FIX proper semaphore releasing in some requests
- FIX not recover ONTIMEINTERVAL threads when -ngsi9 is used
- FIX large input buffers correctly taken care of

* Fri Jun 28 2013 Fermin Galan <fermin@tid.es> 0.5.0-1 (FIWARE-2.3.3-1)
- Resiliancy and recovery to database connection problems (now they don't break the broker with segfault)
- General improvement and enrichment of test harness, making them more resilient and stable
- Improvements in subscription processing, taking into account expiration in csubs collection queries
- Renaming option -cm -> -ngsi9
- Decoupling -ngsi9 and -fwdPort/fwdHost, now they can be uses indepently
- FIX creating ONTIMEINTERVAL threads at startup
- FIX changing name of databases for testharness (orion -> testharness) to avoid accidental deleting production database
- FIX add nc, curl and libxml2 to test RPM dependencies
- FIX now expiration is honoured in discoverContextAttribute
- FIX using 0 for -fwdPort to avoid registerContext loop with default command line
- Add verbose mode in accumulator-server.py
- Add number REST operation in accumulator-server.py
- Add garbage-collection.py script
- Other minor bugfixing

* Thu Jun 13 2013 Fermin Galan <fermin@tid.es> 0.4.1-1 (FIWARE-2.3.3-1)
- Bugfix in version reported by -version and curl ../version

* Thu Jun 13 2013 Fermin Galan <fermin@tid.es> 0.4.0-1 (FIWARE-2.3.3-1)
- REST interface for changing log and trace levels
- Memory leak fixes

* Mon Jun 04 2013 Fermín Galán <fermin@tid.es> 0.3.0-1 (FIWARE-2.3.3-1)
- CLI argument -logAppend
- Handlers for SIGUSR1 and SIGUSR2 signals to stop/resume logging
- Handlers for SIGTERM and SIGINT signals for smart exiting
- Fixing concurrency problems at mongo backend
- Memory leak fixes

* Mon May 27 2013 Fermín Galán <fermin@tid.es> 0.2.0-1 (FIWARE-2.3.2-1)
- All convenience functions implemented
- Associations extension support 
- Memory leak fixes
- Fix throttling processing in ONCHANGE subscriptions
- Fix expiration checking for ONCHANGE subscriptions
- Signal handler for SIGINT and SIGTERM. pidfile removed on SIGINT and SIGTERM
- CLI argument -localip
- MongoDB authentication implemented
- CLI argument -cm

* Tue May 14 2013 Fermín Galán <fermin@tid.es> 0.1.0-1 (FIWARE-2.3.2-1)
- Initial version of Orion Context Broker, based on old SAMSON Broker
