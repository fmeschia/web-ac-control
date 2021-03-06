/*****

Web A/C Remote Control - the Arduino remote
by Francesco Meschia

This program implements the web application that controls the
A/C unit

================================
Copyright 2015 Francesco Meschia

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/
var express = require('express');
var app = express();
var httpapp = express();
var https = require('https');
var http = require('http')
var auth = require('basic-auth');
var exec = require('child_process');
var session = require('express-session');
var log4js = require('log4js');
var mysql = require('mysql');
var fs = require('fs'); 
var config = require('./config');

var options = {
	 key: fs.readFileSync(config.ssl.privatekey),
	 cert: fs.readFileSync(config.ssl.certificate)
};

var passwords = JSON.parse(fs.readFileSync(__dirname+'/passwords.json', 'utf8'));

var server=https.createServer(options, app);
var serverhttp = http.createServer(httpapp);

serverhttp.listen(80);
server.listen(443, function() {
	console.log('Listening');
});

httpapp.use(function(req,res,next) {
	res.redirect('https://' + req.hostname + req.url);
});

var sessionMiddleware = session({
	secret: config.middleware.secret,
	resave: false,
	saveUninitialized: true
});

app.use(sessionMiddleware);
var io = require('socket.io')(server);
io.use(function(socket, next) {
    sessionMiddleware(socket.request, socket.request.res, next);
});

app.use(function(req, res, next) {
    var user = auth(req);
	var logger = log4js.getLogger();
	var authenticated = 0;
	if (user !== undefined && 
		user['name'] !== undefined && user['name'] !== '' &&
	    user['pass'] !== undefined && user['pass'] !== '') {
		if (user['pass'] === passwords[user['name']]) {
			authenticated = 1;
		} else {
			logger.warn('Unauthenticated user '+user['name']);
		}
	}
	if (!authenticated) {
        res.statusCode = 401;
        res.setHeader('WWW-Authenticate', 'Basic realm="Home Gateway"');
        res.end('Unauthorized');
    } else {
		req.session.user = user['name'];
		logger.info('Authenticated user '+user['name']);
        next();
    }
});

app.get('/', function(req, res){
  res.sendFile(__dirname + '/index.html');
});

app.get('/tempdata.csv', function(req,res) {
	var connection = mysql.createConnection({
	  host     : config.database.host,
	  database : config.database.db,
	  user     : config.database.user,
	  password : config.database.password
	});
 	var csv = "Time,Temperature\n";
	var y = [];
	connection.connect();
	var query = 'SELECT date, temp FROM temperature where TIME_TO_SEC(TIMEDIFF(SYSDATE(),date)) < 2*86400';
	connection.query(query, function(err, rows, fields) {	
		for (var i=0; i<rows.length; i++) {
			csv += rows[i].date + "," + (parseFloat(rows[i].temp)*1.8+32.0) + '\n';
		}
		res.send(csv);
	});
	connection.end();
});

io.on('connection', function(socket){
	var logger;
	if (socket.request.session !== undefined) {
		if (socket.request.session.user != undefined) {
			logger = log4js.getLogger(socket.request.session.user);
		} else {
			logger = log4js.getLogger();
		} 
	} else {
		logger = log4js.getLogger();
	}
  	socket.on('turnon', function(msg){
		logger.info('Turning ON');
		// code = turn ON, fan auto, 70 F, A/C
		exec.execFile(__dirname+ '/../nRF2401/ardu-ir',['0xea002aa1'],{cwd:__dirname+'/../nRF2401/'});
  	});
  	socket.on('turnoff', function(msg){
		logger.info('Turning OFF');
		exec.execFile(__dirname+ '/../nRF2401/ardu-ir',['0x6a002aa9'],{cwd:__dirname+'/../nRF2401/'});
  	});
  	socket.on('readtemp', function(msg){
  		logger.info('Reading temperature');
		exec.execFile(__dirname+ '/../nRF2401/ardu-temp', [], function(error, stdout, stderr) {
			var temp = Math.round(parseFloat(stdout) * 18 + 320)/10;
			logger.debug('Temperature = ' + temp + " F");
			socket.emit("tempreadout",temp);
		});
  	});
});



//http.listen(443, function(){
//  console.log('Listening');
//});
