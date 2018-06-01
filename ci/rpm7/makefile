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
