/*
 * @Author: wei
 * @Date: 2020-06-16 09:19:56
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-16 09:47:59
 * @Description: plugin main source file
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/plugin.cc
 */
#include <mysql/plugin.h>
#include "trx_info.h"

/*SYS_VAR*/
struct st_mysql_daemon multi_master_descriptor=
{
    MYSQL_DAEMON_INTERFACE_VERSION
};

static SYS_VAR * multi_master_system_vars[] = {
    NULL
};

static SHOW_VAR multi_master_status_vars[] = {
     {NULL, NULL, SHOW_LONG, SHOW_SCOPE_GLOBAL}
};


/*plugin global var*/

/*plugin main functions*/
int plugin_multi_master_init(MYSQL_PLUGIN plugin_info)
{
    plugin_trx_info_ptr = new TrxInfo;

    //register function ptr

    return 0;
}

int plugin_multi_master_check_uninstall(MYSQL_PLUGIN plugin_info)
{

    return 0;
}

int  plugin_multi_master_deinit(MYSQL_PLUGIN plugin_info)
{
    delete plugin_trx_info_ptr;
    return 0;
}

/*plugin declare*/
mysql_declare_plugin(multi_master_log_plugin){
    MYSQL_DAEMON_PLUGIN,
    &multi_master_descriptor,
    "multi_master_log_plugin",
    "pokemonwei",
    "Multi Master Plugin (0.1.0)",
    PLUGIN_LICENSE_PROPRIETARY,
    plugin_multi_master_log_init,
    plugin_multi_master_log_check_uninstall,
    plugin_multi_master_log_deinit,
    0x0001,
    multi_master_status_vars,
    multi_master_system_vars,
    NULL,
    0,
} mysql_declare_plugin_end;

