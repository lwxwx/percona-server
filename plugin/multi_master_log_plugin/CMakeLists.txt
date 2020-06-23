# Multi Master Redo Log Plugin

## include
include_directories(include)
include_directories(.)
include_directories(../../storage/innobase)
include_directories(../../storage/innobase/include)
include_directories(lib/easylogger/)
# include_directories(lib/libmysqlgcs/)

## lib
## {LINK_LIB}
include_directories(./lib/libmysqlgcs/include)
add_subdirectory(./lib/libmysqlgcs)

SET(LINK_LIB
    mysqlgcs_mmlp
)

## src
aux_source_directory(src MULTI_MASTER_LOG_SOURCE)

MYSQL_ADD_PLUGIN(multi_master_log_plugin
    ${MULTI_MASTER_LOG_SOURCE}
    DEFAULT MODULE_OUTPUT_NAME "multi_master_log"
    LINK_LIBRARIES ${LINK_LIB}
)

