Spine  = require('spine')
App    = require('index')
Façade = require('lib/façade')

class Unblock extends Spine.Controller

  # args(doUnblock, controller)
  constructor: ->
    super

    @bind 'release', => 
      delete @controller

    @delay -> Façade.SetWindowText('Unblock')
      
  events:
    'submit form' :   'unblock'

  elements:
    '[name=puk]' :   'puk'
    '[type=submit]'  :    'submitBtn'

  className: 'unblock'

  @templ: require('views/unblock')

  viewopts: ->
    Façade.GetPINOpts (opts) =>
      minLength : opts['minlen']
      maxLength : opts['maxlen']

  params: ->
    cleaned = (key) =>
      (@[key].val() or '').trim()

    puk  : cleaned 'puk'    

  render: ->
    @html Unblock.templ(@viewopts())

  unblock: (e) ->
    @log '@unblock'

    e.preventDefault()
    @submitBtn.enable(false)

    params = @params() 

    if msg = Unblock.valid(params)
      @controller.alert(msg: msg, closable: true)
      @submitBtn.enable()
      return false

    @doUnblock(params.puk); @submitBtn.enable()
  
  # private

  @valid: (params) ->  
    Façade.GetPINOpts (opts) => 
      return "PUK must be between #{opts['minlen']} and #{opts['maxlen']} caracters." unless params.puk.length > opts['minlen'] and params.puk.length < opts['maxlen']    

module.exports = Unblock