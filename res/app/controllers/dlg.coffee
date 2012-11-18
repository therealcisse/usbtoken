Spine = require('spine')

class Dlg extends Spine.Controller

  className: 'dlg fade'
  
  constructor: (args) ->

    ((@events ||= {})["click ##{args.buttons[0].id}"] = (evt) -> evt.preventDefault(); evt.stopPropagation(); args.buttons[0].fn(evt); false) if args.buttons[0]
    ((@events ||= {})["click ##{args.buttons[1].id}"] = (evt) -> evt.preventDefault(); evt.stopPropagation(); args.buttons[1].fn(evt); false) if args.buttons[1]
    
    ((@events ||= {})["click ##{args.buttons[2].id}"] = (evt) -> evt.preventDefault(); evt.stopPropagation(); args.buttons[2].fn(evt); false) if args.buttons[2]

    super

  @tmpl: require('views/dlg')

  render: ->
    @html Dlg.tmpl(@)
    @

    
module.exports = Dlg