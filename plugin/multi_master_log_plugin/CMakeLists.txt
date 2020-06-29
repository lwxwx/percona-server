# Multi Master Redo Log Plugin

## include
include_directories(include)
include_directories(.)
include_directories(../../storage/innobase)
include_directories(../../storage/innobase/include)
include_directories(lib/easylogger/)

## PhxPaxos
include_directories(lib/phxpaxos/include)
include_directories(lib/phxpaxos/third_party/gflags/include)
include_directories(lib/phxpaxos/third_party/glog/include)
include_directories(lib/phxpaxos/third_party/leveldb/include)
include_directories(lib/phxpaxos/third_party/protobuf/include)

# include_directories(lib/libmysqlgcs/)

## lib
## {LINK_LIB}
# include_directories(lib/libmysqlgcs/include/mysql/gcs)
# include_directories(lib/libmysqlgcs/)
# add_subdirectory(./lib/libmysqlgcs)

# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/.lib)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/third_party/gflags/lib)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/third_party/glog/lib)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/third_party/leveldb/lib)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/third_party/protobuf/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/static)
SET(STATIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/static)

# link_libraries(
#     lib/phxpaxos/third_party/leveldb/lib/libleveldb.a
#     lib/phxpaxos/third_party/protobuf/lib/libprotobuf.a
#     lib/phxpaxos/third_party/glog/lib/libglog.a
#     lib/phxpaxos/third_party/gflags/lib/libgflags.a
#     lib/phxpaxos/lib/libphxpaxos.a
#     lib/phxpaxos/lib/libphxpaxos_plugin.a
# )

# find_library(leveldb_lib libleveldb.a lib/phxpaxos/third_party/leveldb/lib/libleveldb.a)

SET(LINK_LIB
    # "libphxpaxos_plugin.a libphxpaxos.a libleveldb.a libprotobuf.a libglog.a libgflags.a"
    ${STATIC_DIR}/libphxpaxos_plugin.a
    ${STATIC_DIR}/libphxpaxos.a
    ${STATIC_DIR}/libleveldb.a
    ${STATIC_DIR}/libprotobuf.a
    ${STATIC_DIR}/libglog.a
    ${STATIC_DIR}/libgflags.a
    snappy
)

# add_custom_target(CopyTask
#   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/lib/libphxpaxos.a ${CMAKE_CURRENT_BINARY_DIR}/lib/phxpaxos/lib/
#   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/lib/libphxpaxos_plugin.a ${CMAKE_CURRENT_BINARY_DIR}/lib/phxpaxos/lib/
#   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/third_party/leveldb/lib/libleveldb.a ${CMAKE_CURRENT_BINARY_DIR}/lib/phxpaxos/lib/
#   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/phxpaxos/third_party/protobuf/lib/libprotobuf.a ${CMAKE_CURRENT_BINARY_DIR}/lib/phxpaxos/lib/
#   )

# add_library(mmlp_lib STATIC test/test.cc)
# target_link_libraries(mmlp_lib ${LINK_LIB})

## src
aux_source_directory(src MULTI_MASTER_LOG_SOURCE)

MYSQL_ADD_PLUGIN(multi_master_log_plugin
    ${MULTI_MASTER_LOG_SOURCE}
    DEFAULT
    MODULE_OUTPUT_NAME "multi_master_log"
    LINK_LIBRARIES
    ${LINK_LIB}
)

