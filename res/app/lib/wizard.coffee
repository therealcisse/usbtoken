Spine = require('spine')

class Wizard extends Spine.Module

  @include Spine.Log
  @include Spine.Events

  logPrefix: '(Wizard)'

  steps: []

  setStep: (step) ->
    @trigger "stepChanged", step
  
  unRendered: ->

  # args(app)
  constructor: (options) ->
    super

    for key, value of options
      @[key] = value    

    @bind 'stepChanged', (step) ->
      @app.become step 
      
      @app.delay (=> @alert(step.alertMsg)) if step.alertMsg
      
      @app.delay (=> @info(step.infoMsg)) if step.infoMsg

  info: -> @app.info arguments...
  alert: -> @app.alert arguments...

  rendered: => 
    @setStep @steps.shift()

  end: (step, params) =>
    @log "end:#{step.name}"

    if next = @steps.shift() 

      # Make variables of current step available to the next step      
      next.args['vars'] or= {}     
      next.args['vars'][key] = val for key, val of params
      
      return @app.delay (=> @setStep(next)), 1000

    @app.delay (=> @app.navigate '#/keys'), 750

  cancelled: (step) =>
    @log "cancelled:#{step.name}"

module.exports = Wizard