Summary:          Orion Context Broker
Name:             contextBroker
Version:          %{_broker_version}
Release:          1%{?dist}
License:          AGPLv3
Packager:         Fermín Galán <fermin@tid.es>
BuildRoot:        %{_topdir}/BUILDROOT/
BuildArch:        x86_64
Requires(pre):    shadow-utils
Requires(post):   /sbin/chkconfig, /usr/sbin/useradd
Requires(preun):  /sbin/chkconfig, /sbin/service
Requires(postun): /sbin/service
Requires:         libstdc++, boost-thread, boost-filesystem, libmicrohttpd 
BuildRequires:    gcc, make, cmake, scons, gcc-c++, libmicrohttpd-devel, boost-devel
Group:            Applications/Engineering
Vendor:           Telefónica I+D


# Don't byte compile python code
%global __os_install_post %(echo '%{__os_install_post}' | sed -e 's!/usr/lib[^[:space:]]*/brp-python-bytecompile[[:space:]].*$!!g')

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

## Project information
%define _owner orion
%define _service_name contextBroker

## System folders
# _localstatedir is a system var that goes to /var
%define _orion_log_dir %{_localstatedir}/log/%{name}
%define _src_project_dir %{_sourcedir}/../../
# The _install_dir is defined into the makefile
# The _toddir is defined into the makefile
# The _broker_version is defined into the makefile

# -------------------------------------------------------------------------------------------- #
# Package RPM for tests 
# -------------------------------------------------------------------------------------------- #
%package tests
Requires: %{name}, python, python-flask, nc, curl, libxml2, contextBroker 
Summary: Test suite for %{name}

%description tests
Test suite for %{name}

# -------------------------------------------------------------------------------------------- #
# Prep section, setup macro:
# -------------------------------------------------------------------------------------------- #
%prep
# Read from SOURCE and write into BUILD

echo "[INFO] Preparing installation"

## Create the rpm/BUILDROOT folder
rm -Rf $RPM_BUILD_ROOT && mkdir -p $RPM_BUILD_ROOT

## Copy src files
cp -R %{_src_project_dir}CMakeLists.txt \
      %{_src_project_dir}ContributionPolicy.txt \
      %{_src_project_dir}etc \
      %{_src_project_dir}LICENSE \
      %{_src_project_dir}makefile \
      %{_src_project_dir}README.md \
      %{_src_project_dir}scripts \
      %{_src_project_dir}src \
      %{_src_project_dir}test \
      %{_builddir}

# Copy service files
#cp -R %{_src_project_dir}etc %{buildroot}

# -------------------------------------------------------------------------------------------- #
# Build section:
# -------------------------------------------------------------------------------------------- #
%build
# Read from BUILD and write into BUILD

echo "[INFO] Building RPM"

# Compile the code into the BUILDROOT directory with the architecture x86_64
make debug DESTDIR=$RPM_BUILD_ROOT BUILD_ARCH=%{build_arch}

# -------------------------------------------------------------------------------------------- #
# Pre-install section:
# -------------------------------------------------------------------------------------------- #
%pre
# Read from BUILD and write into BUILDROOT

# Creating the user and group (orion)
echo "[INFO] Creating %{_owner} user"
grep ^%{_owner} /etc/passwd
RET_VAL=$?
if [ "$RET_VAL" != "0" ]; then
      /usr/sbin/useradd -s "/bin/bash" -d %{_install_dir} %{_owner}
      RET_VAL=$?
      if [ "$RET_VAL" != "0" ]; then
         echo "[ERROR] Unable create %{_owner} user" \
         exit $RET_VAL
      fi
fi

# Backup previous sysconfig file (if any)
DATE=$(date "+%Y-%m-%d")
if [ -f "$RPM_BUILD_ROOT/%{_install_dir}/config/%{name}" ]; then
   mv $RPM_BUILD_ROOT/%{_install_dir}/config/%{name} $RPM_BUILD_ROOT/%{_install_dir}/config/%{name}.orig-$DATE
   chown %{_owner}:%{_owner} $RPM_BUILD_ROOT/%{_install_dir}/config/%{name}.orig-$DATE
