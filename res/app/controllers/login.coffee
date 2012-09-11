Spine  = require('spine')
App    = require('index')
Façade = require('lib/façade')

class Login extends Spine.Controller  

  @SO_LOGIN: 'so_login'
  @USER_LOGIN: 'user_login'

  # args(doLogin, controller, type)
  constructor: ->
    super
    
    @bind 'release', => 
      delete @controller

  events:
    'submit form'         :   'submit'
    'click form .cancel'  :   'cancel'

  elements:
    '[name=pin]'  :	  'pin'

  className: 'login'

  @templ: require('views/login')

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    pin  : cleaned 'pin'

  @viewopts: (type) ->
    Façade.getOptions (opts) =>
      type      : type
      minLength : opts['min-pin-length'],
      maxLength : opts['max-pin-length']

  render: ->
    @html Login.templ(Login.viewopts(@type))

  submit: (e) ->

    e.preventDefault()

    params = @params()       

    if msg = Login.valid(params)
   	  @controller.alert(msg) 
   	  return false

    @delay => @doLogin(params)

    false

  cancel: (e) ->

    e.preventDefault()
    e.stopPropagation()

    @controller.cancelled?(@)

    false
  
  # private

  @valid: (params) ->  
    Façade.getOptions (opts) => 
      return "PIN must be between #{opts['min-pin-length']} and #{opts['max-pin-length']} caracters." unless params.pin.length > opts['min-pin-length'] and params.pin.length < opts['max-pin-length'] 

module.exports = Login