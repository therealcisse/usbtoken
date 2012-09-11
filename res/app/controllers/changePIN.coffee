Spine = require('spine')
Façade = require('lib/façade')
Token = require('models/token')

class ChangePIN extends Spine.Controller

  className: 'changepin'

  # args(doAction, app, minLength, maxLength)
  constructor: (args) ->
    super

    @bind 'release', => 
      delete @app

    @bind 'setpin-error', (msg) => @app.alert(msg: msg, closable: true)    

  events:
    'submit form.changepin'   :    'submit'
    'click  form .cancel'     :    'cancel'

  elements:
    '[name=oldpin]'        :    'oldpin'   
    '[name=pin]'           :    'pin' 
    '[name=pin_confirm]'   :    'pin_confirm'

  @templ: require('views/changepin')

  render: ->
    @html ChangePIN.templ()

  cancel: ->
    Façade.getStatus (status) =>
      (return @app.trigger('alert', 'You must reset your PIN.'); false) if status is Token.ChangePIN
      @navigate('#/keys')
      false

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    oldpin       : cleaned 'oldpin'      
    pin          : cleaned 'pin'      
    pin_confirm  : cleaned 'pin_confirm'      

  submit: (e) -> 
    @log '@actn'
    e.preventDefault()

    params = @params()

    if msg = @valid(params.oldpin, params.pin, params.pin_confirm)
      @app.alert(msg) 
      return false

    # doAction process
    @doAction(params)
    false
  
  # private

  valid: (oldpin, pin, pin_confirm) -> 

    return "Old PIN must be between #{@minLength} and #{@maxLength} caracters." unless oldpin.length > @minLength and oldpin.length < @maxLength
    
    return "New PIN must be between #{@minLength} and #{@maxLength} caracters." unless pin.length > @minLength and pin.length < @maxLength 
   
    return "The PIN confirmation does not match." unless pin is pin_confirm 
    
module.exports = ChangePIN