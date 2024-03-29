Spine  = require('spine')
App    = require('index')
Façade = require('lib/façade')

class Unblock extends Spine.Controller

  # args(doUnblock, controller)
  constructor: ->
    super

    @bind 'release', => 
      delete @controller

    @doUnblock.err = =>
      @puk.val('')
      @submitBtn.enable()
      @delay => @puk[0].focus()

    @delay -> Façade.SetWindowText(app.$T('title_unblock'))
      
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
      @doUnblock.err()
      return false

    @doUnblock(params.puk)
  
  # private

  @valid: (params) ->  
    Façade.GetPINOpts (opts) => 
      return app.$T('invalid_puk_length').Format(opts['minlen'], opts['maxlen']) unless params.puk.length >= opts['minlen'] and params.puk.length <= opts['maxlen']    

module.exports = Unblock