fi

# -------------------------------------------------------------------------------------------- #
# Pre-install section:
# -------------------------------------------------------------------------------------------- #
%install
# Read from BUILD and write into BUILDROOT

# RPM_BUILD_ROOT = BUILDROOT
# %{_install_dir}=/opt/%{name}

echo "[INFO] Installing the %{name}"
make install_debug DESTDIR=$RPM_BUILD_ROOT
strip $RPM_BUILD_ROOT/usr/bin/%{name}
chmod 555 $RPM_BUILD_ROOT/usr/bin/%{name}

echo "[INFO] Creating installation directories "
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/bin
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/init.d
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/profile.d
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/doc
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/share
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/config
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}_tests
mkdir -p $RPM_BUILD_ROOT/var/log/%{name}

echo "[INFO] Copying files into the %{_install_dir}"
rsync -Pav $RPM_BUILD_ROOT/usr/bin/%{name} $RPM_BUILD_ROOT/%{_install_dir}/bin/%{name} 
rm -Rf $RPM_BUILD_ROOT/usr

cp LICENSE                               $RPM_BUILD_ROOT/%{_install_dir}/doc
cp scripts/managedb/garbage-collector.py $RPM_BUILD_ROOT/%{_install_dir}/share
cp scripts/managedb/lastest-updates.py   $RPM_BUILD_ROOT/%{_install_dir}/share
cp etc/init.d/contextBroker.centos       $RPM_BUILD_ROOT/%{_install_dir}/init.d/%{_service_name}
cp etc/config/%{name}                    $RPM_BUILD_ROOT/%{_install_dir}/config/%{name}

