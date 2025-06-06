#
# This file is part of Project SkyFire https://www.projectskyfire.org. 
# See COPYRIGHT file for Copyright information
#

########### authserver ###############

file(GLOB_RECURSE sources_Authentication Authentication/*.cpp Authentication/*.h)
file(GLOB_RECURSE sources_Realms Realms/*.cpp Realms/*.h)
file(GLOB_RECURSE sources_Server Server/*.cpp Server/*.h)
file(GLOB sources_Localdir *.cpp *.h)

source_group(CMake FILES CMakeLists.txt)
source_group(Authentication FILES ${sources_Authentication})
source_group(Realms FILES ${sources_Realms})
source_group(Server FILES ${sources_Server})
source_group(localdir FILES ${sources_Localdir})

if (USE_COREPCH)
  set(authserver_PCH_HDR PrecompiledHeaders/authPCH.h)
  set(authserver_PCH_SRC PrecompiledHeaders/authPCH.cpp)
endif()

set(authserver_SRCS
  ${authserver_SRCS}
  ${sources_Authentication}
  ${sources_Realms}
  ${sources_Server}
  ${sources_Localdir}
)

if( WIN32 )
  set(authserver_SRCS
    ${authserver_SRCS}
    ${sources_windows_Debugging}
  )
  if ( MSVC )
    set(authserver_SRCS
      ${authserver_SRCS}
      authserver.rc
    )
  endif ()
endif()

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/server/shared
  ${CMAKE_SOURCE_DIR}/src/server/shared/Database
  ${CMAKE_SOURCE_DIR}/src/server/shared/Debugging
  ${CMAKE_SOURCE_DIR}/src/server/shared/Packets
  ${CMAKE_SOURCE_DIR}/src/server/shared/Cryptography
  ${CMAKE_SOURCE_DIR}/src/server/shared/Cryptography/Authentication
  ${CMAKE_SOURCE_DIR}/src/server/shared/Logging
  ${CMAKE_SOURCE_DIR}/src/server/shared/Threading
  ${CMAKE_SOURCE_DIR}/src/server/shared/Utilities
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Authentication
  ${CMAKE_CURRENT_SOURCE_DIR}/Realms
  ${CMAKE_CURRENT_SOURCE_DIR}/Server
  ${ACE_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

add_executable(authserver
  ${authserver_SRCS}
  ${authserver_PCH_SRC}
)

add_dependencies(authserver revision.h)

if( NOT WIN32 )
  set_target_properties(authserver PROPERTIES
    COMPILE_DEFINITIONS _SKYFIRE_REALM_CONFIG="${CONF_DIR}/authserver.conf"
  )
endif()

target_link_libraries(authserver
  shared
  ${MYSQL_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${ACE_LIBRARY}
)

if( WIN32 )
  if ( MSVC )
    #add libcrypto-3 dll to bin Folder
    add_custom_command(TARGET authserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_CRYPTO3_DLL} ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )

    #add libssl-3 dll to bin Folder
    add_custom_command(TARGET authserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_SSL3_DLL} ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )

    #add mysql dll to bin folder
    add_custom_command(TARGET authserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${MYSQL_DLL} ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )
    
    add_custom_command(TARGET authserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/authserver.conf.dist ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )
  elseif ( MINGW )
    add_custom_command(TARGET authserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/authserver.conf.dist ${CMAKE_BINARY_DIR}/bin/
    )
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    #add libcrypto-3 dll to bin Folder
    add_custom_command(TARGET authserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_CRYPTO3_DLL} ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )

    #add libssl-3 dll to bin Folder
    add_custom_command(TARGET authserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_SSL3_DLL} ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )

    #add mysql dll to bin folder
    add_custom_command(TARGET authserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${MYSQL_DLL} ${CMAKE_BINARY_DIR}/bin/
    )
    add_custom_command(TARGET authserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/authserver.conf.dist ${CMAKE_BINARY_DIR}/bin/
    )
  endif()
endif()

if( UNIX )
  install(TARGETS authserver DESTINATION bin)
  install(FILES  authserver.conf.dist DESTINATION ${CONF_DIR})
  install(FILES  authserverd.service.dist DESTINATION ${CONF_DIR})
elseif( WIN32 )
  install(TARGETS authserver DESTINATION "${CMAKE_INSTALL_PREFIX}")
  install(FILES authserver.conf.dist DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(authserver ${authserver_PCH_HDR} ${authserver_PCH_SRC})
endif()
