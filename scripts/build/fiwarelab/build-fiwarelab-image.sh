#/bin/bash

# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

# We assume the following preconditions in the place where this script runs:
#
# - The script runs *inside* base image base_centos_6 type
# - The user that runs the script has sudoers permissions
# - The following commands are installed and accesible in the default PATH: yum, 
#    mktemp, chkconfig, iptables

# Install welcome message
MOTD=$(mktemp)
cat > $MOTD <<EOF

               OOOOOOOOO
           OOOOOOO    OOOOO
        OOOOOOOO  OOO     OOO
     OOOOOOOO   OOOO  OO   OOO
  OOOOOO    OOO     OOOO   OOOO
 OOOOOO   OOOO       OOOO    OOOO
 OOOO     OOOOO       OO       OOO
OOOOO      OOO         O        OOO
OOOOO         OO       O         OOOO
OOOOO           O      OOOO        OOO
OO OO            O     OOOO         OOO
OO OOO            O   OOOO           OOO
 OO OOOO          OOOOO   O          OOO
  OO OOOO         OOOOO    OO        OOO
   OOO OOOO        OOO       O      OOOO
     OO  OOO        O         OOOO OOOO
      OOO  OOO      O         OOOOOOOO
       OOO  OOO     OO    OOOO OOOOOO
         OOO  OOO  OOOO OOO      OOO
           OO   OO OOOOO        OOOO
            OOO  OOOOOO        OOOO
              OO  OOOO        OOOO
               OOO OOOOOO  OOOOOO
                 OOO OOOOOOOOOOO
                   OOOOOOOOOOO
                      OOOOO



Welcome! This VM includes Orion Context Broker and Cygnus. In order to update to
the lastest version of them, it is strongly recommended you run the following commands:
 
 yum install contextBroker
 yum install cygnus
 
Orion Context Broker and Cygnus run as service by default (in port 1026 and 5050 respectively).
You need to have these ports allowed in this VM security group in order to allow external access.
In addition, note that both ports have been enabled in iptables configuration (you can disable
iptables flushing out all its rules with 'sudo iptables -F', relying exclusively in VM security
groups for port-level access control).
 
This image also contains an instance of Rush: an HTTP relayer that can be used to securize the
outgoing HTTP calls from the Context Broker. The rush instance is installed in /opt/Rush folder.
An instance of Redis compatible with Rush is also installed. More information about Rush can be
found in its Github page: https://github.com/telefonicaid/Rush
 
Detailed documentation about Orion Context Broker and its related components can be found at
FIWARE Catalog: http://catalogue.fiware.org/enablers/publishsubscribe-context-broker-orion-context-broker.
EOF
sudo mv $MOTD /etc/motd

# Install MongoDB
MONGO_REPO=$(mktemp)
cat > $MONGO_REPO <<EOF
[mongodb]
name=MongoDB Repository
baseurl=http://downloads-distro.mongodb.org/repo/redhat/os/x86_64/
gpgcheck=0
enabled=1
EOF
sudo mv $MONGO_REPO /etc/yum.repos.d/mongodb.repo

MONGO_VERSION=2.6.9
sudo yum install -y mongodb-org-$MONGO_VERSION \
                    mongodb-org-server-$MONGO_VERSION \
                    mongodb-org-shell-$MONGO_VERSION \
                    mongodb-org-mongos-$MONGO_VERSION \
                    mongodb-org-tools-$MONGO_VERSION

# Default MongoDB configures set doesn't enables smallfiles by default. Enabling it ensures that MongoDB will 
# work even in VM with small disk size
cp /etc/mongod.conf /tmp/
echo ' '                                           >> /tmp/mongod.conf
echo '# Added by FIWARE Lab Orion building script' >> /tmp/mongod.conf
echo 'smallfiles = true'                           >> /tmp/mongod.conf
sudo mv /tmp/mongod.conf /etc/mongod.conf

sudo chkconfig mongod on
sudo /etc/init.d/mongod start

# Set FIWARE repository
FIWARE_REPO=$(mktemp)
cat > $FIWARE_REPO <<EOF
[testbed-fi-ware]
name=Fiware Repository
baseurl=http://repositories.testbed.fiware.org/repo/rpm/x86_64/
gpgcheck=0
enabled=1
EOF
sudo mv $FIWARE_REPO /etc/yum.repos.d/fiware.repo

# Install contextBroker
sudo yum install -y contextBroker
sudo chkconfig contextBroker on
sudo /etc/init.d/contextBroker start

# Install Cygnus
sudo yum install -y cygnus

# CentOS seems to have a quite restrictive iptables configuration in the base image. Thus,
# we need to explictely set an rule for incoming traffic on ports 1026 and 5050. Alternativelly, 
# we could flush out all the rules (iptables -F) and rely in the FIWARE Lab cloud security (based
# on security groups)
sudo iptables -I INPUT 1 -p tcp -m state --state NEW -m tcp --dport 1026 -j ACCEPT
sudo iptables -I INPUT 2 -p tcp -m state --state NEW -m tcp --dport 5050 -j ACCEPT
sudo /etc/init.d/iptables save

# Install Rush
# TBD
