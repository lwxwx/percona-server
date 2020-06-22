/*
 * @Author: wei
 * @Date: 2020-06-16 09:19:56
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-20 09:46:03
 * @Description: plugin main source file
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/plugin.cc
 */
#include <mysql/plugin.h>
#include "trx_info.h"
#include "plugin_interface.h"
#include "../mml_plugin_functions.h"

/*SYS_VAR*/
struct st_mysql_daemon multi_master_log_descriptor=
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
int plugin_multi_master_log_init(MYSQL_PLUGIN plugin_info)
{
    mml_plugin_interface_active = 1;

    plugin_trx_info_ptr = new TrxInfo;

    //register function ptr
    mml_plugin_mtr_redo_record_add_ptr = mml_plugin_mtr_redo_record_add;
    mml_plugin_mtr_redo_record_new_ptr = mml_plugin_mtr_redo_record_new;
    mml_plugin_wr_trx_commit_ptr = mml_plugin_wr_trx_commit;
    mml_plugin_trx_start_ptr = mml_plugin_trx_start;

    return 0;
}

int plugin_multi_master_log_check_uninstall(MYSQL_PLUGIN plugin_info)
{

    return 0;
}

int  plugin_multi_master_log_deinit(MYSQL_PLUGIN plugin_info)
{
    delete plugin_trx_info_ptr;
    return 0;
}

/*plugin declare*/
mysql_declare_plugin(multi_master_log_plugin){
    MYSQL_DAEMON_PLUGIN,
    &multi_master_log_descriptor,
    "multi_master_log_plugin",
    "pokemonwei",
    "Multi Master Log Plugin (0.1.0)",
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

