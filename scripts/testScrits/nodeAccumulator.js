// Simple server for Orion contexBroker notifications: 
var http = require('http');
var https = require('https');

//verbose mode flag. Set it to true to print the input posted data
var vm = false;

//Ports to listen
var port1=1028;
var port2=1029;
var port3=1030;
//delayed reponse port
var port4=1031;

//accumulator 
var requests = 0;
// show requests every X ms
var muestreo = 1000;
// delayed response
var delayedtime=2000;

console.log("### ACCUMULATOR SERVER STARTED ###", Date());

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

// start sever 1
srv.listen(port1);

// Create an HTTP server
var srv2 = http.createServer(function(req, res) {
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
      res.end('OK-2');
    });
  });
});

// start sever 2
srv2.listen(port2);

// Create an HTTP server 3
var srv3 = http.createServer(function(req, res) {
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

// start sever 3
srv3.listen(port3);



var srv4 = http.createServer(function(req, res) {
  'use strict'; 
  requests++;

  req.on('data', function(data) {
     if(vm){
	console.log('Notification #B: ' + data);
    }
  });
  req.on('end', function() {
    setTimeout(function() {
      res.writeHead(200, {'Content-Type': 'text/plain'});
      res.end('delayed OK-4');
    }, delayedtime);
  });
});

// start sever 4
srv4.listen(port4);


setInterval(function () {
  console.log('Req: ' + requests);
  if (requests != 0) {
    requests = 0;
  }
}, muestreo);

