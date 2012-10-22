Spine = require('spine')
Token = require('models/token')
Façade = require('lib/façade')

class KeyMgr

class KeyMgr.KeyView extends Spine.Controller

  logPrefix: '(KeyView)'

  class @Toolsbar extends Spine.Controller

    logPrefix: '(KeyView.Toolsbar)'

    className: 'toolsbar toolsbar-key'
    
    events: 
      'click .purge'     : 'purge'
      'click .export'    : 'export'

    # args(app)
    constructor: ->
      super

      @bind 'release', ->
        delete @app

    @templ: require('views/key-mgr/toolsbar.key')

    render: =>
      @html Toolsbar.templ()

    # CRUD

    purge: (evt) ->
      @log('purge')
      evt.preventDefault();

      false

    export: (evt) ->
      @log('export')
      evt.preventDefault();

      false

  className: 'key-view v-scroll'

  @templ: require('views/key-mgr/key')

  # get and render key
  render: =>
    @log "KeyView@rendered:#{@id}"

    Façade.getKey @id, (key, err) =>

      if err
        # show error
        return @

      @log "Found key: #{key.id}"

      @key.html KeyView.templ(key)
      @ # necessary or toolsbar will be lost

  # args(app)
  constructor: ->
    super

    # create views and bind event handlers
    @toolsbar = new KeyView.Toolsbar(app: @app)
    @key      = new Spine.Controller(className: 'key')

    @bind 'release', ->

      @toolsbar.release()
      @key.release()
      
      delete @app      
      
    @append @toolsbar.render(), @key 
    @delay -> Façade.SetWindowText('Keys')   

# KeyView >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 
class KeyMgr.KeyList extends Spine.Controller

  logPrefix: '(KeyList)'

  selectKeys: []

  # args(app)
  constructor: ->
    super

    @toolsbar = new KeyList.Toolsbar(app: @app)
    @keys     = new Spine.Controller(className: 'keys')

    @app.bind 'selectionChanged', @selectionChanged

    @bind 'release', ->
      @app.unbind 'selectionChanged', @selectionChanged

      @toolsbar.release()
      @keys.release()
      
      delete @app      
      
    @append @toolsbar.render(), @keys
    @delay -> Façade.SetWindowText('Keys')

  selectionChanged: (key, hasSelection) =>
    @log 'KeyList.Toolsbar#selectionChanged'

    if hasSelection then @selectKeys.push(key) else @selectKeys.splice(@selectKeys.indexOf(key), 1)
    @el.toggleClass('has-selection', @selectKeys.length>0)  

  className: 'key-list v-scroll'
    
  # get and render keys
  render: =>
    @log 'KeyList@rendered'

    Façade.getKeys (keys) =>
      @keys.append new KeyList.Key(key: key, app: @app).render() for key in keys
      @ # necessary or toolsbar will be lost

  class @Key extends Spine.Controller

    events:
      'click'                     : 'toggle'
      'click .action-delete'      : 'purge'
      'click .action-export'      : 'export'
      'click .action-view'        : 'view'

    className: 'key entry'

    tag: 'li'

    toggle: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @trigger("selectionChanged", @key, (@checked = not @checked; @checked))

      false

    purge: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @log "delete:#{@key.id}"

      false

    export: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @log "export:#{@key.id}"

      false

    view: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @navigate("#/key/#{@key.id}")

      false                

    # args(key, app)
    constructor: (args) ->
      @attributes = id: "key-#{args['key'].id}"
      super

      @bind 'selectionChanged', (key, checked) => 
        @el.toggleClass('checked', checked)
        if checked then @app.selectKey(key) else @app.unSelectKey(key)

      @bind 'release', =>
        delete @app

    @templ: require('views/key-mgr/_key')

    render: ->
      @html Key.templ(@key)

  # Un attached
  class @Toolsbar extends Spine.Controller

    logPrefix: '(KeyList.Toolsbar)'

    className: 'toolsbar toolsbar-keys'

    events:
      'click .reload'       : 'reload'
      'click .purge'        : 'purge'
      'click .export'       : 'export'
    
    # args(app)
    constructor: ->
      super
      
      @bind 'release', ->
        delete @app

    @templ: require('views/key-mgr/toolsbar.keys')

    # CRUD      

    purge: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @log "delete keys"

      false

    export: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @log "export keys"

      false

    reload: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @log "reload"

      false

    render: =>
      @html Toolsbar.templ()

# KeyList >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class KeyMgr.GenForm extends Spine.Controller

  className: 'gen-form'

  render: -> 
    @

# GenForm >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class KeyMgr.ImportForm extends Spine.Controller

  className: 'import-form'

  render: -> 
    @

# ImportForm >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

module.exports = KeyMgr