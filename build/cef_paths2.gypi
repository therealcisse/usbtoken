# Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'includes': [
    # Bring in the autogenerated source file lists.
    'cef_paths.gypi',
   ],
  'variables': {
    'includes_common': [
      'include/cef_base.h',
      'include/cef_pack_resources.h',
      'include/cef_pack_strings.h',
      'include/cef_runnable.h',
      'include/cef_trace_event.h',
      'include/cef_version.h',
      'include/internal/cef_build.h',
      'include/internal/cef_export.h',
      'include/internal/cef_ptr.h',
      'include/internal/cef_string.h',
      'include/internal/cef_string_list.h',
      'include/internal/cef_string_map.h',
      'include/internal/cef_string_multimap.h',
      'include/internal/cef_string_types.h',
      'include/internal/cef_string_wrappers.h',
      'include/internal/cef_time.h',
      'include/internal/cef_tuple.h',
      'include/internal/cef_types.h',
      'include/internal/cef_types_wrappers.h',
      '<@(autogen_cpp_includes)',
    ],
    'includes_capi': [
      'include/capi/cef_base_capi.h',
      '<@(autogen_capi_includes)',
    ],
    'includes_wrapper': [
      'include/wrapper/cef_byte_read_handler.h',
      'include/wrapper/cef_stream_resource_handler.h',
      'include/wrapper/cef_xml_object.h',
      'include/wrapper/cef_zip_archive.h',
    ],
    'includes_win': [
      'include/internal/cef_types_win.h',
      'include/internal/cef_win.h',
    ],
    'includes_mac': [
      'include/cef_application_mac.h',
      'include/internal/cef_mac.h',
      'include/internal/cef_types_mac.h',
    ],
    'libs_opensc': [
      'OpenSC/lib/$(ConfigurationName)/opensc.lib',
    ],
    'libs_openssl': [
      'openssl/lib/$(ConfigurationName)/libeay32.lib',
      'openssl/lib/$(ConfigurationName)/ssleay32.lib',
    ],
    'libs_p11': [
      'Libp11/lib/$(ConfigurationName)/libp11.lib',    
    ],    
    'includes_linux': [
      'include/internal/cef_linux.h',
      'include/internal/cef_types_linux.h',
    ],
    'libcef_sources_common': [
      'libcef_dll/cef_logging.h',
      'libcef_dll/cpptoc/cpptoc.h',
      'libcef_dll/ctocpp/ctocpp.h',
      'libcef_dll/libcef_dll.cc',
      'libcef_dll/libcef_dll2.cc',
      'libcef_dll/resource.h',
      'libcef_dll/transfer_util.cpp',
      'libcef_dll/transfer_util.h',
      '<@(autogen_library_side)',
    ],
    'libcef_dll_wrapper_sources_common': [
      'libcef_dll/cef_logging.h',
      'libcef_dll/cpptoc/base_cpptoc.h',
      'libcef_dll/cpptoc/cpptoc.h',
      'libcef_dll/ctocpp/base_ctocpp.h',
      'libcef_dll/ctocpp/ctocpp.h',
      'libcef_dll/transfer_util.cpp',
      'libcef_dll/transfer_util.h',
      'libcef_dll/wrapper/cef_byte_read_handler.cc',
      'libcef_dll/wrapper/cef_stream_resource_handler.cc',
      'libcef_dll/wrapper/cef_xml_object.cc',
      'libcef_dll/wrapper/cef_zip_archive.cc',
      'libcef_dll/wrapper/libcef_dll_wrapper.cc',
      'libcef_dll/wrapper/libcef_dll_wrapper2.cc',
      '<@(autogen_client_side)',
    ],
    'epToken_sources_common': [
      'epToken/AbstractOp.cpp',
      'epToken/README.md',
      'epToken/client_app.cpp',
      'epToken/client_app.h',
      'epToken/client_app_delegates.cpp',
      'epToken/client_handler.cpp',
      'epToken/client_handler.h',
      'epToken/client_renderer.cpp',
      'epToken/client_renderer.h',
      'epToken/client_switches.cpp',
      'epToken/client_switches.h',
      'epToken/config.h',
      'epToken/ep_pkcs11_scheme.cpp',
      'epToken/ep_pkcs11_scheme.h',
      'epToken/ep_pkcs15.cpp',
      'epToken/ep_thread_ctx.c',
      'epToken/resource_util.h',
      'epToken/string_util.cpp',
      'epToken/string_util.h',
      'epToken/token.cpp',
      'epToken/usbtoken.cpp',
      'epToken/usbtoken.h',
      'epToken/usbtoken_extensions.js',
      'epToken/util.h',
      'epToken/util_opensc.c',
      'epToken/util_opensc.h',
      'epToken/util_pkcs15.cpp',
      'epToken/watcher.cpp',
      'epToken/inc/Op.h',
      'epToken/inc/ep_pkcs15.h',
      'epToken/inc/ep_thread_ctx.h',
      'epToken/inc/pkcs15.h',
      'epToken/inc/token.h',
      'epToken/inc/util_pkcs15.h',
      'epToken/inc/watcher.h',
      'epToken/res/public/application.css',
      'epToken/res/public/application.js',
      'epToken/res/public/bootstrap.css',
      'epToken/res/public/bootstrap.js',
      'epToken/res/public/favicon.ico',
      'epToken/res/public/font-awesome.css',
      'epToken/res/public/index.html',
      'epToken/res/public/jquery-ui-1.8.23.custom.min.js',
      'epToken/res/public/loader.gif',
      'epToken/res/public/loading.gif',
      'epToken/res/public/logo.png',
      'epToken/res/public/fonts/fontawesome-webfont.eot',
      'epToken/res/public/fonts/fontawesome-webfont.svg',
      'epToken/res/public/fonts/fontawesome-webfont.ttf',
      'epToken/res/public/fonts/fontawesome-webfont.woff',        
      'epToken/conf/openssl.conf',  
      'epToken/conf/opensc.conf',  
      'epToken/profiles/epass2003.profile',  
    ],
    'epToken_sources_win': [
      'epToken/targetver.h',  
      'epToken/epToken.rc',
      'epToken/resource.h',
      'epToken/resource_util_win.cpp',
      'epToken/client_app_win.cpp',
      'epToken/client_handler_win.cpp',   
      'epToken/usbtoken_win.cpp',   
    ],
    'epToken_sources_mac': [
      'epToken/epToken_mac.mm',
      'epToken/client_handler_mac.mm',
      'epToken/resource_util_mac.mm',
    ],
    'epToken_sources_mac_helper': [
    ],
    'epToken_bundle_resources_mac': [
    ],
    'epToken_sources_linux': [
      'epToken/epToken_gtk.cpp',
      'epToken/client_handler_gtk.cpp',
      'epToken/resource_util_linux.cpp',
    ],
    'epToken_bundle_resources_linux': [
    ],
  'epToken_sources_win_service' : [ 
    'epToken/TokenMonitor/ServiceBase.cpp',
    'epToken/TokenMonitor/ServiceInstaller.cpp',
    'epToken/TokenMonitor/TokenService.cpp',
    'epToken/TokenMonitor/inc/ServiceBase.h',
    'epToken/TokenMonitor/inc/ServiceInstaller.h',
    'epToken/TokenMonitor/inc/ThreadPool.h',
    'epToken/TokenMonitor/inc/TokenService.h',
    'epToken/TokenMonitor/inc/config.h',
  ],  
  },
}
