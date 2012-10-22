Spine        = require('spine')
PersonalInfo = require('controllers/personalinfo')
Wizard       = require('lib/wizard')
GetPass      = require('controllers/get_pass')
Login        = require('controllers/login')
Façade       = require('lib/façade')

class Init extends Wizard

  doSetPersonalInfo: (params) ->
    @log "doSetPersonalInfo##{params}"
    @controller.next @, params  

  doSetPIN: (params) ->
    @log "setPIN#{params.pin}"
    @controller.next @, params   

  doSetPUK: (params) ->
    @log "setPUK#{params.puk}"

    df = app.Loading()
    @delay (=> @controller.fn(params.pin, params.puk, params.fullName); df())   

  unRenderMsg: (evt) -> 
    'Votre supporte ne sera pas re-initializer'

  unRendered: -> 
    window.jQuery(window).unbind('beforeunload', @unRenderMsg)
  
  constructor: ->
    super

    window.jQuery(window).bind('beforeunload', @unRenderMsg)

    @steps = [      

      {
        Clss: PersonalInfo
        args:
          name: 'personal-info'
          controller: @
          fn: @doSetPersonalInfo
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

    @app.delay -> Façade.SetWindowText('Initialize')
    
module.exports = Init