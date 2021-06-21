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

# Default prefix for installation
# Used by RPM generation
ifndef DESTDIR
	DESTDIR=/
endif

# Install to /usr unless told otherwise
ifndef INSTALL_DIR
	INSTALL_DIR=/usr
endif

ifndef CPU_COUNT
	CPU_COUNT:=$(shell cat /proc/cpuinfo | grep processor | wc -l)
endif

ifndef ORION_WS
	ORION_WS:=$(shell pwd)
endif

# Directory for the rpm stage
ifndef TOPDIR
	RPM_TOPDIR=$(ORION_WS)/rpm
endif

# Version for the contextBroker-* packages 
ifndef BROKER_VERSION
	BROKER_VERSION:=$(shell grep "\#define ORION_VERSION" src/app/contextBroker/version.h | sed -e 's/^.* "//' -e 's/"//' | sed -e 's/-/_/g')
endif

# Release ID for the contextBroker-* packages
ifndef BROKER_RELEASE
	BROKER_RELEASE=dev
endif

ifndef BUILD_ARCH
    BUILD_ARCH:=$(shell uname -m)
endif

ifndef MOCK_CONFIG
    MOCK_CONFIG=epel-6-tid
endif

ifndef MONGO_HOST
    MONGO_HOST=localhost
endif

all: prepare_release release

di: install_debug

compile_info:
	./scripts/build/compileInfo.sh

compile_info_release:
	./scripts/build/compileInfo.sh --release

prepare_release: compile_info_release
	mkdir -p  BUILD_RELEASE || true
	cd BUILD_RELEASE && cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_ARCH=$(BUILD_ARCH) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)

prepare_debug: compile_info
	mkdir -p  BUILD_DEBUG || true
	cd BUILD_DEBUG && cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_ARCH=$(BUILD_ARCH) -DDEBUG=True -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)

prepare_coverage: compile_info
	mkdir -p  BUILD_COVERAGE || true
	cd BUILD_COVERAGE && cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_ARCH=$(BUILD_ARCH) -DUNIT_TEST=True -DCOVERAGE=True -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)

prepare_unit_test: compile_info
	@echo '------------------------------- prepare_unit_test starts ---------------------------------'
	mkdir -p  BUILD_UNITTEST || true
	cd BUILD_UNITTEST && cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_ARCH=$(BUILD_ARCH) -DUNIT_TEST=True -DCOVERAGE=True -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	@echo '------------------------------- prepare_unit_test ended ---------------------------------'

release: prepare_release
	cd BUILD_RELEASE && make -j$(CPU_COUNT)

debug: prepare_debug
	cd BUILD_DEBUG && make -j$(CPU_COUNT)

# Requires root access, i.e. use 'sudo make install' to install
install: release
	cd BUILD_RELEASE && make install DESTDIR=$(DESTDIR)

# Requires root access, i.e. use 'sudo make install' to install
install_debug: debug
	cd BUILD_DEBUG && make install DESTDIR=$(DESTDIR)

install_scripts:
	cp scripts/accumulator-server.py $(INSTALL_DIR)/bin 
	cp scripts/accumulator-mqtt.py $(INSTALL_DIR)/bin
	cp scripts/managedb/garbage-collector.py $(INSTALL_DIR)/bin

install_coverage: prepare_coverage
	cd BUILD_COVERAGE && make install DESTDIR=$(DESTDIR)

