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
# iot_support at tid dot es

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
Packager:   Fermín Galán <fermin.galanmarquez@telefonica.com>
URL:        http://catalogue.fiware.org/enablers/publishsubscribe-context-broker-orion-context-broker
Source:     %{name}-%{broker_version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot
Requires:  libstdc++, boost-thread, boost-filesystem, gnutls, libgcrypt, libcurl, openssl, logrotate, libuuid
Buildrequires: gcc, cmake, gcc-c++, gnutls-devel, libgcrypt-devel, libcurl-devel, openssl-devel, boost-devel, libuuid-devel
Requires(pre): shadow-utils

%description
The Orion Context Broker is an implementation of the Publish/Subscribe Context Broker GE,
providing an NGSI interface. Using this interface, clients can do several operations:

* Query context information. The Orion Context Broker stores context information updated
  from applications, so queries are resolved based on that information. Context 
  information consists on *entities* (e.g. a car) and their *attributes* (e.g. the speed
  or location of the car).
* Update context information, e.g. send updates of temperature
* Get notified when changes on context information take place (e.g. the temperature
  has changed)
* Register context provider applications, e.g. the provider for the temperature sensor
  within a room

%prep
if [ -d $RPM_BUILD_ROOT/usr ]; then
   rm -rf $RPM_BUILD_ROOT
fi

%setup

%pre
getent group %{owner} >/dev/null || groupadd -r %{owner}
getent passwd %{owner} >/dev/null || useradd -r -g %{owner} -m -d /opt/orion -s /bin/bash -c 'Orion account' %{owner}
# Backup previous sysconfig file (if any)
DATE=$(date "+%Y-%m-%d")
if [ -f "/etc/sysconfig/%{name}" ]; then
   cp /etc/sysconfig/%{name} /etc/sysconfig/%{name}.orig-$DATE
   chown %{owner}:%{owner} /etc/sysconfig/%{name}.orig-$DATE
fi
exit 0

%post
DATE=$(date "+%Y-%m-%d")
/sbin/chkconfig --add %{name}
mkdir -p /var/log/%{name}
mkdir -p /var/run/%{name}
chown -R %{owner}:%{owner} /var/log/%{name}
chown -R %{owner}:%{owner} /var/run/%{name}
# Secure the configuration file to prevent un-authorized access
chown %{owner}:%{owner} /etc/sysconfig/%{name}
chmod 600 /etc/sysconfig/%{name}
cat <<EOMSG
contextBroker requires additional configuration before the service can be
started. Edit '/etc/sysconfig/%{name}' to provide the needed database
configuration.

Note that if you have a previously existing '/etc/sysconfig/%{name}' it
has been renamed to /etc/sysconfig/%{name}.orig-$DATE.

After configuring /etc/sysconfig/%{name} execute 'chkconfig %{name} on' to
enable %{name} after a reboot.
EOMSG

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf %{_builddir}

%build
make release DESTDIR=$RPM_BUILD_ROOT BUILD_ARCH=%{build_arch}

%install
make install DESTDIR=$RPM_BUILD_ROOT
cp -R %{_sourcedir}/etc $RPM_BUILD_ROOT

# rpmbuild seems to do the strip step automatically. However, this would fail after chmod, so we "manually" do
# it as part of our install script
strip $RPM_BUILD_ROOT/usr/bin/contextBroker
chmod 555 $RPM_BUILD_ROOT/usr/bin/contextBroker
#FIXME: I think next line is not actually needed, thus it has been commented out for a quarentine period (starting on 24/11/2014)
#mkdir -p $RPM_BUILD_ROOT/var/%{name}
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/etc/profile.d
mkdir -p $RPM_BUILD_ROOT/usr/share/contextBroker/tests
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/contextBroker
cp -r test/functionalTest/cases $RPM_BUILD_ROOT/usr/share/contextBroker/tests
find $RPM_BUILD_ROOT/usr/share/contextBroker/tests/cases/ -name *.DISABLED -exec rm {} \;
rm $RPM_BUILD_ROOT/usr/share/contextBroker/tests/cases/CMakeLists.txt
cp LICENSE $RPM_BUILD_ROOT/usr/share/doc/contextBroker
cp scripts/testEnv.sh test/functionalTest/testHarness.sh test/functionalTest/testDiff.py test/functionalTest/harnessFunctions.sh $RPM_BUILD_ROOT/usr/share/contextBroker/tests
cp scripts/accumulator-server.py $RPM_BUILD_ROOT/usr/share/contextBroker/tests 
cp test/functionalTest/httpsPrepare.sh $RPM_BUILD_ROOT/usr/share/contextBroker/tests
cp scripts/managedb/garbage-collector.py $RPM_BUILD_ROOT/usr/share/contextBroker
cp scripts/managedb/latest-updates.py $RPM_BUILD_ROOT/usr/share/contextBroker
cp scripts/monit_log_processing.py $RPM_BUILD_ROOT/usr/share/contextBroker
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


# The contextBroker-test package is still taken into account, although it is very old and probably obsolete. If we
# recover it in the future, dependencies and so on need to be reviewed. The following fragment (removed from
# install documentation) could be useful:
#
#    The contextBroker-test package (optional) depends on the following packages: python, python-flask, 
#    python-jinja2, curl, libxml2, libxslt, nc, mongo-10gen and contextBroker. The mongo-10gen dependency needs 
#    to configure MongoDB repository, check [this piece of documentation about that](http://docs.mongodb.org/manual/tutorial/install-mongodb-on-red-hat-centos-or-fedora-linux/).
#
%package tests
Requires: %{name}, python, python-flask, python-jinja2, nc, curl, libxml2, mongo-10gen 
Summary: Test suite for %{name}
%description tests
Test suite for %{name}

%files tests -f MANIFEST.broker-tests

%preun
if [ "$1" == "0" ]; then
  /etc/init.d/%{name} stop
  /sbin/chkconfig --del %{name}
fi

%preun tests

%postun 
if [ "$1" == "0" ]; then
  rm -rf  /usr/share/contextBroker
  /usr/sbin/userdel -f %{owner}
fi

%postun tests
if [ "$1" == "0" ]; then
  rm -rf  /usr/share/contextBroker/tests
fi

%changelog
* Thu Feb 21 2019 Fermin Galan <fermin.galanmarquez@telefonica.com> 2.2.0-1
- Add: skipInitialNotification URI param option to make initial notification in subscriptions configurable (#920)
- Add: forcedUpdate URI param option to always trigger notifications, no matter if actual update or not (#3389)
- Add: log notification HTTP response (as INFO for 2xx or WARN for other codes)
- Add: notification.lastFailureReason field in subscriptions to get the reason of the last notification failure
- Add: notification.lastSuccessCode field in subscriptions to get the HTTP responde code of the last successful notification
- Fix: NGSIv1 updates with geo:line|box|polygon|json removes location DB field of NGSIv2-located entities (#3442)
- Fix: specify notification/forwarding error cause in log messages (#3077)
- Fix: from=, corr=, srv=, subsrv= correctly propagated to logs in notifications (#3073)
- Fix: avoid "pending" fields in logs
- Fix: use "<none>" for srv= and subsrv= in logs when service/subservice header is not provided
- Fix: bug in notification alarm raising in -notificationMode transient and persistent
- Remove: deprecated feature ID metadata (and associated NGSIv1 operations)

* Wed Dec 19 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 2.1.0-1
- Add: Oneshot Subscription (#3189)
- Add: support to MongoDB 3.6 (#3070)
- Fix: problems rendering structured attribute values when forwarded queries are involved (#3162, #3363, #3282)
- Fix: NGSIv2-NGSIv1 location metadata issues (#3122)
- Fix: wrong inclusion of "location" metadata in geo:json notifications (#3045)
- Fix: metadata filter was lost during csub cache refresh (#3290)
- Fix: NGSIv1-like error responses were used in some NGSIv2 operations
- Fix: cache sem is not taken before subCacheItemLookup() call (#2882)
- Fix: wrong Runtime Error logs on GET /v2/registrations and GET /v2/registrations/{id} calls (#3375)
- Deprecate: Rush support (along with -rush CLI parameter)
- Remove: isDomain field in NGSIv1 registrations (it was never used)

* Fri Sep 28 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 2.0.0-1
- Fix: GET /v2/subscriptions and GET /v2/subscriptions/{id} crashes for permanent subscriptions created before version 1.13.0 (#3256)
- Fix: correct processing of JSON special characters  (such as \n) in NGSIv2 rendering (#3280, #2938)
- Fix: correct error payload using errorCode (previously orionError was used) in POST /v1/queryContext and POST /v1/updateContext in some cases
- Fix: bug in metadata compound value rendering in NGSIv2 (sometimes "toplevel" key was wrongly inserted in the resulting JSON object)
- Fix: missing or empty metadata values were not allowed in NGSIv2 create/update operations (#3121)
- Fix: default types for entities and attributes in NGSIv2 was wrongly using "none" in some cases
- Fix: with NGSIv2 replace operations the geolocalization field is inconsistent in DB (#1142, #3167)
- Fix: duplicated attribute/metadata keys in JSON response in some cases for dateCreated/dateModified/actionType
- Fix: proper management of user attributes/metadata which name matches the one of a builtin (the user defined shadows the builtin for rendering but not for filtering)
- Fix: pre-update metadata content included in notifications (and it shouldn't) (#3310)
- Fix: duplicated items in ?attrs and ?metadata URI params (and "attrs" and "metadata" in POST /v2/op/query and subscriptions) cause duplicated keys in responses/notifications (#3311)
- Fix: dateCreated and dateModified included in initial notification (#3182)
- Hardening: modification of the URL parsing mechanism, making it more efficient, and the source code easier to follow (#3109, step 1)
- Hardening: refactor NGSIv2 rendering code (throughput increase up to 33%/365% in entities/subscriptions rendering intensive scenarios) (#1298)
- Hardening: Mongo driver now compiled using --use-sasl-client --ssl to enable proper DB authentication mechanisms
- Deprecated: NGSIv1 API (along with related CLI parameters: -strictNgsiv1Ids and -ngsiv1Autocast)

* Mon Jul 16 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.15.0-1
- Add: upsert option for the POST /v2/entities operation (#3215)
- Add: transient entities functionality (new NGSIv2 builtin attribute: dateExpires) (#3000)
- Add: "attrs" field in POST /v2/op/query (making "attributes" obsolete) (#2604)
- Add: "expression" field in POST /v2/op/query (#2706)
- Fix: large integer wrong rendering in responses (#2603, #2425, #2506)
- Remove: "scopes" field in POST /v2/op/query (#2706)

* Fri Jun 15 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.14.0-1
- Add: support for APIv2 notifications (POST /v2/op/notify), which opens up for true APIv2 federation (#2986)
- Add: camelCase actionTypes in POST /v2/op/update (append, appendStrict, update, delete and replace) (#2721)
- Fix: bug having entities without attibutes AND matching registrations made the attr-less entity not be returned by GET /v2/entities (#3176)
- Fix: bug when using "legacy" format for custom v2 notifications (#3154)
- Fix: check for providers is done only in UPDATE or REPLACE actionType case in update entity logic (#2874)
- Fix: wrong 404 Not Found responses when updating attribute using NGSIv2 and CB forwards it correctly (#2871)

* Mon Apr 16 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.13.0-1
- Add: support for GET /v2/registrations (#3005)
- Add: support for GET /v2/registrations/<registration-id> (#3008)
- Add: support for POST /v2/registrations to create registrations in APIv2 (#3004)
- Add: support for DELETE /v2/registrations/<registration-id> (#3006)
- Add: support for CORS requests for /v2/registrations and /v2/registrations/<registration-id> (#3049)
- Hardening: refactor request routing logic (#3109, step 1)
- Bug that may lead contextBroker to crash when passing null character "\u0000" with ID parameter in query context payload (#3119)
- Add: CORS Preflight Requests support for /version (#3066)
- Deprecated: ID metadata (and associated NGSIv1 operations)

* Wed Feb 21 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.12.0-1
- Add: support for entity id and type in orderBy parameter (#2934)
- Add: NGSIv1 autocast for numbers, booleans and dates (using flag -ngsiv1Autocast) (#3112)

* Wed Feb 14 2018 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.11.0-1
- Reference distribution changed from RHEL/CentOS 6 to RHEL/CentOS 7
- Add: new BROKER_LOG_LEVEL variable at /etc/config/contextBroker to set log level
- Add: new GENERATE_COREDUMP variable at /etc/config/contextBroker to enable core generation and archiving (useful for debugging)
- Add: handler for SIGHUP termination signal
- Fix: using SIGTERM instead of SIGHUP for stopping broker in init.d service script
- Fix: Invalid description in POST /op/query error response (#2991)
- Fix: Bug that may cause contextBroker to crash when passing inappropriate parameter in query context payload (#3055)
- Fix: Correct treatment of PID-file (#3075)

* Mon Dec 11 2017 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.10.0-1
- Add: CORS Preflight Requests support for all NGSIv2 resources, -corsMaxAge switch, CORS exposed headers (#501, #3030)
- Fix: null not working in q/mq filter in subscriptions (#2998)
- Fix: case-sensitive header duplication (e.g. "Content-Type" and "Content-type") in custom notifications (#2893)
- Fix: bug in GTE and LTE operations in query filters (q/mq), both for GET operations and subscriptions (#2995)
- Fix: Wrong "max one service-path allowed for subscriptions" in NGSIv2 subscription operation (#2948)

* Thu Oct 19 2017 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.9.0-1
- Add: release_date and doc fields are added to the GET /version output to align with FIWARE scheme (#2970)
- Fix: missing lastSuccess/lastFailure associated to initial notification on subscription creation some times when csub cache is in use (#2974)
- Fix: several invalid memory accesses (based on a workaround, not a definitive solution, see issue #2994)
- Fix: broken JSON due to unscaped quotes (") in NGSIv2 error description field (#2955)
- Hardening: NGSIv1 responses don't use pretty-print JSON format any longer, as NGSIv2 responses work (potentially saving around 50% size) (#2760)

* Mon Sep 11 2017 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.8.0-1
- Add: self-notification loop protection, based on Fiware-Correlator and Ngsiv2-AttrsFormat headers and lastCorrelator field at DB (#2937)
- Add: Fiware-Correlator and NgsiV2-AttrsFormat headers cannot be overwritten by the custom notification logic (#2937)
- Fix: several invalid memory accesses
- Fix: bug in parseArg lib that may cause problem printing the error message for wrong CLI usage (#2926)
- Fix: bug in variable substitution of custom notifications that limited the size of the payload of a custom notification to 1024 bytes (new limit: 8MB)
- Fix: bug in custom notifications making counters and timestamps not being updated (affected subscription fields: lastSuccess, lastFailure, lastNotifiction, count)
- Fix: "request payload too large" (>1MB) as Bad Input alarm (WARN log level)
- Hardening: Several changes in argument passing in mongoBackend library to add 'const' in references to objects that are not altered by the function
- Hardening: Several changes in argument passing in mongoBackend library to avoid passing entire objects on the stack, from "X x" to "const X& x"
- Hardening: Mongo driver migrated to legacy-1.1.2 (several bugfixes in the legacy-1.0.7 to legacy-1.1.2 delta)

* Wed Feb 08 2017 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.7.0-1
- Add: HTTPS native notifications (#706), fixing at the same time issue #2844
- Add: new option to accept self-signed certifications used by HTTPS notification endpoints: -insecureNotif (#706)
- Add: metrics REST API (#2750)
- Add: new option to set the timeout for REST request: -reqTimeout (#2762)
- Add: the string "ISO6801" can now be used as an alias for "DateTime" as type for attributes and metadata (#2654)
- Add: new CLI '-disableMetrics' to disable metrics (#2750)
- Add: support for quotes (') in string filters to let attribute/metadata names contain dots (#2805)
- Add: Support for null values in string filters (#2359)
- Fix: libmicrohttpd now uses poll()/epoll() and not select() so fds greater than 1023 can be used for incoming connections (#2724, #2166)
- Fix: modified the upper limit for CLI '-maxConnections', from 1020 to UNLIMITED
- Fix: proper 500 Internal Server Error response with descriptive text when "sort fail" at DB occur (#2752)
- Fix: giving a descriptive response with payload, without reading the request for too big payloads (#2761)
- Fix: check for forbidden chars in entity type and id in POST /v2/op/query request (#2819)
- Fix: removed check for forbidden chars in URI param typePattern (#2819)
- Fix: checking for forbidden characters inside the components of string filters (#1994)
- Fix: including Service-Path in notifications also when the service path is the default one (/). For notifications due to 'standard subscriptions', notifications due to 'custom subscriptions' already work this way (#2584)
- Fix: NGSIv1 updateContext APPEND on existing geo:point attributes (and probably other geo: attributes) failing to update location (#2770)
- Fix: bug when tenants are over 44 bytes long (#2811)
- Fix: pattern matching in q/mq: pattern is always a string (#2226)
- Fix: Partial sanity check for custom url, only if the url contains no replacements using ${xxx} (#2280, #2279)
- Fix: Stricter validity check of URL before accepting a subscription (#2009)
- Fix: if entity::type is given but as an empty string, in payload of POST /v2/subscriptions or PATCH /v2/subscriptions/{sub}, an error is returned (#1985)
- Fix: more intelligent, quote-aware, operator lookup inside q/mq string filters (#2452)
- Fix: weird 'md: { }' added with some update operations (#2831)
- Fix: attribute names that include ':' now are supported, with the single-quote syntax. E.g. q='devices:location' (#2452)
- Fix: admin requests (such as GET /version) with errors now return '400 Bad Request', not '200 OK'
- Fix: attempt to create an entity with an error in service path (and probably more types of errors) now returns '400 Bad Request', not '200 OK' (#2763)
- Fix: not calling regfree when regcomp fails. Potential fix for a crash (#2769)
- Fix: wrongly overlogging metadata abscense in csub docs as Runtime Error (#2796)
- Fix: if HTTP header Content-Length contains a value above the maximum payload size, an error is returned and the message is not read (#2761)
- Add: one more semaphore (the metrics semaphore) to the semWait block in GET /statistics and to the list in GET /admin/sem (#2750)
- Fix: no longer accepting a service path with an empty component (e.g. /comp1//comp3) (#2772)
- Fix: bug in notification queue statistics, "in" value (#2744)
- Fix: Accept header errors in NGSIv2 requests were incorrectly being rendered using "OrionError" element (#2765)

* Mon Dec 05 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.6.0-1
- Add: notification failure/recovery information included in subscriptions (#1960)
- Add: support ISO8601 timezones at DateTime attributes and metadata creation/update time and filters (#2632)
- Add: support for partial representation format at DateTime attributes and metadata creation/update and filters (#2631)
- Add: New CLI option: '-logForHumans', to make the screen log output readable for humans (#2609)
- Fix: threadpool mode not actually used for custom notifications, even if -notificationMode is set so (#2622)
- Fix: -logAppend as default in configuration for RPM package (#2637)
- Fix: Default log output for stdout is now the same format as in the log file. Use new CLI option '-logForHumans' to modify this. (#2609)
- Fix: several bugs related with field validation in POST /v2/op/update operation (#2653, #2658, #2659, #2661, #2664, #2665, #2666, #2673, #2674, #2675, #2676, #2681, #2682, #2685)
- Fix: subscription PATCH not working for notification.attrs and notification.exceptAttrs subfields (#2647)
- Fix: PATCH attr wrongly adding metadata item instead of replacing (#1788)
- Fix: POST /v2/op/update with empty entities vector makes the broker to crash (#2720)
- Fix: allow empty elements in Accept header (useless, but allowed by HTTP RFC) (#2722)
- Fix: NGSIv1 queries are wrongly interpreting entity type as a pattern (#2723)
- Fix: improving performance avoiding unneeded calls to gettid in log library (#2716)

* Mon Oct 31 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.5.0-1
- Add: Added URI parameter metadata=metadata-name-list for NGSIv2 retrieve requests (#2544)
- Add: Added 'metadata' as part of the payload for POST /v2/op/query (#2544)
- Add: implement 'attrs' (and 'attributes' for POST /v2/op/query) in the same way than 'metadata', including special meaning of '*' (#2455)
- Add: metadata timestamps (dateCreated + dateModified) for attributes (as for entities), including filtering capabilities (#876)
- Add: new value for URI param 'options' (options=noAttrDetail) for NGSIv2 queries to exclude attribute detail, making the query faster (#2073) 
- Fix: wrong type in previousValue metadata for notifications triggered by PUT /v2/entities/{id}/attrs/{name}/value (#2553)
- Fix: giving a semaphore in case of mongo exception, which was causing the broker to be unable to serve requests involving the database, and possibly even to crash due to lack of resources (#2571)
- Fix: avoid unused attribute type detail in NGSIv1 queries (which was making the queries slower) (#2073)
- Fix: more accurate actionType in notifications (#2549)
- Fix: over-logging of Runtime Error traces not representing actual error (#2597)
- Fix: support attributes with double underscore as part of the name (#2453)
- Fix: GIT_REV_ORION Docker for master branch needs be master
- Deprecated: dateCreated and dateModified as valid 'options' in NGSIv2 (use 'attrs' instead) (#2455 and #2450)
- Fix: threadpool mode was not actually used, even if -notificationMode is set so; this fix solves the problem for not-custom notifications (partial fix for #2622)

* Fri Sep 30 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.4.0-1
- Add: notification metadata previousValue and actionType (except 'delete') (#2507)
- Add: filtering metadata in notifications (#2507)
- Fix: removed spurious decimals in metadata numbers in NGSIv1 rendering (a follow up of #2176)
- Fix: wrong attributes included in notifications at subscription update time in some cases (both in blacklist and not blacklist cases)
- Fix: csub cache wrong deletion of subscription at subscription update time when new subscription has empty condition attributes list
- Fix: crash when creating entity with metadata when subscription is in place with "mq" on a different metadata (#2496)
- Fix: correct rendering of metadata compound values when a vector (bugfix for issue #1068)
- Fix: crash with request with empty URI PATH (#2527)
- Fix: supporting DateTime filters for metadata (#2443, #2445)
- Fix: wrong matching in metadata existence and not existence filters with compounds
- Fix: missing expression sub-fields (q, etc.) in csubs makes Orion to crash
- Hardening: ensure to use type oriented functions when getting data from DB (safer, but more verbose in error log if DB is not perfectly aligned with what Orion is expecting)

* Fri Sep 02 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.3.0-1
- Add: support for geometry, coords and georel in NGSIv2 subscriptions (AKA geo-subscriptions) (#1678)
- Add: Type pattern for subscriptions (#1853)
- Add: New param 'mq' (both in URI param for list all entities operation and expression sub-field in subscriptions) for matching in metadata (#1156)
- Add: FIWARE::StringQuery::Metadata scope (#1156)
- Add: q-string filters for compound values of attributes, both for queries and subscriptions (#1156)
- Add: mq-string filters for compound values of metadata, both for queries and subscriptions (#1156)
- Add: accepting compound values for metadatas (#1068)
- Add: GET /admin/log operation to retrieve log level (#2352)
- Add: new operation: GET /admin/sem, to see list of the brokers semaphores (#2145)
- Fix: NGSIv2 subscriptions payload validation (#1946, #1964, #1965, #1967, #1973, #1974, #1975, #1979, #1980, #1981, #1983, #1984, #1986, #1988, #1999, #2000, #2006, #2007, #2018, #2093, #2095, #2099, #2100, #2184)
- Fix: Return error on NGSIv1 register requests with entity isPattern set to "true" (#2332)
- Fix: UTC as time in log file (#2232)
- Fix: Removed capping in NGSIv2 update forwarding (#2193)
- Fix: In some cases, compound attribute values were forwarded as empty strings (#2237)
- Fix: attribute overwritten when JSON value used in PUT /v2/entities/<eId>/attrs/<attrName>/values (#2246 and #2248)
- Fix: error returned if a NGSIv2 subscription contains no id nor idPattern (#1939)
- Fix: subscription equal filter evaluation on update context logic (#2222)
- Fix: capturing invalid id patterns at json parse time for v2 subscriptions - this bug made the broker crash when receiving invalid id-patterns (#2257)
- Fix: out-of-NGSI service not found error now using 400 Bad Request HTTP response code (instead of 200 OK). Error message has been unified to "service not found" in all cases (#1887).
- Fix: using 404 Not Found - PartialUpdate error when  NGSIv2 forwarded update is partially done (previously 204 No Content was used, without providing actual informaton about the partial update)
- Fix: CPrs responding with not NGSIv1-compliant messages was progressed to clients as 204 Not Content in NGSIv2 update forward scenarios (now responding with 404 Not Found)
- Fix: NGSIv2 "attribute already exists" error case was wrongly using 400 Bad Request, now it uses 422 Unprocessable Entity (according to rules defined in #1286)
- Fix: NGSIv2 "duplicated attribute (in request)" error case was wronly using 422 Unprocessable Entity, now it uses 400 Bad Request (according to rules defined in #1286)
- Fix: NGSIv2 "entity does not have such attribute" error case was wrongly using "requested entity has not been found" descriptions (#1890)
- Fix: missing error payload for NotFound error in 'PUT /v2/entities/<eId>/attrs/<attrName>' and 'PUT /v2/entities/<eId>/attrs/<attrName>/value' operations (#1909)
- Fix: missing error payload for TooManyResults error in 'DELETE /v2/entities/<eId>' operation (#1346)
- Fix: correctly rending as JSON error responses that were being rendered as plain text (#1989, #1991) 
- Fix: NGSIv2 "InvalidModification" errors changed to "Unprocessable" (according to rules defined in #1286)
- Fix: Using default values for types of Entities, Attributes, and Metadata aligned with the type names used by schema.org (#2223)
- Fix: Allowing trailing semicolon in a string filter, but only one (#2086)
- Fix: NGSIv2 geoquery syntax errors using 400 Bad Request instead of 422 Unprocessable Entity (according to rules defined in #1286)
- Fix: NGSIv2 URI PATH must be all in lowercase, otherwise "service not found" (#2057)
- Fix: A bug fixed in URI param 'q', avoiding false matches when using string match operator (~=) that matches AND some operator that does not match.
- Fix: no longer accepting 'keyValues-style' values for metadatas (e.g. "metadata": { "m1": 6 } )
- Fix: GET /v2/subscriptions/subId error for non existing subscriptions (now using 404 Not Found, previously using 400 Bad Request)
- Fix: 472 Unknown returned as HTTP status code by POST /v2/op/update in some cases (now using 404 Not Found).
- Fix: Correctly returning error on present but empty attribute types in PATCH v2/entites/{id} operation (#1785)
- Fix: Error returned on encountering non-existing attributes in PATCH /v2/entities/<id>/attrs (#1784)
- Fix: Fixed a bug about string lists in URI parameter 'q' (strings treated as numbers, if possible. E.g. '123' => 123)
- Fix: use X-Real-IP header as preferred option for from= field in log traces (#2353)
- Fix: use op= instead of function= in log traces (along with moving the file/line identification from msg= to op=) (#2353)
- Fix: reorder field in log traces lo align with IoTP Operations requirements (#2353)
- Fix: using WARN log level instead of WARNING log level to align with IoTP Operations requirements (#2353)
- Fix: Fixed metadata rendering in NGSIv1, so that the JSON data type is taken into account and not empty string is returned for non-string values (#1068, just a part of it)
- Fix: Fixed a bug about lists in URI parameter 'q' (last list item was skipped)
- Fix: render float values with 9 digits precision in all cases, avoding spurious decimals (#2207, #2176, #2383)
- Fix: Error payload returned on encountering non-existing entity/attribute in PUT /v2/entities/<entity-id>/attrs/<attr-name> (#1360)
- Fix: Added citation marks surrounding strings as response in text/plain.
- Fix: Return NGSIv2 error if citation-mark is missing in PUT /v2/entities/{id}/attrs/{attrName}/value payload for plain text (previously it was NGSIv1 error format) (#2386)
- Fix: invalid regex patterns detection (#968)
- Fix: Better error returned on invalid geoquery (#2174)
- Fix: Better error returned on attribute not found in GET /v2/entities/<entity-id>/attrs/<attr-name>/value (#2220)
- Fix: Metadata identification based on just name instead of name+type (metadata representation at DB changed from vector to object)  (#1112)
- Fix: Bug in log level configuration via REST fixed, and log levels in broker modified (#2419)
- Fix: Distinguish between strings and numbers in q string filter (#1129)
- Fix: proper response for 'GET /v2/entities/<id>?attrs=<attrName>' and 'GET /v2/entities/<id>/attrs?attrs=<attrName>' when <attrName> doesn't exist (#2241)
- Fix: Rendering problems in metadata vector (#2446)
- Fix: correct error response for missing value in URI param in /admin/ requests (#2420)
- Fix: In error responses for ngsi9 registrations, the duration is no longer rendered.
- Fix: Do not allow forbidden chars in description for subscription (#2308)
- Fix: Correct error returned when trying to replace a non-existing attribute in PUT /v2/entities/<entity-id>/attrs/<attr-name> (#2221)
- Fix: Empty string detection for entity id, entity type and attribute name in NGSIv2 request URLs (#1426, #1487)
- Fix: Trailing slashes in URI path no longer silently removed, but considered and giving adequate error responses
- Fix: Initial notifications were counted twice when running with subscription cache (which gave erroneous timesSent in GET /v2/subscriptions/<subId>)
- Fix: Avoid rendering at the same time id and idPattern fields in NGSIv2 subscriptions
- Fix: Error returned on GET/DELETE requests specifying Content-Type (#2128)
- Fix: Error responses with '405 Method Not Allowed' now return payload to describe the error, for v2 requests only (#2075, #2078 and #2083)
- Fix: Implemented more sofisticated HTTP Accept Header handling (Issues #1037, #1849, #1886, #2175, and #2208)
- Hardening: csubs data model simplification, using a list of attributes for "conditions" instead of a complex structure (#1851)

* Thu Jun 02 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.2.0-1
- Add: notification attributes blacklist for NGSIv2 notifications (#2064)
- Add: NGSIv2 custom notifications (#2015)
- Add: -disableCustomNotification CLI to disable custom notification functionality (#2135)
- Add: 'values' option for GET /v2/types operation (#1170)
- Add: ':' as alias for '==' in the NGSIv2 Simple Query Language (#1903)
- Fix: NGSIv2 header name change: X-Total-Count -> Fiware-Total-Count (#2124)
- Fix: NGSIv2 header name change: X-Ngsiv2-AttrsFormat -> Ngsiv2-AttrsFormat (#2124)
- Fix: crash in multi-token query string when attribute value regex comes in first place (#2164)
- Fix: crash when updating entities which corresponding document at DB is somehow large (#2112)
- Fix: several fixes related with 'attrsFormat' subscription field in NGSIv2 API (#2133, #2163)
- Fix: bug disabling throttling when creating a v2 subscription (#2114)
- Fix: wrong text payload when subscription string filter error occurs (#2106)
- Fix: total count return in GET /v2/types operation (#2115)
- Fix: -noCache now avoids populating cache on create/update subscription operations (#2147)
- Fix: accessing to csub cache in update subscription logic to update count and lastNotification without taking semaphore (#2146)
- Fix: potential bug in internal logic at NGSIv2 GET subscription operations, possibly provoking crashes
- Fix: New keyword "DateTime" to replace "date" for attributes that express date/time (#2198)
- Fix: Improved error text "ContentLengthRequired" to replace "LengthRequired" (#2198)
- Fix: POST /v2/op/query was not supporting FIWARE::Location::NGSIv2 filters
- Fix: Expression lost after refreshing subscription cache (#2202)
- Fix: Avoided a possible infinite loop in sub-cache synchronization function (subscriptions stopped working due to this bug)
- Deprecated: /ngsi10 and /ngsi9 as URL path prefixes
- Fix: throttling set to zero (#2030)

* Tue May 03 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.1.0-1
- Add: NGSIv2 notifications (#1875)
- Add: ~= operator to simple query language in order to implement attribute value patterns (#978)
- Add: string support in <, >, <=, >= and range filters (#1945)
- Add: support for geo:line, geo:box and geo:polygon types as ways of specifying entity location in NGSIv2 (#1038)
- Add: GeoJSON support as a way of specifying entity location in NGSIv2 (#1920)
- Add: Add permanent subscriptions (#1949)
- Add: pause subscriptions using the NGSIv2 API (#1328)
- Add: description field for NGSIv2 subscriptions (#1918)
- Add: change/add path /entities/{id}/attrs (#2028)
- Add: Correlator as HTTP header and in log file (#1916)
- Fix: updates on default service path entity causes wrong update in same id/type entity in not-default service path (#1956)
- Fix: crash with some invalid subscriptions in NGSIv2 API (#1947, #1972)
- Fix: csub cache sync logic not persisting lastNotification when the field doesn't initially exist in DB
- Fix: csub cache at mongoBackend update context subscription logic wrongly setting expiration with current time, ignoring actual expires/duration in the request
- Fix: GET /v2/types response payload, from key-map to array (#1832)
- Fix: GET /v2/types response payload, now using array for attribute types (#1636)
- Fix: using "/#" as default service path instead of "/" in subscriptions (#1961, #2024)
- Fix: POST /v2/subscriptions, attributes in notification are optional (#1952)
- Fix: no longer considering service (tenant) for subscriptions when broker is started without '-multitenant' CLI option (#1898)
- Fix: NGSIv2 subscription payload (#2014)
- Fix: right X-Total-Count in GET /v2/types operation (#2046)
- Fix: right 404 Not Found response in GET /v2/types/{type} operation when type doesn't exist (#2056)
- Fix: 'null' attribute value not surrounded by quotes (") in NGSIv1 renderings (#2058)
- Fix: non-string attribute values were breaking JSON in attributeFormat=object mode (#2058)
- Fix: JSON response in log management operations
- Fix: capturing invalid URLs at subscription time (and returning error response) that otherwise would make Orion crash (#2092)
- Deprecate: "location" metadata to specify entity location

* Tue Mar 29 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 1.0.0-1
- Add: new value for URI param 'options': unique - like 'values', but avoiding duplicates (Issue #1050)
- Add: PUT /admin/log?level=XXX to change log level in run-time (Issue #1913)
- Add: check min length for IDs in v2 (Issue #1836)
- Fix: over-logging at error level due to not checking field existente in BSON objects
- Fix: aggregating entities without type and type "" in same entry at GET entity types operation 
- Fix: DB error relog done at ERROR level (previously it was using WARNING)
- Fix: check for invalid chars in attribute type (Issue #1847, #1844)
- Fix: check for field too long in attribute type (Issue #1845)
- Fix: check for metadatas with empty name (Issue #1438)
- Fix: accepting backslash-slash in JSON v1 payload (Issue #1852)
- Fix: PATCH /v2/entities correct interpretaion of null value and missing type in payload
- Fix: PATCH /v2/entities missing value field (Issue #1789)
- Fix: GET /v2/entities/id/attrs/name/value for type date (Issue #1885)
- Fix: PUT /v2/entities/id/attrs/name/value forbidden chars (JSON) (Issue #1905)
- Fix: PUT /v2/entities/id/attrs/name/value changing type to 'none' (text/plain) (Issue #1904)
- Hardening: use safeMongo get*Field() methods in places not previously using them (mainly in csubs cache library)
- Remove: deprecated functionality related with ONTIMEINTERVAL subscriptions
- Remove: XML support (deprecated functionality) (Issue #1862)

* Mon Feb 29 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.28.0-1
- Add: implemented new operation: POST /v2/op/query (Issue #1080)
- Add: implemented new operation: POST /v2/op/update (Issue #1715)
- Add: orderBy URI param in NGSIv2 queries to sort entities by attribute, entity dates or proximity (Issue #1103)
- Add: orderBy URI param in NGSIv1 queries to sort entities by attribute or entity dates (Issue #1103)
- Add: options=values mode for entity queries in NGSIV2 (Issue #1049)
- Add: NGSIv2 URI param 'georel' (along with 'geometry' and 'coords') proper support (Issue #1677)
- Add: scope FIWARE::Location::NGSIv2 to allow using NGSIv2 geo-queries also with NGSIv1 (Issue #1677)
- Add: support for geo:point type as a way of specifying location attribute in NGSIv2 (Issue #1038)
- Add: date support in attribute values and q filters (Issue #1039)
- Add: dateCreated and dateModified options to get entity creation and modification times as "virtual" attributes (Issue #876)
- Add: ?type param for GET entity in v2 (Issue #915, #972, #990, #998)
- Add: ?type param for DELETE entity in v2 (Issue #986, #994)
- Add: ?type param for PATCH entity in v2 (Issue #980)
- Add: ?type param for POST entity in v2 (Issue #982, #984)
- Add: ?type param for PUT entity in v2 (Issue #988, #992, #1000)
- Add: ?type URL parameter in Location header upon entity creation in NGSIv2 (Issue #1765)
- Fix: error level traces ignoring -logLevel NONE
- Fix: wrong over-logging at error level updating attributes having metadata without type 
- Fix: '+' supported in entity ids and names in URLs (Issue #1675)
- Fix: libmicrohttpd 0.9.48 included in contextBroker as static lib (previous Orion versions used 0.9.22 as dynamic library) (Issue #1675)
- Fix: list of attribute names in URI param 'type' (Issue #1749)
- Fix: long servicepath component in NGSIv2 (Issue #1423, #1764, #1774)
- Fix: syntax change in string query 'q' for exist and not-exist (Issue #1751)
- Fix: sanity check for string query 'q' - detect 'left-hand-side missing' (Issue #1754)
- Fix: more sanity checks for string query 'q' (q empty, parts of 'q' empty - parts of 'q' are separated by ';')
- Fix: error message when updating attribute value for two entities with same id (Issue #1387)
- Fix: bug causing false not-a-number when it really is a valid number (very very rarely)
- Fix: not detecting forbidden chars in entityID for PATCH v2 (Issue #1782)
- Fix: detect forbidden chars in entity ids and attr names in URI (Issue #1793)
- Fix: segfault caused by parameter without value in query string (Issue #1805)
- Fix: some of the operations reported an incorrect 'Allow' HTTP Header on Bad Verb, now fixed
- Fix: Returning 422 InvalidModification instead of 404 NotFound when POSTing entity (Issue #1817)
- Fix: using string "none" as default entity/attribute/metadata type in NGSIv2 (Issue #1830)
- Hardening: sanity checks for numbers (Issue #1306)

* Mon Feb 01 2016 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.27.0-1
- Add: proper alarm management, including activation/deactivation in logs (Issue #1582)
- Add: Enable log summary traces using -logSummary CLI (Issue #1585)
- Add: New CLI parameter '-relogAlarms' to see ALL possible alarm-provoking failures in the log-file
- Add: subscriptions triggered by modifications in any attribute without explicitly list them (aka ONANYCHANGE) (Issue #350)
- Add: 'q' expression evaluation in NGSIv2 subscription (at subscription creation/update and update context times) (Issue #1316 and #1658)
- Add: srv=, subsrv= and from= fields to log (Issue #1593)
- Add: POST /v2/subscriptions operation (Issue #1316)
- Add: DELETE /v2/subscriptions operation (Issue #1654)
- Add: PATCH /v2/subscriptions/{subId} operation (Issue #1658)
- Add: reset for notification counters in threadpool mode
- Add: id fields checking for NGSIv2 API and (if -strictNgsiv1Ids is enabled) NGSIv1 API (#1601)
- Fix: entity/attribute operations to align them with the last JSON representation format defined for NGSIv2 (Issue #1259)
- Fix: avoid rendering invalid JSON characters in response payloads (Issue #1172)
- Fix: avoid escaping / in callback field in GET /v2/subscriptions and GET /v2/subscriptions/<subId> operations
- Fix: wrong accumulation in counter sentOK
- Fix: more descriptive error messages for CPr update/query forward fail
- Fix: avoid -g compiler flag in release build
- Fix: supporting decimal values for seconds in ISO8601 strings (Issue #1617)
- Fix: enforcing "http" or "https" schema and better detection of missing port at URLs parsing (Issue #1652)
- Fix: fixed wrong interpretation of empty string keyvalues as empty objects in compounds (Issue #1642)
- Fix: subscription service path is lost at update subscription time (Issue #1693)
- Fix: Mongo driver migrated to legacy-1.0.7 (to get the fix for https://jira.mongodb.org/browse/CXX-699) (Issue #1568)

* Wed Dec 09 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.26.1-1
- Default -subIvalCache changed to 60 seconds
- Add: servicePath header filtering in GET /v2/subscriptions operation (Issue #1557)
- Add: New command-line-option '-logLevel <level>', levels: NONE, ERROR, WARNING, INFO, DEBUG (Issue #1583)

* Tue Dec 01 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.26.0-1
- Add: Add queue+threads for notifications (notificationMode thread:<queue size>:<num threads>) (Issue #1449)
- Add: possibility to limit the maximum number of simultaneous connections, using the CLI option -maxConnections (Issue #1384)
- Add: possibility to use thread pool for incoming connections, using CLI option -reqPoolSize (Issue #1384)
- Add: Unpatterned subscriptions now also in subscription cache (Issue #1475)
- Add: Built-in Timing/Profiling (Issue #1367)
- Add: clearer statistics in GET /statistics and GET /cache/statistics operations
- Add: Simulated/drop notifications mode is now orthogonal to -notificationMode (Issue #1505)
- Add: finer-grain statistics switches: -statCounters, -statSemWait (old -mutexTimeStat), -statTiming and -statNotifQueue
- Fix: avoid indirect usage of DB connections concurrently due to cursors (Issue #1558)
- Fix: safer treatment of database fields lastNotification, expiration and throttling in 
       the 'csub' collection, assuming that the field can be either int or long (it is always written as long)
- Fix: number/bool correct rendering in NGSIv1 compound values
- Fix: ONCHANGE notifications triggered by update context operation are filled avoiding querying entities collection (#881)
- Fix: wrong notification values (duplicated) when triggering update context operations are too close (race condition)
- Fix: A bug in the request reading logic that may cause unexpected Parse Errors or even SIGSEGVs
- Fix: Semaphore releasing bug in unsubscribeContext when subscription ID is the empty string (Issue #1488)
- Fix: Fixed an uptil now unknown bug with throttling
- Fix: Only 1 query at csubs collection/cache is done per context element processing at updateContext (previously one query per attribute were done) (Issue #1475)
- Fix: 'count' and 'lastNotificationTime' now maintained by subCache (and synched via DB) (Issue #1476)
- Fix: updates including several attributes with the same name are now reported as InvalidModification (Issue #908)
- Fix: no longer adding subscriptions from all tenants in the cache if broker isn't multitenant (Issue #1555)
- Fix: resetting temporal counters at synching the subscription cache (Issue #1562)
- Fix: using the text "too many sbuscriptions" at cache statistics operations in the case of too many results
- Fix: crash due to subscription ID not conforming to supposed syntax in request "GET /v2/subscriptions/XXX" (Issue #1552)
- Hardening: exhaustive try/catch (mainly at mongoBackend module) 
- Deprecated: ONTIMEINTERVAL subscriptions

* Mon Nov 02 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.25.0-1
- Add: NGSIv2 operation GET /v2/subscriptions (#1126)
- Add: NGSIv2 operation GET /v2/subscription/<id> (#1317)
- Add: URI params 'geometry' and 'coords' for GET /v2/entities (#1177)
- Add: CLI argument -notificationMode. Default mode is 'transient' (previous version used 'permanent' implicitly) (#1370)
- Add: CLI argument -connectionMemory for connection memory limit. Default value is 64Kb (#1384)
- Add: CLI argument -noCache to disable cache (recovering $where from 0.23.0 code base to search for subscriptions always in DB)
- Add: githash in --version (#1363)
- Add: contextBroker standard error log in RPM init script (#1175)
- Fix: broken subscription cache (#1308), including the semaphore for subscription cache (#1401)
- Fix: conv op to std op mapping wrongly using patterns in some GET operations (#1322)
- Fix: using 'count' in all paginated queries in NGSIv2 (get entities list was missing) (#1041)
- Fix: add charset=utf-8 for notifications (#1340)
- Fix: incorrect 'details' field rendering (leading to illegal JSON) for error responses in some cases (#843)
- Fix: escaping dot (.) in attrNames field in entities collection in DB (#1323)
- Fix: compound attribute values support in CPr update forwarding (#1440)
- Fix: traces to stdout only if CLI '-fg' is set (#1272)
- Fix: avoid noisy error output in RPM init script (#309)
- Fix: performance problem in NGSIv2 API due to uncontrolled log trace printing
- Fix: trying to update several matching entities associated to the same ID in NGSIv2 API now returns in TooManyResults error (#1198)
- Fix: more accurate text for details field in zero content-length error responses (#1203)
- Fix: description text for parse errors in NGSIv2 JSON requests (#1200)
- Fix: leftover whitespaces in NGSIv2 error names (#1199) 
- Fix: clearer error on NGSIv2 entity attribute update, without value in payload (#1164)
- Fix: changed  error code from 472 (Invalid Parameter) to 422 (Invalid Modification) in NGSIv1 in the case of missing attribute value (#1164)
- Fix: return error response in NGSIv2 API when the combined length of id/type/servicePath exceeds (mongo) index key length limit (#1289)
- Fix: error responses for NGSIv2 PATCH request on entity without payload (#1257)
- Fix: clearer errors for 'entity not found' and 'conflict too many results' on NGSIv2 update PATCH (#1260)
- Fix: update PATCH request with invalid service-path never returned a response (#1280)
- Fix: error must be Not Found, when updating an unknown attribute of an existing entity on NGSIv2 update PATCH (#1278)
- Fix: POST /v2/entities returns error if the entity to create already exists (#1158)
- Fix: error description for PUT/POST/PATCH request with empty JSON object as payload in NGSIv2 (#1305)
- Fix: improve error description in NGSIv2 PUT on entity, when entity is not found or there is a conflict due to many results (#1320)
- Fix: empty/absent attribute value support, both in NGSIv1 and NGSIv2 (#1187, #1188 and #1358)
- Hardening: mongoBackend checking for field existence and proper type to avoid broker crashes due to DB corruption (#136)
- Hardening: safer input/ouput logic for csub documents in MongoDB and cache
- Remove: proxyCoap binary from RPM (#1202)
- Remove: deprecated command line arguments -ngsi9, -fwdHost and -fwdPort
- Remove: deprecated functionality related with associations

* Mon Sep 14 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.24.0-1
- Add:  FIWARE::StringQuery scope, implementing filtering capabilities (equal, unqual, greater/less than, ranges, existence) (Issue #864)
- Add:  APPEND_STRICT action type for POST /v1/updateContext operation
- Add:  REPLACE action type for POST /v1/updateContext operation
- Add:  Implemented a RAM-cache for patterned ngsi10 subscriptions (Issue #1048)
- Add:  API version 2 only supports JSON so we can now distinguish between STRINGS, NUMBERS (all treated as floats for now),
        and BOOLEANS (true, false), for the VALUE for ContextAttribute and Metadata (No Issue)
- Add:  POST /v2/entities - entity creation for API version 2 (Issue #970)
- Add:  POST /v2/entities/{entityId} - append or update attributes (by entity ID) for API version 2 (Issue #981)
- Add:  URI parameter "op=append", for POST /v2/entities/{entityId} (Issue #983)
- Add:  DELETE /v2/entities/{id} - delete an entity (Issue #985)
- Add:  Number and boolean rendering for attribute simple values (not compounds) in /v1 operations for JSON encoding
- Add:  URI parameter "attrs=a1,a2,a3..." for GET /v2/entities/{entityId} (Issue #971)
- Add:  PUT /v2/entities/{entityId} - replace attributes (by entity ID) for API version 2 (Issue #987)
- Add:  GET /v2/entities/{entityId}/attrs/{attrName}/value - get a single attribute value (Issue #997)
- Add:  URI parameters id, idPattern and type for GET /v2/entities (Issues #952 #953 #969)
- Add:  PATCH /v2/entities/{entityId} - modify an entity (Issue #979)
- Add:  URI parameter 'q' for GET /v2/entities (Issue #967)
- Add:  PUT /v2/entities/{entityId}/attrs/{attrName}/value (Issue #999)
- Add:  DELETE /v2/entities/{entityId}/attrs/{attrName} (Issue #993)
- Add:  PUT /v2/entities/{entityId}/attrs/{attrName} (Issue #991)
- Add:  GET /v2/types/{type} (Issue #1028)
- Add:  GET /v2/types (Issue #1027)
- Add:  text/plain encoding for GET /v2/entities/{entityId}/attrs/{attrName}/value operation (both Accept header and ?options=text) (Issue #1179)
- Add:  monit_log_processing.py script to contextBroker RPM (Issue #1083) 
- Add:  httpsPrepare.sh script to contextBroker-tests RPM (Issue #2)
- Fix:  Fixed a bug about generic error handling for API version 2 (Issue #1087)
- Fix:  Fixed a bug about error handling of invalid Service-Path for API version 2 (Issue #1092)
- Fix:  Check for illegal characters in URI parameters (name and value) (Issue #1141)
- Fix:  Checking for forbidden chars in many fields of JSON payload for API version 2 (Issue #1093)
- Fix:  Bug fix for receiving compound metadata values (which is not supported) (Issue #1110)
- Fix:  Error handling for non-string entity::id/type (Issue #1108), attribute::type, and metadata::type/value
- Fix:  Metadata updates involving boolean/number values were not correctly processed (Issue #1113)
- Fix:  Removed the check of forbidden characters for the values of the URI parameter 'idPattern'
- Fix:  Made broker respond with 400 Bad Request on encountering invalid items in URI parameter 'options' (Issue #1169)
- Fix:  Several older memory leaks has been fixed thanks to improvements in leak-detecting tool

* Fri Jul 10 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.23.0-1
- Add: Reuse curl context for outgoing notifications, so now connections are persistent if the HTTP server doesn't close them
- Add: CLI paramter -cprForwardLimit to cap the number of forwarding request for a single client request. (Issue #1016).
- Fix: Fixed a bug about pagination problem due to missing ErrorCode with details in the case of queries with forwarding to context providers.  (Issue #945)
- Fix: Taking into account the pagination limit before adding "potential" entities to be queried in CPrs. (Issue #945)
- Fix: Add multiservice flag for the default configuration in the RPM. (Issue #964)
- Fix: ONCHANGE subscription sends notification when value under condition it is not updated with updateContext-APPEND in some cases (Issue #943)
- Fix: Updating attribute metadata without updating attribute value at the same time is now supported (Issue #766)
- Fix: Remove unconditional tracing on xmlTreat()/jsonTreat() which was impacting on performance (specially in high-load scenarios)
- Fix: Use NOSIGNAL as libcurl option to avoid crashes in that library. (Issue #1016)
- Add: New operation "GET /v2/entities" (Issue #947)
- Add: New operation "GET /v2" (Issue #956)
- Add: New operation "GET /v2/entities/{id}" (Issue #950)
- Add: New operation "GET /v2/entities/{entityId}" (Issue #950)
- Add: New operation "GET /v2/entities/{entityId}/attrs/{attrName}" (Issue #989)

* Mon May 25 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.22.0-1
- Add: CORS support for GET requests, configuring allowed origin with -corsOrigin CLI parameter (Issue #501)
- Add: New CLI parameter for the mutex policy: -reqMutexPolicy (Issue #910)
- Add: Measuring the accumulated time waiting for the DB semaphores.
       The measures are added to the REST request /statistics, but only if
       the new CLI parameter -mutexTimeStat is set (Issue #911)
- Fix: Removing trailing slashes for path of URI before treating the request (Issue #828)
- Fix: Fixed a bug regarding subscriptions with empty entity-type (Issue #691)
- Fix: updateContext propagated to the entire Service Path subtree, instead of just scoping to the particular Service Path in the Fiware-Service header (Issue #885)
- Fix: Mongo driver migrated to legacy-1.0.2
- Add: Database connection pool for mongo. Default size of the pool is 10 connections. This is changed using the CLI parameter -dbPoolSize (Issue #909)
- Add: New CLI parameter for Mongo write concern: -writeConcern (Issue #889)

* Sun May 10 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.21.0-1
- Add: support for MongoDB 2.6/3.0 (MongoDB 2.4 should *not* be used any longer as inverted geo-queries will not work in that version) (Issue #415)
- Add: context providers full functionality (Issue #787)
- Fix: unhardwire format to forward requests to context providers (Issue #722)
- Fix: using proper GJSON for location.coords entities information (Issue #415)
- Fix: attribute storing in DB (from vector to keymap) to enable CB Active/Active architectures. As a consecuence of this, '.' in the name of attributes is now stored as
       '=' in the key (as MongoDB doesn't allow to use '.' for keys). This is transparant from the point of view of the user, given that '=' is a forbidden character in
       the external API (Issue #878)
- Fix: simplifying 'GET /v1/contextTypes' and 'GET /v1/contextTypes/<entityType>' operations, removing attribute types (no longer used as attribute identification since 0.17.0). As
       a consequence, the 'attributeFormat' URI parameter is no longer needed in the 'GET /v1/contextTypes', so it is ignored.
- Fix: DELETE updates are resolved locally, previously they were forwarded to CPrs (Issue #755)
- Fix: The URI parameter 'notifyFormat' is now treated the same way for subscriptions as for registrations.
- Add: Notifications and forwarded queries/updates are now controlled by a timeout. (Issue #880)
       The default timeout is set to 5000 milliseconds (5 seconds), including connection, writing the request and reading the response.
       To change this timeout, a new CLI option '-httpTimeout' has been added to the broker.
- Fix: Changed the name, type and unit of the CLI option '-timeout'.
       New name:  '-dbTimeout',
       New type:  integer (was a floating point number)
       New unit:  milliseconds (was seconds, but decimals were allowed as it was a floating point number)
- Add: Updates of ngsi9 subscriptions now also support the URI parameter notifyFormat

* Tue Mar 31 2015 Fermin Galan <fermin.galanmarquez@telefonica.com> 0.20.0-1
- Add: new CLI parameter '-timeout' to specify DB connection timeout in the case of using MongoDB replica sets (Issue #589).
- Fix: All convenience operations to use the service routines of standard operations (Issue #117)
- Fix: Correct rendering of response to convop "POST /v1/contextEntities/{entityId::id}/attributes/{attributeName} (Issue #772)
- Fix: Made 'POST /v1/contextEntities/{entityId::id}' and 'POST /v1/contextEntities' accept entity::id and entity::type
       in both payload and URL, as long as the values coincide.
- Fix: A few more of the 'responses without entityId::type filled in' found and fixed (Issue #585)
- Fix: The convenience operations "/v1/contextAttributes/{entity::id}/attributes" now share implementation with
       convenience operations "/v1/contextAttributes/{entity::id}" and are thus 100% identical.
- Fix: The convenience operation "PUT /v1/contextAttributes/{entity::id}/attributes/{attribute::name}" had a bug
       about the metadata in the update, this error has been fixed.
- Fix: A registerContext request that uses associations must specify "attributeAssociations" instead of "attributes" to be consistent with DiscoveContextAvailability messages (Issue #823).
- Fix: Fixed a bug that made the broker crash when compound attribute values were used in convenience operations.
- Fix: Implemented URI-parameter 'notifyFormat=XML/JSON' for NGSI9 subscriptions.

* Wed Feb 11 2015 Fermin Galan <fermin@tid.es> 0.19.0-1 (FIWARE-4.2.2-1)
- Fix: The option '-multiservice' was ignored and the broker ran always in multiservice mode. Now the option is taken taken into account (Issue #725)
- Fix: Forwarded requests always in XML (Issue #703)
- Fix: internal substitution of servicePath defaults ("/" for update-like operations, "/#" for query-like operations)
- Fix: Temporal hack to allow for forwarding of 'attribute not found in entity found' (Issue #716)
- Fix: Broker crashes dealing with "GET /v1/contextTypes" operation when DB contains entities of the same type, ones with attributes and others without them (Issue #740)
- Fix: Service-Path supported for registrations and discoveries (Issue #719)
- Fix: Service-Path supported for forwarding of registrations between Config Manager and Context Broker (Issue #719)
- Fix: Service-Path supported for forwarding to Context Providers (Issue #719)
- Fix: Hash sign (#) in service-path, or multiple service-paths not valid for updateContext requests (Issue #746)
- Fix: Not forwarding the default Service-Path to context providers (Issue #714)

* Fri Jan 16 2015 Fermin Galan <fermin@tid.es> 0.18.1-1 (FIWARE-4.2.1-1)
- Fix: Fixed the bug about recovering ONTIMEINTERVAL subscriptions (Issue #693)
- Fix: Fixed the crash in queryContext with an invalid geoscope (Issue #690)
- Fix: Protect the broker against payload-less responses to forwarded requests (Issue #699)
- Fix: Fixed bug that prevented types from entities with no attributes from being listed (Issue #686)

* Fri Dec 19 2014 Fermin Galan <fermin@tid.es> 0.18.0-1 (FIWARE-4.1.3-1)
- Add:  Service-Path in ngsi10 notifications (Issue #674)
- Add:  Forbidden characters in the URL path are now detected (Issue #672)
- Add:  X-Auth-Token forwarded in ngsi10 notifications (Issue #673)
- Add:  Service-Path is taken into account in ngsi10 subscriptions (Issue #675)
- Fix:  Bug: POST /v1/conextEntities didn't save the entity type (from payload) (Issue #684)
- Fix:  Service-Path is taken into account in 'GET /v1/contextTypes' and 'GET /v1/contextTypes/{entityType}' operations (Issue #676)

* Fri Nov 28 2014 Fermin Galan <fermin@tid.es> 0.17.0-1 (FIWARE-4.1.2-1)
- Add: New convop: "POST /v1/contextEntities" (Issue #613).
       Note also that AppendContextElementRequest has been added an EntityId field.
- Add: Convenience operations that respond with AppendContextElementResponse now get the
       EntityId info included in the response.
- Add: New name for URI param 'attributesFormat': 'attributeFormat' (better English).
       The old name will be supported as well. (Issue #633)
- Add: Fiware-ServicePath '#' syntax for including path children in the query (without '#' only the service path itself is included). (Issue #646)
- Add: "/" is used as implicit service path for entities. (Issue #646)
- Add: Queries without Fiware-ServicePath are resolved on "/#" implicitly (i.e. all the paths). (Issue #646)
- Fix: Attribute type is no longer used as attribute "identification key" (from now on, only name is used for that purpose) (Issue #640)
- Fix: Changed max-length of tenant names from 20 characters to 50.
       Also the database name (CLI-option '-db') has been given a maximum length of 10 bytes. (Issue #616)
- Fix: No longer responding with 'Service not found: $URL'. (Issue #619)
- Fix: Requests with payload containing forbidden characters are now rejected. (Issue #619)
- Fix: Fixed a bug that made the broker crash on problems with EntityId during XML-parse (in very rare situations).
- Fix: 'WSG84' identifier changed to 'WGS84' (it was a typo), although the old one is still supported to ensure backward compatibility. (Issue #627)
- Fix: Fixed a leak for each forwarding of messages to Context Providers.
- Fix: Changed max-length of service path elements from 10 characteres to 50. (Issue #649)
- Fix: Service path is no longer ignored at entity creation time for entities without type. (Issue #642)
- Fix: The broker crashed on receiving compounds for some convops, e.g. /v1/contextEntities/{entityId} (Issue #647)
- Fix: Using 443 as default port for "https" notifications. (Issue #639)
- Fix: Fixed RPM package so PID file is stored in /var/run/contextBroker instead of /var/log/contextBroker. (Issue #612)
- Fix: Payload no longer accepts 'operator' as part of a Scope. (Issue #618)
- Fix: Made compound attribute values for for convenience operations (Issue #660)

* Mon Nov 03 2014 Fermin Galan <fermin@tid.es> 0.16.0-1 (FIWARE-4.1.1-1)
- Add: Adding alternative (preferred) URLs: '/ngsi10' => '/v1' AND '/ngsi9' => '/v1/registry' (Issue #559)
- Add: New convenience operation '/v1/contextEntities' - to get a list of ALL existing entities (Issue #563)
- Add: New scopeType FIWARE::Filter::Existence (only with value "entity::type" in this version)
- Add: New URI parameter '!exist=entity::type' to filter entities whose type is empty (Issue #570)
- Add: New conv op URI parameter 'entity::type=TYPE' to filter by type (Issue #570)
- Add: New convenience operations PUT/POST/GET/DELETE /v1/contextEntities/type/<type>/id/<id> (Issue #581)
- Add: New convenience operations PUT/POST/GET/DELETE /v1/contextEntities/type/<type>/id/<id>/attributes/ATTRIBUTE_NAME (Issue #581)
- Add: New convenience operations PUT/POST/GET/DELETE /v1/contextEntities/type/<type>/id/<id>/attributes/ATTRIBUTE_NAME/META_ID_VALUE (Issue #581)
- Add: New convenience operations POST/GET /v1/registry/contextEntities/type/<type>/id/<id> (Issue #581)
- Add: New convenience operations POST/GET /v1/registry/contextEntities/type/<type>/id/<id>/attributes/ATTRIBUTE_NAME (Issue #581)
- Fix: Upgrading from a previous RPM package caused the orion user to disappear.
- Fix: Change service path HTTP header name: 'ServicePath' -> 'Fiware-ServicePath' (Issue #541)
- Fix: Avoid unneeded additional query on csubs/casubs collections to get details of ONCHANGE triggered subscription when processing updateContext (Issue #371)
- Fix: Some convenience operations responsed always with empty type (Issue #585)
- Fix: 'FIWARE_Location' scope type changed to 'FIWARE::Location' (although the old one is still supported)
- Fix: More detailed error responses for some convenience operations:
       - POST/DELETE /v1/contextEntities/EID/attributes/ATTR_NAME
       - POST/DELETE /v1/contextEntities/type/TYPE/id/ID/attributes/ATTR_NAME
       Also the POST variant of these two URLs always returned 200 OK. Not anymore.

* Wed Oct 01 2014 Fermin Galan <fermin@tid.es> 0.15.0-1 (FIWARE-3.5.3-1)
- Add: context providers basic functionality (Issue #499)
- Add: alternative rendering of ContextAttribute for JSON (Issue #490)
- Add: new API operation to retreive all existing context entity types (Issue #519)
- Add: new API operation to retreive all attributes (union set) of a given context entity type (Issue #519)
- Add: support for associations in JSON (Issue #334).
- Add: support for MongoDB replica sets (using the new -rplSet CLI parameter) (Issue #493)
- Fix: Associations now return error messages when appropiate (Issue #334).
- Fix: each send is now considered a separate transaction (Issue #478)
- Fix: lseek after each write in log file in order to work with logrotate truncate (Issue #411)
- Fix: made the broker not support URL-based multiservice (tenants) anymore (Issue #522)
- Fix: removed all the code related to URL-tenants (Issue #522)

* Fri Aug 01 2014 Fermin Galan <fermin@tid.es> 0.14.1-1 (FIWARE-3.5.1-1)
- Fix:  errors in JSON rendering for '/statistics' and '/version' fixed (Issue #428).
- Fix:  using same location attribute in UPDATE fails (issue http://stackoverflow.com/questions/24431177/ge-orion-context-broker-when-we-make-an-update-of-the-entity-does-not-allow-us)
- Fix:  Context Broker no crashed anymore due to different tenant names but equal in insensitve case (workaround in Issue #431).
- Logging modifications (Issue #428):
   o The name of the log file has been changed from contextBrokerLog to contextBroker.log
   o Changed the log line format to use a key-value approach
   o Added LM_I, for transactions. Each time a new transaction is initiated, an LM_I is issued.
     Upon terminating each transaction, another LM_I is issued to reflect this.
   o All log lines contain the id of the current transaction, or N/A if no transaction is in progress
   o Changed the time reference to localtime (previously GMT was used)
   o Stopped using LM_RE, LM_V* and _LM_RVE and removed tho CLI options -v, -vv, -vvv, -vvvv, -vvvvv
   o Added an LM_I for when thr broker starts and another when it exits (in a controlled manner).
- The broker now uses libcurl for outgoing HTTP communications.
- Fix: double-quotes in output payload have been escaped (Issue #456)
- Fix: Added tests to make sure that latitude and longitude are within range
       (-90 <= latitude <=90)  and (-180 <= longitude <= 180) and properly stored in database (Issue #461)
- Fix: RPM binary complied in release mode (previous versions used debug)
- Add: file to disable prelinking automatically along with RPM

* Fri Jun 27 2014 Fermin Galan <fermin@tid.es> 0.14.0-1 (FIWARE-3.4.3-1)
- Add: Pagination, using URI parameters 'offset' and 'limit' (and 'details' for extra details) (issue #395)
- Add: ServicePath support for specifying to which service path entities belong in NGSI10 queryContext and updateContext operations (issue #392)
- Fix: entitiesQuery() method (used in queryContext and NGSI10 subscriptions) has been re-written, making it more clear and optimal (PR #405)

* Fri May 30 2014 Fermin Galan <fermin@tid.es> 0.13.0-1 (FIWARE-3.4.2-1)
- Add: Rush can now be used as relayer for the broker, using the option '-rush' (issue #251)
- Add: Custom metadata support (issue #252)
- Add: Multi-service/tenant support, using the option -multiservice (issue #322)
- Add: Generic URI parameters supported (issue #372)
- Add: Notification mime-type selected in URI parameter 'notifyFormat' (supported values: "XML" and "JSON") (issue #372)
- Fix: Raising error on updateContext with location metadata doesn't involve actual change of location attribute (issue #351)
- Fix: The functionality to change the Log configuration via REST had stopped working. Fixed.
- FIX: The Log configuration via REST used "/log/traceLevel", whereas the documentation states "/log/trace".
       Updated the broker to support both "/log/trace" and "/log/traceLevel"
- Fix: Changed the XML tags AttributeAssociationList and AttributeAssociation to start with lowercase 'a'. (issue #378)
- Fix: Concatenation of strings now done by the compiler (in numerous places). (issue #384)
- Fix: Passing all complex parameters by reference or pointer (issues #263 and #354)

* Wed Apr 30 2014 Fermin Galan <fermin@tid.es> 0.12.0-1 (FIWARE-3.4.1-1)
- Add:  The broker now supports https, see CLI options '-https', '-cert' and '-key'
- Add:  New command line option '--silent', to suppress all log messages except errors. (Issue #291)
- Fix:  All responses with StatusCode now have the 'reasonPhrase' in accordance with the 'code' (Issue 242).
- Fix:  Every Context Element gets its corresponding Context Element Response when processing updateContext operations
        (previous version was interrupted in the middle if the processing if one Context Element resulted in error).
- Fix:  Requests with unacceptable Content-Type (HTTP header) get a "415 Unsupported Media Type" and a descriptive payload 'orionError'.
        Requests with unsupported 'Accept' format (HTTP header) get a "406 Not Acceptable" and a descriptive payload 'orionError'.
        Requests with "both 415 and 406" get a "415 Unsupported Media Type" and a descriptive payload 'orionError' in XML. (Issue #48)
- Fix:  The broker now performs retries when connecting to mongodb at startup. (Issue #308)
- Fix:  Responses containing context attributes with *compound values* AND metadata is now correctly rendered (Issue #323).
- Fix:  Now it is possible to have 'not in use' as a valid context value.
- Fix:  The ISO8601 parser has been improved. (Issue #231)
- Fix:  An important bug in the log trace library has been fixed. This fix *may* solve 'strange errors' (Issue #313)
- Fix:  Fixed a bug about empty context attribute values, probably introduced when compound values were invented (release 0.11.0).
- Fix:  Fixed the bug for failing long requests (Issue #333)
- Fix:  Changed the string for associations from 'Include Assciations' to 'IncludeAssociations' (small part of issue #334)

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

* Tue Jun 04 2013 Fermín Galán <fermin@tid.es> 0.3.0-1 (FIWARE-2.3.3-1)
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
