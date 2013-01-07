Spine        = require('spine')
GetLabel     = require('controllers/get-label')
Wizard       = require('lib/wizard')
GetPass      = require('controllers/get_pass')
Login        = require('controllers/login')
Façade       = require('lib/façade')

class Init extends Wizard
  
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
    @delay (=> @controller.fn(params.pin, params.puk, params['label']).done df)   

  unRenderMsg: (evt) -> 
    app.$T('token_wont_be_initialized')

  unRendered: -> 
    # window.jQuery(window).unbind('beforeunload.init')
  
  constructor: ->
    super

    @constructor.HEADER = app.$T('msg_init')

    # window.jQuery(window).bind('beforeunload.init', @unRenderMsg)

    @steps = [      

      {
        Clss: GetLabel
        args:
          name: 'get-label'
          controller: @
          header: Init.HEADER
          title: app.$T('label_token_name')      
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