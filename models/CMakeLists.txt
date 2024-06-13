include(FindProtobuf)
find_package(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER
 latchyConfiguration.proto
 )
 
add_library(models ${PROTO_HEADER} ${PROTO_SRC})
include_directories(${Protobuf_INCLUDE_DIRS})    # Required by protoc when building the model object files

# We use this to make global the model include files, to be used by our sources elsewhere
list (APPEND LAT_GLOBAL_INCL ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
set (LAT_GLOBAL_INCL ${LAT_GLOBAL_INCL})



