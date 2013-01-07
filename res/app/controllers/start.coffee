Spine = require('spine')

Token  = require('models/token')
Façade = require('lib/façade')

class Start extends Spine.Controller

  @events:
    'click .action' : 'doDetect'
    'click .init'   : 'doInit'

  # args(app)
  constructor: ->
    super
    
    @bind 'release', => 
      delete @app

    @token_removed  = @doDetect
    @token_inserted = @doDetect

  className: 'start'

  @tmpl: require('views/start')

  render: ->
    @el.html Start.tmpl()
    @

  # setStatus: (status) ->
  #   @app.el.attr('class', "#{@app.className} #{status}")
  #   @el.attr('class', "#{@className} #{status}")

  #   switch status
  #     when Token.Locked   then @app.alert 'Votre PUK a ete bloque, veuillez re-initialisez votre support.'
  #     when Token.Blank    then @app.alert 'Le support doit etre initialize.'
  #     when Token.Absent   then @app.alert "Aucun supporte detecte, veuillez inserer votre supporte physique."
  #     when Token.ReadOnly then @app.alert "Le supporte est en mode lecture."
  #     when Token.InUse    then @app.alert "Le supporte est en cours d'utilisation."

  # doDetectToken: =>
  #   @log "@detectToken"
    
  #   Façade.GetStatus (status, err) =>
  #     @app.clearAllMsgs()

  #     onError = -> status = Token.Absent

  #     onError() if err or not status

  #     # Start page
  #     if status in [ Token.Absent, Token.Locked, Token.Blank, Token.InUse, Token.ReadOnly ]
  #       @setStatus(status)
  #       return false

  #     switch status

  #       when Token.Blocked

  #         @navigate("#/unblock")

  #       when Token.SetPIN

  #         @navigate("#/setpin")          
        
  #       when Token.AuthRequired
        
  #         @navigate("#/login")
        
  #       when Token.LoggedIn
        
  #         @navigate("#/keys")

  #   false    

  _rendered: => do @doDetect

  doInit: =>
    @navigate '#/init'

  doDetect: =>
    @app.el.attr('class', "#{@app.className} detecting") #
    @el.attr('class', "#{@className} detecting") #
    @app.info(app.$T('msg_is_detecting'))
    
    @delay @app.doDetectToken, 200 

  # statusChanged: (status) =>
  #   @log "Start#statusChanged:#{status}"
  #   @el.attr('class', status) if status in [ Token.Absent, Token.Locked, Token.Blank, Token.InUse, Token.ReadOnly ]

  
module.exports = Start