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
#    mktemp, chkconfig, iptables, wget
# - Internet access to download some stuff need during the building process
#
# Note that the working directory is used to download some intermediate materials
# (which are cleaned up before ending the work), thus writting permissions on that
# directory are needed. As long as this directory would be the home directory or /tmp, 
# it should work without problems.
#
# Note that Cygnus and Rush (and Redis) are not configured as active services (i.e.
# started at system startup). It is up to the user to decide if he/she want to
# activate them or not. The only active service is Orion Context Broker (and MongoDB).

# --------------- Context Broker ---------------

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
 yum install cygnus-ngsi
 
Orion Context Broker and Cygnus run as service by default (in port 1026, and 5050 and 8081 respectively).
You need to have these ports allowed in this VM security group in order to allow external access.
In addition, note that both ports have been enabled in iptables configuration (you can disable
iptables flushing out all its rules with 'sudo iptables -F', relying exclusively in VM security
groups for port-level access control).
 
This image also contains an instance of Rush: an HTTP relayer that can be used to securize the
outgoing HTTP calls from the Context Broker. The rush instance is installed in /opt/Rush folder.
An instance of Redis compatible with Rush is also installed. More information about Rush can be
found in its Github page: https://github.com/telefonicaid/Rush

However, neither Rush nor Redis are started by default so you need to use /etc/init.d/redis
and /etc/init.d/rush to do so. In addition, Orion needs to be configured to use Rush
(see https://fiware-orion.readthedocs.io/en/master/admin/rush/index.html). In order to
start Rush and Redis services at VM startup you can use:

 chkconfig redis on
 chkconfig rush on

Note that the certificate used by Rush is self-signed (using 'fiware' password) and uses fake 
data. If you want to use your own certificate you have to use your own server.crt, server.csr 
and server.key files in /opt/Rush/util. The /opt/Rush/utils/create_http_certificates.sh may help 
you to generated certificate files.
 
Detailed documentation about Orion Context Broker and its related components can be found at
FIWARE Catalog: http://catalogue.fiware.org/enablers/publishsubscribe-context-broker-orion-context-broker.
EOF
sudo mv $MOTD /etc/motd

# Install MongoDB
MONGO_REPO=$(mktemp)
cat > $MONGO_REPO <<EOF
[mongodb]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/\$releasever/mongodb-org/3.6/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-3.6.asc
EOF
sudo mv $MONGO_REPO /etc/yum.repos.d/mongodb.repo

MONGO_VERSION=3.6.6
sudo yum install -y mongodb-org-$MONGO_VERSION \
                    mongodb-org-server-$MONGO_VERSION \
                    mongodb-org-shell-$MONGO_VERSION \
                    mongodb-org-mongos-$MONGO_VERSION \
                    mongodb-org-tools-$MONGO_VERSION

sudo chkconfig mongod on
sudo /etc/init.d/mongod start

# Set FIWARE repository
FIWARE_REPO=$(mktemp)
cat > $FIWARE_REPO <<EOF
[fiware]
name=Fiware Repository
baseurl=http://repositories.lab.fiware.org/repo/rpm/\$releasever
gpgcheck=0
enabled=1
EOF
sudo mv $FIWARE_REPO /etc/yum.repos.d/fiware.repo

# Install contextBroker
sudo yum install -y contextBroker
sudo chkconfig contextBroker on
sudo /etc/init.d/contextBroker start

# Install Cygnus
sudo yum install -y cygnus-ngsi

# CentOS seems to have a quite restrictive iptables configuration in the base image. Thus,
# we need to explictely set an rule for incoming traffic on ports 1026, 5050 and 8081. Alternativelly, 
# we could flush out all the rules (iptables -F) and rely in the FIWARE Lab cloud security (based
# on security groups)
sudo iptables -I INPUT 1 -p tcp -m state --state NEW -m tcp --dport 1026 -j ACCEPT
sudo iptables -I INPUT 2 -p tcp -m state --state NEW -m tcp --dport 5050 -j ACCEPT
sudo iptables -I INPUT 3 -p tcp -m state --state NEW -m tcp --dport 8081 -j ACCEPT
sudo /etc/init.d/iptables save

# --------------- Context Broker ---------------

# Install Node
wget http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
sudo rpm -i epel-release-6-8.noarch.rpm
rm epel-release-6-8.noarch.rpm
sudo yum -y install nodejs npm

# Install Redis (partially based on http://alexanderkohout.de/blog/how-to-install-redis/)
sudo yum -y install gcc
wget http://download.redis.io/releases/redis-2.6.17.tar.gz
tar xfvz redis-2.6.17.tar.gz
rm redis-2.6.17.tar.gz
cd redis-2.6.17/deps
make hiredis jemalloc linenoise lua
cd ..
make
sudo make install
sudo mkdir /etc/redis
sudo mkdir /var/redis
REDIS_CONFIG=$(mktemp)
cp redis.conf ${REDIS_CONFIG}
sed -i "/daemonize no/c\daemonize yes" ${REDIS_CONFIG}
sed -i "/dir .\//c\dir /var/redis" ${REDIS_CONFIG}
sed -i "/logfile ""/c\logfile /var/log/redis.log" ${REDIS_CONFIG}
sudo mv $REDIS_CONFIG /etc/redis/redis.conf
cd ..
rm -r redis-2.6.17

# Redis init script from https://gist.github.com/paulrosania/257849
REDIS_INIT=$(mktemp)
cat > $REDIS_INIT <<'EOF'
#!/bin/sh
#
# redis - this script starts and stops the redis-server daemon
#
# chkconfig:   - 85 15 
# description:  Redis is a persistent key-value database
# processname:  redis-server
# config:      /etc/redis/redis.conf
# config:      /etc/sysconfig/redis
# pidfile:     /var/run/redis.pid

# Source function library.
. /etc/rc.d/init.d/functions

# Source networking configuration.
. /etc/sysconfig/network

# Check that networking is up.
[ "$NETWORKING" = "no" ] && exit 0

redis="/usr/local/bin/redis-server"
prog=$(basename $redis)

REDIS_CONF_FILE="/etc/redis/redis.conf"

[ -f /etc/sysconfig/redis ] && . /etc/sysconfig/redis

lockfile=/var/lock/subsys/redis

start() {
    [ -x $redis ] || exit 5
    [ -f $REDIS_CONF_FILE ] || exit 6
    echo -n $"Starting $prog: "
    daemon $redis $REDIS_CONF_FILE
    retval=$?
    echo
    [ $retval -eq 0 ] && touch $lockfile
    return $retval
}

stop() {
    echo -n $"Stopping $prog: "
    killproc $prog -QUIT
    retval=$?
    echo
    [ $retval -eq 0 ] && rm -f $lockfile
    return $retval
}

restart() {
    stop
    start
}

reload() {
    echo -n $"Reloading $prog: "
    killproc $redis -HUP
    RETVAL=$?
    echo
}

force_reload() {
    restart
}

rh_status() {
    status $prog
}

rh_status_q() {
    rh_status >/dev/null 2>&1
}

case "$1" in
    start)
        rh_status_q && exit 0
        $1
        ;;
    stop)
        rh_status_q || exit 0
        $1
        ;;
    restart|configtest)
        $1
        ;;
    reload)
        rh_status_q || exit 7
        $1
        ;;
    force-reload)
        force_reload
        ;;
    status)
        rh_status
        ;;
    condrestart|try-restart)
        rh_status_q || exit 0
	    ;;
    *)
        echo $"Usage: $0 {start|stop|status|restart|condrestart|try-restart|reload|force-reload}"
        exit 2
