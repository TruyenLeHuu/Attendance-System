const mongoose = require('mongoose')
const Schema  = mongoose.Schema
const rfid = new Schema ({
    ID:{
        type:String
    },
    time:{ type: Date, default: Date.now }
})
const rfidModel = mongoose.model('Rfid', rfid)
module.exports= rfidModel