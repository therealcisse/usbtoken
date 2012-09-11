Token = require('models/token')
Key = require('models/key')
Spine = require('spine')

# All the methods are asynchronous

class Façade extends Spine.Module
  @extend Spine.Log

  @logPrefix: '(Façade)'

  @trace: false

  @data:
    status: Token.AuthRequired
    pin:  '1234' 
    puk:  '1111'
    sopin:  '5678' 
    sopuk:  '2222'
    keys: [
      Key.create(friendlyName: 'Key 1')
      Key.create(friendlyName: 'Key 2')
      Key.create(friendlyName: 'Key 3')
      Key.create(friendlyName: 'Key 4')
      Key.create(friendlyName: 'Key 5')
      Key.create(friendlyName: 'Key 6')
      Key.create(friendlyName: 'Key 7')
      Key.create(friendlyName: 'Key 8')#
      Key.create(friendlyName: 'Key 9')
      Key.create(friendlyName: 'Key 10')
      Key.create(friendlyName: 'Key 11')
      Key.create(friendlyName: 'Key 12')
      Key.create(friendlyName: 'Key 13')
      Key.create(friendlyName: 'Key 14')
      Key.create(friendlyName: 'Key 15')
      Key.create(friendlyName: 'Key 16')#
      Key.create(friendlyName: 'Key 17')
      Key.create(friendlyName: 'Key 18')
      Key.create(friendlyName: 'Key 19')
      Key.create(friendlyName: 'Key 20')
      Key.create(friendlyName: 'Key 21')
      Key.create(friendlyName: 'Key 22')
      Key.create(friendlyName: 'Key 23')
      Key.create(friendlyName: 'Key 24')#
      Key.create(friendlyName: 'Key 25')
      Key.create(friendlyName: 'Key 26')
      Key.create(friendlyName: 'Key 27')
      Key.create(friendlyName: 'Key 28')
      Key.create(friendlyName: 'Key 29')
      Key.create(friendlyName: 'Key 30')
      Key.create(friendlyName: 'Key 31')
      Key.create(friendlyName: 'Key 32')                  
    ] 
    options:

      # User PIN

      'min-pin-length' : 3
      'max-pin-length' : 8

      'min-puk-length' : 3
      'max-puk-length' : 8      

      'pin-attempts' : 3
      'puk-attempts' : 3

      'remaining-pin-attempts' : 3
      'remaining-puk-attempts' : 3

      # SO PIN

      'min-so-pin-length' : 3
      'max-so-pin-length' : 8

      'min-so-puk-length' : 3
      'max-so-puk-length' : 8     

      'so-pin-attempts' : 3
      'so-puk-attempts' : 3

      'remaining-so-pin-attempts' : 3
      'remaining-so-puk-attempts' : 3

      # other options

      'onepin' : false

  # getters

  @getKeys: (fn) ->
    fn @data['keys']

  @getKey: (id, fn) ->
    try 
      fn Key.find(id)
    catch e
      fn undefined, true

  @getStatus: (fn) -> 
    fn Façade['data']['status']

  @getOptions: (fn) ->  
    fn Façade['data']['options']

  # ops

  @login: (pin, fn) ->
    @log "@login:pin-attempts=#{Façade['data']['options']['pin-attempts']}"
    @log "@login:remaining-pin-attempts=#{Façade['data']['options']['remaining-pin-attempts']}"

    Façade['data']['options']['remaining-pin-attempts'] -= 1

    if Façade['data']['options']['remaining-pin-attempts'] is 0
      Façade['data']['status'] = Token.Blocked
      return fn(0)

    if pin is Façade['data']['pin']
      Façade['data']['options']['remaining-pin-attempts'] = Façade['data']['options']['pin-attempts']
      Façade['data']['status'] = Token.LoggedIn
      fn()
    else      
      fn(Façade['data']['options']['remaining-pin-attempts']) # error


  @soLogin: (sopin, fn) ->

  @unblock: (puk, fn) =>
    @log "@unblock:puk-attempts=#{Façade['data']['options']['puk-attempts']}"
    @log "@unblock:remaining-puk-attempts=#{Façade['data']['options']['remaining-puk-attempts']}"

    if Façade['data']['options']['remaining-puk-attempts'] is 0
      Façade['data']['status'] = Token.Locked
      return fn(0)

    Façade['data']['options']['remaining-puk-attempts'] -= 1

    if puk is Façade['data']['puk']
      Façade['data']['options']['remaining-pin-attempts'] = Façade['data']['options']['pin-attempts']
      Façade['data']['options']['remaining-puk-attempts'] = Façade['data']['options']['puk-attempts']
      Façade['data']['status'] = Token.ChangePIN
      fn()
    else      
      fn(Façade['data']['options']['remaining-puk-attempts']) # error

  @soUnblock: (sopuk, fn) ->

  @logout: (fn) -> 
    Façade['data']['status'] = Token.AuthRequired
    fn()

  @resetPIN: (puk, newpin, fn) =>

    if Façade['data']['puk'] is puk 
    
      Façade['data']['pin'] = newpin
      Façade['data']['status'] = Token.LoggedIn
      fn()

    else

      fn(true)

  @soResetPIN: (newsopin, fn) -> 
    Façade['data']['sopin'] = newsopin
    Façade['data']['status'] = Token.LoggedIn
    fn()

  @changePIN: (oldpin, pin, fn) => 
  
    if oldpin is Façade['data']['pin']

      Façade['data']['pin'] = pin
      Façade['data']['status'] = Token.LoggedIn
      fn()

    else

      fn(true)


  @soChangePIN: (sopin, fn) -> 
    @soSetPIN(sopin, fn)    

module.exports = Façade