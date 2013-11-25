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

# Version for the contextBroker-* packages (except contextBroker-fiware)
ifndef BROKER_VERSION
	BROKER_VERSION:=$(shell grep "\#define ORION_VERSION" src/app/contextBroker/version.h | sed -e 's/^.* "//' -e 's/"//')
endif

# Release ID for the contextBroker-* packages (execept contextBroker-fiware)
ifndef BROKER_RELEASE
	BROKER_RELEASE=dev
endif

# Version for the contextBroker-fiware package
ifndef FIWARE_VERSION
	FIWARE_VERSION=3.1.1
endif

# Release ID for the contextBroker-fiware package
ifndef FIWARE_RELEASE
	FIWARE_RELEASE=dev
endif

ifndef BUILD_ARCH
    BUILD_ARCH:=$(shell uname -m)
endif

ifndef MOCK_CONFIG
    MOCK_CONFIG=epel-6-tid
endif

ifndef XSD_DIR
    XSD_DIR=/tmp/xsd
endif

all: prepare_release release

di: install_debug

compile_info:
	./scripts/compileInfo.sh

compile_info_release:
	./scripts/compileInfo.sh --release

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
	cp scripts/garbage-collector.py $(INSTALL_DIR)/bin

install_coverage: prepare_coverage
	cd BUILD_COVERAGE && make install DESTDIR=$(DESTDIR)

