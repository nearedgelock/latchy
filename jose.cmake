#
# Drive the josé build process - We need to complete this before we can add
# its library to our main project.
#
# José has a dependency on jansson and zlib, which we assume are delivered by vcpkg.
#
# This uses the Jose sources exposed by clevisLIb

set(jose_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/jose")
set(INSTALL_DIR "${jose_BINARY_DIR}/install")

# Manually configuring and building jose
message (NOTICE "Starting configuration, build and installation of jose")
execute_process(
  COMMAND mkdir -p ${jose_BINARY_DIR}
)

message (NOTICE "jose setup at location ${jose_BINARY_DIR}, using ${CMAKE_CURRENT_SOURCE_DIR}/clevisLib/jose")
set(ENV{PKG_CONFIG} "/usr/bin/pkg-config")

# Build arguments list
set(MESON_SETUP_ARGS
  --buildtype=debug
  -Dbuild_static=true
  -Dbuild_dynamic=false
  -Dbuild_executable=false
  --default-library=static
  --prefix=${INSTALL_DIR}
)

list(APPEND MESON_SETUP_ARGS "--pkg-config-path=${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-linux/lib/pkgconfig")

message (NOTICE "meson arguments (for setup) are ${MESON_SETUP_ARGS}")

execute_process(
  WORKING_DIRECTORY ${jose_BINARY_DIR}
  COMMAND meson setup  ${MESON_SETUP_ARGS} . ${CMAKE_CURRENT_SOURCE_DIR}/clevisLib/jose    
  RESULT_VARIABLE JOSE_SETUP_RESULT
)

if(NOT ${JOSE_SETUP_RESULT} EQUAL 0)
    message(NOTICE "Output is ${JOSE_SETUP_OUTPUT}")
    message(FATAL_ERROR "Failed to setup jose")
endif()

message (NOTICE "jose build")
execute_process(
  WORKING_DIRECTORY ${jose_BINARY_DIR}
  COMMAND ninja
)

message (NOTICE "jose installation")
execute_process(
  WORKING_DIRECTORY ${jose_BINARY_DIR}
  COMMAND ninja install

  RESULT_VARIABLE JOSE_INSTALL_RESULT
)

if(NOT ${JOSE_INSTALL_RESULT} EQUAL 0)
    message(FATAL_ERROR "Failed to build / install jose")
endif()

include_directories(${jose_BINARY_DIR}/install/include)
link_directories(${jose_BINARY_DIR}/lib)

add_library(libjose_static STATIC IMPORTED)
set_target_properties(libjose_static PROPERTIES IMPORTED_LOCATION ${jose_BINARY_DIR}/lib/libjose_static.a)
set_target_properties(libjose_static PROPERTIES LINK_WHOLE_ARCHIVE true)

# We must force the inclusion of certain object files as the static linker will not include then 
# even though they are needed.
set (JOSE_INCLUDE_OBJECT 
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_aesgcm.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_aesgcmkw.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_ec.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_ecmr.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_ecdh.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_ecdhes.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_hmac.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_hash.c.o"
  "${jose_BINARY_DIR}/lib/libjose_static.a.p/openssl_oct.c.o"
  )


