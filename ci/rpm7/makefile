# Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# Author: Dmitrii Demin

ifndef DESTDIR
	DESTDIR=/
endif

ifndef INSTALL_DIR
	INSTALL_DIR=/usr/local
endif

ifndef CPU_COUNT
	CPU_COUNT:=$(shell cat /proc/cpuinfo | grep processor | wc -l)
endif

ifndef BUILD_ARCH
	BUILD_ARCH:=$(shell uname -m)
endif

ifndef MONGO_HOST
	MONGO_HOST=localhost
endif

prepare:
	@echo '------------------------------- make prepare ---------------------------------'

	rm -Rf BUILD || true
	mkdir -p  BUILD || true
	mkdir -p /tmp/builder/logs || true
	./scripts/build/compileInfo.sh --release	rm -Rf BUILD || true
	mkdir -p  BUILD || true
	mkdir -p /tmp/builder/logs || true
	./scripts/build/compileInfo.sh --release
	. scripts/testEnv.sh

	@echo '------------------------------------------------------------------------------'

build_unit: prepare
	@echo '------------------------------- make build_unit--------------------------------'

	cd BUILD && cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_ARCH=$(BUILD_ARCH) -DUNIT_TEST=True -DCOVERAGE=True -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	cd BUILD && make -j$(CPU_COUNT)

	@echo '------------------------------------------------------------------------------'

build_functional: prepare
	@echo '------------------------------- make build_functional ------------------------'

	cd BUILD && cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_ARCH=$(BUILD_ARCH) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	cd BUILD && make -j$(CPU_COUNT)

	@echo '------------------------------------------------------------------------------'

install:
	@echo '------------------------------- make install ---------------------------------'

	cp scripts/accumulator-server.py $(INSTALL_DIR)/bin/
	cp scripts/managedb/garbage-collector.py $(INSTALL_DIR)/bin/

	cd BUILD && make install DESTDIR=$(DESTDIR)

	@echo '------------------------------------------------------------------------------'

unit:
	@echo '------------------------------- make unit ------------------------------------'

	BUILD/test/unittests/unitTest -t 0-255 -dbhost ${MONGO_HOST} --gtest_output=xml:/tmp/builder/logs/unit.xml

	@echo '------------------------------------------------------------------------------'

functional:
	@echo '------------------------------- make functional ------------------------------'

	./test/functionalTest/testHarness.sh

	@echo '------------------------------------------------------------------------------'
