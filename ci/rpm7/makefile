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
	@echo '------------------------------------- prepare  -----------------------------------------'

	./scripts/build/compileInfo.sh --release
	. scripts/testEnv.sh

install:
	@echo '------------------------------------- install ------------------------------------------'

	cp scripts/accumulator-server.py $(INSTALL_DIR)/bin/
	cp scripts/managedb/garbage-collector.py $(INSTALL_DIR)/bin/

build_unit: prepare
	@echo '------------------------------------- make build_unit ----------------------------------'

	rm -Rf BUILD_UNIT || true && mkdir -p BUILD_UNIT || true
	mkdir -p /tmp/builder/logs || true

	cd BUILD_UNIT && cmake .. -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_ARCH=$(BUILD_ARCH) -DUNIT_TEST=True -DCOVERAGE=True -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	cd BUILD_UNIT && make -j$(CPU_COUNT)

install_unit: install
	@echo '------------------------------------- make install_unit --------------------------------'

	cd BUILD_UNIT && make install DESTDIR=$(DESTDIR)

unit:
	@echo '------------------------------------- make unit ----------------------------------------'

	BUILD_UNIT/test/unittests/unitTest -t 0-255 -dbhost ${MONGO_HOST} --gtest_output=xml:/tmp/builder/logs/unit.xml

build_functional: prepare
	@echo '------------------------------------- make build_functional ----------------------------'

	rm -Rf BUILD_FUNCTIONAL || true && mkdir -p BUILD_FUNCTIONAL || true

	cd BUILD_FUNCTIONAL && cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_ARCH=$(BUILD_ARCH) -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	cd BUILD_FUNCTIONAL && make -j$(CPU_COUNT)

install_functional: install
	@echo '------------------------------------- make install_functional --------------------------'

	cd BUILD_FUNCTIONAL && make install DESTDIR=$(DESTDIR)

functional:
	@echo '------------------------------------- make functional ----------------------------------'

	# ft_env needed for accumulator-server.py
	. /opt/ft_env/bin/activate
	./test/functionalTest/testHarness.sh --stopOnError

style:
	@echo '------------------------------------- make style ---------------------------------------'

	./scripts/style_check_in_makefile.sh

files_compliance:
	@echo '------------------------------------- make file_compliance -----------------------------'

	scripts/check_files_compliance.py .

payload_check:
	@echo '------------------------------------- make payload_check -------------------------------'

	test/jsonCheck/jsonCheck.sh
