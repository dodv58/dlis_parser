var binn = require("binn.js");
var zeromq = require('zeromq');
const fs = require('fs');

module.exports.parseFile = parseFile;
var socket = zeromq.socket("rep");
socket.bindSync('ipc:///tmp/dlis-socket-' + process.pid);

var dlis = require("./build/Release/dlis_parser");

var instance = {
    userInfo: null, 
    onEnd:null,
    wells: null,
    numberOfWell: 0,
    dataDir: ""
}

socket.on("message", function(buffer) {
    let retval = 1;
    socket.send(retval);
    let myObj = binn.decode(buffer);
    if(myObj.ended){
        // update dataset top/bottom
        for(const well of instance.wells){
            well.dataDir = instance.dataDir;
            for(const dataset of well.datasets){
                if(dataset.name == "EQUIPMENT" || dataset.name == "TOOL"){
                    dataset.curves.forEach(curve => {
                        curve.fs.end();
                        delete curve.fs;
                    })
                }
                for(frame of myObj.frames){
                    if(dataset._id == obname2Str(frame)){
                        if(dataset.direction == 'DECREASING'){
                            dataset.top = frame.index_max;
                            dataset.bottom = frame.index_min;
                        }else {
                            dataset.top = frame.index_min;
                            dataset.bottom = frame.index_max;
                        }
                        dataset.curves.forEach(curve => {
                            curve.startDepth = dataset.top;
                            curve.stopDepth = dataset.bottom;
                        })
                    }
                }
            }
        }
        instance.onEnd(instance.wells); 
    } else {
        eflr_data(myObj);
    }
});


const sendingDataType =  {
    _SET: 0,
    _OBJECT: 1,
    _OBJ_VALUE: 2,
}

