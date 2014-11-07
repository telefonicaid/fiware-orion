# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # For a complete reference, please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "chef/centos-6.5"
  config.vm.provision :shell, path: "scripts/bootstrap-centos65.sh"
  config.vm.network "forwarded_port", host: 1026, guest: 1026 # Orion port
  config.vm.network "forwarded_port", host: 5683, guest: 5683 # CoAP

end
