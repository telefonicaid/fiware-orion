## Vagrant support
If you want yo have an Orion Context Broker ready to develop in your machine easily we provide a Vagrant file so that you can get up and running. Just run:

    vagrant up

You just need to install Vagrant if you want to use this, and at least Virtualbox. Especially useful for those that use virtual machines to develop on a Mac or a Linux distribution that is not CentOS. Keep in mind that the Vagrant file specifies CentOS 6.5 as a base operating system.

For example, to compile in debug mode and install it in the home directory simply run:

     vagrant ssh -c 'INSTALL_DIR=/home/vagrant make di -C fiware-orion'

After a few minutes you can run contextBroker. Again, you can either ssh into the machine with 

    vagrant ssh

and run it from the command-line, or directly run the broker with something like

    vagrant ssh -c 'contextBroker -multiservice -t 0-255'

Orion Context Broker will be accessible at `127.0.0.1:1026`.

You can also use these commands to automate building and running from your favorite IDE.

*NOTE:* The virtualbox machine that is created uses additional resources to those from the broker itself. It uses around 512 MiB of RAM and around 1.20 GiB of disk space with Orion already compiled in debug mode.



The bootstrap script basically goes through the installation instructions in the README.