/*
 * @Author: wei,liu
 * @Date: 2020-06-16 09:19:56
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-11-12 20:00
 * @Description: plugin main source file
 * @FilePath: /multi_master_log_plugin/src/plugin.cc
 */
#include <mysql/plugin.h>
#include "mysql.h"
#include "trx_info.h"
#include "plugin_interface.h"
#include "../mml_plugin_functions.h"
#include <climits>
#include "debug.h"
#include "locktable_client.h"

/*SYS_VAR*/
struct st_mysql_daemon multi_master_log_descriptor=
{
    MYSQL_DAEMON_INTERFACE_VERSION
};

//nodes string
char * group_name_ptr = NULL;
char * phxpaxos_local_node_ptr = NULL;
char * phxpaxos_peer_nodes_ptr = NULL;
char * brpc_local_node_ptr = NULL;
char * brpc_peer_nodes_ptr = NULL;

//debug flag
int DEBUG_REDO_LOG_COLLECT = 0;
int DEBUG_PHXPAXOS_PRINT = 0;
int DEBUG_PHXPAXOS_TIME = 0;
int DEBUG_SLICE_TIME = 0;
int DEBUG_REMOTE_ID_TIME = 0;
int DEBUG_LOG_SEND_TIME = 0;
int DEBUG_TRX_TIME = 0;
int DEBUG_LOG_REQUIRE_TIME = 0;
int DEBUG_CONFLICT_TIME = 0;
int DEBUG_REMOTE_LOCKTABLE_TIME = 0;

//select flag
int SELECT_LOG_ASYNC_TYPE = 0;
int SELECT_TRX_ID_ALLOCATE_TYPE = 0;
int SELECT_CONFLICT_HANDLE_TYPE = 0;
int SELECT_REMOTE_LOCKTABLE = 0;

//remote id arg
char * remote_id_server_addr = NULL;
unsigned long long remote_id_handle_time = 0;
unsigned long long remote_id_handle_count = 0;
unsigned long long remote_id_get_time = 0;
unsigned long long remote_id_get_count = 0;
unsigned long long remote_id_over_wait_count = 0;

//slice id arg
unsigned long slice_node_no;
unsigned long long slice_gen_time;
unsigned long long slice_gen_count;

//phxpaxos time
char * phxpaxos_log_path = NULL;
unsigned long long  phxpaxos_conflict_count = 0;
unsigned long long  phxpaxos_propose_count = 0;
unsigned long long  phxpaxos_conflict_time = 0;
unsigned long long  phxpaxos_propose_time = 0;
unsigned long long  phxpaxos_other_count = 0;

//log send time
unsigned long long log_send_succeed_count = 0;
unsigned long long log_send_succeed_sum_time = 0;
unsigned long long log_send_failed_count = 0;
unsigned long long log_send_failed_sum_time = 0;
unsigned long long log_send_async_rpc_count = 0;
unsigned long long log_send_async_rpc_time = 0;
unsigned long long log_send_async_rpc_failed_count = 0;
unsigned long long log_send_async_rpc_failed_time = 0;

//log require 
unsigned long long log_require_succeed_count = 0;
unsigned long long log_require_succeed_sum_time = 0;
unsigned long long log_require_failed_count = 0;
unsigned long long log_require_failed_sum_time = 0; 
unsigned long long log_require_async_rpc_count = 0;
unsigned long long log_require_async_rpc_time = 0;
unsigned long long log_require_async_rpc_failed_count = 0;
unsigned long long log_require_async_rpc_failed_time = 0;

//conflict handle
unsigned long long conflict_succeed_time = 0;
unsigned long long conflict_succeed_count = 0;
unsigned long long conflict_failed_count = 0;
unsigned long long conflict_failed_time = 0;

//trx time
unsigned long long trx_count = 0;
unsigned long long trx_sum_time = 0;

//lock_table arg
char * remote_locktable_server_addr = NULL;
int remote_locktable_local_port = 0;
unsigned long long remote_locktable_sum_time = 0;
unsigned long long remote_locktable_sum_cnt = 0;
std::vector<std::pair<unsigned int,unsigned int>> RPC_time;
std::mutex RPC_time_mutex;

