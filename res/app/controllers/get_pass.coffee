Spine  = require('spine')
Façade = require('lib/façade')

class GetPass extends Spine.Controller

  @PIN: 'pin'
  @PUK: 'puk'

  elements:
    '[name=pw]'         : 'pw'
    '[name=pw_confirm]' : 'pw_confirm'
    '[type=submit]'  :    'submitBtn'

  events:
    'submit form'         : 'submit'
    'click form .cancel'  : 'cancel'

  className: 'getpw'

  # args(controller, type)
  constructor: ->
    super

    @bind 'release', =>
      delete @controller

    @fn.err = =>
      @pw.val('')
      @pw_confirm.val('')
      @submitBtn.enable()
      @delay => @pw[0].focus()

  @viewopts: (self, type, title) ->

    Façade.GetPINOpts (opts) =>

      return switch type

        when GetPass.PIN

          {  
            title: title or 'Enter PIN',
            label: 'PIN',
            actionLabel: 'Ok',
            minLength : opts['minlen'],
            maxLength : opts['maxlen']
            hasCancel: self.controller.hasCancel
          }

        when GetPass.PUK

          {
            title: title or 'Enter PUK',
            label: 'PUK',
            actionLabel: 'Ok',
            minLength : opts['minlen'],
            maxLength : opts['maxlen']
            hasCancel: self.controller.hasCancel
          }

  @templ: require('views/getpass')

  render: ->
    @html GetPass.templ(GetPass.viewopts(@, @type, @title))

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    type        : @type
    pw          : cleaned 'pw'
    pw_confirm  : cleaned 'pw_confirm'

  @valid: (params) ->

    Façade.GetPINOpts (opts) ->

      switch params.type

        when GetPass.PIN

          return "PIN must be between #{opts['minlen']} and #{opts['maxlen']} caracters." unless params.pw.length >= opts['minlen'] and params.pw.length <= opts['maxlen']  
          return "The PIN confirmation does not match." unless params.pw is params.pw_confirm  

        when GetPass.PUK

          return "PUK must be between #{opts['minlen']} and #{opts['maxlen']} caracters." unless params.pw.length >= opts['minlen'] and params.pw.length <= opts['maxlen']  
          return "The PUK confirmation does not match." unless params.pw is params.pw_confirm  

  cancel: (evt) ->
    evt.preventDefault()
    evt.stopPropagation()

    @controller.cancelled?(@)

    false
  
  submit: (evt) ->

    evt.preventDefault()
    @submitBtn.enable(false)

    params = @params()

    if msg = GetPass.valid(params)
      @controller.alert(msg: msg, closable: true)
      @fn.err()
      return false

    for key, val of @vars
      params[key] = val

    params[@type] = params['pw']
    delete params['pw']

    @fn(params)

    false

module.exports = GetPass