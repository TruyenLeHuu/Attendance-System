const environment = require('../model/environment');
const rfid = require('../model/rfid');
var that = (module.exports = {
  mainPage: async (req, res, next) => {
    res.render(__dirname + "/../views/main.ejs");
  },
  addData: async(data) => {
    // console.log(data)
    try
    { 
      const newEnv = new environment({
          temp : data.temp,
          hum: data.hum,
          smoke: data.smoke,
          time: data.time,
          status: data.status
      })
      await newEnv.save()
      console.log("Add data environment successfully!")
    }
    catch (err) {
        console.log("Create error :" + {message:err})
    }
  },
  addEnv: async(req,res,next)=>{
          try
          {   
              var data = JSON.parse(req.body.json)
              // await io.emit('changeTemHum', newSensor);
              const newEnv = new environment({
                  temp : data.temp,
                  hum: data.hum,
              })
              await newEnv.save()
              console.log(newEnv)
              res.json({message:"Sent sensor data successfully"})
          }
          catch (err) {
              res.json({message:"Error"})
          }
      },
  addID:  async(req,res,next)=>{
        try
        {   
            const newID = new rfid({
              ID : req.body.id,
            })
            await newID.save()
            console.log(req.body.id)
            res.json({message:"Sent id data successfully"})
        }
        catch (err) {
            res.json({message:"Error"})
        }
      }
});
