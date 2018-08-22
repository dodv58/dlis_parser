var binn = require("binn.js");
var zeromq = require('zeromq');

module.exports.parseFile = parseFile;

var socket = zeromq.socket("rep");
socket.bindSync('ipc:///tmp/dlis-socket');
console.log("bind done");

var dlis = require("./build/Release/dlis_parser");
function initDlis(userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onCurveDataCb, onEnd) {
//    var socket = zeromq.socket("rep");
    //var socket = zeromq.socket("pull");
    var instance = {
        parser: dlis,
        socket,
        well: {},
        userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onCurveDataCb, onEnd
    }
    socket.on("message", function(buffer) {
        let myObj = binn.decode(buffer);
        let retval;
        if(myObj.ended){
            onEnd(); 
        } else {
            switch(myObj.functionIdx){
                case functionIdx.EFLR_DATA:
                    retval = eflr_data(instance, myObj);
                    break;
                case functionIdx.GET_REPCODE:
                    retval = get_repcode();
                    break;
                case functionIdx.IFLR_HEADER:
                    retval = iflr_header(instance, myObj);
                    break;
                case functionIdx.IFLR_DATA:
                    retval = iflr_data(instance, myObj);
                    break;
            }
        }
        socket.send(retval);
    });

//    socket.bindSync('ipc:///tmp/dlis-socket');
    //socket.connect('ipc:///tmp/dlis-socket');

    return instance;
}

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
}

let parsingIndex = 0; //index of parsing channel in parsing frame
let parsingData = [];
let fdata = [];

var cnt = 0;

let setType;
let channels = {};
let dlisFrames = {};
let dlisOrigin = {};
let currentSet;
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
function safeObname2Str(obj) {
    if (typeof obj === "object" && Number.isInteger(obj.origin) && Number.isInteger(obj.copy_number) && obj.name) {
        return obname2Str(obj);
    }
    return obj;
}
function obname2Str(obj) {
    return obj.origin + "-" + obj.copy_number + "-" + obj.name;
}

function parseFile(fileName, userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onCurveDataCb, onEnd) {
    var DlisEngine = initDlis(userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onCurveDataCb, onEnd);
    let temp = DlisEngine.parser.parseFile(fileName);
}
function eflr_data(dlisInstance, myObj) {
    //console.log(cnt++);
    //console.log("===> " + myObj);
    switch(myObj.sending_data_type) {
    case sendingDataType._SET:
        setType = myObj.type;
        if(setType == "CHANNEL") {
            currentSet = channels; 
        }else if(setType == "FRAME"){
            currentSet = dlisFrames;
        }else if (setType == "ORIGIN"){ 
            currentSet = dlisOrigin;
        }
        else {
            currentSet = null;
        }
        break;
    case sendingDataType._OBJECT:
        //console.log("---->OBJECT --- " + JSON.stringify(myObj, null, 2));
        const objName = obname2Str(myObj);
        if(currentSet){
            delete myObj["sending_data_type"];
            delete myObj["functionIdx"];
            currentSet[objName] = myObj;
            if(setType == "ORIGIN" && objName.indexOf("DEFINING_ORIGIN") != -1){
                myObj.userInfo = dlisInstance.userInfo;
                dlisInstance.onWellInfoCb(myObj);
            }
            else if(setType == "FRAME") {
                dlisInstance.onDatasetInfoCb(myObj);
            }
            else if(setType == "CHANNEL"){
                dlisInstance.onCurveInfoCb(myObj);
            }
        }
        break;
    }
    return 23;
}
function get_repcode(){
    /*
    if (parsingData.length <= parsingIndex) return -1;
    return parsingData[parsingIndex].repcode;
    */
    if(parsingData.length <= parsingIndex) {
        let obj = {repcode: -1, dimension: -1};
        return binn.encode(obj);
    }else {
        let obj = binn.encode(parsingData[parsingIndex]);
        //parsingIndex++;
        return obj;
    }
}
function iflr_header(instance, myObj) {
    //console.log("iflr " + cnt);cnt++;
    //console.log("==>frame: " + JSON.stringify(dlisFrames, null, 2));
    //console.log("==>channel: " + JSON.stringify(channels, null, 2));
    //console.log("==>origin: " + JSON.stringify(dlisOrigin, null, 2));
    //process.exit(1);
    if(fdata.length > 0){
        instance.onCurveDataCb(fdata);
    }

    var frameName = obname2Str(myObj.frame_name);
    //console.log("frame name: " + frameName);
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
        fdata.push({name:channelName, data:[]});
    }
    parsingIndex = 0;
    //console.log("___frame name: "+ obname2Str(myObj.frame_name) + " index: "+ myObj.fdata_index + " number of channels: " + parsingData.length);
    return 24;
}
function iflr_data(instance, myObj) {
    fdata[parsingIndex].data = myObj.values;
    console.log(fdata[parsingIndex].name + ": " + fdata[parsingIndex].data);
    parsingIndex++;
    return 25;
}

//parse(process.argv[2]);

