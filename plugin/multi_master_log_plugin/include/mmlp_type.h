/*
 * @Author: wei
 * @Date: 2020-06-24 09:38:32
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-24 09:56:35
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

#endif