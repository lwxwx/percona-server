#include "sql/multi_master_handler.h"
#include <mysql/plugin.h>
#include<iostream>

struct st_mysql_daemon multi_master_descriptor=
{
    MYSQL_DAEMON_INTERFACE_VERSION
};

char * host_config_path = NULL;
char * mess_config_path = NULL;

MYSQL_SYSVAR_STR(host_config,
    host_config_path,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "host config information",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_STR(mess_config,
    mess_config_path,
     PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Message type and group config information",
    NULL,
    NULL,
    NULL
);

static SYS_VAR * multi_master_system_vars[] = {
    MYSQL_SYSVAR(host_config),
    MYSQL_SYSVAR(mess_config),
    NULL
};

static SHOW_VAR multi_master_status_vars[] = {
     {NULL, NULL, SHOW_LONG, SHOW_SCOPE_GLOBAL}
};

int plugin_multi_master_init(MYSQL_PLUGIN plugin_info)
{
    // std::cout  << "Init Multi Master Plugin" << std::endl;
    // std::cout << "host config : " << host_config_path << std::endl;
    // std::cout << "message config : " << mess_config_path << std::endl;
    return 0;
}

int plugin_multi_master_check_uninstall(MYSQL_PLUGIN plugin_info)
{
    return 0;
}

int  plugin_multi_master_deinit(MYSQL_PLUGIN plugin_info)
{
    return 0;
}


mysql_declare_plugin(multi_master){
    MYSQL_DAEMON_PLUGIN,
    &multi_master_descriptor, // todo
    "multi_master",
    "pokemonwei",
    "Multi Master Plugin (0.1.0)",
    PLUGIN_LICENSE_PROPRIETARY,
    plugin_multi_master_init,
    plugin_multi_master_check_uninstall,
    plugin_multi_master_deinit,
    0x0001,
    multi_master_status_vars,
    multi_master_system_vars,
    NULL,
    0,
} mysql_declare_plugin_end;


