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

  className: 'start'

  @tmpl: require('views/start')

  render: ->
    @el.html Start.tmpl()
    @

  doInit: =>
    @navigate '#/init'

  doDetect: =>
    @app.el.attr('class', "#{@app.className} detecting") #
    @el.attr('class', "#{@className} detecting") #
    @app.info('Detection en cour . . .')
    
    @delay @app.detectToken, 1500 

  #statusChanged: (status) =>
    #@log "Start#statusChanged:#{status}"
    #@el.attr('class', status) if status in [ Token.Absent, Token.Locked, Token.Blank, Token.InUse, Token.ReadOnly ]

  
module.exports = Start