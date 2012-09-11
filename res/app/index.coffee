Spine = require('spine')
Spine.Route = require('spine/lib/route')

Token        = require('models/token')
Topbar       = require('controllers/topbar')
Start        = require('controllers/start')
Login        = require('controllers/login')
PersonalInfo = require('controllers/personalinfo')
Unblock      = require('controllers/unblock')
KeyMgr       = require('controllers/keyMgr')
ChangePIN    = require('controllers/changePIN')
Init         = require('controllers/init')
Façade       = require('lib/façade')
ResetPIN     = require('controllers/resetPIN')

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
      
      @html(msg).find('.alert')[p['animation']] p['duration'], => @delay(@hide(id), p['delay']) if p['closable']

    hide: (e) =>
      e = e and @$('#' + e) or @el.find('.alert')
      => e.slideUp -> e.remove()

    clearAll: -> 
      @hide()()

  # args()
  constructor: ->
    super

    @routes
    
      '/unblock'            :   -> @verify(Token.Blocked) @setStatus
  
      '/changepin'          :   -> @verify(Token.LoggedIn) => @become @changepin()
  
      '/login'              :   -> @verify(Token.AuthRequired) @setStatus
  
      '/keys'               :   -> @verify(Token.LoggedIn) => @become @loggedin(KeyMgr.KeyList, app: @)

      '/key/:id'            :   (params) -> @verify(Token.LoggedIn) => params.app = @; @become @loggedin(KeyMgr.KeyView, params)

      '/key/gen'            :   -> @verify(Token.LoggedIn) => @become @loggedin(KeyMgr.GenForm, app: @)
      
      '/key/import/:type'   :   (params) -> @verify(Token.LoggedIn) => params.app = @; @become @loggedin(KeyMgr.ImportForm, params)

      '/logout'             :   @routeLogout

      '/minimize'           :   -> @log("minimize"); false

      '/close'              :   -> @log("close"); false
      
      '/init'               :   -> @verify(Token.LoggedIn) => @become @loggedin(Init, app: @)
      
      '/personal-info'      :   -> @verify(Token.LoggedIn) => @become @loggedin(PersonalInfo, controller: @, fn: @routePersonalInfo)
      
      '/setpin'              :   (params) -> @verify(Token.ChangePIN) => @become @setpw(type: GetPass.PIN, fn: @routeSetPIN, controller: @)      

      '/'                   :   @routeDetect

    @bind 'statusChanged', @statusChanged
    
    @topbar = new Topbar(el: '.topbar', app: @)
    @body   = new Spine.Controller(el: '.body')
    @msg    = new Msg(el: '#msg')

    Spine.Route.setup()

  reload: -> document.location.reload()

  verify: (expectedStatus) ->
    (fn) => 
      Façade.getStatus (status) =>
        
        if status is expectedStatus            
          @clearAllMsgs() 

          @el.attr('class', status or App.INITIAL_STATUS_CLASS) #
            
          return fn(status)
        
        showByAlert = (msg) => @alert msg: msg

        showByInnerHTML = (msg) =>  @msg.el.find('.alert').html msg

        msg = 
          switch expectedStatus
            when Token.AuthRequired then 'You must be connected'
            when Token.changePIN then 'You must reset your PIN'
            else 'An unknown error occured'

        waitPeriod = 4
        doTell = (sec) =>
          () =>
            return @navigate('#/') if sec is 0
            
            if sec is waitPeriod # first msg
              showByAlert "#{msg}, redirecting in #{sec-1} . . ."
            else
              showByInnerHTML "#{msg}, redirecting in #{sec-1} . . ."

            @delay doTell(sec-1), 1100
        
        doTell(waitPeriod)()

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
      duration: opts['duration'] or 600

  alert: (opts) =>
    # display alert message with animation

    opts = if typeof(opts) is 'string' then {msg: opts} else opts

    @msg.show
      type: 'error', 
      msg: opts['msg'], 
      closable: opts['closable'] or false, 
      delay: opts['delay'] or 6000, 
      animation: opts['animation'] or 'slideDown', 
      duration: opts['duration'] or 600    

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

#      when Token.ChangePIN
        # show setpin form