post_install_libs:
	mkdir -p /usr/local/include/contextBroker 

	cd /usr/local/include/contextBroker  && rm -rf common && mkdir -p common
	cp src/lib/common/*.h /usr/local/include/contextBroker/common         
	cp $(CMAKE_BUILD_TYPE)/src/lib/common/libcommon.a  /usr/local/lib 

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



# Requires root access, i.e. use 'sudo make install_libs' to install
install_libs: release
	make post_install_libs CMAKE_BUILD_TYPE=BUILD_RELEASE

# Requires root access, i.e. use 'sudo make install_debug_libs' to install
install_debug_libs: debug
	make post_install_libs CMAKE_BUILD_TYPE=BUILD_DEBUG


rpm: 
	mkdir -p ~/rpmbuild/BUILD
	mkdir -p ~/rpmbuild/RPMS
	mkdir -p ~/rpmbuild/SOURCES
	mkdir -p ~/rpmbuild/SPECS
	mkdir -p ~/rpmbuild/SRPMS
	rm -f ~/rpmbuild/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	git archive --format tar --prefix=contextBroker-$(BROKER_VERSION)/ HEAD |  gzip >  $(HOME)/rpmbuild/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	rpmbuild -ba rpm/contextBroker.spec \
		--define 'broker_version $(BROKER_VERSION)' \
		--define 'broker_release $(BROKER_RELEASE)' \
		--define 'fiware_version $(FIWARE_VERSION)' \
		--define 'fiware_release $(FIWARE_RELEASE)' \
		--define 'build_arch $(BUILD_ARCH)'

mock: 
	mkdir -p ~/rpmbuild/{BUILD,RPMS,S{OURCE,PEC,RPM}S}
	rm -f ~/rpmbuild/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	git archive --format tar --prefix=contextBroker-$(BROKER_VERSION)/ HEAD |  gzip >  $(HOME)/rpmbuild/SOURCES/contextBroker-$(BROKER_VERSION).tar.gz
	rpmbuild -bs rpm/contextBroker.spec \
		--define 'broker_version $(BROKER_VERSION)' \
		--define 'broker_release $(BROKER_RELEASE)' \
		--define 'fiware_version $(FIWARE_VERSION)' \
		--define 'fiware_release $(FIWARE_RELEASE)' \
		--define 'build_arch $(BUILD_ARCH)'
	/usr/bin/mock -r $(MOCK_CONFIG)-$(BUILD_ARCH) ~/rpmbuild/SRPMS/contextBroker-$(BROKER_VERSION)-$(BROKER_RELEASE).src.rpm -v \
		--define='broker_version $(BROKER_VERSION)' \
		--define='broker_release $(BROKER_RELEASE)' \
		--define='fiware_version $(BROKER_FIWARE_VERSION)' \
		--define 'fiware_release $(FIWARE_RELEASE)' \
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

lint_all:
	scripts/cpplint.py src/*.cpp src/*.h

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
	   BUILD_UNITTEST/test/unittests/unitTest -t 0-255 --gtest_output=xml:BUILD_UNITTEST/unit_test.xml; \
        else \
	   BUILD_UNITTEST/test/unittests/unitTest -t 0-255 --gtest_output=xml:BUILD_UNITTEST/unit_test.xml --gtest_filter=${TEST_FILTER}; \
        fi
	@echo '------------------------------- unit_test ended ---------------------------------'

functional_test: install_debug build_unit_test
	if [ -z "${BROKER_PORT}" ]; then \
	    echo "Execute '. scripts/testEnv.sh' before executing the tests"; \
	    exit 1; \
	fi
	make test -C BUILD_UNITTEST ARGS="-D ExperimentalTest" TEST_VERBOSE=1 || true
	@if [ -e test/testharness/*.diff ]; then \
           echo "A .diff file was found in test/testharness, which means that ctest failed running the test. This can happen if a \"Ok\""; \
           echo "token is used in the tests specification. Run \"scripts/testHarness.sh /test/testharness\" manually to find the problem."; \
	   exit 1; \
	fi
	@xsltproc scripts/cmake2junit.xsl BUILD_UNITTEST/Testing/`cat BUILD_UNITTEST/Testing/TAG| head -n1`/Test.xml  > BUILD_UNITTEST/functional_test.xml

test: unit_test functional_test

coverage: install_coverage
	# Init coverage
	echo "Initializing coverage files"
	mkdir -p coverage
	lcov -i --zerocounters --directory BUILD_COVERAGE/
	lcov --capture --initial --directory BUILD_COVERAGE -b BUILD_COVERAGE --output-file coverage/broker.init.info
	# Execute test for coverage
	echo "Executing coverage test"
	BUILD_COVERAGE/test/unittests/unitTest --gtest_output=xml:BUILD_COVERAGE/unit_test.xml
	if [ -z "${BROKER_PORT}" ]; then \
	    echo "Execute '. scripts/testEnv.sh' before executing the tests"; \
	    exit 1; \
	fi
	make test -C BUILD_COVERAGE ARGS="-D ExperimentalTest" TEST_VERBOSE=1 || true
	@if [ -e test/testharness/*.diff ]; then \
           echo "A .diff file was found in test/testharness, which means that ctest failed running the test. This can happen if a \"Ok\""; \
           echo "token is used in the tests specification. Run \"scripts/testHarness.sh /test/testharness\" manually to find the problem."; \
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
	lcov -r coverage/broker.info "test/unittests/*" -o coverage/broker.info
	lcov -r coverage/broker.info "src/lib/logMsg/*" -o coverage/broker.info
	lcov -r coverage/broker.info "src/lib/parseArgs/*" -o coverage/broker.info
	genhtml -o coverage coverage/broker.info

coverage_unit_test: build_unit_test
	# Init coverage
	echo "Initializing coverage files"
	mkdir -p coverage
	lcov -i --zerocounters --directory BUILD_UNITTEST/
	lcov --capture --initial --directory BUILD_UNITTEST -b BUILD_UNITTEST --output-file coverage/broker.init.info
	# Execute test for coverage
	echo "Executing coverage test"
	BUILD_UNITTEST/test/unittests/unitTest --gtest_output=xml:BUILD_UNITTEST/unit_test.xml
	# Generate test report
	echo "Generating coverage report"
	lcov --directory BUILD_UNITTEST --capture -b BUILD_UNITTEST --output-file coverage/broker.test.info 
	lcov --add-tracefile coverage/broker.init.info --add-tracefile coverage/broker.test.info --output-file coverage/broker.info
	lcov -r coverage/broker.info "/usr/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/usr/local/include/*" -o coverage/broker.info
	lcov -r coverage/broker.info "/opt/local/include/google/*" -o coverage/broker.info
	# Remove unit test libraries and libraries developed before contextBroker project init
	lcov -r coverage/broker.info "test/unittests/*" -o coverage/broker.info	
	lcov -r coverage/broker.info "src/lib/logMsg/*" -o coverage/broker.info
	lcov -r coverage/broker.info "src/lib/parseArgs/*" -o coverage/broker.info
	genhtml -o coverage coverage/broker.info

coverage_functional_test: install_coverage
	# Init coverage
	echo "Initializing coverage files"
	mkdir -p coverage
	lcov -i --zerocounters --directory BUILD_COVERAGE/
	lcov --capture --initial --directory BUILD_COVERAGE -b BUILD_COVERAGE --output-file coverage/broker.init.info
	# Execute test for coverage
	echo "Executing coverage test"
	if [ -z "${BROKER_PORT}" ]; then \
	    echo "Execute '. scripts/testEnv.sh' before executing the tests"; \
	    exit 1; \
	fi
	make test -C BUILD_COVERAGE ARGS="-D ExperimentalTest" TEST_VERBOSE=1 || true
	@if [ -e test/testharness/*.diff ]; then \
           echo "A .diff file was found in test/testharness, which means that ctest failed running the test. This can happen if a \"Ok\""; \
           echo "token is used in the tests specification. Run \"scripts/testHarness.sh /test/testharness\" manually to find the problem."; \
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
	lcov -r coverage/broker.info "test/unittests/*" -o coverage/broker.info	
	lcov -r coverage/broker.info "src/lib/logMsg/*" -o coverage/broker.info
	lcov -r coverage/broker.info "src/lib/parseArgs/*" -o coverage/broker.info
	genhtml -o coverage coverage/broker.info

valgrind:
	@echo For detailed info: tail -f /tmp/valgrindTestSuiteLog
	(cd test/valgrind; ./valgrindTestSuite.sh)

files_compliance:
	scripts/check_files_compliance.py .

xml_check:
	test/xmlCheck/xmlCheck.sh --xsd-dir $(XSD_DIR)

.PHONY: rpm mock mock32 mock64 valgrind
