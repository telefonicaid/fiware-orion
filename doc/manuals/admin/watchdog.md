# Watchdog

Although Orion Context Broker is highly stable, it may fail (see the
section on [diagnosis procedures](diagnosis.md) for
more information about detecting problems with the broker). Thus, it is
recommendable to use a watchdog process to detect if the broker process
has stopped running, so it can be re-started automatically and/or you
get a notification of the problem.

You can write the watchdog program yourself (e.g. a script invoked by
cron in a regularly basic that checks /etc/init.d/contextBroker status
and starts it again if is not working and/or send you a notification
email) or use existing tools. This section includes a procedure using
[Monit](http://mmonit.com/monit/).

First of all, install monit:

    sudo apt-get install monit

Create a directory for monit stuff, eg:

    /home/orion/monit_CB

Create monitBROKER.conf file in that directory. In this example, we
configure monit to restart contextBroker if CPU load is greater than 80%
for two cycles or if allocated memory is greater than 200MB for five
cycles (that would be a symptom of memory leaking). In addition to
resource checking, monit will restart the process if it is down. The
duration of a cycle is defined using a monit command line parameter
(described below).

    ###############################################################################
    ## Monit control file
    ###############################################################################
    ##
    ## Comments begin with a '#' and extend through the end of the line. Keywords
    ## are case insensitive. All path's MUST BE FULLY QUALIFIED, starting with '/'.
    ##
    ##
    ###############################################################################
    ## Global section
    ###############################################################################
    ##

    set logfile /var/log/contextBroker/monitBROKER.log

    set statefile /var/log/contextBroker/monit.state

    ###############################################################################
    ## Services
    ###############################################################################
    ##

    check host localhost with address localhost
       if failed (url http://localhost:1026/version and content == '<version>') for 3 cycles then
          exec "/etc/init.d/contextBroker stop"

    check file monitBROKER.log with path /var/log/contextBroker/monitBROKER.log
       if size > 50 MB then
          exec "/bin/bash -c '/bin/rm /var/log/contextBroker/monitBROKER.log; monit -c /home/localadmin/monit_CB/monitBROKER.conf -p /var/log/contextBroker/monit.pid reload'"

    check process contextBroker  with pidfile /var/log/contextBroker/contextBroker.pid    start program = "/etc/init.d/contextBroker start"    stop program  = "/etc/init.d/contextBroker stop"
        if cpu > 60% for 2 cycles then alert
        if cpu > 80% for 5 cycles then restart
        if totalmem > 200.0 MB for 5 cycles then restart

Make root the owner of that file and set permissions only for owner:

    sudo chown root:root monitBROKER.conf
    sudo chmod 0700 monitBROKER.conf

Create monit start script start\_monit\_BROKER.sh. The "-d" command line
parameter is used to specify the checking cycle duration (in the example
we are setting 10 seconds).

    monit -v -c /home/orion/monit_CB/monitBROKER.conf -d 10 -p /var/log/contextBroker/monit.pid

Make root the owner of that file and set execution permissions:

    sudo chown root:root start_monit_BROKER.sh
    sudo chmod a+x start_monit_BROKER.sh  

To run monit do:

    cd /home/orion/monit_CB
    sudo ./start_monit_BROKER.sh

To check that monit is working properly, check that the process exist,
e.g.:

    # ps -ef | grep contextBroker
    500      27175     1  0 21:06 ?        00:00:00 monit -v -c /home/localadmin/monit_CB/monitBROKER.conf -d 10 -p /var/log/contextBroker/monit.pid
    500      27205     1  0 21:06 ?        00:00:00 /usr/bin/contextBroker -port 1026 -logDir /var/log/contextBroker -pidpath /var/log/contextBroker/contextBroker.pid -dbURI mongodb://localhost/ -db orion;

Then, kill contextBroker, e.g.:

    #kill 27205

and check with ps that after a while (less than 30 seconds)
contextBroker is up again.
