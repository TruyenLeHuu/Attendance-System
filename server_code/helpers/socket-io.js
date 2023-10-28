const environment = require('../model/environment');
module.exports = function (io, mqtt) {
    io.on("connection", function (socket) {
        console.log("Socket connected")
        socket.on("disconnect",()=>{
            console.log("Socket disconnected")
        }),
        socket.on("Get-data", async ()=>{
            try
            { 
                environment.find().sort({ timestamp: -1 })
                    .limit(30)
                    .exec(function(err, documents) 
                    {
                        // console.info(documents);
                        io.sockets.emit('Send-data',documents)
                        if (err) {
                            console.error('Get data error :', err);
                        }
                    })
                console.log("Get data environment successfully!")
            }
            catch (err) {
                console.log("Get data error :" + {err})
            }
        }),
        socket.on("Signal-fire",()=>{
            mqtt.sendFireSignal();
        }),
        socket.on("Signal-normal",()=>{
            mqtt.sendNormalSignal();
        })
    })
}