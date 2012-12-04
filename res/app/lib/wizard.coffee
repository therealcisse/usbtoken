Spine = require('spine')

class Wizard extends Spine.Module

  @include Spine.Log
  @include Spine.Events

  logPrefix: '(Wizard)'

  steps: []

  setStep: (step) ->
    @app.clearAllMsgs()
    @trigger "stepChanged", step
  
  unRendered: ->

  # args(app)
  constructor: (options) ->
    super

    for key, value of options
      @[key] = value    

    @bind 'stepChanged', (step) ->
      @app.become step 
      
      @app.delay (=> @app.alert(step.alertMsg)) if step.alertMsg
      
      @app.delay (=> @app.info(step.infoMsg)) if step.infoMsg

  info: -> @app.info arguments...
  alert: -> @app.alert arguments...

  rendered: => 
    @setStep @steps.shift()

  next: (step, params) =>
    @log "end:#{step.name}"

    if next = @steps.shift() 

      # Make variables of current step available to the next step      
      next.args['vars'] or= {}     
      next.args['vars'][key] = val for key, val of params
      
      return @app.delay (=> @setStep(next)), 200

    @app.delay (=> @app.navigate '/'), 550

  cancelled: (step) =>
    @log "cancelled:#{step.name}"
    @app.navigate "/" # unless @unRendered()

module.exports = Wizard