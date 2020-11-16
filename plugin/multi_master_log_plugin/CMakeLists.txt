# Multi Master Redo Log Plugin

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## mmlp include
include_directories(include)
include_directories(.)
include_directories(../../storage/innobase)
include_directories(../../storage/innobase/include)
include_directories(lib/easylogger/)

include_directories(benchmark)

## phxpaxos_api
add_subdirectory(lib/phxpaxos_api)
include_directories(lib/phxpaxos_api)
include_directories(lib/phxpaxos/include)

## brpc_api
add_subdirectory(lib/brpc_api)
include_directories(lib/brpc_api)
include_directories(lib/brpc_api/proto)
include_directories(${BRPC_INCLUDE_PATH})
include_directories(${BRPC_PROTOBUF_INCLUDE})
message(status "BRPC -> " ${BRPC_INCLUDE_PATH})
message(status "PROTOBUF -> " ${BRPC_PROTOBUF_INCLUDE})
# include_directories(lib/brpc_api/include_dir/brpc_include)
# include_directories(lib/brpc_api/include_dir/protobuf_include)

## src
aux_source_directory(src MULTI_MASTER_LOG_SOURCE)
aux_source_directory(benchmark BENCHMARK_SOURCE)

MYSQL_ADD_PLUGIN(multi_master_log_plugin
    ${MULTI_MASTER_LOG_SOURCE} ${BENCHMARK_SOURCE}
    DEFAULT
    MODULE_OUTPUT_NAME "multi_master_log"
    LINK_LIBRARIES
    #phxpaxos_api
    brpc_api
)