#        @become @setpw(type: GetPass.PIN, fn: @routeSetPIN, controller: @)        

      when Token.Blank
        # show blank alert

        @become @start(status)

      when Token.Locked
        # show locked alert

        @become @start(status)

      when null

        # detecting

        @become @start(status)

    false

  become: (s) ->
    @changeView new s.Clss(s.args)

    @info s.info if s.info?
    @alert s.alert if s.alert?

  start: (status) ->
    ret = 
      Clss: Start
      args: 
        app: @

    switch status
      when Token.Locked then ret.alert = 'Votre PUK a ete bloque, veuillez re-initialisez votre support.'
      when Token.Blank  then ret.alert = 'Le support doit etre initialize.'
      when Token.Absent then ret.alert = "Aucun support n'a ete detecte."
      when null         then ret.info  = 'Detection en cour, veuillez inserer votre support physique.'

    ret

  #TODO(use as change PIN, and use GetPass for set PIN/PUK)
  changepin: ->
    Façade.getOptions (opts) =>
      Clss: ChangePIN
      args:
        app: @
        doAction: @routeChangePIN
        minLength: opts['min-pin-length']
        maxLength: opts['max-pin-length']

#  setpw: (args) ->
#    Clss: GetPass
#    args: args

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
    alert: 'Votre PIN a ete bloque, veuillez entrez votre PUK.'

  loggedin: (Clss, args) ->
    Clss: Clss
    args: args or {}

  changeView: (view) ->
    if @currentView
      # animate removal of currentView?

      @currentView.release?()
      @currentView.unRendered?()
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

  dlg: (cmpnt, options={}) ->
    @delay -> window.jQuery(cmpnt.render().el).modal(options)

  # Routes

  routeDetect: =>
    @log '@routeDetect'

    Façade.getStatus (status, err) =>

      if err
        # show msg
        return false
      
      # Start page
      if status in [ Token.Absent, Token.Locked, Token.Blank, null ]
        @setStatus(status)
        return

      switch status

        when Token.Blocked

          @navigate("#/unblock")

        when Token.ChangePIN

          @navigate("#/setpin")          
        
        when Token.AuthRequired
        
          @navigate("#/login")
        
        when Token.LoggedIn
        
          @navigate("#/keys")

        else
        
          @setStatus(null)

      false

  selectKey: (key) ->
    @trigger('selectionChanged', key, true)

  unSelectKey: (key) ->
    @trigger('selectionChanged', key, false)

  routePersonalInfo: (params) =>
    @log '@routePersonalInfo'

    #TODO (save personal info)
      
    @navigate '#/keys'
    @delay (-> @info(msg: 'Your personal information was successfully saved.', closable: true)), 100    

  routeChangePIN: (params) =>
    @log '@routeChangePIN'    

    Façade.changePIN params.oldpin, params.pin, (err) =>
      if err
        @alert("Error #{if params.task is 'setpin' then 'setting' else 'changing'} PIN")
        return false

      @navigate '#/keys'
      @delay (-> @info(msg: 'Your PIN was successfully changed.', closable: true)), 100

  routeLogin: (params) =>
    @log '@routeLogin'

    Façade.login params.pin, (remainingAttempts) =>

      if remainingAttempts >= 0

        if remainingAttempts is 0

          @navigate('#/')
        
        else
        
          @alert("PIN invalide, il ne vous reste que #{remainingAttempts} essaie#{if remainingAttempts > 1 then 's' else ''} avant le blockage de votre PIN.")

        return false

      @navigate('#/keys')
  
  routeLogout: ->
    @log '@routeLogout'

    Façade.logout (err) =>
      if err
        # show msg
        return false

      @navigate('#/')
      @delay (-> @info(msg: 'You have successfully logged out.', closable: true)), 100

  #@deprecated
  routeUnblock: (params) =>
    @log '@routeUnblock'

    Façade.unblock params.puk, (err) =>
      if err >= 0

        if err is 0

          @setStatus(Token.Locked)

        else

          @alert("PUK, invalide, il ne vous reste que #{err} essaie#{if err > 1 then 's' else ''} avant le blockage permanent de votre support.")

        return false
      
      @info(msg: 'Your PIN was successfully unblocked . . .', closable: true)
      @delay (-> @navigate("#/setpin/#{params.puk}")), 1000

module.exports = App