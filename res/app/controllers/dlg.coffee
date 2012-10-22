Spine = require('spine')

class Dlg extends Spine.Controller

  className: 'dlg'
  
  constructor: (args) ->

    for btn in args.buttons
      (@events ||= {})["click ##{btn.id}"] = btn.fn # (evt) -> evt.preventDefault(); evt.stopPropagation(); f(evt); false

    super

  @tmpl: require('views/dlg')

  render: ->
    @html Dlg.tmpl(@)
    @

    
module.exports = Dlg