# Multi-Master Plugin

aux_source_directory(src/ MULTI_MASTER_SOURCE)

MYSQL_ADD_PLUGIN(multi_master
    ${MULTI_MASTER_SOURCE}
    DEFAULT MODULE_OUTPUT_NAME "multi_master"
)