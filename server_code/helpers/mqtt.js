var mqtt = require('mqtt');
var config = require('../config/config')
const { addData } = require("../controllers/controller");
var client = mqtt.connect('mqtt://' + config.mqtt_host + ":" + config.mqttPort, {
    clientId: 'Server_Client',
    username: config.user_name,
    password: config.password,
    reconnectPeriod: 1000,
    keepalive: 300,
    clean: false,
});
module.exports = function (io) {
    client.on('connect', function () {
        // console.log("Connect mqtt successful!")
        client.subscribe('status/environments', {qos: 1});
    });
    client.on('message', function (topic, message) {
        let date_ob = new Date();
        // current date
        // adjust 0 before single digit date
        let date = ("0" + date_ob.getDate()).slice(-2);
        // current month
        let month = ("0" + (date_ob.getMonth() + 1)).slice(-2);
        // current year
        let year = date_ob.getFullYear();
        // current hours
        let hours = date_ob.getHours();
        // current minutes
        let minutes = date_ob.getMinutes();
        // current seconds
        let seconds = date_ob.getSeconds();
        switch (topic) {
            case 'Status/Connected':
                console.log("Connected: " + message.toString());
                // io.sockets.emit('Esp-connect', message.toString());
                break;
                
            case 'Status/Disconnected':
                console.log("Disconnect: "+ message.toString() + " "+ year + "-" + month + "-" + date + " " + hours + ":" + minutes + ":" + seconds);
                // io.sockets.emit('Hardware-disconnect', message.toString());
                break;
            case 'status/environments':
                console.log("Environment: " + message.toString());
                if (message.toString().length > 10){
                    var jsondata = JSON.parse(message.toString());
                    jsondata.time = year + "-" + month + "-" + date + " " + hours + ":" + minutes + ":" + seconds;
                    addData(jsondata);
                }
                io.sockets.emit('Environment-update', message.toString());
                break;
        }
    });
    exports.sendFireSignal = function () {
        client.publish('controller/buzzer', "1", {qos: 1, retain: false});

    }
    exports.sendNormalSignal = function () {
        client.publish('controller/buzzer', "0", {qos: 1, retain: false});
    }
    return exports;
}
