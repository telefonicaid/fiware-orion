#
# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# Author: Leandro Guillen

# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # For a complete reference, please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  
  # Base OS selection: uncomment only one
  #config.vm.box = "aetn/CENTOS_6_3_X86_64"  # Cent OS 6.3
  #config.vm.box = "rafacas/centos63-plain"	# Cent OS 6.3
  config.vm.box = "hansode/centos-6.3-x86_64"  # Cent OS 6.3
  
  #config.vm.box = "chef/centos-6.5"		    # Cent OS 6.5
   
  config.vm.provision :shell, path: "scripts/bootstrap/centos63.sh" # Default is CentOS 6.3
  config.vm.network "forwarded_port", host: 1026, guest: 1026 # Orion port
  config.vm.network "forwarded_port", host: 5683, guest: 5683 # CoAP

end
