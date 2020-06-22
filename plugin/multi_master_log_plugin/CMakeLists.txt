# Multi Master Redo Log Plugin

## include
include_directories(include)
include_directories(.)
include_directories(../../storage/innobase)
include_directories(../../storage/innobase/include)
include_directories(lib/easylogger/)

## lib
## {LINK_LIB}

## src
aux_source_directory(src MULTI_MASTER_LOG_SOURCE)

MYSQL_ADD_PLUGIN(multi_master_log_plugin
    ${MULTI_MASTER_LOG_SOURCE}
    DEFAULT MODULE_OUTPUT_NAME "multi_master_log"
    LINK_LIBRARIES ${LINK_LIB}
)

