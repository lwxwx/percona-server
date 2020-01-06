# Multi-Master Plugin

include_directories(multi-master-tool/multi_net_io/include)
include_directories(multi-master-tool/easy_logger/include)
include_directories(${LIBEVENT_INCLUDE_DIRS})

add_subdirectory(multi-master-tool)

aux_source_directory(src/ MULTI_MASTER_SOURCE)

MYSQL_ADD_PLUGIN(multi_master
    ${MULTI_MASTER_SOURCE}
    DEFAULT MODULE_OUTPUT_NAME "multi_master"
    LINK_LIBRARIES multi_net_io
)