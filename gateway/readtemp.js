/*****

Web A/C Remote Control - the Arduino remote
by Francesco Meschia

This program reads the temperature from the Arduino remote unit
and stores into the SQL database.

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

var mysql		= require('mysql');
var exec 		= require('child_process');
var config      = require('./config');

exec.execFile(__dirname+ '/../nRF2401/ardu-temp', [], finished);

function finished(error, stdout, stderr) {
	var connection = mysql.createConnection({
	  host     : config.database.host,
	  database : config.database.db,
	  user     : config.database.user,
	  password : config.database.password
	});
	connection.connect();
	console.log('raw stdout = '+stdout);
	temp = parseFloat(stdout);
	console.log('temp       = '+temp);
	if (isNaN(temp)) {
		console.error((new Date()) + ' - raw stdout = '+stdout);
		temp = null;
	}
	var record = {date: new Date(), temp:temp};
	connection.query('INSERT INTO temperature SET ?', record);
    //connection.query('DELETE FROM temperature WHERE TIME_TO_SEC(TIMEDIFF(SYSDATE(),date)) > 7*86400');
	connection.end();
}
