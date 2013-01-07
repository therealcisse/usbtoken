Spine = require('spine')

Token    = require('models/token')
Façade   = require('lib/façade')
Wizard   = require('lib/wizard')
GetLabel = require('controllers/get-label')

emailRegex = /^[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,6}$/i

class KeyMgr

class KeyMgr.X509View extends Spine.Controller

  logPrefix: '(X509View)'

  class @Toolsbar extends Spine.Controller

    logPrefix: '(X509View.Toolsbar)'

    className: 'toolsbar toolsbar-key'
    
    events: 
      'click .purge'     : 'purgeKey'
      'click .export'    : 'exportX509'

    # args(app)
    constructor: ->
      super

      @bind 'release', ->
        delete @controller

    @templ: require("views/key-mgr/toolsbar.X509")

    render: =>
      @html X509View.Toolsbar.templ()

    # CRUD

    purgeKey: (evt) ->
      @log "purgeKey:@delete:#{@id}"
      df = app.Loading()

      evt.stopPropagation();
      evt.preventDefault();      

      Façade.DelX509 @id, (ok) =>
        df()

        if not ok
          @controller.app.alert(msg: app.$T('err_while_x_x').Format(app.$T('deleting'), app.$T('x509')), closable: true)
          return false            

        @controller.app.info(msg: app.$T('success_while_x_x').Format('X509.Certificate',  app.$T('deleted')), closable: true)
        @controller.app.delay (-> @navigate("#/keys")), 100
        return false

    doExportX509: (params) =>
      @log "X509Certificate:@doExportX509:#{@controller._key.id}"
      df = app.Loading()

      Façade.ExportX509 @id, params.fileName, params.format, (ok) =>
        df()

        if not ok
          @controller.app.alert(msg: app.$T('err_while_x_x').Format(app.$T('exporting'), app.$T('x509')), closable: true)
          @doExportX509.err()
          return false            

        @controller.app.info(msg: app.$T('success_while_x_x').Format(app.$T('x509'),  app.$T('exported')), closable: true)
        @doExportX509.close()
        return false        

    exportX509: (evt) ->
      @log "X509Certificate:@exportX509:#{@id}"      

      evt.stopPropagation();
      evt.preventDefault();      

      # Get Name & Format through a dlg
      window.jQuery((new KeyMgr.KeyList.X509Certificate.ExportForm({key: @controller._key, controller: @controller.app, close: ((f) -> f.el.closest('.dlg').modal('hide')), fn: @doExportX509})).render().el).modal({})
      false       

  className: 'key-view v-scroll'

  # @PubKeyTempl: require('views/key-mgr/pubkey')
  # @PrKeyTempl: require('views/key-mgr/prkey')
  @tmpl: require('views/key-mgr/x509-certificate')

  render: =>
    @key.html '<div class="spinner" style="margin: 3.5em 2em;"></div>'
    @

  # get and render key
  rendered: =>
    @log "X509View@rendered:#{@id}"
    df = app.Loading()

    Façade["Get#{@type}"] @id, (key, err) =>
      df()

      @_key = key

      if err
        # show error
        return @

      @log "Found key: #{key.id}"

      @key.html X509View.tmpl(key)
      @ # necessary or toolsbar will be lost

  # args(app)
  constructor: ->
    super

    # create views and bind event handlers
    @toolsbar = new X509View.Toolsbar(id: @id, controller: @)
    @key      = new Spine.Controller(className: 'v-scroll key')

    @bind 'release', ->

      @toolsbar.release()
      @key.release()
      
      delete @app      
      
    @append @toolsbar.render(), @key 
    @delay -> Façade.SetWindowText(app.$T('title_keys'))   

