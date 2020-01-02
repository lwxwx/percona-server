# Multi-Master Plugin

aux_source_directory(. MULTI_MASTER_SOURCE);

MYSQL_ADD_PLUGIN(multi_master
    ${MULTI_MASTER_SOURCE}
    MODULE_ONLY MODULE_OUTPUT_NAME "multi_master"
)