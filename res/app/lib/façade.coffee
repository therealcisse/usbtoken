Spine = require('spine')

Token = require('models/token')

class Façade extends Spine.Module
  @extend Spine.Log

  @logPrefix: '(Façade)'

  @trace: true

  @cnt: 0

  @SetWindowText: (text) ->
    if text then document.title = "#{text} | Epsilon Token Manager" else document.title = "Epsilon Token Manager"

  @GetPINOpts: (fn) ->
    fn(Façade._pinopts)

  @ShowIfHidden: ->
    app.sendMessage 'show'

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
    jQuery.Deferred(->
      
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

        app.view.delay Façade.ShowIfHidden unless response.status is Token.Absent

        @resolve(response.status)

      callback.MsgId = "#{++Façade.cnt}"

      req = MsgId: callback.MsgId
      req.authData = Façade.authData if Façade.authData
      
      # TODO(When to include reader option)
      req.reader = Façade.reader if Façade.reader

      app.addFn "getstatus", callback
      app.sendMessage "getstatus", [req]

      @always fn

    ).promise()

  # ops

  @GetKey: (id, kMessageName, callback) ->
    app.addFn kMessageName, callback
    app.sendMessage kMessageName, [{MsgId: callback.MsgId, id: id, authData: Façade.authData}]

  @GetPrKey: (id, fn) ->
    @log "GetPrKey:#{id}"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GetPrKey: Reply: #{response.ok}"
        @resolve(response.key, not response.ok)

      callback.MsgId = "#{++Façade.cnt}"      
      Façade.GetKey(id, 'get.prkey', callback)

      @always fn

    ).promise()


  # @GetPubKey: (id, fn) ->
  #   @log "GetPubKey:#{id}"
  #   jQuery.Deferred(->

  #     callback = (response) =>
  #       console.log "@GetPubKey: Reply: #{response.ok}"
  #       @resolve(response.key)

  #     callback.MsgId = "#{++Façade.cnt}"      
  #     Façade.GetKey(id, 'get.pubkey', callback)

  #     @always fn

  #   ).promise()


  @GetX509Certificate: (id, fn) ->
    @log "GetX509Certificate:#{id}"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GetX509Certificate: Reply: #{response.ok}"
        @resolve(response.key, not response.ok)

      callback.MsgId = "#{++Façade.cnt}"      
      Façade.GetKey(id, 'get.x509-certificate', callback)  

      @always fn      

    ).promise()


  # @GetPubKeys: (fn) ->
  #   @log "GetPubKeys"
  #   jQuery.Deferred(->

  #     callback = (response) =>
  #       console.log "@GetPubKeys: Reply: #{response.ok}"
  #       @resolve(response.keys)

  #     callback.MsgId = "#{++Façade.cnt}"

  #     app.addFn "get.pubkeys", callback
  #     app.sendMessage "get.pubkeys", [{MsgId: callback.MsgId, authData: Façade.authData}]

  #     @always fn
    
  #   ).promise()
    

  @GetPrKeys: (fn) ->
    @log "GetPrKeys"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GetPrKeys: Reply: #{response.ok}"
        @resolve(response.keys)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "get.prkeys", callback
      app.sendMessage "get.prkeys", [{MsgId: callback.MsgId, authData: Façade.authData}]

      @always fn

    ).promise()

  @SetLang: (langId, fn) ->
    @log "SetLang"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@SetLang: Reply: #{response.ok}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "set.lang", callback
      app.sendMessage "set.lang", [{MsgId: callback.MsgId, LangID: parseInt(langId)}]

      @always fn

    ).promise()
  
  @GetCerts: (fn) ->
    @log "GetCerts"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GetCerts: Reply: #{response.ok}"
        @resolve(response.keys)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "get.x509-certificates", callback
      app.sendMessage "get.x509-certificates", [{MsgId: callback.MsgId, authData: Façade.authData}]

      @always fn
    
    ).promise()


  @GenKey: (label, fn) ->
    @log "GenKey:#{label}"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GenKey: Reply: #{response.ok}"
        @resolve(response.ok, response.keyid)

      callback.MsgId = "#{++Façade.cnt}"

      req = 
        MsgId: callback.MsgId
        label: label
        authData: Façade.authData

      app.addFn "keygen", callback
      app.sendMessage "keygen", [req]  

      @always fn  

    ).promise()


  @GenX509Req: (id, cn, o, ou, city, region, country, emailAddress, fn) ->
    @log "GenX509Req"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GenX509Req: Reply: #{response.ok}"
        @resolve(response.ok)

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

      @always fn
    
    ).promise()


  @ImportPrKey: (data, data_len, label, format, passphrase, fn) ->
    @log "ImportPrKey(#{data_len}, #{label}, #{format})"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@GenKey: Reply: #{response.ok}"
        @resolve(response.ok)

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

      @always fn   
    
    ).promise()


  # @ImportPubKey: (data, data_len, label, format, fn) ->
  #   @log "ImportPubKey(#{data_len}, #{label}, #{format})"
  #   jQuery.Deferred(->

  #     callback = (response) =>
  #       console.log "@ImportPubKey: Reply: #{response.ok}"
  #       @resolve(response.ok)

  #     callback.MsgId = "#{++Façade.cnt}"

  #     req = 
  #       MsgId: callback.MsgId
  #       data: data
  #       data_len: data_len
  #       label: label
  #       format: format 
  #       authData: Façade.authData

  #     app.addFn "import.pubkey", callback
  #     app.sendMessage "import.pubkey", [req]  

  #     @always fn      

  #   ).promise()

  
  @ImportX509: (data, data_len, label, format, fn) ->
    @log "ImportX509(#{data_len}, #{label}, #{format})"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@ImportX509: Reply: #{response.ok}"
        @resolve(response.ok)

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

      @always fn

    ).promise()


  @ExportX509: (id, fileName, format, fn) ->
    @log "ExportX509(#{id}, #{format})"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@ExportX509: Reply: #{response.ok}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      req = 
        MsgId: callback.MsgId
        id: id
        format: format 
        fileName: fileName
        authData: Façade.authData    

      app.addFn "export.x509", callback
      app.sendMessage "export.x509", [req]

      @always fn
    
    ).promise()


  @DelX509: (id, fn) ->
    @log "DelX509(#{id})"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@DelX509: Reply: #{response.ok}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      req = 
        MsgId: callback.MsgId
        id: id
        authData: Façade.authData    

      app.addFn "del.x509", callback
      app.sendMessage "del.x509", [req]

      @always fn

    ).promise()


  @DelPrKey: (id, fn) ->
    @log "DelPrKey(#{id})"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@DelPrKey: Reply: #{response.ok}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      req = 
        MsgId: callback.MsgId
        id: id
        authData: Façade.authData    

      app.addFn "del.prkey", callback
      app.sendMessage "del.prkey", [req] 

      @always fn   
    
    ).promise()


  # @SetPIN: (pin, fn) ->
  #   @log "@SetPIN"
  #   jQuery.Deferred(->

  #     callback = (response) =>
  #       console.log "@SetPIN: Reply: #{response.status}"
  #       @resolve(response.ok)

  #     callback.MsgId = "#{++Façade.cnt}"

  #     app.addFn "setpin", callback
  #     app.sendMessage "setpin", [{MsgId: callback.MsgId, pin: pin}] 

  #     @always fn 

  #   ).promise()


  @ChangePIN: (pincode, pin, fn) ->
    @log "@ChangePIN"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@ChangePIN: Reply: #{response.status}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "changepin", callback
      app.sendMessage "changepin", [{MsgId: callback.MsgId, pincode: pincode, pin: pin}]
      @always fn
    
    ).promise()


  @Unblock: (puk, pin, fn) ->
    @log "@UnblockPIN"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@Unblock: Reply: #{response.status}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "unblock", callback
      app.sendMessage "unblock", [{MsgId: callback.MsgId, puk: puk, pin: pin}]

      @always fn
    
    ).promise()


  # @EraseToken: (fn) ->
  #   @log "@EraseToken"
  #   jQuery.Deferred(->

  #     callback = (response) =>
  #       console.log "@EraseToken: Reply: #{response.status}"
  #       @resolve(response.ok)

  #     callback.MsgId = "#{++Façade.cnt}"

  #     app.addFn "erase", callback
  #     app.sendMessage "erase", [{MsgId: callback.MsgId}]

  #     @always fn  
    
  #   ).promise()


  @InitToken: (pin, puk, label, fn) ->
    @log "@InitToken"
    jQuery.Deferred(->

      callback = (response) =>
        console.log "@InitToken: Reply: #{response.status}"
        @resolve(response.ok)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "init", callback
      app.sendMessage "init", [{MsgId: callback.MsgId, pin: pin, puk: puk, label: label}]

      @always fn
    
    ).promise()


  @Logout: (fn) ->
    delete Façade.authData
    fn(true)

  @Login: (pin, fn) ->
    @log "@Login"
    jQuery.Deferred(->

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

          return @resolve(null)

        @resolve(response.tries_left)

      callback.MsgId = "#{++Façade.cnt}"

      app.addFn "verifypin", callback
      app.sendMessage "verifypin", [{MsgId: callback.MsgId, pin: pin}]
      @always fn
    
    ).promise()


module.exports = Façade