# X509View >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 
class KeyMgr.KeyList extends Spine.Controller

  logPrefix: '(KeyList)'

  selectedKeys: []

  getSelectedKeys: => @selectedKeys

  # args(app)
  constructor: ->
    super

    @toolsbar = new KeyList.Toolsbar(app: @app, keyList: @)
    @keys     = new Spine.Controller(className: 'keys')

    @app.bind 'selectionChanged', @selectionChanged

    @bind 'release', ->
      @app.unbind 'selectionChanged', @selectionChanged

      @toolsbar.release()
      @keys.release()
      
      delete @app      
      
    @append @toolsbar.render(), @keys
    @delay -> Façade.SetWindowText(app.$T('title_keys'))

  selectionChanged: (key, hasSelection=true) =>
    @log 'KeyList.Toolsbar#selectionChanged'

    (if hasSelection then @selectedKeys.push(key) else @selectedKeys.splice(@selectedKeys.indexOf(key), 1)) if key
    @el.toggleClass('has-selection', @selectedKeys.length>0)  

  className: 'key-list v-scroll'

  addKeys: (keys) =>
    @keys.append((new KeyMgr.KeyList[key.type](key: key, app: @app, keyList: @)).render()) for key in keys
    
  # get and render keys
  render: =>
    @log 'KeyList@rendered'
    @keys.html '<div class="spinner" style="margin: 3.5em 2em;"></div>'
    @

  rendered: =>
    @selectedKeys = []
    @selectionChanged()

    df = app.Loading()

    Façade.GetCerts (x509s) =>
      @$('.spinner').remove() if x509s.length
      
      @addKeys x509s
      Façade.GetPrKeys (prkeys) => 
        @$('.spinner').remove() if prkeys.length
      
        @addKeys prkeys            

        # @delay (->
        #   Façade.GetPubKeys (keys) =>
        #     @addKeys keys
        #     df()
        # ), 100

        df()

        if prkeys.length is 0 and @$('.spinner').length
          @keys.html "<div style='margin: 3.5em 2em;'>#{app.$T('msg_no_keys')} :-(</div>"

  class @Key extends Spine.Controller

    events:
      'click'                     : 'toggle'
      'click .action-export'      : 'export'
      'click .action-view'        : 'view'    

    tag: 'li'

    toggle: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @trigger("selectionChanged", @key, (@checked = not @checked; @checked))

      false

    export: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @log "export:#{@key.id}"

      false

    view: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @navigate("#/#{@key.type}/#{@key.id}")

      false                

    # args(key, app, keyList)
    constructor: (args) ->
      @attributes = id: "key-#{args['key'].id}"
      super

      @bind 'selectionChanged', (key, checked) => 
        @el.toggleClass('checked', checked)
        if checked then @app.selectKey(key) else @app.unSelectKey(key)

      @bind 'release', =>
        delete @app
        delete @keyList

  class @PrKey extends @Key

    events:
      'click'                     : 'toggle'
      'click .action-delete'      : 'purgePrKey'
      'click .action-gen-csr'     : 'genCSR'

    className: 'key prkey entry'    

    constructor: ->
      super

    @templ: require('views/key-mgr/_prkey')

    render: =>
      @html @constructor.templ(@key)

    purgePrKey: (evt) ->
      @log "PrKey:@delete:#{@key.id}"
      df = app.Loading()

      evt.stopPropagation();
      evt.preventDefault();

      Façade.DelPrKey @key.id, (ok) =>
        df()

        if not ok
          @app.alert(msg: app.$T('err_while_x_x').Format(app.$T('deleting'),  app.$T('prkey')), closable: true)
          return false      

        @keyList.selectionChanged(@key, false)
        @el.remove()
        @app.info(msg: app.$T('success_while_x_x').Format(app.$T('prkey'),  app.$T('deleted')), closable: true)
        return false

    genCSR: (evt) ->

      evt.stopPropagation();
      evt.preventDefault();

      @log "PrKey.genCSR"
      @navigate "#/gen-csr/#{@key.id}"  

      false    
    
  class @X509Certificate extends @Key

    events:
      'click'                     : 'toggle'
      'click .action-delete'      : 'purgeX509'
      'click .action-export'      : 'exportX509'
      'click .action-view'        : 'view'   

    className: 'key x509-certificate entry'    

    constructor: ->
      super    

    @templ: require('views/key-mgr/_x509-certificate')

    render: =>
      @html @constructor.templ(@key)

    doExportX509: (params) =>
      @log "X509Certificate:@doExportX509:#{@key.id}"
      df = app.Loading()

      Façade.ExportX509 @key.id, params.fileName, params.format, (ok) =>
        df()

        if not ok
          @app.alert(msg: app.$T('err_while_x_x').Format(app.$T('exporting'), app.$('x509')), closable: true)
          @doExportX509.err()
          return false            

        @app.info(msg: app.$T('success_while_x_x').Format(app.$('x509'),  app.$T('exported')), closable: true)
        @doExportX509.close()
        return false        

    exportX509: (evt) ->
      @log "X509Certificate:@exportX509:#{@key.id}"      

      evt.stopPropagation();
      evt.preventDefault();      

      # Get Name & Format through a dlg
      window.jQuery((new @constructor.ExportForm({key: @key, controller: @app, close: ((f) -> f.el.closest('.dlg').modal('hide')), fn: @doExportX509})).render().el).modal({})
      false   

    class @ExportForm extends Spine.Controller
      className: 'export-x509 dlg fade'

      @templ: require('views/key-mgr/export_x509')

      @events:
        'submit form'         :   'submit'
        'click form .cancel'  :   'cancel'      

      @elements:
        "[name=fileName]"    :   "fileName"
        "[name=format]"     :   "format"
        "[type=submit]"   :   'submitBtn'

      # args(controller, cancelled, id, fn)
      constructor: ->
        super

        @bind 'release', =>
          delete @controller

        @viewopts = {fileName: @constructor.GetFileName(@key.label)}

        @fn.err = => @delay (=> @submitBtn.enable(); @fileName[0].focus())     
        @fn.close = => @close(@)

      render: ->
        @html @constructor.templ(@viewopts)
        @

      params: ->

        cleaned = (key) =>
          (@[key].val() or '').trim()

        "fileName" : cleaned 'fileName'    
        "format"   : cleaned 'format'    

      submit: (e) ->
        @submitBtn.enable(false)
        e.preventDefault()

        params = @params()       

        if msg = @constructor.valid(@, params)
          @controller.alert(msg: msg, closable: true)
          @fn.err?()
          return false

        @delay -> @fn(params)

        false

      cancel: (e) ->

        e.preventDefault()
        e.stopPropagation()

        @close(@)

        false
      
      # private

      @valid: (self, params) ->
        return app.$T('x_required').Format(app.$T('label_file_name')) unless params['fileName'].length
        
      @GetFileName: (label) ->
        return label if label.indexOf('/') is -1
        return label

    purgeX509: (evt) ->
      @log "X509Certificate:@delete:#{@key.id}"
      df = app.Loading()

      evt.stopPropagation();
      evt.preventDefault();      

      Façade.DelX509 @key.id, (ok) =>
        df()

        if not ok
          @app.alert(msg: app.$T('err_while_x_x').Format(app.$T('deleting'),  app.$T('x509')), closable: true)
          return false            

        @keyList.selectionChanged(@key, false)
        @el.remove()
        @app.info(msg: app.$T('success_while_x_x').Format(app.$T('x509'),  app.$T('deleted')), closable: true)
        return false

      false

  # Un attached
  class @Toolsbar extends Spine.Controller

    logPrefix: '(KeyList.Toolsbar)'

    className: 'toolsbar toolsbar-keys'

    events:
      'click .reload'       : 'reloadKeys'
      'click .purge'        : 'purgeSelection'
      # 'click .export'       : 'exportSelection'
    
    # args(app)
    constructor: ->
      super
      
      @bind 'release', ->
        delete @app
        delete @keyList

    @templ: require('views/key-mgr/toolsbar.keys')

    # CRUD      

    purgeSelection: (evt) ->
      @log "@purgeSelection:delete keys"
      df = app.Loading()

      evt.preventDefault()
      evt.stopPropagation()  

      keys = @keyList.getSelectedKeys()  

      done = => @app.info(msg: app.$T('success_while_x_x').Format(app.$T('selected_keys'),  app.$T('deleted')), closable: true, duration: 300)

      removeX509 = (key) => 
        ret = $.Deferred()

        Façade.DelX509 key.id, (ok) =>

          if ok

            @keyList.selectionChanged(key, false)
            jQuery("#key-#{key.id}").remove()

            # @app.info(msg: app.$T('success_while_x_x').Format("{0} <b>#{key.label}</b>".Format(app.$T('x509')),  app.$T('deleted')), closable: true, duration: 300)

            # @delay (-> removeKeys()), 100
            ret.resolve()

          else

            @app.alert(msg: app.$T('err_while_x_x').Format(app.$T("deleting"), "{0} <b>#{key.label}</b>".Format(app.$T('x509'))), closable: true)
            df()

        ret

      removePrKey = (key) => 
        ret = $.Deferred()

        Façade.DelPrKey key.id, (ok) =>

          if ok

            @keyList.selectionChanged(key, false)
            jQuery("#key-#{key.id}").remove()

            # @app.info(msg: app.$T('success_while_x_x').Format("{0} <b>#{key.label}</b>".Format(app.$T('prkey')),  app.$T('deleted')), closable: true, duration: 300)

            # @delay (-> removeKeys()), 100
            ret.resolve()

          else

            @app.alert(msg: app.$T('err_while_x_x').Format(app.$T("deleting"), "<b>#{key.label}</b>"), closable: true)
            df()

        ret

      removeKeys = =>
        return (df(); done(); (@reloadKeys.apply @, [evt] unless @keyList.keys.$('.key').length)) unless key = keys[keys.length - 1]
      
        switch key.type
          
          when 'X509Certificate' 
            
            removeX509(key).done removeKeys

          
          when 'PrKey'
            
            removePrKey(key).done removeKeys

      removeKeys()            
      false

    # exportSelection: (evt) ->
    #   evt.preventDefault()
    #   evt.stopPropagation()

    #   @log "export keys"

    #   false

    reloadKeys: (evt) ->
      evt.preventDefault()
      evt.stopPropagation()

      @keyList.render()
      @keyList.rendered()

      false

    render: =>
      @html Toolsbar.templ()

