
class SysMenu
  
  @templ: require('views/sysmenu')

  @render: ->
  	SysMenu.templ()
    
module.exports = SysMenu