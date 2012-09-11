Spine = require('spine')

class Key extends Spine.Model
  @configure 'Key',

      # Subject's attributes

      'commonName',
      'email',
      'locality',
      'state',
      'organization',
      'organizationalUnit',
      'country',
      'street',

      # key attributes

      'dn',
      'serial',
      'signatureAlgorithm',
      'beginDate',
      'endDate',
      'keyUsage',
      'friendlyName',
      'basicContraints',
      'engine',

      # Issuer's attributes

      'issuerCommonName',
      'issuerEmail',
      'issuerLocality',
      'issuerState',
      'issuerOrganization',
      'issuerOrganizationalUnit',
      'issuerCountry',
      'issuerStreet'

  @BasicContraints:
    CA : 0x02

  @KeyUsage:
    DigitalSignature : 0x02,
    NonRepudiation   : 0x04,
    KeyEncipherment  : 0x08,
    DataEncipherment : 0x16

Key.KeyUsage.ALL = 
    Key.KeyUsage.DigitalSignature | 
    Key.KeyUsage.NonRepudiation   | 
    Key.KeyUsage.KeyEncipherment  | 
    Key.KeyUsage.DataEncipherment

module.exports = Key