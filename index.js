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
            for(const dataset of well.datasets){
                if(dataset.name == "EQUIPMENT"){
                    dataset.curves.forEach(curve => {
                        curve.fs.end();
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
            const _curves = ['NAME', 'SERIAL-NUMBER', 'MAXIMUM-DIAMETER', 'HOLE-SIZE'];
            _curves.forEach((name) => {
                let _curve = {
                    name: "EQUIPMENT_" + name,
                    unit: "",
                    startdepth: 0,
                    stopdepth: 0,
                    step: 1,
                    path: instance.dataDir + "EQUIPMENT_" + name + ".txt",
                    dimension: 1,
                    description: "",
                    type: "TEXT",
                    fs: fs.createWriteStream(instance.dataDir + "EQUIPMENT_" + name + ".txt")
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

            if(channels){
                //import curve to db
                myObj['CHANNELS'].forEach(function(channelName, index){
                    const channel = channels[obname2Str(channelName)];
                    if(index == 0 && myObj['INDEX-TYPE']){
                        dataset.unit = channel['UNITS'] ? channel['UNITS'][0] : "";
                    }
                    let _dimension = 1;
                    let _type = 'NUMBER';
                    if(channel['DIMENSION']){
                        for(const x of channel['DIMENSION']){
                            _dimension *= x;
                        }
                    }
                    if(_dimension > 1){
                        _type = 'ARRAY';
                    }
                    let curve = {
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
                datasets.push(dataset);
            }
            instance.wells[instance.numberOfWell - 1].datasets.push(dataset);
        }
        else if(setType == "CHANNEL"){
            //console.log("===> CCC " + JSON.stringify(myObj));
            myObj.path = myObj.path.replace(instance.userInfo.dataPath + '/', '');
            channels[obname2Str(myObj)] = myObj;
        }
        else if(setType == "EQUIPMENT"){
            console.log(JSON.stringify(myObj))
            const lastDatasetIndex = instance.wells[instance.numberOfWell - 1].datasets.length - 1;
            const dataset = instance.wells[instance.numberOfWell - 1].datasets[lastDatasetIndex];
            dataset.bottom += 1;
            for(const property in myObj){
                for(const curve of dataset.curves){
                    if("EQUIPMENT_" + property.toUpperCase() == curve.name){
                        curve.fs.write(dataset.bottom + " " + myObj[property] + "\n");
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

