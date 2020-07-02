/*
 * @Author: wei
 * @Date: 2020-06-24 09:38:32
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-02 16:35:54
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/mmlp_type.h
 */

#ifndef MMLP_TYPE_HEADER
#define MMLP_TYPE_HEADER

#include<string.h>
#include<thread>
#include<string>

using ThreadID =  std::thread::id;
using redolog_lsn_t =  uint64_t;
using TrxID = uint64_t;

using plugin_mlog_id_t = int;
using plugin_space_id_t = uint32_t;
using plugin_page_no_t = uint32_t;
using plugin_page_offset_t = unsigned long;

extern char * phxpaxos_log_path;

extern unsigned long long  phxpaxos_conflict_count;
extern unsigned long long  phxpaxos_propose_count;
extern unsigned long long  phxpaxos_conflict_time;
extern unsigned long long  phxpaxos_propose_time;
extern unsigned long long  phxpaxos_other_count;


#endif