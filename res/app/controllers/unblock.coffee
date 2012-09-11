Spine  = require('spine')
App    = require('index')
Façade = require('lib/façade')

class Unblock extends Spine.Controller

  # args(doUnblock, controller)
  constructor: ->
    super

    @bind 'release', => 
      delete @controller
      
  events:
    'submit form' :   'unblock'

  elements:
    '[name=puk]' :   'puk'

  className: 'unblock'

  @templ: require('views/unblock')

  viewopts: ->
    Façade.getOptions (opts) =>
      minLength : opts['min-pin-length']
      maxLength : opts['max-pin-length']

  params: ->
    cleaned = (key) =>
      (@[key].val() or '').trim()

    puk  : cleaned 'puk'    

  render: ->
    @html Unblock.templ(@viewopts())

  unblock: (e) ->
    @log '@unblock'

    e.preventDefault()

    params = @params() 

    if msg = Unblock.valid(params)
      @controller.alert(msg) 
      return false

    @doUnblock(params.puk)
  
  # private

  @valid: (params) ->  
    Façade.getOptions (opts) => 
      return "PUK must be between #{opts['min-pin-length']} and #{opts['max-pin-length']} caracters." unless params.puk.length > opts['min-pin-length'] and params.puk.length < opts['max-pin-length']    

module.exports = Unblock