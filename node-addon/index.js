var binn = require("binn.js");
var zeromq = require('zeromq');

var dlis = require("./build/Release/dlis_parser");

var socket = zeromq.socket("rep");
//var socket = zeromq.socket("pull");


const sendingDataType =  {
    _SET: 0,
    _OBJECT: 1,
    _OBJ_VALUE: 2,
}
const functionIdx = {
    EFLR_DATA: 0,
    IFLR_HEADER: 1,
    IFLR_DATA: 2,
    GET_REPCODE: 3,
    GET_DIMENSION: 4
}

let setType;
let channels = {};
let dlisFrames = {};
/*
    channels = {
        "channel1": {
            repcode: 19,
            dimension: 1,
            ...
         },
         "channel2": {
            repcode: 19,
            dimension: 1,
            ...
         }
    }

    dlisFrame = {
        "frame1": {
            channels: [
                "channel1", "channel2"
            ]
        }
    }
*/

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
let parsingIndex = 0;
let parsingData = [];
let parsingValueCnt = 0;
let fdata = [];

var cnt = 0;

function eflr_data(myObj) {
    //console.log(cnt++);
    //console.log(myObj)
    switch(myObj.sending_data_type) {
    case sendingDataType._SET:
        setType = myObj.type;
        if(setType == "CHANNEL") {
            currentSet = channels;
        }else if(setType == "FRAME"){
            currentSet = dlisFrames;
        }else {
            currentSet = null;
        }
        break;
    case sendingDataType._OBJECT:
        //console.log("---->OBJECT --- " + JSON.stringify(myObj, null, 2));
        const objName = obname2Str(myObj);
        if(currentSet){
            currentSet[objName] = myObj;
        }
        break;
    }
    return 23;
}
function get_repcode(){
    if (parsingData.length <= parsingIndex) return -1;
    //console.log("return repcode: " + parsingData[parsingIndex].repcode);
    return parsingData[parsingIndex].repcode;
}
function get_dimension(){
    return parsingData[parsingIndex].dimension;
}
function iflr_header(myObj) {
    //console.log("iflr " + cnt);cnt++;
    //process.exit(1);
    var frameName = obname2Str(myObj.frame_name);
    console.log("frame name: " + frameName);
    var frameObj = dlisFrames[frameName];
    var channelList = frameObj["CHANNELS"];
    parsingData.length = 0;
    fdata.length = 0;
    for (var chan of channelList) {
        let channelName = obname2Str(chan);
        let channelObj = channels[channelName];
        let repcode = channelObj["REPRESENTATION-CODE"][0];
        let dimension = channelObj["DIMENSION"][0];
        parsingData.push({repcode, dimension});
        fdata.push({name:chan, data:[]});
    }
    parsingIndex = 0;
    console.log("___frame name: "+ obname2Str(myObj.frame_name) + " index: "+ myObj.fdata_index + " number of channels: " + parsingData.length);
    return 24;
}
function iflr_data(myObj) {
    parsingValueCnt++;
    fdata[parsingIndex].data.push(myObj.value);
    if(parsingValueCnt >= parsingData[parsingIndex].dimension){
        //console.log(fdata[parsingIndex].name + ": " + fdata[parsingIndex].data);
        parsingIndex++;
        parsingValueCnt = 0;
    }
    return 25;
}

/*
    {
        origin: "",
        copy_number:""
        name: ""
        "abc": []
    }
*/

socket.on("message", function(buffer) {
    let myObj = binn.decode(buffer);
    //console.log(myObj);
    //console.log(cnt + " --- " + myObj.functionIdx);
    //cnt++;
    let retval = 0;
    switch(myObj.functionIdx){
        case functionIdx.EFLR_DATA:
            retval = eflr_data(myObj);
            break;
        case functionIdx.GET_REPCODE:
            retval = get_repcode();
            break;
        case functionIdx.GET_DIMENSION:
            retval = get_dimension();
            break;
        case functionIdx.IFLR_HEADER:
            retval = iflr_header(myObj);
            break;
        case functionIdx.IFLR_DATA:
            retval = iflr_data(myObj);
            break;
    }
    socket.send("" + retval);
});

socket.bindSync('ipc:///tmp/dlis-socket');
//socket.connect('ipc:///tmp/dlis-socket');

console.log("connect done");
var temp = dlis.parse("../AP_1225in_MREX_E_MAIN.dlis");
//console.log(temp);
//console.log(JSON.stringify(channels, null, 2));
//console.log(JSON.stringify(dlisFrames, null, 2));
