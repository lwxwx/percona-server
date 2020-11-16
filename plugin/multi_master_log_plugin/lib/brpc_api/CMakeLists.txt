
find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
find_library(BRPC_LIB NAMES brpc)
if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
	message(FATAL_ERROR "Fail to find brpc")
endif()
include_directories(${BRPC_INCLUDE_PATH})

# find_path(PROTOBUF_INLCUDE_PATH google/protobuf/message.h)
# find_library(PROTOBUF_LIB NAMES protobuf)
# if((NOT PROTOBUF_INLCUDE_PATH) OR (NOT PROTOBUF_LIB))
#   message(FATAL_ERROR "Fail to find Protobuf")
# endif()
# include_directories(${PROTOBUF_INLCUDE_PATH})
# message(status "PROTOBUF -> " ${PROTOBUF_INCLUDE_PATH})
# message(status "PROTOBUF -> " ${PROTOBUF_LIB})
if( DEFINED ENV{BRPC_PROTOBUF_LIB_DIR})
	set(BRPC_PROTOBUF_INCLUDE  $ENV{BRPC_PROTOBUF_LIB_DIR}/../include/  )
	set(BRPC_PROTOBUF_LIB $ENV{BRPC_PROTOBUF_LIB_DIR}/libprotobuf.so)
else()
	find_package(Protobuf)
	set(BRPC_PROTOBUF_INCLUDE ${PROTOBUF_INCLUDE_DIR} )
	set(BRPC_PROTOBUF_LIB ${PROTOBUF_LIBRARY})
endif()
set(BRPC_PROTOBUF_INCLUDE ${BRPC_PROTOBUF_INCLUDE} CACHE STRING "brpc protobuf include path ")
message(status "BRPC_API PROTOBUF -> " ${BRPC_PROTOBUF_INCLUDE})
include_directories(${BRPC_PROTOBUF_INCLUDE})

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
		${BRPC_PROTOBUF_LIB}
    ${LEVELDB_LIB}
    ${SSL_LIB}
    ${CRYPTO_LIB}
)

set(BRPC_LINK_LIB
    ${BRPC_LIB}
		${BRPC_DEP_DYNAMIC_LIB}
)


SET(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
execute_process(COMMAND bash "-c" "rm *.cc *.h && protoc ./* --cpp_out=./"
	WORKING_DIRECTORY "${PROTO_DIR}"
	)


#
# add_custom_command(OUTPUT ${PROTO_SOURCE}
#   PRE_BUILD
#   COMMAND protoc ${PROTO_DIR}/*.proto --cpp_out=${CMAKE_BINARY_DIR}
#   )
include_directories(${PROTO_DIR})

aux_source_directory(${PROTO_DIR} PROTO_SOURCE)

SET(BRPC_API_SOURCE
    log_transfer.cc
    ${PROTO_SOURCE}
   # ${PROTO_HEADER}
)

add_library(brpc_api STATIC ${BRPC_API_SOURCE})
target_link_libraries(brpc_api ${BRPC_LINK_LIB})
