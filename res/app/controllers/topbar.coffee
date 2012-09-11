Spine = require('spine')
Token = require('models/token')

# Unattached
class Topbar extends Spine.Controller

  # args(app)
  constructor: ->
    super
    
    @app.bind 'statusChanged', @statusChanged
    
    @bind 'release', => 
      @app.unbind 'statusChanged', @statusChanged
      delete @app

    @el.on 'dragstart', (evt) => 
      @log 'Topbar#ondragstart'
      
      evt.preventDefault()
      evt.stopPropagation()

      evt.originalEvent.dataTransfer.setDragImage document.getElementById('blank'), 0, 0
      evt.originalEvent.dataTransfer.effectAllowed = 'none'
      
      false

    @el.on 'drag', (evt) =>
      @log 'Topbar#ondrag'

  statusChanged: (status) =>
    @log "Topbar#statusChanged:#{status}"

    switch status
      
      when Token.Blocked
        # show lock icon  

        ;

      when Token.AuthRequired
        # hide tools & logout buttons

        ;

      when Token.Absent
        # show . . .  

        ;

      when Token.ChangePIN
        # show . . .

        ;

      when Token.LoggedIn
        # show tools & logout buttons

        ;

      when Token.Locked
        # show locked icon

        ;

      when Token.Blank
        # show blank icon

        ;

      when null
        # Detecting

        ;


module.exports = Topbar