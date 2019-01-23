var binn = require("binn.js");
var zeromq = require('zeromq');
const fs = require('fs');

module.exports.parseFile = parseFile;

var socket = zeromq.socket("rep");
socket.bindSync('ipc:///tmp/dlis-socket-' + process.pid);
console.log("bind done " + process.pid);

var dlis = require("./build/Release/dlis_parser");

var instance = {
    userInfo: null, 
    onWellInfoCb: null,
    onDatasetInfoCb: null, 
    onCurveInfoCb: null,
    onEnd:null
}

socket.on("message", function(buffer) {
    let myObj = binn.decode(buffer);
    let retval = 1;
    if(myObj.ended){
        instance.onEnd(); 
    } else {
        switch(myObj.functionIdx){
            case functionIdx.EFLR_DATA:
                retval = eflr_data(myObj);
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

function eflr_data( myObj) {
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
                myObj.userInfo = instance.userInfo;
                instance.onWellInfoCb(myObj);
            }
            else if(setType == "FRAME") {
                instance.onDatasetInfoCb(myObj);
            }
            else if(setType == "CHANNEL"){
                myObj.path = myObj.path.replace(instance.userInfo.dataPath + '/', '');
                instance.onCurveInfoCb(myObj);
            }
        }
        break;
    }
    return 23;
}

function parseFile(fileName, userInfo, onWellInfoCb, onDatasetInfoCb, onCurveInfoCb, onEnd) {
    instance.userInfo = userInfo;
    instance.onWellInfoCb = onWellInfoCb;
    instance.onDatasetInfoCb = onDatasetInfoCb;
    instance.onCurveInfoCb = onCurveInfoCb;
    instance.onEnd = onEnd;

    let dataDir = "";
    if(userInfo.dataPath){
        dataDir = userInfo.dataPath + '/dlis_out/' + userInfo.username + '/' + Date.now() + '/';
    } else {
        dataDir = __dirname + '/dlis_out/' + userInfo.username + '/' + Date.now() + '/';
    }
    mkdirSyncRecursive(dataDir);
    const temp = dlis.parseFile(fileName, dataDir);
}
