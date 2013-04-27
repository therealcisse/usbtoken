{
  'variables': {  
  },
  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'variables': {
        },
        'defines': [
          'DEBUG',
          'ENABLE_DISASSEMBLER',
          # 'V8_ENABLE_CHECKS',
          'OBJECT_PRINT',
          'VERIFY_HEAP',
          '_DEBUG', 
          # '_WIN32_WINNT=0x0602', 
          'WINVER=0x0501', 
          'WIN32', 
          '_WINDOWS', 
          'NOMINMAX', 
          'PSAPI_VERSION=1', 
          '_CRT_RAND_S', 
          'CERT_CHAIN_PARA_HAS_EXTRA_FIELDS', 
          'WIN32_LEAN_AND_MEAN', 
          '_ATL_NO_OPENGL', 
          '_HAS_EXCEPTIONS=0', 
          '_SECURE_ATL', 
          'CHROMIUM_BUILD', 
          'TOOLKIT_VIEWS=1', 
          'USE_LIBJPEG_TURBO=1', 
          'ENABLE_ONE_CLICK_SIGNIN', 
          'ENABLE_REMOTING=1', 
          'ENABLE_WEBRTC=1', 
          'ENABLE_PEPPER_THREADING', 
          'ENABLE_CONFIGURATION_POLICY', 
          'ENABLE_INPUT_SPEECH', 
          'ENABLE_NOTIFICATIONS', 
          'ENABLE_GPU=1', 
          'ENABLE_EGLIMAGE=1', 
          'USE_SKIA=1', 
          '__STD_C', 
          '_CRT_SECURE_NO_DEPRECATE', 
          '_SCL_SECURE_NO_DEPRECATE', 
       #   'NTDDI_VERSION=0x06020000', 
          'ENABLE_TASK_MANAGER=1', 
          'ENABLE_WEB_INTENTS=1', 
          'ENABLE_EXTENSIONS=1', 
          'ENABLE_PLUGIN_INSTALLATION=1', 
          'ENABLE_PLUGINS=1', 
          'ENABLE_SESSION_SERVICE=1', 
          'ENABLE_THEMES=1', 
          'ENABLE_BACKGROUND=1', 
          'ENABLE_AUTOMATION=1', 
          'ENABLE_GOOGLE_NOW=1', 
          'ENABLE_LANGUAGE_DETECTION=1', 
          'ENABLE_PRINTING=1', 
          'ENABLE_CAPTIVE_PORTAL_DETECTION=1', 
          'ENABLE_APP_LIST=1', 
          'ENABLE_SETTINGS_APP=1', 
          'USING_CEF_SHARED', 
          '__STDC_CONSTANT_MACROS', 
          '__STDC_FORMAT_MACROS', 
          'DYNAMIC_ANNOTATIONS_ENABLED=1', 
          'WTF_USE_DYNAMIC_ANNOTATIONS=1',           
        ],
        'conditions': [
          ['OS=="win"', {        
            'msvs_settings': {
              'VCCLCompilerTool': {
                'Optimization': '0',
                "AdditionalOptions": [
                  "/MP /we4389"
                ],
                'conditions': [
                  ['OS=="win"', 
                  # {
                  #   'RuntimeLibrary': '3',  # /MDd
                  # },
                  {
                    'RuntimeLibrary': '1',  # /MTd
                  },
                  ],
                ],
              },
              'VCLinkerTool': {
                'LinkIncremental': '2',                
                "AdditionalOptions": [
                  "/safeseh /dynamicbase /ignore:4199 /ignore:4221 /nxcompat"
                ],                
              },
            },
          }], # OS=="win"
        ] # conditions
      },  # Debug
      'Release': {
        'variables': {
        },
        'defines': [
          # '_WIN32_WINNT=0x0602', 
          'WINVER=0x0501', 
          'WIN32', 
          '_WINDOWS', 
          'NOMINMAX', 
          'PSAPI_VERSION=1', 
          '_CRT_RAND_S', 
          'CERT_CHAIN_PARA_HAS_EXTRA_FIELDS', 
          'WIN32_LEAN_AND_MEAN', 
          '_ATL_NO_OPENGL', 
          '_HAS_EXCEPTIONS=0', 
          '_SECURE_ATL', 
          'CHROMIUM_BUILD', 
          'TOOLKIT_VIEWS=1', 
          'USE_LIBJPEG_TURBO=1', 
          'ENABLE_ONE_CLICK_SIGNIN', 
          'ENABLE_REMOTING=1', 
          'ENABLE_WEBRTC=1', 
          'ENABLE_PEPPER_THREADING', 
          'ENABLE_CONFIGURATION_POLICY', 
          'ENABLE_INPUT_SPEECH', 
          'ENABLE_NOTIFICATIONS', 
          'ENABLE_GPU=1', 
          'ENABLE_EGLIMAGE=1', 
          'USE_SKIA=1', 
          '__STD_C', 
          '_CRT_SECURE_NO_DEPRECATE', 
          '_SCL_SECURE_NO_DEPRECATE', 
        #  'NTDDI_VERSION=0x06020000', 
          'ENABLE_TASK_MANAGER=1', 
          'ENABLE_WEB_INTENTS=1', 
          'ENABLE_EXTENSIONS=1', 
          'ENABLE_PLUGIN_INSTALLATION=1', 
          'ENABLE_PLUGINS=1', 
          'ENABLE_SESSION_SERVICE=1', 
          'ENABLE_THEMES=1', 
          'ENABLE_BACKGROUND=1', 
          'ENABLE_AUTOMATION=1', 
          'ENABLE_GOOGLE_NOW=1', 
          'ENABLE_LANGUAGE_DETECTION=1', 
          'ENABLE_PRINTING=1', 
          'ENABLE_CAPTIVE_PORTAL_DETECTION=1', 
          'ENABLE_APP_LIST=1', 
          'ENABLE_SETTINGS_APP=1', 
          'USING_CEF_SHARED', 
          '__STDC_CONSTANT_MACROS', 
          '__STDC_FORMAT_MACROS', 
          'NDEBUG', 
          'NVALGRIND', 
          'DYNAMIC_ANNOTATIONS_ENABLED=0',        
        ],
        'conditions': [
          ['OS=="win"', {
            'msvs_settings': {
              'VCCLCompilerTool': {
                'Optimization': '2',
                'InlineFunctionExpansion': '2',
                'EnableIntrinsicFunctions': 'true',
                'FavorSizeOrSpeed': '0',
                'StringPooling': 'true',
                "AdditionalOptions": [
                  "/MP /we4389 /Oy-"
                ],                
                'conditions': [
                  ['OS=="win"', 
                  # {
                  #   'RuntimeLibrary': '2',  #/MD
                  # }, 
                  {
                    'RuntimeLibrary': '0',  #/MT
                  },
                  ],
                ],
              },
              'VCLinkerTool': {
                'LinkIncremental': '1',
                'OptimizeReferences': '2',
                'EnableCOMDATFolding': '2',
                "AdditionalOptions": [
                  "/safeseh /dynamicbase /ignore:4199 /ignore:4221 /nxcompat"
                ],                                
              },
            },
          }],  # OS=="win"
        ],  # conditions
      },  # Release
    },  # configurations

    'msvs_settings': {
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
        'TargetMachine': '1',
      },
    },

  }, # target_defaults  
}