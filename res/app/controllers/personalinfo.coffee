Spine = require('spine')

class PersonalInfo extends Spine.Controller

  elements: 
    '[name=fullName]'     :   'fullName'
    '[name=email]'        :   'email'
    '[name=telephone]'    :   'telephone'
    '[name=address]'      :   'address'
    '[name=city]'         :   'city'
    '[name=country]'      :   'country'

  events:
    'submit form'         :   'submit'
    'click form .cancel'  :   'cancel'  

  className: 'personal-info v-scroll'

  @templ: require('views/personal-info')

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    fullName    : cleaned 'fullName'    
    email       : cleaned 'email'    
    telephone   : cleaned 'telephone'    
    address     : cleaned 'address'    
    city        : cleaned 'city'    
    country     : cleaned 'country'  

  @viewopts: -> {}

  # args(controller)
  constructor: ->
    super

    @bind 'release', =>
      delete @controller

  render: ->
    @html PersonalInfo.templ(PersonalInfo.viewopts())

  submit: (e) ->

    e.preventDefault()

    params = @params()       

    if msg = PersonalInfo.valid(params)
      @controller.alert(msg) 
      return false

    @delay => @fn(params)

    false

  cancel: (e) ->

    e.preventDefault()
    e.stopPropagation()

    @controller.cancelled?(@)

    false
  
  # private

  @valid: (params) ->
    # 
    
module.exports = PersonalInfo