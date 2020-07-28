# include dir for mmlp
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR})

SET(DEP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include_dir)
# SET(DEP_STATIC_LIB_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/static_dir)
SET(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)

include_directories(${DEP_INCLUDE_DIR}/brpc_include)
include_directories(${DEP_INCLUDE_DIR}/protobuf_include)
include_directories(${DEP_INCLUDE_DIR}/leveldb_include)
include_directories(${DEP_INCLUDE_DIR}/gflags_include)
include_directories(${PROTO_DIR})

# SET(BRPC_STATIC_DEP_LIB
#     ${DEP_STATIC_LIB_DIR}/libprotobuf.a
#     ${DEP_STATIC_LIB_DIR}/libprotoc.a
#     ${DEP_STATIC_LIB_DIR}/libleveldb.a
#     ${DEP_STATIC_LIB_DIR}/libgflags.a
#     #${DEP_STATIC_LIB_DIR}/libssl.a
#     #${DEP_STATIC_LIB_DIR}/libcrypto.a
#     libssl.so
#     libcrypto.so
#     snappy
# )

SET(BRPC_DYNAMIC_DEP_LIB
    $ENV{BRPC_PROTOBUF_LIB}
   # ${DEP_STATIC_LIB_DIR}/libprotobuf.a
    $ENV{BRPC_LEVELDB_LIB}
    $ENV{BRPC_GFLAGS_LIB}
    libssl.so
    libcrypto.so
)

SET(BRPC_DYNAMIC_LIB
    $ENV{BRPC_LIB}
)


# SET(BRPC_STATIC_LIB
#     ${DEP_STATIC_LIB_DIR}/libbrpc.a
# )

set(BRPC_LINK_LIB
    ${BRPC_DYNAMIC_LIB}
    ${BRPC_DYNAMIC_DEP_LIB}
)

aux_source_directory(${PROTO_DIR} PROTO_SOURCE)

SET(BRPC_API_SOURCE
    log_transfer.cc
    ${PROTO_SOURCE}
   # ${PROTO_HEADER}
)

add_library(brpc_api STATIC ${BRPC_API_SOURCE})
target_link_libraries(brpc_api ${BRPC_LINK_LIB})