let setType;
let channels = {};
let seq_num = 0;
function safeObname2Str(obj) {
    if (typeof obj === "object" && Number.isInteger(obj.origin) && Number.isInteger(obj.copy_number) && obj.name) {
        return obname2Str(obj);
    }
    return obj;
}
function obname2Str(obj) {
    if(!obj.seq_num){
        return seq_num + "-" + obj.origin + "-" + obj.copy_number + "-" + obj.name;
    }
    else return obj.seq_num + "-" + obj.origin + "-" + obj.copy_number + "-" + obj.name;
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
    //console.log(setType + " ===> " + JSON.stringify(myObj));
    switch(myObj.sending_data_type) {
    case sendingDataType._SET:
        setType = myObj.type;
        if(setType == "EQUIPMENT"){
            const dataset = {
                _id: "EQUIPMENT",
                name: "EQUIPMENT",
                top: 0,
                bottom: 0,
                step: 1,
                direction: "INCREASING",
                curves: [],
                params: []
            }
            const _curves = ['NAME', 'TRADEMARK-NAME', 'STATUS', 'TYPE', 'SERIAL-NUMBER', 'LOCATION', 'HEIGHT', 'LENGTH', 'MAXIMUM-DIAMETER', 'MINIMUM-DIAMETER', 'VOLUME', 'WEIGHT', 'HOLE-SIZE', 'PRESSURE', 'TEMPERATURE', 'VERTICAL-DEPTH', 'RADIAL-DRIFT', 'ANGULAR-DRIFT'];
            _curves.forEach((name) => {
                let _curve = {
                    name: name,
                    unit: "",
                    startdepth: 0,
                    stopdepth: 0,
                    step: 1,
                    path: instance.dataDir.replace(instance.userInfo.dataPath + '/', '') + "EQUIPMENT_" + name + ".txt",
                    dimension: 1,
                    description: "",
                    type: "TEXT",
                    fs: fs.createWriteStream(instance.dataDir + "EQUIPMENT_" + name + ".txt")
                }
                dataset.curves.push(_curve);
            })
            instance.wells[instance.numberOfWell - 1].datasets.push(dataset);
        }
        else if(setType == "TOOL"){
            const dataset = {
                _id: "TOOL",
                name: "TOOL",
                top: 0,
                bottom: 0,
                step: 1,
                direction: "INCREASING",
                curves: [],
                params: []
            }
            const _curves = ['DESCRIPTION', 'TRADEMARK-NAME', 'GENERIC-NAME', 'PARTS', 'STATUS', 'CHANNELS', 'PARAMETERS'];
            _curves.forEach((name) => {
                let _curve = {
                    name: name,
                    unit: "",
                    startdepth: 0,
                    stopdepth: 0,
                    step: 1,
                    path: instance.dataDir.replace(instance.userInfo.dataPath + '/', '') + "TOOL_" + name + ".txt",
                    dimension: 1,
                    description: "",
                    type: "TEXT",
                    fs: fs.createWriteStream(instance.dataDir + "TOOL_" + name + ".txt")
                }
                dataset.curves.push(_curve);
            })
            instance.wells[instance.numberOfWell - 1].datasets.push(dataset);

        }
        
        break;
    case sendingDataType._OBJECT:
        myObj.name = myObj.name.trim();
        //console.log(setType + " ===> " + JSON.stringify(myObj, null, 2));
        const objName = obname2Str(myObj);
        if(setType == "FILE-HEADER"){
            seq_num = parseInt(myObj["SEQUENCE-NUMBER"]);
            instance.numberOfWell += 1;
        } else if(setType == "ORIGIN"){
            if(instance.wells.length < instance.numberOfWell){
                instance.wells.push({
                    filename: myObj['FILE-SET-NAME'] ? myObj['FILE-SET-NAME'][0] : "",
                    name: myObj['WELL-NAME'] ? myObj['WELL-NAME'][0].trim() : myObj.name,
                    datasets: []
                })
            }
        }
        else if(setType == "FRAME") {
            let _direction = 'INCREASING';
            if(myObj['SPACING']){
                if(myObj['SPACING'][0] < 0) _direction = 'DECREASING';
            } else if(myObj['DIRECTION']){
                _direction = myObj['DIRECTION'][0];
            }
            let _step = myObj['SPACING'] ? myObj['SPACING'][0] : 0;
            let _top = myObj['INDEX-MIN'] ? myObj['INDEX-MIN'][0] : 0;
            let _bottom = myObj['INDEX-MAX'] ? myObj['INDEX-MAX'][0] : 0;
            if(_direction == 'DECREASING'){
                _step = -_step;
                const tmp = _top;
                _top = _bottom;
                _bottom = tmp;
            }

            const dataset = {
                _id: obname2Str(myObj),
                name: myObj.name,
                top: _top,
                bottom: _bottom,
                step: _step,
                direction: _direction,
                curves: [],
                params: [],
                unit: 'm'
            }

            //console.log("onDatasetInfo: " + JSON.stringify(dataset));

            if(Object.entries(channels).length != 0 || channels.constructor != Object){
                //import curve to db
                myObj['CHANNELS'].forEach(function(channelName, index){
                    const channel = channels[obname2Str(channelName)];
                    if(index == 0 && myObj['INDEX-TYPE']){
                        dataset.unit = channel['UNITS'] ? channel['UNITS'][0] : "";
                        return;
                    }
                    let _dimension = 1;
                    let _type = channel['REPRESENTATION-CODE'][0] < 19 ? 'NUMBER' : 'TEXT';
                    if(channel['DIMENSION']){
                        for(const x of channel['DIMENSION']){
                            _dimension *= x;
                        }
                    }
                    if(_dimension > 1){
                        _type = 'ARRAY';
                    }
                    let curve = {
                        _id : "",
                        name: channelName.name,
                        unit: channel['UNITS'] ? channel['UNITS'][0] : "",
                        startDepth: dataset.top,
                        stopDepth: dataset.bottom,
                        step: dataset.step,
                        path: channel.path,
                        dimension: _dimension,
                        description: channel['LONG-NAME'] ? channel['LONG-NAME'][0] : "",
                        type: _type
                    }
                    dataset.curves.push(curve);
                    //console.log("==> CCC " + JSON.stringify(channel, null, 2));
                })
            } else {
                //30/05/2019
                myObj['CHANNELS'].forEach(function(channelName, index){
                    const curve = {
                        _id: obname2Str(channelName), 
                        name: channelName.name,
                        unit: "",
                        startDepth: dataset.top,
                        stopDepth: dataset.bottom,
                        step: dataset.step,
                        path: "",
                        dimension: 1,
                        description: "",
                        type: "NUMBER"
                    }
                    dataset.curves.push(curve);
                })
                //30/05/2019
            }
            instance.wells[instance.numberOfWell - 1].datasets.push(dataset);
        }
        else if(setType == "CHANNEL"){
            //console.log("===> CCC " + JSON.stringify(myObj));
            myObj.path = myObj.path.replace(instance.userInfo.dataPath + '/', '');
            channels[obname2Str(myObj)] = myObj;
            //30/05/2019
            const _datasets = instance.wells[instance.numberOfWell - 1].datasets;
            if(_datasets.length > 0){
               _datasets.forEach(function(_dataset){
                   _dataset.curves.forEach(function(_curve, index){
                       if(_curve._id == obname2Str(myObj)){
                            if(index == 0 && myObj['INDEX-TYPE']){
                                _dataset.unit = myObj['UNITS'] ? myObj['UNITS'][0] : "";
                                return;
                            }
                            let _dimension = 1;
                            let _type = myObj['REPRESENTATION-CODE'][0] < 19 ? 'NUMBER' : 'TEXT';
                            if(myObj['DIMENSION']){
                                for(const x of myObj['DIMENSION']){
                                    _dimension *= x;
                                }
                            }
                            if(_dimension > 1){
                                _type = 'ARRAY';
                            }
                            _curve.dimension = _dimension;
                            _curve.type = _type;
                            _curve.unit= myObj['UNITS'] ? myObj['UNITS'][0] : "";
                            _curve.path = myObj.path;
                            _curve.description= myObj['LONG-NAME'] ? myObj['LONG-NAME'][0] : "";
                       }
                   })
               })
            }
            //30/05/2019
        }
        else if(setType == "EQUIPMENT" || setType == "TOOL"){
            const lastDatasetIndex = instance.wells[instance.numberOfWell - 1].datasets.length - 1;
            const dataset = instance.wells[instance.numberOfWell - 1].datasets[lastDatasetIndex];
            dataset.bottom += 1;
            let line = "";
            for(const property in myObj){
                for(const curve of dataset.curves){
                    if(property.toUpperCase() == curve.name){
                        if(property.toUpperCase() == "NAME"){
                            curve.fs.write(dataset.bottom + " " + myObj[property] + "\n");
                        }
                        else {
                            line = '';
                            for(value of myObj[property]){
                                if(typeof value == "object"){
                                    for(p in value){
                                        line += value[p] + '_';
                                    }
                                    line = line.slice(0, -1);
                                }
                                else {
                                    line += value;
                                }
                                line += ';';
                            }
                            line = line.slice(0, -1);
                            curve.fs.write(dataset.bottom + " " + line + "\n");
                        }
                    }
                }
            }
        }
        break;
    }
    return 23;
}

function parseFile(fileName, userInfo, onEnd) {
    instance.userInfo = userInfo;
    instance.onEnd = onEnd;
    instance.wells = [];
    instance.numberOfWell = 0;
    channels = {};

    let dataDir = "";
    if(userInfo.dataPath){
        dataDir = userInfo.dataPath + '/dlis_out/' + userInfo.username + '/' + Date.now() + '/';
    } else {
        dataDir = __dirname + '/dlis_out/' + userInfo.username + '/' + Date.now() + '/';
    }
    mkdirSyncRecursive(dataDir);
    instance.dataDir = dataDir;
    const temp = dlis.parseFile(fileName, dataDir);
}

