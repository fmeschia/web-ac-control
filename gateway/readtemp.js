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
