Spine   = require('spine')
Wizard  = require('lib/wizard')
Unblock = require('controllers/unblock')
GetPass = require('controllers/get_pass')
Façade  = require('lib/façade')
Token   = require('models/token')

class ResetPIN extends Wizard

  doSetPIN: (params) ->
    @log 'ResetPIN@doSetPIN'

    @controller.fn.err = @controller.doSetPIN.err
    
    df = app.Loading()
    @delay (=> @controller.fn(params).done df)

    # Façade.resetPIN params.puk, params.pin, (err) =>
    #   if err
    #     @controller.alert "Error resetting PIN"
    #     return false

    #   @delay (=> @controller.info msg: 'Your PIN was successfully changed.', closable: true), 100  
      
    #   @controller.end @

  doUnblock: (puk) -> 
    @log 'ResetPIN@doUnblock'

    # Façade.unblock puk, (err) =>
    #   if err >= 0

    #     if err is 0

    #       @controller.app.setStatus(Token.Locked)

    #     else

    #       @controller.alert "PUK, invalide, il ne vous reste que #{err} essaie#{if err > 1 then 's' else ''} avant le blockage permanent de votre support."

    #     return false
      
    #   @controller.info msg: 'Your PIN was successfully unblocked . . .', closable: true

    @controller.next @, puk: puk

  constructor: ->

    @steps = [
      
      {
        Clss: Unblock
        args:
          name: 'unblock'
          controller: @
          doUnblock: @doUnblock
      }

      {
        Clss: GetPass
        infoMsg:
          msg: app.$T('msg_reset_pin')
          closable: true
        args: 
          name: 'getpin'
          title: app.$T('title_reset_pin')
          controller: @
          type: GetPass.PIN
          fn: @doSetPIN
      }
    ]

    super

module.exports = ResetPIN