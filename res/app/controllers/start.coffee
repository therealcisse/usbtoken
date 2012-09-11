Spine = require('spine')
Token = require('models/token')

class Start extends Spine.Controller

  # args(app)
  constructor: ->
    super
    @app.bind 'statusChanged', @statusChanged
    @bind 'release', => 
      @app.unbind 'statusChanged', @statusChanged
      delete @app

  className: 'start'

  @tmpl: require('views/start')

  render: ->
    @el.html Start.tmpl()
    @

  detecting: ->

  locked: ->
  
  blocked: ->

  blank: ->

  absent: ->

  statusChanged: (status) =>
    @log "Start#statusChanged:#{status}"
    
    switch status

      when Token.Absent
        # show absent alert, let user retry the detection process

        @absent()

      when Token.Locked
        # show locked alert, let user retry the detection process

        @locked()

      when Token.Blocked
        # show blocked alert, let user retry the detection process

        @blocked()

      when Token.Blank
        # show uninitialized alert

        @blank()

      when null

        # Absent : DEFAULT
        @detecting()
  
module.exports = Start