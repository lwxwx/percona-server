/*
 * @Author: wei
 * @Date: 2020-06-24 09:38:32
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 14:31:22
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/mmlp_type.h
 */

#ifndef MMLP_TYPE_HEADER
#define MMLP_TYPE_HEADER

#include<string.h>
#include<thread>
#include<string>
#include<stdint.h>

// #include "trx_log.pb.h"


#define LOG_DIR "/tmp/mmlp/"
#define LOG_FILE_SUFFIX ".log"
#define REDO_LOG_FILE_NAME "redo_collect_log"

using ThreadID =  std::thread::id;
using redolog_lsn_t =  uint64_t;
using TrxID = uint64_t;

using plugin_mlog_id_t = int;
using plugin_space_id_t = uint32_t;
using plugin_page_no_t = uint32_t;
using plugin_page_offset_t = unsigned long;

extern char * group_name_ptr;
extern char * phxpaxos_local_node_ptr;
extern char * phxpaxos_peer_nodes_ptr;
extern char * brpc_local_node_ptr;
extern char * brpc_peer_nodes_ptr;

extern int DEBUG_REDO_LOG_COLLECT;
extern int DEBUG_PHXPAXOS_PRINT;
extern int DEBUG_PHXPAXOS_TIME;
extern int DEBUG_SLICE_TIME;
extern int DEBUG_REMOTE_ID_TIME;
extern int DEBUG_LOG_SEND_TIME;
extern int DEBUG_TRX_TIME;
extern int DEBUG_LOG_REQUIRE_TIME;
extern int DEBUG_CONFLICT_TIME;

extern int SELECT_LOG_ASYNC_TYPE;
extern int SELECT_TRX_ID_ALLOCATE_TYPE;
extern int SELECT_CONFLICT_HANDLE_TYPE;

extern char * remote_id_server_addr;
extern unsigned long long remote_id_handle_time;
extern unsigned long long remote_id_handle_count;
extern unsigned long long remote_id_get_time;
extern unsigned long long remote_id_get_count;
extern unsigned long long remote_id_over_wait_count;

extern unsigned long slice_node_no;
extern unsigned long long slice_gen_time;
extern unsigned long long slice_gen_count;

extern char * phxpaxos_log_path;

extern unsigned long long  phxpaxos_conflict_count;
extern unsigned long long  phxpaxos_propose_count;
extern unsigned long long  phxpaxos_conflict_time;
extern unsigned long long  phxpaxos_propose_time;
extern unsigned long long  phxpaxos_other_count;

extern unsigned long long log_send_succeed_count;
extern unsigned long long log_send_succeed_sum_time;
extern unsigned long long log_send_failed_count;
extern unsigned long long log_send_failed_sum_time;
extern unsigned long long log_send_async_rpc_count;
extern unsigned long long log_send_async_rpc_time;
extern unsigned long long log_send_async_rpc_failed_count;
extern unsigned long long log_send_async_rpc_failed_time;

extern unsigned long long log_require_succeed_count;
extern unsigned long long log_require_succeed_sum_time;
extern unsigned long long log_require_failed_count;
extern unsigned long long log_require_failed_sum_time; 
extern unsigned long long log_require_async_rpc_count;
extern unsigned long long log_require_async_rpc_time;
extern unsigned long long log_require_async_rpc_failed_count;
extern unsigned long long log_require_async_rpc_failed_time;

extern unsigned long long conflict_succeed_time;
extern unsigned long long conflict_succeed_count;
extern unsigned long long conflict_failed_count;
extern unsigned long long conflict_failed_time;

extern unsigned long long conflict_page_failed;
extern unsigned long long conflict_row_failed;

extern unsigned long long conflict_page_percent;
extern unsigned long long conflict_row_percent;
extern unsigned long long conflict_trx_length;
extern unsigned long long conflict_detect_method;
extern unsigned long long conflict_detect_level;

extern unsigned long long trx_count;
extern unsigned long long trx_sum_time;

#endif