post_install_libs:
	mkdir -p /usr/local/include/contextBroker 

	cd /usr/local/include/contextBroker  && rm -rf common && mkdir -p common
	cp src/lib/common/*.h /usr/local/include/contextBroker/common         
	cp $(CMAKE_BUILD_TYPE)/src/lib/common/libcommon.a  /usr/local/lib 

	chmod a+r /usr/local/include/contextBroker/common/compileInfo.h

	cd /usr/local/include/contextBroker  && rm -rf convenience && mkdir -p convenience
	cp src/lib/convenience/*.h /usr/local/include/contextBroker/convenience         
	cp $(CMAKE_BUILD_TYPE)/src/lib/convenience/libconvenience.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf convenienceMap && mkdir -p convenienceMap
	cp src/lib/convenienceMap/*.h /usr/local/include/contextBroker/convenienceMap         
	cp $(CMAKE_BUILD_TYPE)/src/lib/convenienceMap/libconvenienceMap.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf jsonParse && mkdir -p jsonParse
	cp src/lib/jsonParse/*.h /usr/local/include/contextBroker/jsonParse         
	cp $(CMAKE_BUILD_TYPE)/src/lib/jsonParse/libjsonParse.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf ngsi && mkdir -p ngsi
	cp src/lib/ngsi/*.h /usr/local/include/contextBroker/ngsi         
	cp $(CMAKE_BUILD_TYPE)/src/lib/ngsi/libngsi.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf ngsi10 && mkdir -p ngsi10
	cp src/lib/ngsi10/*.h /usr/local/include/contextBroker/ngsi10         
	cp $(CMAKE_BUILD_TYPE)/src/lib/ngsi10/libngsi10.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf ngsi9 && mkdir -p ngsi9
	cp src/lib/ngsi9/*.h /usr/local/include/contextBroker/ngsi9         
	cp $(CMAKE_BUILD_TYPE)/src/lib/ngsi9/libngsi9.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf ngsiNotify && mkdir -p ngsiNotify
	cp src/lib/ngsiNotify/*.h /usr/local/include/contextBroker/ngsiNotify         
	cp $(CMAKE_BUILD_TYPE)/src/lib/ngsiNotify/libngsiNotify.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf rest && mkdir -p rest
	cp src/lib/rest/*.h /usr/local/include/contextBroker/rest         
	cp $(CMAKE_BUILD_TYPE)/src/lib/rest/librest.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf xmlParse && mkdir -p xmlParse
	cp src/lib/xmlParse/*.h /usr/local/include/contextBroker/xmlParse         
	cp $(CMAKE_BUILD_TYPE)/src/lib/xmlParse/libxmlParse.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf parseArgs && mkdir -p parseArgs
	cp src/lib/parseArgs/*.h /usr/local/include/contextBroker/parseArgs         
	cp $(CMAKE_BUILD_TYPE)/src/lib/parseArgs/libpa.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf logMsg && mkdir -p logMsg
	cp src/lib/logMsg/*.h /usr/local/include/contextBroker/logMsg         
	cp $(CMAKE_BUILD_TYPE)/src/lib/logMsg/liblm.a  /usr/local/lib 

	cd /usr/local/include/contextBroker  && rm -rf serviceRoutines && mkdir -p serviceRoutines
	cp src/lib/serviceRoutines/*.h /usr/local/include/contextBroker/serviceRoutines
	cp $(CMAKE_BUILD_TYPE)/src/lib/serviceRoutines/libserviceRoutines.a  /usr/local/lib

	cd /usr/local/include/contextBroker  && rm -rf orionTypes && mkdir -p orionTypes
	cp src/lib/orionTypes/*.h /usr/local/include/contextBroker/orionTypes
	
	cd /usr/local/include/contextBroker  && rm -rf parse && mkdir -p parse
	cp src/lib/parse/*.h /usr/local/include/contextBroker/parse
	cp $(CMAKE_BUILD_TYPE)/src/lib/parse/libparse.a  /usr/local/lib

	cd /usr/local/include/contextBroker  && rm -rf mqtt && mkdir -p mqtt
	cp src/lib/mqtt/*.h /usr/local/include/contextBroker/mqtt         
	cp $(CMAKE_BUILD_TYPE)/src/lib/mqtt/libmqtt.a  /usr/local/lib


# Requires root access, i.e. use 'sudo make install_libs' to install
install_libs: release
	make post_install_libs CMAKE_BUILD_TYPE=BUILD_RELEASE

# Requires root access, i.e. use 'sudo make install_debug_libs' to install
install_debug_libs: debug
	make post_install_libs CMAKE_BUILD_TYPE=BUILD_DEBUG

rpm-ts:
	# This target assumes that scripts/build/timestampVersion.sh has been previously called before "make rpm-ts"
	rm -f rpm/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	git archive --format tar --prefix=contextBroker-$(BROKER_VERSION)/ HEAD |  gzip >  $(RPM_TOPDIR)/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	# It seems that git archive doesn't take into account changes in the local copy not commited. Thus, we need to do this "in place" .tar.gz
	# replacement to inject the modified version.sh file
	cd $(RPM_TOPDIR)/SOURCES && tar xfvz contextBroker-$(BROKER_VERSION).tar.gz && cd -
	cp src/app/contextBroker/version.h $(RPM_TOPDIR)/SOURCES/contextBroker-$(BROKER_VERSION)/src/app/contextBroker/version.h
	rm $(RPM_TOPDIR)/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	cd $(RPM_TOPDIR)/SOURCES && tar cfvz contextBroker-$(BROKER_VERSION).tar.gz contextBroker-$(BROKER_VERSION) && cd -
	rm -rf $(RPM_TOPDIR)/SOURCES/contextBroker-$(BROKER_VERSION)
	# -------------
	git checkout src/app/contextBroker/version.h
	rpmbuild -ba $(RPM_TOPDIR)/SPECS/contextBroker.spec \
		--define '_topdir $(RPM_TOPDIR)' \
		--define 'broker_version $(BROKER_VERSION)' \
		--define 'broker_release $(BROKER_RELEASE)' \
		--define 'build_arch $(BUILD_ARCH)'

rpm: 
	rm -f rpm/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	git archive --format tar --prefix=contextBroker-$(BROKER_VERSION)/ HEAD |  gzip >  $(RPM_TOPDIR)/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	rpmbuild -ba $(RPM_TOPDIR)/SPECS/contextBroker.spec \
		--define '_topdir $(RPM_TOPDIR)' \
		--define 'broker_version $(BROKER_VERSION)' \
		--define 'broker_release $(BROKER_RELEASE)' \
		--define 'build_arch $(BUILD_ARCH)'

mock: 
	mkdir -p ~/rpmbuild/{BUILD,RPMS,S{OURCE,PEC,RPM}S}
	rm -f ~/rpmbuild/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	git archive --format tar --prefix=contextBroker-$(BROKER_VERSION)/ HEAD |  gzip >  $(HOME)/rpmbuild/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	rpmbuild -bs rpm/contextBroker.spec \
		--define 'broker_version $(BROKER_VERSION)' \
		--define 'broker_release $(BROKER_RELEASE)' \
		--define 'build_arch $(BUILD_ARCH)'
	/usr/bin/mock -r $(MOCK_CONFIG)-$(BUILD_ARCH) ~/rpmbuild/SRPMS/contextBroker-$(BROKER_VERSION)-$(BROKER_RELEASE).src.rpm -v \
		--define='broker_version $(BROKER_VERSION)' \
		--define='broker_release $(BROKER_RELEASE)' \
		--define='build_arch $(BUILD_ARCH)'
	mkdir -p packages
	cp /var/lib/mock/$(MOCK_CONFIG)-$(BUILD_ARCH)/result/*.rpm packages

mock64: /var/lib/mock/$(MOCK_CONFIG)-x86_64
	make mock BUILD_ARCH=x86_64 MOCK_CONFIG=$(MOCK_CONFIG)

mock32: /var/lib/mock/$(MOCK_CONFIG)-i386
	make mock BUILD_ARCH=i386  MOCK_CONFIG=$(MOCK_CONFIG)

/var/lib/mock/$(MOCK_CONFIG)-x86_64:
ifeq ($(MOCK_CONFIG),epel-6-tid)
	sudo cp rpm/epel-6-tid-x86_64.cfg /etc/mock/epel-6-tid-x86_64.cfg
endif
	/usr/bin/mock --init -r $(MOCK_CONFIG)-x86_64 -v

/var/lib/mock/$(MOCK_CONFIG)-i386:
ifeq ($(MOCK_CONFIG),epel-6-tid)
	sudo cp rpm/epel-6-tid-i386.cfg /etc/mock/epel-6-tid-i386.cfg
endif
	/usr/bin/mock --init -r $(MOCK_CONFIG)-i386 -v

deb: clean
	rm -rf package/deb
	sed -e  "s/BROKER_VERSION/$(BROKER_VERSION)/"  -e "s/BROKER_RELEASE/$(BROKER_RELEASE)/" CHANGELOG > debian/changelog
	dpkg-buildpackage -b

clean:
	rm -rf BUILD_RELEASE
	rm -rf BUILD_DEBUG
	rm -rf BUILD_COVERAGE
	rm -rf BUILD_UNITTEST

style:
	./scripts/style_check_in_makefile.sh
	rm LINT LINT_ERRORS


style_check:
	@scripts/style_check.sh

style_check_v:
	@VERBOSE=1 scripts/style_check.sh

lint_changed:
	git diff --name-only | grep "\.cpp\|\.h" | xargs scripts/cpplint.py

uncrustify_changed:
	git diff --name-only | grep "\.cpp\|\.h" | xargs uncrustify --no-backup -c scripts/uncrustify.cfg

build_unit_test: prepare_unit_test 
	@echo '------------------------------- build_unit_test starts ---------------------------------'
	cd BUILD_UNITTEST && make -j$(CPU_COUNT)
	@echo '------------------------------- build_unit_test ended ---------------------------------'

unit_test: build_unit_test
	@echo '------------------------------- unit_test starts ---------------------------------'
	if [ -z "${TEST_FILTER}" ]; then \
	   BUILD_UNITTEST/test/unittests/unitTest -t 0-255 -dbhost ${MONGO_HOST} --gtest_output=xml:BUILD_UNITTEST/unit_test.xml; \
        else \
	   BUILD_UNITTEST/test/unittests/unitTest -t 0-255 -dbhost ${MONGO_HOST} --gtest_output=xml:BUILD_UNITTEST/unit_test.xml --gtest_filter=${TEST_FILTER}; \
        fi
	@echo '------------------------------- unit_test ended ---------------------------------'

functional_test: install
	./test/functionalTest/testHarness.sh

functional_test_debug: install_debug
	./test/functionalTest/testHarness.sh

ft:  functional_test
ftd: functional_test_debug

test: unit_test functional_test

coverage: install_coverage
	# Init coverage
	echo "Initializing coverage files"
	mkdir -p coverage
	lcov -i --zerocounters --directory BUILD_COVERAGE/
	lcov --capture --initial --directory BUILD_COVERAGE -b BUILD_COVERAGE --output-file coverage/broker.init.info
	# Execute test for coverage
	echo "Executing coverage test"
	BUILD_COVERAGE/test/unittests/unitTest -t 0-255 --gtest_output=xml:BUILD_COVERAGE/unit_test.xml
	if [ -z "${CONTEXTBROKER_TESTENV_SOURCED}" ]; then \
	    echo "Execute '. scripts/testEnv.sh' before executing the tests"; \
	    exit 1; \
	fi
	make test -C BUILD_COVERAGE ARGS="-D ExperimentalTest" TEST_VERBOSE=1 || true
	@if [ -e test/functionalTest/cases/*.diff ]; then \
           echo "A .diff file was found in test/functionalTest/cases, which means that ctest failed running the test. This can happen if a \"Ok\""; \
           echo "token is used in the tests specification. Run \"test/functionalTest/testHarness.sh test/functionalTest/cases\" manually to find the problem."; \
	   exit 1; \
	fi
	@xsltproc scripts/cmake2junit.xsl BUILD_COVERAGE/Testing/`cat BUILD_COVERAGE/Testing/TAG| head -n1`/Test.xml  > BUILD_COVERAGE/functional_test.xml
	# Generate test report
	echo "Generating coverage report"
	lcov --directory BUILD_COVERAGE --capture -b BUILD_COVERAGE --output-file coverage/broker.test.info 
	lcov --add-tracefile coverage/broker.init.info --add-tracefile coverage/broker.test.info --output-file coverage/broker.info
	lcov -r coverage/broker.info "/usr/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/usr/local/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/opt/local/include/google/*" -o coverage/broker.info
	# Remove unit test libraries and libraries developed before contextBroker project init
	lcov -r coverage/broker.info "*/test/unittests/*" -o coverage/broker.info
	lcov -r coverage/broker.info "*/src/lib/logMsg/*" -o coverage/broker.info
	lcov -r coverage/broker.info "*/src/lib/parseArgs/*" -o coverage/broker.info
	genhtml -o coverage coverage/broker.info

coverage_unit_test: build_unit_test
	# Init coverage
	echo "Initializing coverage files"
	mkdir -p coverage
	lcov -i --zerocounters --directory BUILD_UNITTEST/
	lcov --capture --initial --directory BUILD_UNITTEST -b BUILD_UNITTEST --output-file coverage/broker.init.info
	# Execute test for coverage
	echo "Executing coverage test"
	BUILD_UNITTEST/test/unittests/unitTest -t 0-255 --gtest_output=xml:BUILD_UNITTEST/unit_test.xml
	# Generate test report
	echo "Generating coverage report"
	lcov --directory BUILD_UNITTEST --capture -b BUILD_UNITTEST --output-file coverage/broker.test.info 
	lcov --add-tracefile coverage/broker.init.info --add-tracefile coverage/broker.test.info --output-file coverage/broker.info
	lcov -r coverage/broker.info "/usr/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/usr/local/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/opt/local/include/google/*" -o coverage/broker.info
	# Remove unit test libraries and libraries developed before contextBroker project init
	lcov -r coverage/broker.info "*/test/unittests/*" -o coverage/broker.info
	lcov -r coverage/broker.info "*/src/lib/logMsg/*" -o coverage/broker.info
	lcov -r coverage/broker.info "*/src/lib/parseArgs/*" -o coverage/broker.info
	# app/ contains application itself, not libraries which make sense to measure unit_test coverage
	lcov -r coverage/broker.info "*/src/app/*" -o coverage/broker.info
	genhtml -o coverage coverage/broker.info

coverage_functional_test: install_coverage
	# Init coverage
	echo "Initializing coverage files"
	mkdir -p coverage
	lcov -i --zerocounters --directory BUILD_COVERAGE/
	lcov --capture --initial --directory BUILD_COVERAGE -b BUILD_COVERAGE --output-file coverage/broker.init.info
	# Execute test for coverage
	echo "Executing coverage test"
	if [ -z "${CONTEXTBROKER_TESTENV_SOURCED}" ]; then \
	    echo "Execute '. scripts/testEnv.sh' before executing the tests"; \
	    exit 1; \
	fi
	make test -C BUILD_COVERAGE ARGS="-D ExperimentalTest" TEST_VERBOSE=1 || true
	@if [ -e test/functionalTest/cases/*.diff ]; then \
           echo "A .diff file was found in test/functionalTest/cases, which means that ctest failed running the test. This can happen if a \"Ok\""; \
           echo "token is used in the tests specification. Run \"test/functionalTest/testHarness.sh test/functionalTest/cases\" manually to find the problem."; \
	   exit 1; \
	fi
	@xsltproc scripts/cmake2junit.xsl BUILD_COVERAGE/Testing/`cat BUILD_COVERAGE/Testing/TAG| head -n1`/Test.xml  > BUILD_COVERAGE/functional_test.xml
	# Generate test report
	echo "Generating coverage report"
	lcov --directory BUILD_COVERAGE --capture -b BUILD_COVERAGE --output-file coverage/broker.test.info 
	lcov --add-tracefile coverage/broker.init.info --add-tracefile coverage/broker.test.info --output-file coverage/broker.info
	lcov -r coverage/broker.info "/usr/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/usr/local/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/opt/local/include/google/*" -o coverage/broker.info
	# Remove unit test libraries and libraries developed before contextBroker project init
	lcov -r coverage/broker.info "*/test/unittests/*" -o coverage/broker.info
	lcov -r coverage/broker.info "*/src/lib/logMsg/*" -o coverage/broker.info
	lcov -r coverage/broker.info "*/src/lib/parseArgs/*" -o coverage/broker.info
	genhtml -o coverage coverage/broker.info

valgrind: install_debug
	@echo For detailed info: tail -f /tmp/valgrindTestSuiteLog
	test/valgrind/valgrindTestSuite.sh

files_compliance:
	scripts/check_files_compliance.py .

json_check:
	test/jsonCheck/jsonCheck.sh

payload_check: json_check

cppcheck:
	cppcheck --xml -j 8 --enable=all -I src/lib/ src/ 2> cppcheck-result.xml
	cat cppcheck-result.xml | grep "error file" | wc -l

sonar_metrics: coverage
	scripts/build/sonarProperties.sh $(BROKER_VERSION) > sonar-project.properties 
	cd BUILD_COVERAGE/src && gcovr --gcov-exclude='.*parseArgs.*' --gcov-exclude='.*logMsg.*' -x -o ../../coverage.xml && cd ../../
	cppcheck --xml -j 8 --enable=all -I src/lib/ -i src/lib/parseArgs -i src/lib/logMsg src/ 2>cppcheck-result.xml

.PHONY: rpm mock mock32 mock64 valgrind
