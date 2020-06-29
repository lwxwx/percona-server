/*
 * @Author: wei
 * @Date: 2020-06-16 09:19:56
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-29 23:36:22
 * @Description: plugin main source file
 * @FilePath: /Percona-Share-Storage/percona-server/plugin/multi_master_log_plugin/src/plugin.cc
 */
#include <mysql/plugin.h>
#include "trx_info.h"
#include "plugin_interface.h"
#include "../mml_plugin_functions.h"
#include <climits>

/*SYS_VAR*/
struct st_mysql_daemon multi_master_log_descriptor=
{
    MYSQL_DAEMON_INTERFACE_VERSION
};

char * local_node_ptr = NULL;
char * group_name_ptr = NULL;
char * peer_nodes_ptr = NULL;
unsigned long long  phxpaxos_conflict_count = 0;
unsigned long long  phxpaxos_propose_count = 0;

MYSQL_SYSVAR_STR(local_node,
    local_node_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "local ip and port",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_STR(group_name,
    group_name_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "xcom group name",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_STR(peer_nodes,
    peer_nodes_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "peer nodes ip and port",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_ULONGLONG(phxpaxos_conflict,
    phxpaxos_conflict_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "CONFLICT COUNT",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(phxpaxos_propose,
    phxpaxos_propose_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "PROPOSE COUNT",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/*plugin global var*/
static SYS_VAR * multi_master_system_vars[] = {
    MYSQL_SYSVAR(group_name),
    MYSQL_SYSVAR(local_node),
    MYSQL_SYSVAR(peer_nodes),
    MYSQL_SYSVAR(phxpaxos_conflict),
    MYSQL_SYSVAR(phxpaxos_propose),
    NULL
};

static SHOW_VAR multi_master_status_vars[] = {
     {NULL, NULL, SHOW_LONG, SHOW_SCOPE_GLOBAL}
};


/*plugin main functions*/
int plugin_multi_master_log_init(MYSQL_PLUGIN plugin_info)
{
    mml_plugin_interface_active = 1;

    plugin_trx_info_ptr = new TrxInfo;
    plugin_trx_info_ptr->init(group_name_ptr,local_node_ptr,peer_nodes_ptr);

    //std::cout << std::endl << "Get Group Name From Plugin Var : " << group_name_ptr << std::endl;
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