/* node sysvar */
MYSQL_SYSVAR_STR(phxpaxos_local_node,
    phxpaxos_local_node_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "phxpaxos local ip and port",
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

MYSQL_SYSVAR_STR(phxpaxos_peer_nodes,
    phxpaxos_peer_nodes_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "phxpaxos peer nodes ip and port",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_STR(brpc_local_node,
    brpc_local_node_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "brpc nodes ip and port",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_STR(brpc_peer_nodes,
    brpc_peer_nodes_ptr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "brpc peer nodes ip and port",
    NULL,
    NULL,
    NULL
);

/* debug sysvar */
MYSQL_SYSVAR_INT(debug_redo_log_collect,
    DEBUG_REDO_LOG_COLLECT,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "redo log collect debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_phxpaxos_print,
    DEBUG_PHXPAXOS_PRINT,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "phxpaxos print debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);


MYSQL_SYSVAR_INT(debug_phxpaxos_time,
    DEBUG_PHXPAXOS_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "phxpaxos time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_slice_time,
    DEBUG_SLICE_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "slice time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_remote_id_time,
    DEBUG_REMOTE_ID_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "remote id time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_log_send_time,
    DEBUG_LOG_SEND_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "log send time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_trx_time,
    DEBUG_TRX_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "trx time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_log_require_time,
    DEBUG_LOG_REQUIRE_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "log require time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_conflict_time,
    DEBUG_CONFLICT_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "conflict time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

/* select sysvar */
MYSQL_SYSVAR_INT(select_log_async_type,
    SELECT_LOG_ASYNC_TYPE,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "log async type",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(select_trx_id_allocate_type,
    SELECT_TRX_ID_ALLOCATE_TYPE,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "trx id allocate type",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

/*remote id sysvar */
MYSQL_SYSVAR_STR(remote_id_server_addr,
    remote_id_server_addr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "xcom group name",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_ULONGLONG(remote_id_handle_time,
    remote_id_handle_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "REMOTE ID HANDLE TIME",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(remote_id_handle_count,
    remote_id_handle_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "REMOTE ID HANDLE COUNT",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(remote_id_get_time,
    remote_id_get_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "REMOTE ID Get TIME",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(remote_id_get_count,
    remote_id_get_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "REMOTE ID Get COUNT",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(remote_id_over_wait_count,
    remote_id_over_wait_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "REMOTE ID OVER WAITI COUNT",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/*slice id sysvar*/
MYSQL_SYSVAR_ULONG(slice_node_no,
    slice_node_no,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Slice Node No",
    NULL,
    NULL,
    0,
    0,
    ULONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(slice_gen_time,
    slice_gen_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Slice Gen Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(slice_gen_count,
    slice_gen_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Slice Gen Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);


/* phxpaxos sysvar */
MYSQL_SYSVAR_STR(phxpaxos_log_path,
    phxpaxos_log_path,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "peer nodes ip and port",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_ULONGLONG(phxpaxos_conflict_count,
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

MYSQL_SYSVAR_ULONGLONG(phxpaxos_propose_count,
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

MYSQL_SYSVAR_ULONGLONG(phxpaxos_other_count,
    phxpaxos_other_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "PROPOSE COUNT",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(phxpaxos_propose_time,
    phxpaxos_propose_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "PROPOSE TIME",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(phxpaxos_conflict_time,
    phxpaxos_conflict_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG ,
    "CONFLICT TIME",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/* log send sysvar */
MYSQL_SYSVAR_ULONGLONG(log_send_succeed_count,
    log_send_succeed_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send succeed count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_succeed_sum_time,
    log_send_succeed_sum_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send succeed sum time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_failed_count,
    log_send_failed_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send failed count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_failed_sum_time,
    log_send_failed_sum_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send failed sum time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_async_rpc_count,
    log_send_async_rpc_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send failed sum time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_async_rpc_time,
    log_send_async_rpc_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send failed sum time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_async_rpc_failed_count,
    log_send_async_rpc_failed_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send failed sum time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_send_async_rpc_failed_time,
    log_send_async_rpc_failed_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "Log send failed sum time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/* log require sysvar*/
MYSQL_SYSVAR_ULONGLONG(log_require_succeed_count,
    log_require_succeed_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require Succeed Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_require_succeed_sum_time,
    log_require_succeed_sum_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require Succeed Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_require_failed_count,
    log_require_failed_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require Failed Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_require_failed_sum_time,
    log_require_failed_sum_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require Failed Sum Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_require_async_rpc_time,
    log_require_async_rpc_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require ASYNC SUCCESS Sum Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);


MYSQL_SYSVAR_ULONGLONG(log_require_async_rpc_count,
	log_require_async_rpc_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require ASYNC SUCCESS Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_require_async_rpc_failed_time,
	log_require_async_rpc_failed_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require ASYNC Failed Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(log_require_async_rpc_failed_count,
	log_require_async_rpc_failed_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Log Require ASYNC Failed Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/* conflict sysvar*/
MYSQL_SYSVAR_ULONGLONG(conflict_succeed_time,
    conflict_succeed_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "No Conflict Sum Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(conflict_succeed_count,
    conflict_succeed_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "No Conflict Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(conflict_failed_time,
    conflict_failed_time,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Conflict Sum Time",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(conflict_failed_count,
    conflict_failed_count,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "Conflict Count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/* trx sysvar */
MYSQL_SYSVAR_ULONGLONG(trx_count,
    trx_count,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "trx count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(trx_sum_time,
    trx_sum_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "trx count",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

/* lock_table sysvar */
MYSQL_SYSVAR_INT(select_remote_locktable,
    SELECT_REMOTE_LOCKTABLE,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "if select remote lock_table",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_INT(debug_remote_locktable_time,
    DEBUG_REMOTE_LOCKTABLE_TIME,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "remote lock_table time debug",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_STR(remote_locktable_server_addr,
    remote_locktable_server_addr,
    PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "lock_table server addr",
    NULL,
    NULL,
    NULL
);

MYSQL_SYSVAR_INT(remote_locktable_local_port,
    remote_locktable_local_port,
    PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_PERSIST_AS_READ_ONLY,
    "locktable info receiver nodes port",
    NULL,
    NULL,
    0,
    0,
    INT_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(remote_locktable_sum_time,
    remote_locktable_sum_time,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "locktable sum time in rpc",
    NULL,
    NULL,
    0,
    0,
    ULONG_LONG_MAX,
    0
);

MYSQL_SYSVAR_ULONGLONG(remote_locktable_sum_cnt,
    remote_locktable_sum_cnt,
    PLUGIN_VAR_READONLY | PLUGIN_VAR_OPCMDARG,
    "locktable sum count",
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
    MYSQL_SYSVAR(phxpaxos_local_node),
    MYSQL_SYSVAR(phxpaxos_peer_nodes),
    MYSQL_SYSVAR(brpc_local_node),
    MYSQL_SYSVAR(brpc_peer_nodes),

    MYSQL_SYSVAR(debug_redo_log_collect),
    MYSQL_SYSVAR(debug_phxpaxos_print),
    MYSQL_SYSVAR(debug_phxpaxos_time),

    MYSQL_SYSVAR(debug_slice_time),
    MYSQL_SYSVAR(debug_log_send_time),
    MYSQL_SYSVAR(debug_trx_time),
	MYSQL_SYSVAR(debug_log_require_time),
	MYSQL_SYSVAR(debug_conflict_time),
	MYSQL_SYSVAR(debug_remote_id_time),
    MYSQL_SYSVAR(debug_remote_locktable_time),

    MYSQL_SYSVAR(select_log_async_type),
    MYSQL_SYSVAR(select_trx_id_allocate_type),
    MYSQL_SYSVAR(select_remote_locktable),

	MYSQL_SYSVAR(remote_id_server_addr),
	MYSQL_SYSVAR(remote_id_get_time),
	MYSQL_SYSVAR(remote_id_get_count),
	MYSQL_SYSVAR(remote_id_handle_time),
	MYSQL_SYSVAR(remote_id_handle_count),
	MYSQL_SYSVAR(remote_id_over_wait_count),

    MYSQL_SYSVAR(slice_node_no),
    MYSQL_SYSVAR(slice_gen_time),
    MYSQL_SYSVAR(slice_gen_count),

    MYSQL_SYSVAR(phxpaxos_log_path),
#if PHXPAXOS_ID_COMPLIE
    MYSQL_SYSVAR(phxpaxos_conflict_count),
    MYSQL_SYSVAR(phxpaxos_propose_count),
    MYSQL_SYSVAR(phxpaxos_other_count),
    MYSQL_SYSVAR(phxpaxos_conflict_time),
    MYSQL_SYSVAR(phxpaxos_propose_time),
#endif
    MYSQL_SYSVAR(log_send_succeed_count),
    MYSQL_SYSVAR(log_send_succeed_sum_time),
    MYSQL_SYSVAR(log_send_failed_count),
    MYSQL_SYSVAR(log_send_failed_sum_time),
    MYSQL_SYSVAR(log_send_async_rpc_count),
    MYSQL_SYSVAR(log_send_async_rpc_time),
    MYSQL_SYSVAR(log_send_async_rpc_failed_count),
    MYSQL_SYSVAR(log_send_async_rpc_failed_time),

	MYSQL_SYSVAR(log_require_succeed_sum_time),
	MYSQL_SYSVAR(log_require_succeed_count),
	MYSQL_SYSVAR(log_require_failed_sum_time),
	MYSQL_SYSVAR(log_require_failed_count),
	MYSQL_SYSVAR(log_require_async_rpc_count),
    MYSQL_SYSVAR(log_require_async_rpc_time),
    MYSQL_SYSVAR(log_require_async_rpc_failed_count),
    MYSQL_SYSVAR(log_require_async_rpc_failed_time),
	
	MYSQL_SYSVAR(conflict_succeed_time),
	MYSQL_SYSVAR(conflict_succeed_count),
	MYSQL_SYSVAR(conflict_failed_time),
	MYSQL_SYSVAR(conflict_failed_count),

    MYSQL_SYSVAR(trx_count),
    MYSQL_SYSVAR(trx_sum_time),

    MYSQL_SYSVAR(remote_locktable_server_addr),
    MYSQL_SYSVAR(remote_locktable_local_port),
    MYSQL_SYSVAR(remote_locktable_sum_time),
    MYSQL_SYSVAR(remote_locktable_sum_cnt),

    NULL
};

static SHOW_VAR multi_master_status_vars[] = {
     {NULL, NULL, SHOW_LONG, SHOW_SCOPE_GLOBAL}
};

void inform_receive()
{
    plugin_remote_locktable_receiver_ptr->run();
}

/*plugin main functions*/
int plugin_multi_master_log_init(MYSQL_PLUGIN plugin_info)
{
    if(SELECT_REMOTE_LOCKTABLE)
    {
        mml_locktable_active=1;
        plugin_remote_locktable_receiver_ptr = new InformReceiver;
        plugin_remote_locktable_receiver_ptr->init();
        std::thread receiver(inform_receive);
        receiver.detach();
        plugin_remote_locktable_sender_ptr = new RemoteLockTable;
        plugin_remote_locktable_sender_ptr->init();

        mml_locktable_send_request_ptr = mml_locktable_send_request;
    }
    else
    {
        mml_plugin_interface_active = 1;
        plugin_trx_info_ptr = new TrxInfo;
        plugin_trx_info_ptr->init();        
        //std::cout << std::endl << "Get Group Name From Plugin Var : " << group_name_ptr << std::endl;
        //register function ptr
        mml_plugin_mtr_redo_record_add_ptr = mml_plugin_mtr_redo_record_add;
        mml_plugin_mtr_redo_record_new_ptr = mml_plugin_mtr_redo_record_new;
        mml_plugin_wr_trx_commit_ptr = mml_plugin_wr_trx_commit;
        mml_plugin_trx_start_ptr = mml_plugin_trx_start;
    }
    
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

