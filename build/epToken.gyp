# Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
    'conditions': [
      [ 'OS=="mac"', {
        # Don't use clang with CEF binary releases due to Chromium tree structure dependency.
        'clang': 0,
      }]
    ]
  },
  'includes': [
    # Bring in the source file lists for epToken.
    'cef_paths2.gypi',
    'common.gypi',
  ],

  'targets': [
    {
      'target_name': 'epToken',
      'type': 'executable',
      'mac_bundle': 1,
      'msvs_guid': 'E34AEE20-7794-11E2-BCFD-0800200C9A66',
      'dependencies': [
        'libcef_dll_wrapper',
      ],
      'defines': [
        'USING_CEF_SHARED',
      ],
      'include_dirs': [
        '.',
        'OpenSC/include',
        'openssl/include',
        'Libp11/include',
        'engine_pkcs11/include',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_wrapper)',
        '<@(epToken_sources_common)',
      ],
      'mac_bundle_resources': [
        '<@(epToken_bundle_resources_mac)',
      ],
      'mac_bundle_resources!': [
        # TODO(mark): Come up with a fancier way to do this (mac_info_plist?)
        # that automatically sets the correct INFOPLIST_FILE setting and adds
        # the file to a source group.
        'epToken/mac/Info.plist',
      ],
      'xcode_settings': {
        'INFOPLIST_FILE': 'epToken/mac/Info.plist',
        # Target build path.
        'SYMROOT': 'xcodebuild',
      },
      'conditions': [
        ['OS=="win"', {
          'defines': [
            'WIN32',
          ],        
          'msvs_settings': {
            'VCLinkerTool': {
              # Set /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
              'EntryPointSymbol' : 'wWinMainCRTStartup',
            },
          },
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
              '-lshlwapi.lib',
              '-lrpcrt4.lib',
              '-lopengl32.lib',
              '-lglu32.lib',
              '-ladvapi32.lib',
              '-lgdi32.lib',
              '-luser32.lib',
              '-lcrypt32.lib',
              '-lgdi32.lib',
              '-lwinmm.lib',
              '-llib/$(ConfigurationName)/libcef.lib',
              '<@(libs_openssl)',
              '<@(libs_opensc)',
              '<@(libs_p11)',
            ],
          },
          'sources': [
            '<@(includes_win)',
            '<@(epToken_sources_win)',
            # '<@(includes_opensc)',
            # '<@(includes_openssl)',
            # '<@(includes_p11)',
          ],
        }], # OS=="win"
        [ 'OS=="mac"', {
          'product_name': 'epToken',
          'dependencies': [
            'epToken_helper_app',
          ],
          'copies': [
            {
              # Add library dependencies to the bundle.
              'destination': '<(PRODUCT_DIR)/epToken.app/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/',
              'files': [
                '$(CONFIGURATION)/libcef.dylib',
                '$(CONFIGURATION)/ffmpegsumo.so',
              ],
            },
            {
              # Add other resources to the bundle.
              'destination': '<(PRODUCT_DIR)/epToken.app/Contents/Frameworks/Chromium Embedded Framework.framework/',
              'files': [
                'Resources/',
              ],
            },
            {
              # Add the helper app.
              'destination': '<(PRODUCT_DIR)/epToken.app/Contents/Frameworks',
              'files': [
                '<(PRODUCT_DIR)/epToken Helper.app',
                '$(CONFIGURATION)/libplugin_carbon_interpose.dylib',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/libcef.dylib',
                '@executable_path/../Frameworks/Chromium Embedded Framework.framework/Libraries/libcef.dylib',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
            {
              # This postbuid step is responsible for creating the following
              # helpers:
              #
              # epToken Helper EH.app and epToken Helper NP.app are created
              # from epToken Helper.app.
              #
              # The EH helper is marked for an executable heap. The NP helper
              # is marked for no PIE (ASLR).
              'postbuild_name': 'Make More Helpers',
              'action': [
                'tools/make_more_helpers.sh',
                'Frameworks',
                'epToken',
              ],
            },
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/libcef.dylib',
            ],
          },
          'sources': [
            '<@(includes_mac)',
            '<@(epToken_sources_mac)',
          ],
        }], # OS=="mac"
        [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/files',
              'files': [
                '<@(epToken_bundle_resources_linux)',
              ],
            },
          ],
          'sources': [
            '<@(includes_linux)',
            '<@(epToken_sources_linux)',
          ],
        }], # OS=="linux" or OS=="freebsd" or OS=="openbsd"
      ], # conditions
    },
    {
      'target_name': 'libcef_dll_wrapper',
      'type': 'static_library',
      'msvs_guid': 'A9D6DC71-C0DC-4549-AEA0-3B15B44E86A9',
      'defines': [
        'USING_CEF_SHARED',
      ],
      'include_dirs': [
        '.',
      ],
      'sources': [
        '<@(includes_common)',
        '<@(includes_capi)',
        '<@(includes_wrapper)',
        '<@(libcef_dll_wrapper_sources_common)',
      ],
      'xcode_settings': {
        # Target build path.
        'SYMROOT': 'xcodebuild',
      },
    },
    {
      'target_name': 'TokenMonitor',
      'type': 'executable',
      'mac_bundle': 1,
      'msvs_guid': 'D8068FB0-7794-11E2-BCFD-0800200C9A66',
      'dependencies': [
      ],
      'defines': [
      ],
      'include_dirs': [
        '.',
      ],
      'sources': [
        '<@(epToken_sources_win_service)',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_settings': {
            'VCLinkerTool': {
              # Set /SUBSYSTEM:WINDOWS.
              'SubSystem': '1',
            },
          },
          'link_settings': {
            'libraries': [
              '-lcomctl32.lib',
              '-lshlwapi.lib',
              '-lrpcrt4.lib',
              '-lopengl32.lib',
              '-lglu32.lib',
              '-lgdi32.lib',
            ],
          },
          'sources': [
            '<@(epToken_sources_win_service)',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': 'epToken_helper_app',
          'type': 'executable',
          'variables': { 'enable_wexit_time_destructors': 1, },
          'product_name': 'epToken Helper',
          'mac_bundle': 1,
          'dependencies': [
            'libcef_dll_wrapper',
          ],
          'defines': [
            'USING_CEF_SHARED',
          ],
          'include_dirs': [
            '.',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(CONFIGURATION)/libcef.dylib',
            ],
          },
          'sources': [
            '<@(epToken_sources_mac_helper)',
          ],
          # TODO(mark): Come up with a fancier way to do this.  It should only
          # be necessary to list helper-Info.plist once, not the three times it
          # is listed here.
          'mac_bundle_resources!': [
            'epToken/mac/helper-Info.plist',
          ],
          # TODO(mark): For now, don't put any resources into this app.  Its
          # resources directory will be a symbolic link to the browser app's
          # resources directory.
          'mac_bundle_resources/': [
            ['exclude', '.*'],
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': 'epToken/mac/helper-Info.plist',
          },
          'postbuilds': [
            {
              # The framework defines its load-time path
              # (DYLIB_INSTALL_NAME_BASE) relative to the main executable
              # (chrome).  A different relative path needs to be used in
              # epToken_helper_app.
              'postbuild_name': 'Fix Framework Link',
              'action': [
                'install_name_tool',
                '-change',
                '@executable_path/libcef.dylib',
                '@executable_path/../../../../Frameworks/Chromium Embedded Framework.framework/Libraries/libcef.dylib',
                '${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}'
              ],
            },
          ],
        },  # target epToken_helper_app
      ],
    }],  # OS=="mac"
  ],
}