esac
EOF
sudo mv $REDIS_INIT /etc/init.d/redis
sudo chmod a+x /etc/init.d/redis

# Install Rush
wget https://github.com/telefonicaid/Rush/archive/1.8.3.tar.gz
tar xfvz 1.8.3.tar.gz
sudo mkdir -p /opt
sudo mv Rush-1.8.3/ /opt/Rush
rm -rf 1.8.3.tar.gz

RUSH_INIT=$(mktemp)
cat > $RUSH_INIT <<'EOF'
#!/bin/bash
# chkconfig: 2345 95 20
# description: Rush HTTP Relayer
# Startup script for the Rush processes. It starts one listener and one consumer.
# processname: rush

RUSH_LOCATION=/opt/Rush

function status_rush() {
  ps -ef |grep -v grep | egrep "node.*Rush.*" > /dev/null
  RESULT=$?

  if [[ $RESULT = 0 ]]; then
    echo "Rush running"
  else
    echo "Rush stopped"
  fi

  exit $RESULT
}

function start_rush() {
  ps -ef |grep -v grep | egrep "node.*rush" > /dev/null
  RESULT=$?

  if [[ $RESULT -eq 1 ]]; then
    echo "Starting rush processes"
    nohup nice -n -5 $RUSH_LOCATION/bin/listener &>> /var/log/rush-listener.log&
    nohup nice -n -5 $RUSH_LOCATION/bin/consumer &>> /var/log/rush-consumer.log&
  else
    echo "Rush processes already running"
  fi
}

function stop_rush() {
  ps -ef |grep -v grep | egrep "node.*Rush.*" |awk '{print $2}' |xargs -I '{}' kill '{}'
}

case "$1" in
    start)
        start_rush
    ;;
    stop)
        stop_rush
    ;;
    restart)
        stop_rush
        start_rush
    ;;
    status)
        status_rush
    ;;
esac
EOF
sudo mv $RUSH_INIT /etc/init.d/rush
sudo chmod a+x /etc/init.d/rush

cd /opt/Rush
sudo npm install --production 

# Generate self-signed Rush certificate
GEN_CERT=$(mktemp)
cat > $GEN_CERT <<EOF
openssl genrsa -des3 -passout pass:fiware -out server.key 1024
openssl req -new -passout pass:fiware -passin pass:fiware -subj "/C=US/ST=Denial/L=Springfield/O=Dis/CN=www.example.com" -key server.key -out server.csr
cp server.key server.key.org
openssl rsa -in server.key.org -passin pass:fiware -out server.key
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt
EOF
sudo mv $GEN_CERT /opt/Rush/utils/create_http_certificates.sh
cd /opt/Rush/utils
sudo bash create_http_certificates.sh
