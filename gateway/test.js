var fs = require('fs');
var obj = JSON.parse(fs.readFileSync('passwords.json', 'utf8'));
console.log(obj['julie']);