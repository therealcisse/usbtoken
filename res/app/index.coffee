Spine = require('spine')
Spine.Route = require('spine/lib/route')

Token        = require('models/token')
Start        = require('controllers/start')
Login        = require('controllers/login')
GetLabel     = require('controllers/get-label')
Unblock      = require('controllers/unblock')
KeyMgr       = require('controllers/keyMgr')
ChangePIN    = require('controllers/changePIN')
Init         = require('controllers/init')
Façade       = require('lib/façade')
Wizard       = require('lib/wizard')
ResetPIN     = require('controllers/resetPIN')
Dlg          = require('controllers/dlg')

class App extends Spine.Controller

  @INITIAL_STATUS_CLASS: 'detecting'

  #userType: Token.UserType.SIMPLE

  events:
    'click .locale'  : 'changeLang'

  changeLang: (evt) ->
    
    evt.prevenDefault()
    evt.stopPropagation()

    # show change language dialog

    false

  class Msg extends Spine.Controller
    constructor: ->
      super

    @msgTempl: require('views/msg')

    # args(type, closable?, msg, delay)
    show: (p) ->
      id  = 'id' + new Date().getTime()
      msg = 
        Msg.msgTempl(
          id: id, 
          type: p['type'], 
          closable: p['closable'], 
          msg: p['msg']
        )
      
      @html(msg).find('.alert')[p['animation']] p['duration'], (=> @delay(@hide(id), p['delay']) if p['closable'])

    hide: (e) =>
      e = e and @$('#' + e) or @el.find('.alert')
      => e.slideUp -> e.remove()

    clearAll: -> 
      @hide()()

  # args()
  constructor: ->
    super

    Façade.Load()

    app.setMessageCallback 'debug', ([response]) ->
      console.log(response)

    app.setMessageCallback 'token_removed', ([response]) =>
      if Façade.reader 
        if Façade.reader is response.reader      
          delete Façade.reader
          @delay @reload
      else
        @reload()

    app.setMessageCallback 'token_inserted', ([response]) =>      
      unless Façade.reader and Façade.reader is response.reader
        Façade.reader = response.reader
        @delay @reload

    @routes
    
      '/unblock'            :   -> @setStatus(Token.Blocked)
  
      '/changepin'          :   -> @ifLoggedIn() => @become @changepin()
  
      '/login'              :   -> @setStatus(Token.AuthRequired)
      
      '/erase'              :   @routeErase

      '/gen-csr/:id'        :   (params) -> @ifLoggedIn() => @become @any(KeyMgr.GenForm.GetX509ReqInfo, controller: @, id: params.id, fn: @doGenX509Req)
      
      '/key/gen'            :   -> @ifLoggedIn() => @become @any(KeyMgr.GenForm, app: @)
      
      '/ImportX509Certificate'   :  -> @ifLoggedIn() => @become @any(KeyMgr.ImportX509Certificate, app: @)

      '/ImportPrKey'    :   -> @ifLoggedIn() => @become @any(KeyMgr.ImportPrKey, app: @)

      '/ImportPubKey'   :   -> @ifLoggedIn() => @become @any(KeyMgr.ImportPubKey, app: @)      

      # '/PrKey/:id'           :   (params) -> @ifLoggedIn() => @become @any(KeyMgr.X509View, id: params.id, type: 'PrKey', app: @)
            
      '/X509Certificate/:id'  :   (params) -> @ifLoggedIn() => @become @any(KeyMgr.X509View, id: params.id, type: 'X509Certificate', app: @)
  
      '/keys'               :   -> @ifLoggedIn() => @become @any(KeyMgr.KeyList, app: @)

      '/logout'             :   @routeLogout

      '/init'               :   -> @become @any(Init, app: @, fn: @routeInit)
      
      '/setpin'             :   (params) -> @become @setpw(type: GetPass.PIN, fn: @routeSetPIN, controller: @)      

      '/'                   :   @routeDetectOne

    @bind 'statusChanged', @statusChanged
    
    @body   = new Spine.Controller(el: '.body')
    @msg    = new Msg(el: '#msg')

    Spine.Route.setup()

    @delay -> Façade.SetWindowText()

  reload: -> if document.location.hash in [ '#/', '' ] then document.location.reload() else @navigate('/')

  ifLoggedIn: ->
    (fn) =>
      if Façade.authData then fn() else @navigate("/")

  setStatus: (status) =>
    @trigger('statusChanged', status)

  info: (opts) =>
    # display info message with animation

    opts = if typeof(opts) is 'string' then {msg: opts} else opts

    @msg.show
      type: 'info', 
      msg: opts['msg'], 
      closable: opts['closable'] or false, 
      delay: opts['delay'] or 6000, 
      animation: opts['animation'] or 'slideDown', 
      duration: opts['duration'] or 200

  alert: (opts) =>
    # display alert message with animation

    opts = if typeof(opts) is 'string' then {msg: opts} else opts

    @msg.show
      type: 'error', 
      msg: opts['msg'], 
      closable: opts['closable'] or false, 
      delay: opts['delay'] or 6000, 
      animation: opts['animation'] or 'slideDown', 
      duration: opts['duration'] or 200    

  clearAllMsgs: -> 
    @msg.clearAll()

  # private

  statusChanged: (status) ->
    @log "App#statusChanged:#{status}"

    @el.attr('class', status or App.INITIAL_STATUS_CLASS) #

    switch status

      when Token.Absent
        # show absent alert

        @become @start(status)

      when Token.Blocked
        # show unblock form

        @become @blocked()

      when Token.AuthRequired
        # show login form

        @become @authrequired()

      when Token.Blank
        # show blank alert

        @become @start(status)

      when Token.Locked
        # show locked alert

        @become @start(status)

      when null

        # detecting

        @become @start(status)

  become: (s) ->
    @changeView new s.Clss(s.args)

    @info s.info if s.info?
    @alert s.alert if s.alert?

  start: (status, {alert: alert, info: info}={}) ->
    ret = 
      Clss: Start
      alert: alert
      info: info
      args: 
        app: @
        status: status

    switch status
      when Token.Locked   then ret.alert = 'Votre PUK a ete bloque, veuillez re-initialisez votre support.' if not alert
      when Token.Blank    then ret.alert = 'Le support doit etre initialize.' if not alert
      when Token.Absent   then ret.alert = "Aucun supporte detecte, veuillez inserer votre supporte physique." if not alert
      when Token.ReadOnly then ret.alert = "Le supporte est en mode lecture." if not alert
      when Token.InUse    then ret.alert = "Le supporte est en cours d'utilisation." if not alert
      when null           then ret.info  = 'Detection en cour, veuillez inserer votre support physique.' if not info

    ret

  #TODO(use as change PIN, and use GetPass for set PIN/PUK)
  changepin: ->
    Façade.GetPINOpts (opts) =>
      Clss: ChangePIN
      args:
        app: @
        doAction: @routeChangePIN
        minLength: opts['minlen']
        maxLength: opts['maxlen']

  authrequired: ->
    Clss: Login
    args:
      controller: @
      doLogin: @routeLogin
      type: Login.USER_LOGIN

  blocked: -> 
    Clss: ResetPIN
    args:
      app: @
      fn: @routeUnblock
    alert: 'Votre PIN a ete bloque, veuillez entrez votre PUK.'

  any: (Clss, args) ->
    Clss: Clss
    args: args or {}

  changeView: (view) ->
    return view.rendered?() if view instanceof Wizard

    if @currentView
      # animate removal of currentView?

      @currentView.release?()
      # @currentView.unRendered?()
      delete @currentView

    @currentView = view
    @render()

  render: ->
    # render the topbar and the content area
    # the topbar doesn't change
    # the body is empty

    if @currentView

      # animate addtion of currentView
      if typeof @currentView.render is 'function'

        @body.html(@currentView.render()).show('slide', {direction: 'right', easing: 'easeInOutElastic'}, 700, => @currentView.rendered?())

      else

        @currentView.rendered?()

  confirm: (fn, hidden = -> console.log('hidden');) ->
    msg: 'Are you sure?'
    hidden: hidden
    buttons: [
      {
        id: 'dlg-yes'
        title: 'Yes'
        primary: true
        fn: fn,
      },
      {
        id: 'dlg-no'
        title: 'No'
        fn: (evt) -> window.jQuery(evt.target).closest('.dlg').modal('hide') 
      }
    ]

  dlg: (meta) ->
    @delay -> 
      dlg = window.jQuery((new Dlg(meta)).render().el).modal(meta.options or {})
      dlg.on('hide', meta.hidden) if meta.hidden

  cancelled: =>
    @log "App.cancelled"
    @navigate "/"      

  detectToken: =>
    @log "@detectToken"
    
    Façade.GetStatus (status, err) =>
      @clearAllMsgs()

      return false if err

      # Start page
      if status in [ Token.Absent, Token.Locked, Token.Blank, Token.InUse, Token.ReadOnly ]
        @setStatus(status)
        return false

      switch status

        when Token.Blocked

          @navigate("#/unblock")

        when Token.SetPIN

          @navigate("#/setpin")          
        
        when Token.AuthRequired
        
          @navigate("#/login")
        
        when Token.LoggedIn
        
          @navigate("#/keys")

        #else

          ### Unknow status, try to detect again ###

          # @setStatus(null)

    false

  selectKey: (key) ->
    @trigger('selectionChanged', key)

  unSelectKey: (key) ->
    @trigger('selectionChanged', key, false)

  # Routes

  routeDetectOne: =>
    @log '@routeDetectOne'
    @detectToken()

  # TODO(implement this)
  routeGetLabel: (params) =>
    @log '@routeGetLabel'

    #TODO (save personal info)      
    @delay (=> @navigate('#/keys'); @delay((-> @info(msg: 'Your information was successfully saved.', closable: true)), 100))

  doGenX509Req: (params) ->
    @log "doGenX509Req: #{params}"
    df = app.Loading()

    Façade.GenX509Req params.id, params.cn, params.o, params.ou, params.city, params.region, params.country, params.emailAddress, (ok) =>
      console.log "doGenX509Req:#{ok}"

      df()

      if ok 

        @controller.info msg: "La requete a ete genere avec success.", closable: true, duration: 1500
        @delay (=> @navigate "/"), 1500
        return false

      @controller.alert msg: "Il y a eu une erreur, essayer de nouveau.", closable: true
      @controller.doGenX509Req.err?()

  routeInitPIN: (params) =>
    @log '@routeInitPIN'    

    Façade.InitPIN params.pin, params.puk, (err) =>
      
      if ok
        
        @delay (=> @info(msg: 'Your PIN was successfully changed.', closable: true); @delay((=> @navigate '#/keys'), 1000))
        
        return false

      @routeInitPIN.err?()
      @alert(msg: 'An unknown error occured, please try again.', closable: true)

  routeChangePIN: (params) =>
    @log '@routeChangePIN'    

    Façade.ChangePIN params.oldpin, params.pin, (ok) =>
      
      if ok

        @delay (=> @info(msg: 'Your PIN was successfully changed.', closable: true); @delay((=> @navigate '#/keys'), 1000))
        
        return false

      @routeChangePIN.err?()
      @alert(msg: 'An unknown error occured, please try again.', closable: true)

  routeLogin: (params) =>
    @log '@routeLogin'

    Façade.Login params.pin, (err) =>

      if err is null
        @setStatus(Token.LoggedIn)
        return @navigate '#/keys'

      if err is 0

        @navigate '/' 

      else 

        if err <= 3
        
          @alert msg: "PIN invalide, il ne vous reste que #{err} essaie#{if err > 1 then 's' else ''} avant le blockage de votre PIN.", closable: true

        else 

          @alert msg: "PIN invalide, essayer encore.", closable: true

        @routeLogin.err?()

      false

  routeInit: (pin, puk, label) =>
    @log '@routeInit'

    Façade.InitToken pin, puk, label, (ok) =>

      if ok
        @delay (=> @info(msg: 'Your token was successfully initialized.', closable: true); @delay( (=> @navigate('#/')), 1000))

        # show msg
        return false       

      @routeInit.err?()
      @alert(msg: 'An unknown error occured, please try again.', closable: true)
      @delay (=> @navigate "/"), 700
  
  routeErase: ->
    @log '@routeErase'

    df = app.Loading()
    Façade.EraseToken (ok) =>
      df()

      if ok
        @delay (=> @info(msg: 'Your token was successfully erase . . .', closable: true); @delay(-> @navigate('/'))), 1000
        # show msg
        return false  

      @alert(msg: 'An unknown error occured, please try again.', closable: true)

  routeLogout: ->
    @log '@routeLogout'

    Façade.Logout (err) =>

      if err
        @delay(=> @info(msg: 'You have successfully logged out.', closable: true); @delay( (=> @navigate('/')), 200))        

        # show msg
        return false

      @navigate('/')


  #TODO(support PUK locking?)
  routeUnblock: (params) =>
    @log '@routeUnblock'

    Façade.Unblock params.puk, params.pin, (ok) =>
      
      if ok
        @delay (=> @info(msg: 'You have successfully unblocked.', closable: true); @delay( (=> @navigate('/')), 1000))        

        # show msg
        return false

      @routeUnblock.err?()
      @delay (=> @navigate('/'); @delay( (=> @alert(msg: 'An unknown error occured, please try again.', closable: true)), 1000   ))

      # if err >= 0

      #   if err is 0

      #     @setStatus(Token.Locked)

      #   else

      #     @alert("PUK, invalide, il ne vous reste que #{err} essaie#{if err > 1 then 's' else ''} avant le blockage permanent de votre support.")

      #   return false
      
      # @info(msg: 'Your PIN was successfully unblocked . . .', closable: true)
      # @delay (-> @navigate("#/setpin/#{params.puk}")), 1000

module.exports = App