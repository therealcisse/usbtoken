
var app;
if (!app)
  app = {};

(function() {
 
  native function SendMessage();
  app.sendMessage = function(name, arguments) {
    return SendMessage(name, arguments);
  };


  native function SetMessageCallback();
  app.setMessageCallback = function(name, callback) {
    return SetMessageCallback(name, callback);
  };
  

  native function RemoveMessageCallback();
  app.removeMessageCallback = function(name) {
    return RemoveMessageCallback(name);
  };

  native function GetCurrentLanguage();
  app.GetCurrentLanguage = function() {
    return GetCurrentLanguage();
  };

})();;
