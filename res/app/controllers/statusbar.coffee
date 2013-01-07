Spine = require('spine')

class Statusbar extends Spine.Controller
  
  constructor: ->
    super

    @render()

  @tmpl: require('views/statusbar')
  
  render: ->
  	@html Statusbar.tmpl()

module.exports = Statusbar