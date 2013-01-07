Spine = require('spine')

class Loading extends Spine.Controller
  
  constructor: ->
    super

    @render()

  render: ->
  	@html app.$T('msg_loading')
  

module.exports = Loading