/*
 # Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
 */

//script version
var version="0.0.2"

//verbosity flag
var vm=false;
//silence flag
var silence=false;

// Simple server for Orion contexBroker notifications:
var http = require('http');

//amount of servers to launch
var servers=3;
//Ports to listen
var port=[1028,1029,1030];

//amount of servers with delay to launch
var delayedServers=1;
//forced delayed response port
var delayedPort=[1031];

//accumulator setup
var requests = 0;
// show requests every X ms
var samples = 1000;
// delayed response
var delayedtime=2000;



//Standar CLI options
process.argv.forEach(function (val, index, array) {
	//console.log(index + ': ' + val);
	if (val=="--version"){
		console.log("# Accumulator server version: " + version );
		process.exit(0);
	}
	if (val=="-u"){
		console.log("Usage info: \n " +
				"Accumulator server \n" +
				" Default configuration launch 3 servers on ports 1028,1029,1030 and 1 server on port 1031 with delayed response of 2 seconds. \n \n" +
				"for modify the configuration use: > noode nodeAccumulator.js [-u -v --version --silence -s -p -ds -dp] \n" +
				" \n - Params \n" +
				" '-u' : Shows this usage info \n" +
				" '-sX' : amount of servers to run (without delay). '-d5' will launch 5 servers \n"+
				" '-pX' : Port Server, X is the first port number to setup the server, the rest of listener servers will be added in the next port numbers \n"+
				" '-dsY' : amount of delayed servers tu run with delay. '-ds3' will launch 3 servers  \n"+
				" '-dpY' : Port Delayed Server, Y is the first port number to setup the Delayed server, the rest of listener servers will be added in the next port numbers \n"+
				" '--silence' : no info showed in log \n" +
				" '--version' : shows the version of this script \n"+
				" '-v' : verbose mode, by default OFF \n");
		process.exit(0);
	}
	if (val=="-v"){
		console.log("# Verbosity activated");
		vm=true;
	}
	if (val=="--silence"){
		silence=true;
	}
	if (val.match(/^-s/)) {
		servers=val.slice(2);
	}
	if (val.match(/^-ds/)) {
		delayedServers=val.slice(3);
	}
	if (val.match(/^-p/)) {
		console.log("Setup ports starting at : ",val.slice(2));
		for (var i=0; i<servers; i++){
			port[i]=val.slice(2);
			port[i]=eval(port[i])+i;
		}
	}
	if (val.match(/^-dp/)) {
		console.log("Setup Delayed ports starting at : ",val.slice(3));
		for (var i=0; i<delayedServers; i++){
			delayedPort[i]=val.slice(3);
			delayedPort[i]=eval(delayedPort[i])+i;
		}
	}

});

// Create an HTTP server
var srv =function(){
	return http.createServer(function(req, res) {
		'use strict';
		requests++;

		req.on('data', function(data) {
			if(vm){
				console.log('# Notification Data: ' + data);
			}
		});
		req.on('end', function() {
			setTimeout(function() {
				res.writeHead(200, {'Content-Type': 'text/plain'});
				res.end('OK \n');
			});
		});
	})
};

var delaySrv = function(){
	return http.createServer(function(req, res) {
		'use strict';
		requests++;

		req.on('data', function(data) {
			if(vm){
				console.log('# Notification Delayed server: ' + data);
			}
		});
		req.on('end', function() {
			setTimeout(function() {
				res.writeHead(200, {'Content-Type': 'text/plain'});
				res.end('OK delayed \n');
			}, delayedtime);
		});
	})
};

// start servers
for (var i=0; i<servers; i++){
	srv().listen(port[i]);
	if (!silence){
		console.log("### ACCUMULATOR SERVER  #"+ eval(i+1) +" STARTED at port " + port[i] +" ###", Date());}
};

// start severs with delay
for (var i=0; i<delayedServers; i++){
	delaySrv().listen(delayedPort[i]);
	if(!silence){
		console.log("### ACCUMULATOR Delayed SERVER #"+ eval(i+1) +" + STARTED at port " + delayedPort[i] +" ###", Date());}
};


setInterval(function () {
	if(!silence){
		console.log('Req: ' + requests);
	}
	if (requests != 0) {
		requests = 0;
	}
}, samples);



