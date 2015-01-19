var mysql		= require('mysql');
var exec 		= require('child_process');
var config 		= require('./config');

var connection = mysql.createConnection({
  host     : config.database.host,
  database : config.database.db,
  user     : config.database.user,
  password : config.database.password
});

exec.execFile(__dirname+ '/../nRF2401/ardu-temp', [], finished);


var x = [];
var y = [];
connection.connect();
//var query = 'SELECT date, temp FROM temperature where DATEDIFF(SYSDATE(),date) < 1';
var query = 'SELECT TIME_TO_SEC(TIMEDIFF(date, SYSDATE()))/60 as mydate, temp FROM temperature WHERE TIME_TO_SEC(TIMEDIFF(SYSDATE(),date)) < 86400';
connection.query(query, function(err, rows, fields) {
	
	for (var i=0; i<rows.length; i++) {
		x.push(rows[i].mydate);
		y.push(rows[i].temp);
	}
	var plotly = require('plotly')('fmeschia','0buief609b');
	var data = [
		{
		 	x: x,
	    	y: y, 
	    	type: "scatter",
			line: {
				shape: "hv",
				smoothing: 1
			},
			connectgaps: false
	  	}
	];
	var graphOptions = {
		filename: "date-axes", 
		fileopt: "overwrite", 
		layout: { 
		yaxis: {
			type: "linear",
			range: [22.266666666666666, 22.93333333333333],
			autorange: true,
			title: "Temperature [C]",
			showticklabels: true
		},
		xaxis:{
			showexponent:"first",
			showticklabels:false,
			title:"Time",
			type:"linear",
			autorange:true,
			exponentformat:"none"
		},
		height: 538,
		width: 761,
		autosize: true,
		showlegend: false,
		title: "Temperature over the last 24 hours"
	}};
	plotly.plot(data, graphOptions, function (err, msg) {
		if (err) console.log(err);
		if (msg) console.log(msg);
	});
});
connection.end();

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

