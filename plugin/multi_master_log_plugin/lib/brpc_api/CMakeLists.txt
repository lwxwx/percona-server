
find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
find_library(BRPC_LIB NAMES brpc)
if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
	message(FATAL_ERROR "Fail to find brpc")
endif()
include_directories(${BRPC_INCLUDE_PATH})

find_path(PROTOBUF_INLCUDE_PATH google/protobuf/message.h)
find_library(PROTOBUF_LIB NAMES protobuf)
if((NOT PROTOBUF_INLCUDE_PATH) OR (NOT BRPC_LIB))
	message(FATAL_ERROR "Fail to find Protobuf")
endif()
include_directories(${PROTOBUF_INLCUDE_PATH})

find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
  message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
  message(FATAL_ERROR "Fail to find leveldb")
endif()
include_directories(${LEVELDB_INCLUDE_PATH})

find_library(SSL_LIB NAMES ssl)
if (NOT SSL_LIB)
  message(FATAL_ERROR "Fail to find ssl")
endif()

find_library(CRYPTO_LIB NAMES crypto)
if (NOT CRYPTO_LIB)
  message(FATAL_ERROR "Fail to find crypto")
endif()

set(BRPC_DEP_DYNAMIC_LIB
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIB}
    ${LEVELDB_LIB}
    ${SSL_LIB}
    ${CRYPTO_LIB}
)

set(BRPC_LINK_LIB
    ${BRPC_LIB}
		${BRPC_DEP_DYNAMIC_LIB}
)


SET(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
# execute_process(COMMAND protoc ./*.proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR} WORKING_DIRECTORY ${PROTO_DIR})
file(GLOB_RECURSE PROTO_FILES {PROTO_SOURCE}/*.proto)
message(FATAL_ERROR ${PROTO_FILES})
# foreach(pfile ${PROTO_FILES})
#   message(FATAL_ERROR ${pfile})
# endforeach(pfile)

#
# add_custom_command(OUTPUT ${PROTO_SOURCE}
#   PRE_BUILD
#   COMMAND protoc ${PROTO_DIR}/*.proto --cpp_out=${CMAKE_BINARY_DIR}
#   )
include_directories(${CMAKE_CURRENT_BINARY_DIR})


#aux_source_directory(${CMAKE_CURRENT_BINARY_DIR} PROTO_SOURCE)

SET(BRPC_API_SOURCE
    log_transfer.cc
    ${PROTO_SOURCE}
   # ${PROTO_HEADER}
)

add_library(brpc_api STATIC ${BRPC_API_SOURCE})
target_link_libraries(brpc_api ${BRPC_LINK_LIB})
