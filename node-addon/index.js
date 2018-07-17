var binn = require("binn.js");
var dlis = require("./build/Release/dlis_parser");

const sendingDataType =  {
    _SET: 0,
    _OBJECT: 1,
    _OBJ_VALUE: 2,
}
let currentObj = {};
let templateProps = [];
let setType;
let channels = {};
let dlisFrames = {};
let currentSet;
function safeObname2Str(obj) {
    if (typeof obj === "object" && Number.isInteger(obj.origin) && Number.isInteger(obj.copy_number) && obj.name) {
        return obname2Str(obj);
    }
    return obj;
}
function obname2Str(obj) {
    return obj.origin + "-" + obj.copy_number + "-" + obj.name;
}
dlis.on("eflr-data", function(buffer) {
    var myObj = binn.decode(buffer);
    switch(myObj.sending_data_type) {
    case sendingDataType._SET:
        //console.log("---->SET --- Template:");
        setType = myObj.type;
        if(setType == "CHANNEL") {
            currentSet = channels;
        }else if(setType == "FRAME"){
            currentSet = dlisFrames;
        }else {
            currentSet = null;
        }
        templateProps.length = 0;
        break;
    case sendingDataType._OBJECT:
        //console.log("---->OBJECT ---", "SETTYPE=" + setType, JSON.stringify(currentObj, null, 4));
        const objName = obname2Str(myObj);
        if(currentSet){
            currentSet[objName] = { };
            currentObj = currentSet[objName];
        }else {
            currentObj = {};
        }
        break;
    case sendingDataType._OBJ_VALUE:
        if(!currentObj[myObj.label]){
            currentObj[myObj.label] = [];
        }
        if (typeof myObj.value === 'string') {
            currentObj[myObj.label].push(myObj.value.trim());
        }
        else {
            currentObj[myObj.label].push(safeObname2Str(myObj.value));
        }
        break;
    }
    return 23;
});

let parsingIndex = 0;
let parsingData = [];

dlis.on("get-repcode", function(){
    //console.log("--->get_repcode: " + parsingData[parsingIndex].repcode + " dimension: " + parsingData[parsingIndex].dimension)
    if (parsingData.length <= parsingIndex) return -1;
    return parsingData[parsingIndex].repcode;
});

dlis.on("get-dimension", function(){
    return parsingData[parsingIndex].dimension;
});

dlis.on("iflr-header", function(buffer) {
    var myObj = binn.decode(buffer);
    var frameName = obname2Str(myObj.frame_name);
    var frameObj = dlisFrames[frameName];
    var channelList = frameObj["CHANNELS"];
    parsingData.length = 0;
    for (var chan of channelList) {
        let channelObj = channels[chan];
        let repcode = channelObj["REPRESENTATION-CODE"][0];
        let dimension = channelObj["DIMENSION"][0];
        parsingData.push({repcode, dimension});
    }
    parsingIndex = 0;
    console.log("___frame name: "+ obname2Str(myObj.frame_name) + " index: "+ myObj.fdata_index + " number of channels: " + parsingData.length);
    return 24;
});

dlis.on("iflr-data", function(buffer) {
    parsingIndex++;
    return 25
});

var temp = dlis.parse("../AP_1225in_MREX_E_MAIN.dlis");
console.log(temp);
//console.log(JSON.stringify(channels, null, 2));
//console.log(JSON.stringify(dlisFrames, null, 2));
