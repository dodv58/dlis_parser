var binn = require("binn.js");
var dlis = require("./build/Release/dlis_parser");

const sendingDataType =  {
    _SET: 0,
    _OBJECT: 1,
    _OBJ_TEMPLATE: 2,
    _OBJ_VALUE: 3
}
let currentParsingFrame; // string. co khi thang C++ no parse khuc dau cua IFLR
let currentParsingChannel; // integer. bat dau co khi cos currentParsingFrame

let dlisPackage;
/*
let dlisPackage = {
    "frame1": [{
        repcode: 18,
        dimension: 1
    }, {
        
        repcode: 18,
        dimension: 1
    }]
    "frame2": []
}
*/

function newFrame(frameName) {
    dlisPackage[frameName] = [];
    // TO BE CONTINUE ...
}

function addChannelToFrame(frameName, repcode, dimension) {
    // TODO   
}


dlis.on("eflr-data", function(buffer) {
    var myObj = binn.decode(buffer);
    switch(myObj.sending_data_type) {
    case sendingDataType._SET:
        console.log(myObj);
        break;
    case sendingDataType._OBJECT:
        console.log(myObj);
        break;
    case sendingDataType._:
        console.log(myObj);
        break;
    case sendingDataType._SET:
        console.log(myObj);
        break;
        // ...
        // ...
    }
    //console.log(myObj);
    return 23;
});

dlis.on("iflr-data", function(buffer) {
});

var temp = dlis.parse("../sample.dlis");
console.log(temp);
