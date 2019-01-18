var binn = require("binn.js");
var zeromq = require('zeromq');
const fs = require('fs');

module.exports.parseFile = parseFile;

var socket = zeromq.socket("rep");
socket.bindSync('ipc:///tmp/dlis-socket-' + process.pid);
console.log("bind done " + process.pid);

/*
var socket = zeromq.socket("pull");
socket.connect('ipc:///tmp/dlis-socket');
*/

var dlis = require("./build/Release/dlis_parser");
function initDlis(userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onEnd) {
//    var socket = zeromq.socket("rep");
    //var socket = zeromq.socket("pull");
    var instance = {
        parser: dlis,
        socket,
        userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onEnd
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
                /*    
                case functionIdx.IFLR_HEADER:
                    retval = iflr_header(instance, myObj);
                    break;
                */
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
    EFLR_DATA: 0
    //IFLR_HEADER: 1
}

let parsingIndex = 0; //index of parsing channel in parsing frame
let parsingData = [];
let fdata = [];


let setType;
let channels = {};
let dlisFrames = {};
let dlisOrigin = {};
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

function mkdirSyncRecursive(path){
    if(!/^\//.test(path)){
        path = __dirname + '/' + path;
    }
    let arr = path.split('/').filter(Boolean);
    const curDir = arr.pop();
    const parentDir = '/' + arr.join('/');
    if(!fs.existsSync(parentDir)){
        mkdirSyncRecursive(parentDir);
    } 
    
    fs.mkdirSync(parentDir + '/' + curDir);
}

function parseFile(fileName, userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onEnd) {
    var DlisEngine = initDlis(userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onEnd);
    let dataDir = "";
    if(userInfo.dataPath){
        dataDir = userInfo.dataPath + '/dlis_out/' + userInfo.username + '/' + Date.now() + '/';
    } else {
        dataDir = __dirname + '/dlis_out/' + userInfo.username + '/' + Date.now() + '/';
    }
    mkdirSyncRecursive(dataDir);
    const temp = DlisEngine.parser.parseFile(fileName, dataDir);
}
function eflr_data(dlisInstance, myObj) {
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
            //currentSet[objName] = myObj;
            if(setType == "ORIGIN"){
                myObj.userInfo = dlisInstance.userInfo;
                dlisInstance.onWellInfoCb(myObj);
            }
            else if(setType == "FRAME") {
                dlisInstance.onDatasetInfoCb(myObj);
            }
            else if(setType == "CHANNEL"){
                myObj.path = myObj.path.replace(dlisInstance.userInfo.dataPath + '/', '');
                dlisInstance.onCurveInfoCb(myObj);
            }
        }
        break;
    }
    return 23;
}
/*
function iflr_header(instance, myObj) {
    //console.log("==>frame: " + JSON.stringify(dlisFrames, null, 2));
    //console.log("==>channel: " + JSON.stringify(channels, null, 2));
    //console.log("==>origin: " + JSON.stringify(dlisOrigin, null, 2));
    //process.exit(1);

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
*/
//parse(process.argv[2]);