cp -R test/testharness/*.test            $RPM_BUILD_ROOT/%{_install_dir}_tests
cp scripts/testEnv.sh \
   scripts/testHarness.sh \
   scripts/testDiff.py                   $RPM_BUILD_ROOT/%{_install_dir}_tests
cp scripts/accumulator-server.py         $RPM_BUILD_ROOT/%{_install_dir}_tests

chmod 755 $RPM_BUILD_ROOT/%{_install_dir}/init.d/%{name}


# -------------------------------------------------------------------------------------------- #
# Post-install section:
# -------------------------------------------------------------------------------------------- #
%post
# This section is executed when the rpm is installed (rpm -i)

echo "[INFO] Configuring application"
mkdir -p /usr/share/doc/%{name}
echo "[INFO] Creating links"
ln -s %{_install_dir}/init.d/%{_service_name} /etc/init.d/%{_service_name}
ln -s %{_install_dir}/config/%{name} /etc/sysconfig/%{name}
ln -s %{_install_dir}/bin/%{name} /usr/bin/%{name}
ls -s %{_install_dir}/doc /usr/share/doc/%{name}

echo "[INFO] Creating log directory"
mkdir -p %{_orion_log_dir}
chown %{_owner}:%{_owner} %{_orion_log_dir}
chmod g+s %{_orion_log_dir}
setfacl -d -m g::rwx %{_orion_log_dir}
setfacl -d -m o::rx %{_orion_log_dir}
    
echo "[INFO] Configuring application service"
chkconfig --add %{_service_name}


# Secure the configuration file to prevent un-authorized access
echo "[INFO] Securing the configuration file"
chown %{_owner}:%{_owner} /etc/sysconfig/%{name}
chmod 600 /etc/sysconfig/%{name}
cat <<EOMSG
contextBroker requires additional configuration before the service can be
started. Edit '/etc/sysconfig/%{name}' to provide the needed database
configuration.

Note that if you have a previously existing '/etc/sysconfig/%{name}' it
has been renamed to %{_install_dir}/config/%{name}.orig-$DATE.

EOMSG

# -------------------------------------------------------------------------------------------- #
# Pre-uninstall section:
# -------------------------------------------------------------------------------------------- #
%preun
echo "[INFO] Uninstall the %{name}"
/etc/init.d/%{_service_name} stop
/sbin/chkconfig --del %{_service_name}
echo "[INFO] Deleting links"
rm /etc/init.d/%{_service_name} \
   /etc/sysconfig/%{name} \
   /usr/bin/%{name} 
rm -rf /usr/share/doc/%{name} &> /dev/null
echo "[INFO] Deleting the %{name} folder"
rm -rf %{_install_dir} &> /dev/null
echo "[INFO] %{name} Uninstalled"

# -------------------------------------------------------------------------------------------- #
# Clean section:
# -------------------------------------------------------------------------------------------- #
%clean
echo "[INFO] Cleaning the $RPM_BUILD_ROOT directory"
rm -rf $RPM_BUILD_ROOT

# -------------------------------------------------------------------------------------------- #
# Files to add to the RPM 
# -------------------------------------------------------------------------------------------- #
%files
%defattr(755,%{_owner},%{_owner},755)
%{_install_dir}
/var/log/%{name}


#%files test
%files tests
%defattr(755,%{_owner},%{_owner},755)
/opt/contextBroker_tests


# -------------------------------------------------------------------------------------------- #
# Changelog section 
# -------------------------------------------------------------------------------------------- #

%changelog
* Wed Apr 09 2014 Fermin Galan <fermin@tid.es> 0.11.0-1 (FIWARE-3.3.3-1)
- Add: "geo" queries functionality.
- Add: attribute values can now be not only simple strings but compound expressions with vectors and key-maps
- Add: It is now possible to delete entities using updateContext DELETE with empty attributes list
- Fix: Fixed convenience operation "DELETE /ngsi10/contextEntities/{EntityID}" (partially fixes issue #225)
- Fix: The CLI options now coincide with the documentation.
- Fix: Introduced a second semaphore level as additional security measure to ensure all MongoDB driver invocations run in thread-safe mode.
- Fix: Convenience operations now correctly parse context values. (Bug #219)
- Add: It is now possible to update attribute values using a convenience operation without using metadatas.
- Fix: StatusCode is now correctly rendered in JSON when sent as a standalone message (partially fixes issue #225)
- Fix: Fixed a bug about incoming buffer pollution.
- Fix: Some unknown incoming URLs were not responded to. Now they are. (Bug #277)
- Fix: Now the operations GET, POST and PUT work properly with convenience operations over valueID's.
- Fix: JSON/XML Payload parsing that ended in parse error had a bug resulting in a possible memory leak.
- Fix: Convenience operation GET with non-existent valueIDs is now handled correctly.
- Fix: Stronger checking of XML/JSON payload request conformance: not only mandatory elements are checked,
       but also now a parse error is returned if the client tries to use an unrecognized element in any part of the payload.

* Wed Feb 26 2014 Fermin Galan <fermin@tid.es> 0.10.1-1 (FIWARE-3.3.2-1)
- Fix: concurrently bug in ONTIMEINTERVAL threads running with the same clock frecuency and phase
- Fix: some problems in the previous solution to issue #208

* Thu Feb 20 2014 Fermin Galan <fermin@tid.es> 0.10.0-1 (FIWARE-3.3.2-1)
- Added:  IPv6 support (By default, both IPv4 and IPv6 are activated - the new command line options '-ipv4' and '-ipv6' make the broker activate only one of them)
- Added:  JSON support for all implemented convenience operations
- Fixed:  Concurrency problem for JSON parser is solved
- Fixed:  Bug that prevented the use of UpdateContextRequest in JSON with DELETE as action (closes #224)
- Fixed:  Expiration times are now 64-bit integers (there was a previous problem with overflow) (closes #208)
- Fixed:  Bug that incorrectly allowed using an object within an object instead of an object in an array (closes #223)
- Fixed:  Fixed bug that displayed the tag "contextValue" instead of "value" in some instances for JSON rendering (closes 
#219)
- Fixed:  Removed subscriptionId from SubscribeContextAvailability
- Fixed:  XML tag names for scope rendering ('scopeType'/'scopeValue' instead of 'type'/'value')
- Fixed:  Changed "0" for "000000000000000000000000" in subscriptionId
- Fixed:  Removed some spaces from XML tags (E.g. 'verbose level' => 'verboseLevel')
- Fixed:  Stripped 'reference' of whitespace
- Fixed:  Added an empty contextAttributeList for ContextAttributeResponseVector inside IndividualContextEntityAttributes (/ngsi10/contextEntities/{EID}/attributes) as the XSD demands it
- Fixed:  Possible future error for JSON rendering of UpdateContextAvailabilitySubscriptionRequest
- Fixed:  The reasonPhrase in statusCode/errorCode and OrionError is now 100% tied to the 'code'
- Fixed:  OrionError now also with 'reasonPhrase' following 'code'

* Mon Jan 20 2014 Fermin Galan <fermin@tid.es> 0.9.1-1 (FIWARE-3.3.1-1)
- Added:  tracelevels for input/output for notifications and registrations forwarding
- Added:  creDate attribute now also for attributes in mongodb
- Fixed:  race-condition in HTTP client code, affecting mainly the sending of notifications (closes issue #13)
- Fixed:  modDate is now set for entities and attributes at creation time
- Fixed:  added the missing comma for the ContextRegistration vector at rendering registerContextRequest
- Fixed:  registrationId is now mandatory for registerContextResponse
- Fixed:  correct rendering of ContextRegistrationAttribute (its metadata    XML tag    was incorrect)
- Fixed:  duration no longer returned in response payloads if the incoming duration was invalid
- Fixed:  better RegisterProviderRequest validity check. This fix improves the error responses for ngsi9 convenience operations
- Fixed:  bug regarding the 'default' value of 'subscriptionId'. Before this PR, in the case of an undefined subscriptionId, the string 'No SubscriptionId' was used.
- Fixed:  orionError responses now correctly rendered (the top-level '{' and '}' were missing)

* Mon Dec 16 2013 Fermin Galan <fermin@tid.es> 0.9.0-1 (FIWARE-3.2.3-1)
- Complete JSON support for NGSI9/10 standard operations
- Decoupled NGSI9 and NGSI10 functionality
- Complete convenience operations support (XML payload in/out), except the ones related to attribute domains
- Much improved alignment with FI-WARE NGSI XSD in XML payloads
- Support for metadata ID attribute in queryContext, updateContext and equivalent convenience operations
- FIX avoid sending empty notifications with ONTIMEINTERVAL (issue #47)
- Tracelevels changes and cleanup
- Trace for incomplete payload and unsupported HTTP headers moved from warning log to tracelevel log
- ADD trace for output payload (under LmtRestReply tracelevel)
- Proper "User-Agent" and "Accept" headers in notifications sent
- FIX no Content-Type HTTP header for responses without payload
- FIX using HTTP 400 instead of 500 for payload parsing errors
- FIX proper payload per URL operation (issue #old72)
- FIX buffer overflow problems in logging subsystem

* Wed Oct 30 2013 Fermin Galan <fermin@tid.es> 0.8.1-1 (FIWARE-3.2.1-1)
- First version released as open source
- ADD lastest-updates.py script 
- ADD statistics as a REST service 
- ADD compilation information in REST version message
- REMOVE -reset from contextBroker invocation (too dangerous!)
- FIX licence text on --version, to use AGPLv3
- FIX problems with discoverContextAvailability when "type" and "id" is swapped in DB (spotted in Santander hackathon)
- FIX memory leaks
- FIX libmicrohttp-devel dependency not needed for runtime
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
