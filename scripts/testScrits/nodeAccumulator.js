// Simple server for Orion contexBroker notifications: 
var http = require('http');
var https = require('https');

//verbose mode flag
var vm = false;

#Ports to listen
var port1=1028;
var port2=1029;
var port3=1030;

//accumulator 
var requests = 0;

// Create an HTTP server
var srv = http.createServer(function(req, res) {
  'use strict';

  requests++;

  req.on('data', function(data) {
  if(vm){
  console.log('Notification #A: ' + data);
   }
  });
  req.on('end', function() {
    setTimeout(function() {
      res.writeHead(200, {'Content-Type': 'text/plain'});
      res.end('OK-1');
    });
  });
});

setInterval(function () {
  console.log('Req: ' + requests);
  if (requests != 0) {
    requests = 0;
  }
}, 1000);

// start sever 1
srv.listen(port1);

var srv2 = http.createServer(function(req, res) {
  'use strict';

  req.on('data', function(data) {
     if(vm){
	console.log('Notification #B: ' + data);
    }
  });
  req.on('end', function() {
    setTimeout(function() {
      res.writeHead(200, {'Content-Type': 'text/plain'});
      res.end('OK-2');
    }, 2 * 1000);
  });
});

// start server 2
srv2.listen(port2);

var srv3 = http.createServer(function(req, res) {
  'use strict';

  req.on('data', function(data) {
   if(vm){
    console.log('Notification #C: ' + data);
    }
  });
  req.on('end', function() {
    setTimeout(function() {
      res.writeHead(200, {'Content-Type': 'text/plain'});
      res.end('OK-3');
    }, 2 * 1000);
  });
});

// start sever 3
srv3.listen(port3);

