Spine        = require('spine')
PersonalInfo = require('controllers/personalinfo')
Wizard       = require('lib/wizard')
GetPass      = require('controllers/get_pass')
Login        = require('controllers/login')
Façade       = require('lib/façade')

class Init extends Wizard

  doSetPIN: (params) ->
    @log "setPIN#{params.pin}"

    @controller.end @, params   

  doSetPUK: (params) ->
    @log "setPUK#{params.puk}"

    @controller.end @, params   

  doSOLogin: (params) ->
    @log "doLogin##{params.puk}"

    Façade.login params.pin, (remainingAttempts) =>

      if remainingAttempts >= 0

        if remainingAttempts is 0

          @navigate '#/'
        
        else
        
          @controller.alert "PIN invalide, il ne vous reste que #{remainingAttempts} essaie#{if remainingAttempts > 1 then 's' else ''} avant le blockage de votre PIN."

        return false

      @controller.end @, params   

  doSetPersonalInfo: (params) ->
    @log "doSetPersonalInfo##{params}"

    @controller.end @, params  

  unRenderMsg: (evt) -> 
    'Votre supporte ne sera pas re-initializer'

  unRendered: -> 
    window.jQuery(window).bind('beforeunload', @unRenderMsg)
  
  constructor: ->
    super

    window.jQuery(window).bind('beforeunload', @unRenderMsg)

    @steps = [
      
      {
        Clss: Login
        args:
          name: 'sologin'
          controller: @
          doLogin: @doSOLogin
          type: Login.SO_LOGIN
      }

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
    
module.exports = Init