# KeyList >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class KeyMgr.GenForm extends Wizard

  hasCancel: true 
  
  doGenKey: (params) ->
    @log "doGenKey: #{params}"
    df = app.Loading()

    Façade.GenKey params.label, (ok, keyid) =>
      console.log "doGenKey#{ok}"

      ### Hide loading indicator ###
      df()

      if ok
        # show dlg asking if user wants to generate a CSR

        # hideDlg = (el) -> window.jQuery(el or '.modal').closest('.dlg').modal('hide')
        goBack = => @delay (=> @navigate "/")

        @controller.info msg: app.$T('prkey_gen_success'), closable: true
        goBack()
        # _yes = no

        # @controller.app.dlg({
        #   msg: app.$T('prkey_gen_success')
        #   hidden: -> goBack() if not _yes
        #   buttons: [
        #     {
        #       id: 'dlg-yes'
        #       title: 'Yes'
        #       primary: true              
        #       fn: (evt) => 

        #         _yes = yes

        #         hideDlg(evt.target)

        #         @controller.steps.unshift({
        #           Clss: KeyMgr.GenForm.GetX509ReqInfo
        #           args:
        #             id: keyid
        #             className: 'get-x509-req-info v-scroll gen-key'
        #             controller: @controller
        #             fn: @controller.app.doGenX509Req
        #         })

        #         @controller.next @, {}
        #     },
        #     {
        #       id: 'dlg-no'
        #       title: 'No'
        #       fn: (evt) -> hideDlg(evt.target); # goBack()
        #     }
        #   ]
        #   })

        return false
      
      @controller.alert msg: app.$T('msg_err'), closable: true
      @controller.doGenKey.err?()

  class @GetX509ReqInfo extends Spine.Controller

    className: 'get-x509-req-info v-scroll'

    @templ: require('views/key-mgr/get-x509-req-info')

    @events:
      'submit form'         :   'submit'
      'click form .cancel'  :   'cancel'      

    @elements:
      "[name=cn]"            :   "cn"
      "[name=o]"             :   "o"
      "[name=ou]"            :   "ou"
      "[name=city]"          :   "city"
      "[name=region]"        :   "region"
      "[name=country]"       :   "country"
      "[name=emailAddress]"  :   "emailAddress"
      "[type=submit]"        :   'submitBtn'

    # args(controller, id)
    constructor: ->
      super

      @bind 'release', =>
        delete @controller

      @fn.err = => @delay (=> @submitBtn.enable(); @cn[0].focus()) 

    @viewopts: -> 
      countries: window.countries 

    render: ->
      @html GetX509ReqInfo.templ(GetX509ReqInfo.viewopts())

    params: ->

      cleaned = (key) =>
        (@[key].val() or '').trim()

      "cn"             : cleaned 'cn'    
      "o"              : cleaned 'o'    
      "ou"             : cleaned 'ou'    
      "city"           : cleaned 'city'    
      "region"         : cleaned 'region'    
      "country"        : cleaned 'country'    
      "emailAddress"   : cleaned 'emailAddress'    

    submit: (e) ->
      @submitBtn.enable(false)
      e.preventDefault()

      params = @params()       

      if msg = GetX509ReqInfo.valid(@, params)
        @controller.alert(msg: msg, closable: true)
        @fn.err?()
        return false

      params.id = @id

      @delay -> @fn(params)
      false

    cancel: (e) ->

      e.preventDefault()
      e.stopPropagation()

      @controller.cancelled?(@)

      false
    
    # private

    @valid: (self, params) ->

      ValidateEmail = (emailAddress) -> emailRegex.test(emailAddress)

      return app.$T('x_required').Format(app.$T('label_full_name')) unless params['cn'].length    
      return app.$T('x_required').Format(app.$T('label_email_address')) unless params['emailAddress'].length  
      return app.$T('x_required').Format(app.$T('label_email_address')) unless ValidateEmail(params['emailAddress']) 

  # args(app)
  constructor: ->
    super

    @constructor.HEADER = app.$T('msg_gen_prkey')

    @steps = [      

      {
        Clss: GetLabel
        args:
          name: 'get-info'
          title: app.$T('label_key_name')
          header: KeyMgr.GenForm.HEADER
          className: 'get-label gen-key'
          controller: @
          fn: @doGenKey
      }

    ]

    # @app.delay -> Façade.SetWindowText 'Generate Private Key'

