Token = require('models/token')
Key = require('models/key')
Spine = require('spine')

# All the methods are asynchronous

class Façade extends Spine.Module
  @extend Spine.Log

  @logPrefix: '(Façade)'

  @trace: true

  @cnt: 0

  @SetWindowText: (text) ->
    document.title = "#{text} | Epsilon Token Manager"
    #app.sendMessage 'titleset', [text]

  @GetPINOpts: (fn) ->
    fn(Façade._pinopts)

  @data:
    status: Token.Absent
    pin:  '1234' 
    puk:  '1111'
    sopin:  '5678' 
    sopuk:  '2222'
    keys: [
      Key.create(friendlyName: 'Key 1')
      Key.create(friendlyName: 'Key 2')
      Key.create(friendlyName: 'Key 3')
      Key.create(friendlyName: 'Key 4')
      Key.create(friendlyName: 'Key 5')
      Key.create(friendlyName: 'Key 6')
      Key.create(friendlyName: 'Key 7')
      Key.create(friendlyName: 'Key 8')#
      Key.create(friendlyName: 'Key 9')
      Key.create(friendlyName: 'Key 10')
      Key.create(friendlyName: 'Key 11')
      Key.create(friendlyName: 'Key 12')
      Key.create(friendlyName: 'Key 13')
      Key.create(friendlyName: 'Key 14')
      Key.create(friendlyName: 'Key 15')
      Key.create(friendlyName: 'Key 16')#
      Key.create(friendlyName: 'Key 17')
      Key.create(friendlyName: 'Key 18')
      Key.create(friendlyName: 'Key 19')
      Key.create(friendlyName: 'Key 20')
      Key.create(friendlyName: 'Key 21')
      Key.create(friendlyName: 'Key 22')
      Key.create(friendlyName: 'Key 23')
      Key.create(friendlyName: 'Key 24')#
      Key.create(friendlyName: 'Key 25')
      Key.create(friendlyName: 'Key 26')
      Key.create(friendlyName: 'Key 27')
      Key.create(friendlyName: 'Key 28')
      Key.create(friendlyName: 'Key 29')
      Key.create(friendlyName: 'Key 30')
      Key.create(friendlyName: 'Key 31')
      Key.create(friendlyName: 'Key 32')                  
    ] 
    options:

      # User PIN

      'min-pin-length' : 3
      'max-pin-length' : 8

      'min-puk-length' : 3
      'max-puk-length' : 8      

      'pin-attempts' : 3
      'puk-attempts' : 3

      'remaining-pin-attempts' : 3
      'remaining-puk-attempts' : 3

      # SO PIN

      'min-so-pin-length' : 3
      'max-so-pin-length' : 8

      'min-so-puk-length' : 3
      'max-so-puk-length' : 8     

      'so-pin-attempts' : 3
      'so-puk-attempts' : 3

      'remaining-so-pin-attempts' : 3
      'remaining-so-puk-attempts' : 3

      # other options

      'onepin' : false

  # getters

  @getKeys: (fn) ->
    fn @data['keys']

  @getKey: (id, fn) ->
    try 
      fn Key.find(id)
    catch e
      fn undefined, true

  @getStatus: (fn) -> 

    callback = (kMessageName, [response]) =>
      console.log "@getStatus: Reply from #{kMessageName}: #{response.status}"

      if Façade.authData and response.status isnt Token.LoggedIn       
        delete Façade.authData

      if response.status in [ Token.Blank, Token.Blocked ]
        Façade._pinopts =
          minlen : response.minlen,
          maxlen : response.maxlen

      if response.status is Token.AuthRequired
        Façade._pinopts =
          minlen : response.minlen,
          maxlen : response.maxlen,
          max_tries : response.max_tries,
          tries_left : response.tries_left

      callback.fn(response.status)

    callback.fn = fn

    replyto = "getstatus.#{++Façade.cnt}"

    req = replyto: replyto
    req.authData = Façade.authData if Façade.authData

    app.setMessageCallback replyto, callback
    app.sendMessage "getstatus", [req]

  @getOptions: (fn) ->  
    fn Façade['data']['options']

  # ops

  @SetPIN: (pin, fn) ->
    @log "@SetPIN"

    callback = (kMessageName, [response]) =>
      console.log "@SetPIN: Reply from #{kMessageName}: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn

    replyto = "setpin.#{++Façade.cnt}"
    app.setMessageCallback replyto, callback
    app.sendMessage "setpin", [{replyto: replyto, pin: pin}]  

  @ChangePIN: (pincode, pin, fn) ->
    @log "@ChangePIN"

    callback = (kMessageName, [response]) =>
      console.log "@ChangePIN: Reply from #{kMessageName}: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn

    replyto = "changepin.#{++Façade.cnt}"
    app.setMessageCallback replyto, callback
    app.sendMessage "changepin", [{replyto: replyto, pincode: pincode, pin: pin}]

  @Unblock: (puk, pin, fn) ->
    @log "@UnblockPIN"

    callback = (kMessageName, [response]) =>
      console.log "@Unblock: Reply from #{kMessageName}: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn

    replyto = "unblock.#{++Façade.cnt}"
    app.setMessageCallback replyto, callback
    app.sendMessage "unblock", [{replyto: replyto, puk: puk, pin: pin}]

  @EraseToken: (fn) ->
    @log "@EraseToken"

    callback = (kMessageName, [response]) =>
      console.log "@EraseToken: Reply from #{kMessageName}: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn

    replyto = "erase.#{++Façade.cnt}"
    app.setMessageCallback replyto, callback
    app.sendMessage "erase", [{replyto: replyto}]  

  @InitToken: (pin, puk, label, fn) ->
    @log "@InitToken"

    callback = (kMessageName, [response]) =>
      console.log "@InitToken: Reply from #{kMessageName}: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn

    replyto = "init.#{++Façade.cnt}"
    app.setMessageCallback replyto, callback
    app.sendMessage "init", [{replyto: replyto, pin: pin, puk: puk, label: label}]

  @Logout: (fn) ->
    delete Façade.authData
    fn(true)

  @Login: (pin, fn) ->
    @log "@Login"

    callback = (kMessageName, [response]) =>
      console.log "@Login: Reply from #{kMessageName}: #{response.status}"

      if response.status is Token.LoggedIn
        Façade._pinopts =
          minlen : response.minlen,
          maxlen : response.maxlen,
          max_tries : response.max_tries,
          tries_left : response.tries_left

        ### very crucial ###
        Façade.authData = pin

        return callback.fn(null)

      callback.fn(response.tries_left)

    callback.fn = fn

    replyto = "verifypin.#{++Façade.cnt}"
    app.setMessageCallback replyto, callback
    app.sendMessage "verifypin", [{replyto: replyto, pin: pin}]

  @soLogin: (sopin, fn) ->

  @unblock: (puk, fn) =>
    @log "@unblock:puk-attempts=#{Façade['data']['options']['puk-attempts']}"
    @log "@unblock:remaining-puk-attempts=#{Façade['data']['options']['remaining-puk-attempts']}"

    if Façade['data']['options']['remaining-puk-attempts'] is 0
      Façade['data']['status'] = Token.Locked
      return fn(0)

    Façade['data']['options']['remaining-puk-attempts'] -= 1

    if puk is Façade['data']['puk']
      Façade['data']['options']['remaining-pin-attempts'] = Façade['data']['options']['pin-attempts']
      Façade['data']['options']['remaining-puk-attempts'] = Façade['data']['options']['puk-attempts']
      Façade['data']['status'] = Token.ChangePIN
      fn()
    else      
      fn(Façade['data']['options']['remaining-puk-attempts']) # error

  @soUnblock: (sopuk, fn) ->

  @logout: (fn) -> 
    Façade['data']['status'] = Token.AuthRequired
    fn()

  @resetPIN: (puk, newpin, fn) =>

    if Façade['data']['puk'] is puk 
    
      Façade['data']['pin'] = newpin
      Façade['data']['status'] = Token.LoggedIn
      fn()

    else

      fn(true)

  @soResetPIN: (newsopin, fn) -> 
    Façade['data']['sopin'] = newsopin
    Façade['data']['status'] = Token.LoggedIn
    fn()

  @changePIN: (oldpin, pin, fn) => 
  
    if oldpin is Façade['data']['pin']

      Façade['data']['pin'] = pin
      Façade['data']['status'] = Token.LoggedIn
      fn()

    else

      fn(true)


  @soChangePIN: (sopin, fn) -> 
    @soSetPIN(sopin, fn)    

module.exports = Façade