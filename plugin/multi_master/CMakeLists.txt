# Multi-Master Plugin
## dependent on multi-master-tool submodule

AUX_SOURCE_DIRECTORY(. MULTI_MASTER_SOURCE)

MYSQL_ADD_PLUGIN( multi-master
    ${MULTI_MASTER_SOURCE}
    MODULE_ONLY MODULE_OUTPUT_NAME "multi-master"
)