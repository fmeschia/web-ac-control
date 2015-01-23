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
	//console.log(stdout);
	temp = parseFloat(stdout) / 10.0;
	console.log('temp = '+temp);
	if (isNaN(temp)) temp = null;
	var record = {date: new Date(), temp:temp};
	var query = connection.query('INSERT INTO temperature SET ?', record);
	connection.end();
}
