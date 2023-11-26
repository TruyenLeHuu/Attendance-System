// Import express framework for web 
const express = require('express');
const appExpress = express();
// Import body-parser middleware using to extract body from HTTP request
const bodyParser = require('body-parser');
// Import mongoDB
const mongoose = require('mongoose');
// Import evironment variables
const config = require('./config/config');
// URI of mongo DB
const URI = 'mongodb+srv://1111:1234@cluster0.lxqs9wg.mongodb.net/IOT_PROJECT?retryWrites=true&w=majority'
// Using ejs as a tool simulate HTML
appExpress.set("view engine", "ejs");
// Add router for ejs
appExpress.set("views", "./views");
// Add router resource for client
appExpress.use(express.static("./public"));
// Config body parser
appExpress.use(bodyParser.urlencoded({extended:true}));
appExpress.use(bodyParser.json());
// Create server
var server = require("http").Server(appExpress);

//Import socket io
var io = require("socket.io")(server);

//Import routes
const route = require('./routes/route')(io);

//Route middleware
appExpress.use('/', route);
appExpress.get('/', (req, res) => {
    res.render(__dirname + "./views/main.ejs");
  });
// Import mqtt for server
const mqtt = require('./helpers/mqtt')(io);
// Import socket io for server
require('./helpers/socket-io')(io, mqtt);
// Start and connect mongoDB and server
mongoose
.connect(URI, {useNewUrlParser:true, useUnifiedTopology:true})
.then(()=>{
  console.log("Connected to db")
  //InitRole()
  appPort = config.port;
	appHost = config.host;
	server.listen(appPort, () => {
		console.log(`Server listening at host ${appHost} port ${appPort}`);
    });
}).catch((err) => {
    console.log(err)
})

