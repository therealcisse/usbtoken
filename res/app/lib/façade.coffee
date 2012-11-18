Spine = require('spine')

Token = require('models/token')
Key   = require('models/key')

class Façade extends Spine.Module
  @extend Spine.Log

  @logPrefix: '(Façade)'

  @trace: true

  @cnt: 0

  @SetWindowText: (text) ->
    if text then document.title = "#{text} | Epsilon Token Manager" else document.title = "Epsilon Token Manager"

  @GetPINOpts: (fn) ->
    fn(Façade._pinopts)

  @Load: ->
    app._fns = []

    GetFn = (key) ->
      for fn, i in app._fns when fn.MsgKey is key
        return {i, fnObj: fn}

    app.addFn = (MsgKey, fn) ->
      app._fns.push {MsgKey, fn}

    app.delFn = (key, fn) ->
      f = GetFn(key)
      app._fns.splice(f.i, 1) if f      

    app.setMessageCallback 'kMsg', ([response]) ->
      ret = GetFn(response.MsgKey)
      if ret and ret.fnObj.MsgKey is response.MsgKey and ret.fnObj.fn.MsgId is response.MsgId
        ret.fnObj.fn(response) 
        app.delFn(response.MsgKey)    

  @GetStatus: (fn) -> 
    callback = (response) =>
      console.log "@GetStatus: Reply: #{response.status}"

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

      Façade.reader = response.reader if response.reader

      callback.fn(response.status)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = MsgId: callback.MsgId
    req.authData = Façade.authData if Façade.authData
    
    ### TODO(When to include reader option) ###
    req.reader = Façade.reader if Façade.reader

    app.addFn "getstatus", callback
    app.sendMessage "getstatus", [req]

  # ops

  @GetKey: (id, kMessageName, callback) ->
    app.addFn kMessageName, callback
    app.sendMessage kMessageName, [{MsgId: callback.MsgId, id: id, authData: Façade.authData}]

  @GetPrKey: (id, fn) ->
    @log "GetPrKey:#{id}"

    callback = (response) =>
      console.log "@GetPrKey: Reply: #{response.ok}"
      callback.fn(response.key)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"
    
    Façade.GetKey(id, 'get.prkey', callback)

  @GetPubKey: (id, fn) ->
    @log "GetPubKey:#{id}"

    callback = (response) =>
      console.log "@GetPubKey: Reply: #{response.ok}"
      callback.fn(response.key)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"
    
    Façade.GetKey(id, 'get.pubkey', callback)

  @GetX509Certificate: (id, fn) ->
    @log "GetX509Certificate:#{id}"

    callback = (response) =>
      console.log "@GetX509Certificate: Reply: #{response.ok}"
      callback.fn(response.key)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"
    
    Façade.GetKey(id, 'get.x509-certificate', callback)        

  @GetPubKeys: (fn) ->
    @log "GetPubKeys"
    
    callback = (response) =>
      console.log "@GetPubKeys: Reply: #{response.ok}"
      callback.fn(response.keys)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "get.pubkeys", callback
    app.sendMessage "get.pubkeys", [{MsgId: callback.MsgId, authData: Façade.authData}]

  @GetPrKeys: (fn) ->
    @log "GetPrKeys"
    callback = (response) =>
      console.log "@GetPrKeys: Reply: #{response.ok}"
      callback.fn(response.keys)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "get.prkeys", callback
    app.sendMessage "get.prkeys", [{MsgId: callback.MsgId, authData: Façade.authData}]
  
  @GetCerts: (fn) ->
    @log "GetCerts"

    callback = (response) =>
      console.log "@GetCerts: Reply: #{response.ok}"
      callback.fn(response.keys)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "get.x509-certificates", callback
    app.sendMessage "get.x509-certificates", [{MsgId: callback.MsgId, authData: Façade.authData}]

  @GenKey: (label, fn) ->
    @log "GenKey:#{label}"

    callback = (response) =>
      console.log "@GenKey: Reply: #{response.ok}"
      callback.fn(response.ok, response.keyid)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      label: label
      authData: Façade.authData

    app.addFn "keygen", callback
    app.sendMessage "keygen", [req]    

  @GenX509Req: (id, cn, o, ou, city, region, country, emailAddress, fn) ->
    @log "GenX509Req"

    callback = (response) =>
      console.log "@GenX509Req: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      id: id
      cn: cn
      o: o
      ou: ou
      city: city
      region: region
      country: country
      emailAddress: emailAddress
      authData: Façade.authData

    app.addFn "csr.gen", callback
    app.sendMessage "csr.gen", [req]

  @ImportPrKey: (data, data_len, label, format, passphrase, fn) ->
    @log "ImportPrKey(#{data_len}, #{label}, #{format})"

    callback = (response) =>
      console.log "@GenKey: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"    

    req = 
      MsgId: callback.MsgId 
      data: data
      data_len: data_len
      label: label
      format: format 
      passphrase: passphrase 
      authData: Façade.authData    

    app.addFn "import.prkey", callback
    app.sendMessage "import.prkey", [req]    

  @ImportPubKey: (data, data_len, label, format, fn) ->
    @log "ImportPubKey(#{data_len}, #{label}, #{format})"

    callback = (response) =>
      console.log "@ImportPubKey: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      data: data
      data_len: data_len
      label: label
      format: format 
      authData: Façade.authData

    app.addFn "import.pubkey", callback
    app.sendMessage "import.pubkey", [req]        
  
  @ImportX509Certificate: (data, data_len, label, format, fn) ->
    @log "ImportX509Certificate(#{data_len}, #{label}, #{format})"

    callback = (response) =>
      console.log "@ImportX509Certificate: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      data: data
      data_len: data_len
      label: label
      format: format 
      authData: Façade.authData    

    app.addFn "import.x509", callback
    app.sendMessage "import.x509", [req]

  @ExportX509: (id, fileName, format, fn) ->
    @log "ExportX509(#{id}, #{format})"

    callback = (response) =>
      console.log "@ExportX509: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      id: id
      format: format 
      fileName: fileName
      authData: Façade.authData    

    app.addFn "export.x509", callback
    app.sendMessage "export.x509", [req]

  @DelX509: (id, fn) ->
    @log "DelX509(#{id})"

    callback = (response) =>
      console.log "@DelX509: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      id: id
      authData: Façade.authData    

    app.addFn "del.x509", callback
    app.sendMessage "del.x509", [req]

  @DelPrKey: (id, fn) ->
    @log "DelPrKey(#{id})"

    callback = (response) =>
      console.log "@DelPrKey: Reply: #{response.ok}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    req = 
      MsgId: callback.MsgId
      id: id
      authData: Façade.authData    

    app.addFn "del.prkey", callback
    app.sendMessage "del.prkey", [req]    

  @SetPIN: (pin, fn) ->
    @log "@SetPIN"

    callback = (response) =>
      console.log "@SetPIN: Reply: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "setpin", callback
    app.sendMessage "setpin", [{MsgId: callback.MsgId, pin: pin}]  

  @ChangePIN: (pincode, pin, fn) ->
    @log "@ChangePIN"

    callback = (response) =>
      console.log "@ChangePIN: Reply: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "changepin", callback
    app.sendMessage "changepin", [{MsgId: callback.MsgId, pincode: pincode, pin: pin}]

  @Unblock: (puk, pin, fn) ->
    @log "@UnblockPIN"

    callback = (response) =>
      console.log "@Unblock: Reply: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.setMessageCallback "unblock", callback
    app.sendMessage "unblock", [{MsgId: callback.MsgId, puk: puk, pin: pin}]

  @EraseToken: (fn) ->
    @log "@EraseToken"

    callback = (response) =>
      console.log "@EraseToken: Reply: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "erase", callback
    app.sendMessage "erase", [{MsgId: callback.MsgId}]  

  @InitToken: (pin, puk, label, fn) ->
    @log "@InitToken"

    callback = (response) =>
      console.log "@InitToken: Reply: #{response.status}"
      callback.fn(response.ok)

    callback.fn = fn
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "init", callback
    app.sendMessage "init", [{MsgId: callback.MsgId, pin: pin, puk: puk, label: label}]

  @Logout: (fn) ->
    delete Façade.authData
    fn(true)

  @Login: (pin, fn) ->
    @log "@Login"

    callback = (response) =>
      console.log "@Login: Reply: #{response.status}"

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
    callback.MsgId = "#{++Façade.cnt}"

    app.addFn "verifypin", callback
    app.sendMessage "verifypin", [{MsgId: callback.MsgId, pin: pin}]

module.exports = Façade