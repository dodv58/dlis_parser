var dlis = require("./build/Release/dlis_parser");
dlis.on('eflr-data', function(buffer) {
    console.log(msg);
});
dlis.on('repcode-req', function() {
});
dlis.on('dimension-req', function(){
});
var temp = dlis.parse("../sample.dlis");
console.log(temp);
