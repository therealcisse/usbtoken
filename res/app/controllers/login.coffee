Spine  = require('spine')

Façade = require('lib/façade')

class Login extends Spine.Controller  

  @SO_LOGIN: 'so_login'
  @USER_LOGIN: 'user_login'

  # args(doLogin, controller, type)
  constructor: ->
    super
    
    @bind 'release', => 
      delete @controller

    @doLogin.err = =>
      @pin.val('')
      @submitBtn.enable()
      @delay => @pin[0].focus()      

    @delay -> Façade.SetWindowText('Login')

  events:
    'submit form'         :   'submit'
    'click form .cancel'  :   'cancel'

  elements:
    '[name=pin]'  :   'pin'
    '[type=submit]'  :	  'submitBtn'

  className: 'login'

  @templ: require('views/login')

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    pin  : cleaned 'pin'

  @viewopts: (type) ->
    Façade.GetPINOpts (opts) =>
      type      : type
      minLength : opts['minlen'],
      maxLength : opts['maxlen']

  render: ->
    @html Login.templ(Login.viewopts(@type))

  submit: (e) ->

    e.preventDefault()
    @submitBtn.enable(false)

    params = @params()

    if msg = Login.valid(params)
      @controller.alert(msg: msg, closable: true) 
      @doLogin.err()
      return false

    df = app.Loading()       
    @delay (=> @doLogin(params); df())

    false

  cancel: (e) ->

    e.preventDefault()
    e.stopPropagation()

    @controller.cancelled?(@)

    false
  
  # private

  @valid: (params) ->  
    Façade.GetPINOpts (opts) => 
      return "PIN must be between #{opts['minlen']} and #{opts['maxlen']} caracters." unless params.pin.length >= opts['minlen'] and params.pin.length <= opts['maxlen'] 

module.exports = Login