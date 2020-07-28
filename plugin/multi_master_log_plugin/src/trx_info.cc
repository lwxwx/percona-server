/*
 * @Author: wei
 * @Date: 2020-06-15 10:41:24
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 16:04:10
 * @Description: trx_info and redo log functions
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/trx_info.cc
 */
#include "trx_info.h"
#include "debug.h"
#include "easylogger.h"
#if DEBUG_REDO_LOG_COLLECT
#include <iostream>
#endif

TrxInfo *plugin_trx_info_ptr = NULL;

// #include<chrono>
// #include<mutex>
// std::mutex phxpaxos_propose_mutex;
// std::mutex phxpaxos_count_mutex;


/***
 * TrxInfo
 */
TrxInfo::~TrxInfo()
{
  for(auto it = working_thread_map.begin(); it!= working_thread_map.end(); it++)
  {
    delete it->second;
  }
  for(auto it2 = global_trx_log_map.begin(); it2!= global_trx_log_map.end(); it2++)
  {
    delete it2->second;
  }
  delete send_service_handle_ptr;
}

int TrxInfo::init() {
  global_reply_status = 0;

  id_factory.init((ID_FACTORY_TYPE)SELECT_TRX_ID_ALLOCATE_TYPE);

  send_service_handle_ptr = new TrxLogServiceMessageHandle(this);
  log_transfer.init(send_service_handle_ptr,NULL);

  return 1;
}

int TrxInfo::trx_started() {
  ThreadID tid = std::this_thread::get_id();
  TrxLog *trx_redo = NULL;

  if (working_thread_map.find(tid) == working_thread_map.end()) {
    working_thread_map[tid] = NULL;
  }

  if (working_thread_map[tid] != NULL) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【trx clear】", EasyLogger::LOG_LEVEL::debug);
    }

    trx_redo = working_thread_map[tid];
  } else {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【trx started】", EasyLogger::LOG_LEVEL::debug);
    }

    trx_redo = new TrxLog();

    working_thread_map_mutex.lock();
    working_thread_map[tid] = trx_redo;
    working_thread_map_mutex.unlock();
  }

  trx_redo->trx_started();

  return 1;
}

int TrxInfo::wr_trx_commiting(TrxID local_id) {
  ThreadID tid = std::this_thread::get_id();
  TrxLog *trx_redo = NULL;

  auto tid_it = working_thread_map.find(tid);

  if (tid_it == working_thread_map.end() || tid_it->second == NULL) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(
          ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
          "【No trx start in this thread】 : " + std::to_string(local_id),
          EasyLogger::LOG_LEVEL::debug);
    }

    return -1;
  } else {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【trx commit】 : " + std::to_string(local_id),
                    EasyLogger::LOG_LEVEL::debug);
    }

    trx_redo = working_thread_map[tid];
    // TODO : 检查事务是否为读写事务（是否包含redo log）

    // TODO : 分配全局ID
    TrxID global_id = id_factory.getGlobalTrxID();

    // TODO : 日志空洞检查

    // TODO : 冲突检测

    // TODO : 全局ID赋予事务日志
    trx_redo->wr_trx_commiting(global_id);

    // TODO : 日志广播
    std::string msg = "redo log";

    uint64_t latency = 0;
    int send_ret = 1;
    if (SELECT_LOG_ASYNC_TYPE == 0) {
      send_ret = log_transfer.sync_send_log(global_id, true , *trx_redo, &latency);
    } else if (SELECT_LOG_ASYNC_TYPE == 1) {
      send_ret = log_transfer.async_send_log(global_id, true, *trx_redo, &latency);
    }

    if (DEBUG_LOG_SEND_TIME != 0) {
      if (send_ret == 1) {
        log_send_succeed_count++;
        log_send_succeed_sum_time += latency;
      } else {
        log_send_failed_count++;
        log_send_failed_sum_time += latency;
      }
    }

    if (DEBUG_TRX_TIME != 0) {
      trx_sum_time += trx_redo->us_latency;
      trx_count++;
    }
    working_thread_map[tid] = NULL;

    // rollback : delete trx_redo
    // TODO：多线程添加保护 global_trx_log_map[id] = trx_redo;
    return 1;
  }
}

int TrxInfo::mtr_redolog_record_add(plugin_mlog_id_t type,
                                    plugin_space_id_t space_id,
                                    plugin_page_no_t page_no,
                                    plugin_page_offset_t offset) {
  ThreadID tid = std::this_thread::get_id();

  if (working_thread_map.find(tid) == working_thread_map.end()) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR add:No this thread】 ",
                    EasyLogger::LOG_LEVEL::debug);
    }

    return -1;
  }
  if (working_thread_map[tid] == NULL) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR add:No this trx】 ", EasyLogger::LOG_LEVEL::debug);
    }

    return -1;
  }

  TrxLog *trx_redo = working_thread_map[tid];

  if (trx_redo->add_redolog_record(type, space_id, page_no, offset) <= 0) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR LOG ADD ERROR】: " + std::to_string(type) +
                        "; 【Space_id】 " + std::to_string(space_id) +
                        "; 【Page_no】 " + std::to_string(page_no) +
                        "; 【Offset】" + std::to_string(offset),
                    EasyLogger::LOG_LEVEL::debug);
    }
    return -1;
  } else {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR LOG TYPE】: " + std::to_string(type) +
                        "; 【Space_id】 " + std::to_string(space_id) +
                        "; 【Page_no】 " + std::to_string(page_no) +
                        "; 【Offset】" + std::to_string(offset),
                    EasyLogger::LOG_LEVEL::debug);
    }
    return 1;
  }
  //    return 1;
}

int TrxInfo::mtr_redolog_record_new(size_t size) {
  ThreadID tid = std::this_thread::get_id();

  if (working_thread_map.find(tid) == working_thread_map.end()) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR new:No this thread】 ",
                    EasyLogger::LOG_LEVEL::debug);
    }

    return -1;
  }
  if (working_thread_map[tid] == NULL) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR new:No this trx】 ", EasyLogger::LOG_LEVEL::debug);
    }

    return -1;
  }

  TrxLog *trx_redo = working_thread_map[tid];

  if (trx_redo->new_redolog_record(size) <= 0) {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR LOG NEW ERROR】: " + std::to_string(size),
                    EasyLogger::LOG_LEVEL::debug);
    }
    return -1;
  } else {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【MTR LOG SIZE】: " + std::to_string(size),
                    EasyLogger::LOG_LEVEL::debug);
    }
    return 1;
  }
}

int TrxInfo::check_global_trx_id(TrxID id)
{
  int result = 0;
  global_trx_log_map_mutex.lock_shared();

  if(global_trx_log_map.find(id) != global_trx_log_map.end())
  {
    result = 1;
  }

  global_trx_log_map_mutex.unlock_shared();
  return result;
}

int TrxInfo::insert_global_trx_log(TrxLog * log)
{
  int result = 0;
  TrxID id = log->get_trxID();
  global_trx_log_map_mutex.lock();

  if(global_trx_log_map.find(id) == global_trx_log_map.end())
  {
    global_trx_log_map[id] = log;
    result = 1;
  }
  else
  {
    result = -1;
  }

  global_trx_log_map_mutex.unlock();
  return result;
}

int TrxInfo::handle_trxlog_message(MMLP_BRPC::LogSendRequest & request)
{
  if(check_global_trx_id(request.trxid()) > 0)
  {
    return 0;
  }

  TrxLog * log = new TrxLog();
  log->trx_log_decode_from_msg(request);

  if(insert_global_trx_log(log) <= 0)
  {
    return -1;
  }

  return 1;
}
