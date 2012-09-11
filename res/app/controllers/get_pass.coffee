Spine  = require('spine')
Façade = require('lib/façade')

class GetPass extends Spine.Controller

  @PIN: 'pin'
  @PUK: 'puk'

  elements:
    '[name=pw]'         : 'pw'
    '[name=pw_confirm]' : 'pw_confirm'

  events:
    'submit form'         : 'submit'
    'click form .cancel'  :   'cancel'

  className: 'getpw'

  # args(controller, type)
  constructor: ->
    super

    @bind 'release', =>
      delete @controller

  @viewopts: (type, title) ->

    Façade.getOptions (opts) =>

      return switch type

        when GetPass.PIN

          {  
            title: title or 'Enter PIN',
            label: 'PIN',
            actionLabel: 'Ok',
            minLength : opts['min-pin-length'],
            maxLength : opts['max-pin-length']
          }

        when GetPass.PUK

          {
            title: title or 'Enter PUK',
            label: 'PUK',
            actionLabel: 'Ok',
            minLength : opts['min-puk-length'],
            maxLength : opts['max-puk-length']
          }

  @templ: require('views/getpass')

  render: ->
    @html GetPass.templ(GetPass.viewopts(@type, @title))

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    type        : @type
    pw          : cleaned 'pw'
    pw_confirm  : cleaned 'pw_confirm'

  @valid: (params) ->

    Façade.getOptions (opts) ->

      switch params.type

        when GetPass.PIN

          return "PIN must be between #{opts['min-pin-length']} and #{opts['max-pin-length']} caracters." unless params.pw.length > opts['min-pin-length'] and params.pw.length < opts['max-pin-length']  
          return "The PIN confirmation does not match." unless params.pw is params.pw_confirm  

        when GetPass.PUK

          return "PUK must be between #{opts['min-puk-length']} and #{opts['max-puk-length']} caracters." unless params.pw.length > opts['min-puk-length'] and params.pw.length < opts['max-puk-length']  
          return "The PUK confirmation does not match." unless params.pw is params.pw_confirm  

  cancel: (evt) ->
    evt.preventDefault()
    evt.stopPropagation()

    @delay @controller.cancelled?(@)

    false
  
  submit: (evt) ->

    evt.preventDefault()

    params = @params()

    if msg = GetPass.valid(params)
      @controller.alert(msg)
      return false

    for key, val of @vars
      params[key] = val

    params[@type] = params['pw']
    delete params['pw']

    @delay => @fn(params)

    false

module.exports = GetPass