# GenForm >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class KeyMgr.GetKeyInfo extends Spine.Controller

  className: 'get-key-info'

  @templ: require('views/key-mgr/get-key-info')

  @events:
    'submit form'         :   'submit'
    'click form .cancel'  :   'cancel'      

  @elements:
    "[name=label]"    :   "label"
    "[name=path]"     :   "path"
    "[name=passphrase]"     :   "passphrase"
    "[type=submit]"   :   'submitBtn'

  # args(controller, title)
  constructor: ->
    super

    @bind 'release', =>
      delete @controller

    @viewopts = {title: @title, header: @header, hasPassphrase: @hasPassphrase}

    @fn.err = => @delay (=> @submitBtn.enable(); @label[0].focus())     

  render: ->
    @html KeyMgr.GetKeyInfo.templ(@viewopts)

    if @hasPassphrase
      @el.addClass('has-passphrase')

    @    

  params: ->

    cleaned = (key) =>
      (@[key].val() or '').trim()

    "label"        : cleaned 'label'    
    "path"         : cleaned 'path'    
    "passphrase"   : cleaned 'passphrase'    

  submit: (e) ->
    @submitBtn.enable(false)
    e.preventDefault()

    params = @params()       

    if msg = KeyMgr.GetKeyInfo.valid(@, params)
      @controller.alert(msg: msg, closable: true)
      @fn.err?()
      return false

    params['format'] = KeyMgr.GetKeyInfo.GetFormat(KeyMgr.GetKeyInfo.GetExt params['path'])

    self = @
    f = @path[0].files[0]

    reader = new FileReader
    reader.onload = (evt) ->
      params['data'] = evt.target.result.split(',')[1]
      params['data_len'] = f.size
      
      self.delay -> self.fn(params)

    reader.readAsDataURL f

    false

  cancel: (e) ->

    e.preventDefault()
    e.stopPropagation()

    @controller.cancelled?(@)

    false
  
  # private

  @valid: (self, params) ->
    return app.$T('x_required').Format(self.title) unless params['label'].length    
    return app.$T('x_required').Format(app.$T('label_file_name')) unless params['path'].length  
    
    # if self.hasPassphrase 
      # return "Passphrase is required." if KeyMgr.GetKeyInfo.GetExt(params['path']) in [ 'p12', 'pfx' ] and not params['path'].length

  @GetExt: (path) ->
    (path.substring path.lastIndexOf('.') + 1).toLowerCase()

  @GetFormat: (ext) ->
    formats =
      'p12' :  'PKCS12'
      'pfx' :  'PKCS12'
      'der' :  'DER'
      'cer' :  'DER'
      'crt' :  'PEM'
      'pem' :  'PEM'

    formats[ext] or 'PEM'

