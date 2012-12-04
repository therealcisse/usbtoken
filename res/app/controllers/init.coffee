Spine        = require('spine')
GetLabel     = require('controllers/get-label')
Wizard       = require('lib/wizard')
GetPass      = require('controllers/get_pass')
Login        = require('controllers/login')
Façade       = require('lib/façade')

class Init extends Wizard

  @HEADER: 'Initialize Token'

  hasCancel: true

  doSetLabel: (params) ->
    @log "doSetLabel##{params}"
    @controller.next @, params  

  doSetPIN: (params) ->
    @log "setPIN#{params.pin}"
    @controller.next @, params   

  doSetPUK: (params) ->
    @log "setPUK#{params.puk}"

    @controller.fn.err = @controller.doSetPUK.err

    df = app.Loading()
    @delay (=> @controller.fn(params.pin, params.puk, params['label']); df())   

  unRenderMsg: (evt) -> 
    'Votre supporte ne sera pas re-initializer'

  unRendered: -> 
    # window.jQuery(window).unbind('beforeunload.init')
  
  constructor: ->
    super

    # window.jQuery(window).bind('beforeunload.init', @unRenderMsg)

    @steps = [      

      {
        Clss: GetLabel
        args:
          name: 'get-label'
          controller: @
          header: Init.HEADER
          title: "Token Name"      
          fn: @doSetLabel
      }

      {
        Clss: GetPass
        args:
          name: 'getpin'
          controller: @
          type: GetPass.PIN
          fn: @doSetPIN
      }

      {
        Clss: GetPass
        args:
          name: 'getpuk'
          controller: @
          type: GetPass.PUK
          fn: @doSetPUK
      }
    ]

    @app.delay -> Façade.SetWindowText()
    
module.exports = Init