#include "sql/multi_master_handler.h"
#include <mysql/plugin.h>

#include "event_mess_handle.h"
#include "easylogger.h"

#include<iostream>
#include<string>

struct st_mysql_daemon multi_master_descriptor=
{
    MYSQL_DAEMON_INTERFACE_VERSION
};

// SYS_VAR
char * host_config_path = NULL;
char * mess_config_path = NULL;
char * log_dir_path = NULL;

// plugin global variables
std::string normal_log_path;

EventMessageHandle * event_msg_handle_ptr = NULL;

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


MYSQL_SYSVAR_STR(log_dir,
    log_dir_path,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Dir path for Error log files",
    NULL,
    NULL,
    NULL
);

static SYS_VAR * multi_master_system_vars[] = {
    MYSQL_SYSVAR(host_config),
    MYSQL_SYSVAR(mess_config),
    MYSQL_SYSVAR(log_dir),
    NULL
};

static SHOW_VAR multi_master_status_vars[] = {
     {NULL, NULL, SHOW_LONG, SHOW_SCOPE_GLOBAL}
};

int plugin_multi_master_init(MYSQL_PLUGIN plugin_info)
{
    //global variables initialize
    normal_log_path =  "";
    normal_log_path += log_dir_path;
    normal_log_path += "/multi_master_normal.log";

    EasyLoggerWithTrace_FLUSH(normal_log_path, EasyLogger::LOG_LEVEL::info) << "Multi Master Plugin Init";

    //EventMessageHandle init
    event_msg_handle_ptr = new EventMessageHandle;
    event_msg_handle_ptr->init_handle(host_config_path,mess_config_path);

    //register function ptr

    return 0;
}

int plugin_multi_master_check_uninstall(MYSQL_PLUGIN plugin_info)
{

    return 0;
}

int  plugin_multi_master_deinit(MYSQL_PLUGIN plugin_info)
{
    event_msg_handle_ptr->free_handle();
    delete event_msg_handle_ptr;
    return 0;
}


mysql_declare_plugin(multi_master){
    MYSQL_DAEMON_PLUGIN,
    &multi_master_descriptor,
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