class KeyMgr.ImportPrKey extends Wizard

  doImportPrKey: (params) ->
    @controller.log "doImportPrKey:#{params}"
    df = app.Loading()

    Façade.ImportPrKey params.data, params.data_len, params.label, params.format, params.passphrase, (ok) =>
      console.log "#{ok}"

      ### Hide loading indicator ###
      df()

      if ok
        @controller.info msg: app.$T('x_import_success').Format(app.$T('prkey')), closable: true
        @delay (=> @navigate "/"), 700
        return false
      
      @controller.alert msg: app.$T('msg_err'), closable: true
      @controller.doImportPrKey.err?()    

  constructor: ->
    super

    @constructor.HEADER = app.$T('msg_import_x').Format(app.$T('prkey'))

    @steps = [      

      {
        Clss: KeyMgr.GetKeyInfo
        args:
          name: 'get-key-info'
          controller: @
          header: KeyMgr.ImportPrKey.HEADER
          title: "Key Name"
          className: 'v-scroll import-key prkey'
          hasPassphrase: true
          fn: @doImportPrKey
      }

    ]

class KeyMgr.ImportX509 extends Wizard

  doImportX509: (params) ->
    @controller.log "doImportX509:#{params}"
    df = app.Loading()

    Façade.ImportX509 params.data, params.data_len, params.label, params.format, (ok) =>
      console.log "#{ok}"

      ### Hide loading indicator ###
      df()

      if ok
        @controller.info msg: app.$T('success_while_x_x').Format(app.$T('x509'), app.$T('added')), closable: true
        @delay (=> @navigate "/"), 700
        return false
      
      @controller.alert msg: app.$T('msg_err'), closable: true
      @controller.doImportX509.err?()    

  constructor: ->
    super

    @constructor.HEADER = app.$T('msg_import_x').Format(app.$T('x509'))

    @steps = [      

      {
        Clss: KeyMgr.GetKeyInfo
        args:
          name: 'get-key-info'
          controller: @
          header: KeyMgr.ImportX509.HEADER
          title: app.$T('label_x509_name')
          className: 'import-key x509_certificate'
          fn: @doImportX509
      }

    ]

# ImportForm >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

module.exports = KeyMgr