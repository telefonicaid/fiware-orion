This directory contains some configuration files, to be used (mounting a volume) by mosquitto docker containers. See functional.yml (and some other GitActions) for an example on how they are used.

The `mosquitto_passwd` file containts a harmless credential: `user1` with password `xxxx`, used in `cases/3001_mqtt_alarms` tests. The password file works for mosquitto 2.0 version but other versions (specifically, 1.6) could not recognize its format.

This files can be also being used in a local mosquitto instance. For instance

```
$ sudo cp /path/to/repo/test/functionalTest/mosquittoConf/mosquitto.conf /etc/mosquitto/conf.d/local.conf
$ # edit the /etc/mosquitto/conf.d/local.conf file to change the password_file location to /etc/mosquitto/passwd
$ sudo cp /path/to/repo/test/functionalTest/mosquittoConf/mosquitto_passwd /etc/mosquitto/passwd
$ sudo chmod 640 /etc/mosquitto/passwd
$ sudo chown root:mosquitto /etc/mosquitto/passwd
```

Alternativelly you can add users to password file using a command like this one:

```
$ sudo mosquitto_passwd -c /etc/mosquitto/passwd user1
```


