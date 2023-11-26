const router = require("express").Router();
const {
  mainPage, addID, addEnv,
} = require("../controllers/controller");

module.exports = function (io) {
  router.get("/", mainPage);

  router.post('/temphum', addEnv);
  router.post('/id', addID);
return router;
};
