Spine = require('spine')

class GetLabel extends Spine.Controller

  events:
    'submit form'         :   'submit'
    'click form .cancel'  :   'cancel'  

  className: 'get-label v-scroll'

  @templ: require('views/get-label')

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    "label"   : cleaned 'label'    
    # email       : cleaned 'email'    
    # telephone   : cleaned 'telephone'    
    # address     : cleaned 'address'    
    # city        : cleaned 'city'    
    # country     : cleaned 'country'  

  elements:
    "[name=label]"    :   "label"
    "[type=submit]"   :   'submitBtn'
    # '[name=email]'        :   'email'
    # '[name=telephone]'    :   'telephone'
    # '[name=address]'      :   'address'
    # '[name=city]'         :   'city'
    # '[name=country]'      :   'country'

  # args(controller, title)
  constructor: (args) ->
    super

    @optional or= false

    @bind 'release', =>
      delete @controller

    @viewopts = {title: @title, header: @header, hasCancel: @controller.hasCancel}

    @fn.err = => @delay (=> @submitBtn.enable(); @label[0].focus()) 

  render: ->
    @html GetLabel.templ(@viewopts)

  submit: (e) ->
    @submitBtn.enable(false)
    e.preventDefault()

    params = @params()       

    if !@optional and msg = GetLabel.valid(@, params)
      @controller.alert(msg: msg, closable: true)
      @fn.err?()
      return false

    @delay => @fn(params)

    false

  cancel: (e) ->

    e.preventDefault()
    e.stopPropagation()

    @controller.cancelled?(@)

    false
  
  # private

  @valid: (self ,params) ->
    return app.$T('x_required').Format(self.title) unless params['label'].length
    
module.exports = GetLabel