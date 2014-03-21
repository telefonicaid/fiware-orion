Summary:          Orion Context Broker
Name:             contextbroker
Version:          %{_product_version}
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
BuildRequires:    gcc, make, gcc-c++, libmicrohttpd-devel, boost-devel
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
%define _owner contextbroker
%define _service_name contextbrokerd

## System folders
%define _src_project_dir %{_sourcedir}/../../
%define _install_dir /opt/%{name}
# _localstatedir is a system var that goes to /var
%define _orion_log_dir %{_localstatedir}/log/%{name}


# -------------------------------------------------------------------------------------------- #
# Package RPM for tests 
# -------------------------------------------------------------------------------------------- #
%package tests
Requires: %{name}, python, python-flask, nc, curl, libxml2, mongodb, contextbroker 
Summary: Test suite for %{name}

%description tests
Test suite for %{name}

# -------------------------------------------------------------------------------------------- #
# prep section, setup macro:
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
# pre-install section:
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
# pre-install section:
# -------------------------------------------------------------------------------------------- #
%install
# Read from BUILD and write into BUILDROOT

# RPM_BUILD_ROOT = BUILDROOT
# %{_install_dir}=/opt/contextbroker

echo "[INFO] Installing the %{name}"
make install_debug DESTDIR=$RPM_BUILD_ROOT
strip $RPM_BUILD_ROOT/usr/bin/contextBroker
chmod 555 $RPM_BUILD_ROOT/usr/bin/contextBroker

echo "[INFO] Creating installation directories "
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/bin
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/init.d
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/profile.d
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/doc
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/share
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}/config
mkdir -p $RPM_BUILD_ROOT/%{_install_dir}_tests

echo "[INFO] Copying files into the %{_install_dir}"
mv $RPM_BUILD_ROOT/usr/bin/contextBroker $RPM_BUILD_ROOT/%{_install_dir}/bin/contextbroker 
rm -Rf $RPM_BUILD_ROOT/usr

cp LICENSE                               $RPM_BUILD_ROOT/%{_install_dir}/doc
cp scripts/managedb/garbage-collector.py $RPM_BUILD_ROOT/%{_install_dir}/share
cp scripts/managedb/lastest-updates.py   $RPM_BUILD_ROOT/%{_install_dir}/share
cp etc/init.d/contextBroker.centos       $RPM_BUILD_ROOT/%{_install_dir}/init.d/%{name}
cp etc/config/contextBroker              $RPM_BUILD_ROOT/%{_install_dir}/config/%{name}

cp -R test/testharness/*                 $RPM_BUILD_ROOT/%{_install_dir}_tests
cp scripts/testEnv.sh \
   scripts/testHarness.sh \
   scripts/testDiff.py                   $RPM_BUILD_ROOT/%{_install_dir}_tests
cp scripts/accumulator-server.py         $RPM_BUILD_ROOT/%{_install_dir}_tests

chmod 755 $RPM_BUILD_ROOT/%{_install_dir}/init.d/%{name}


# -------------------------------------------------------------------------------------------- #
# post-install section:
# -------------------------------------------------------------------------------------------- #
%post
# This section is executed when the rpm is installed (rpm -i)

echo "[INFO] Configuring application"
echo "[INFO] Creating links"
ln -s %{_install_dir}/init.d/%{name} /etc/init.d/%{_service_name}
ln -s %{_install_dir}/config/%{name} /etc/sysconfig/%{name}
ln -s %{_install_dir}/bin/contextbroker /usr/bin/contextbroker
ls -s %{_install_dir}/doc /usr/share/doc/contextbroker

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

After configuring /etc/sysconfig/%{name} execute 'chkconfig %{name} on' to
enable %{name} after a reboot.
EOMSG

# -------------------------------------------------------------------------------------------- #
# pre-uninstall section:
# -------------------------------------------------------------------------------------------- #
%preun
echo "[INFO] Uninstall the %{name}"
/etc/init.d/%{_service_name} stop
/sbin/chkconfig --del %{_service_name}

# -------------------------------------------------------------------------------------------- #
# clean section:
# -------------------------------------------------------------------------------------------- #
%clean
echo "[INFO] Cleaning the $RPM_BUILD_ROOT directory"
rm -rf $RPM_BUILD_ROOT

# -------------------------------------------------------------------------------------------- #
# Files to add to the RPM 
# -------------------------------------------------------------------------------------------- #
%files
%defattr(755,%{_owner},%{_owner},755)
/opt/contextbroker


#%files test
%files tests
%defattr(755,%{_owner},%{_owner},755)
/opt/contextbroker_tests

