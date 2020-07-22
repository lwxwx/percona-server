

## PhxPaxos
include_directories(../phxpaxos/include)
include_directories(../phxpaxos/third_party/gflags/include)
include_directories(../phxpaxos/third_party/glog/include)
include_directories(../phxpaxos/third_party/leveldb/include)
include_directories(../phxpaxos/third_party/protobuf/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../brpc_api/include_dir/brpc_include)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/static)
SET(STATIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/static)

#phxpaxos link lib
SET(PHXPAXOS_LINK_LIB
    # "libphxpaxos_plugin.a libphxpaxos.a libleveldb.a libprotobuf.a libglog.a libgflags.a"
    ${STATIC_DIR}/libphxpaxos_plugin.a
    ${STATIC_DIR}/libphxpaxos.a
    ${STATIC_DIR}/libleveldb.a
    ${STATIC_DIR}/libprotobuf.a
    ${STATIC_DIR}/libglog.a
    ${STATIC_DIR}/libgflags.a
    snappy
)

## phxpaxos_api_src
aux_source_directory(. PHXPAXOS_API_SOURCE)

add_library(phxpaxos_api STATIC ${PHXPAXOS_API_SOURCE})

target_link_libraries(phxpaxos_api ${PHXPAXOS_LINK_LIB})