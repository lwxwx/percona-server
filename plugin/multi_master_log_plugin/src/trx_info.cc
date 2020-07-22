/*
 * @Author: wei
 * @Date: 2020-06-15 10:41:24
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-16 09:58:48
 * @Description: trx_info and redo log functions
 * @FilePath: /multi_master_log_plugin/src/trx_info.cc
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
 * Redo Log Section
 */

RedoLogRec *allocate_redolog_record(uint32_t size) {
  RedoLogRec *result = new RedoLogRec;
  // result->no_header = true;
  // result->no_rec = true;
  result->rec = new char[size];
  result->rec_size = size;
  return result;
}

RedoLogRec *allocate_redolog_record() {
  RedoLogRec *result = new RedoLogRec;
  // result->no_header = true;
  // result->no_rec = true;
  result->rec = NULL;
  result->rec_size = 0;
  return result;
}

int destory_redolog_record(RedoLogRec *redo) {
  if (redo == NULL) return -2;

  if (redo->rec != NULL) {
    delete[] redo->rec;
    return 1;
  } else {
    return -1;
  }
}

/***
 * TrxLog
 */
TrxLog::~TrxLog() {
  // for(auto it = uncompleted_rec_list.begin(); it !=
  // uncompleted_rec_list.end(); it++)
  // {
  //     destory_redolog_record(*it);
  // }
  for (auto it = completed_rec_list.begin(); it != completed_rec_list.end();
       it++) {
    destory_redolog_record(*it);
  }
}

int TrxLog::log_clear() {
  // for(auto it = uncompleted_rec_list.begin(); it !=
  // uncompleted_rec_list.end(); it++)
  // {
  //     destory_redolog_record(*it);
  // }
  // uncompleted_rec_list.clear();
  for (auto it = completed_rec_list.begin(); it != completed_rec_list.end();
       it++) {
    destory_redolog_record(*it);
  }
  completed_rec_list.clear();
  trx_is_started = false;
  trx_is_commited = false;
  trx_no = 0;
  if (now_rec != NULL) {
    destory_redolog_record(now_rec);
    now_rec = NULL;
  }
  return 1;
}

int TrxLog::trx_started() {
  log_clear();
  trx_is_started = true;
  if (DEBUG_TRX_TIME != 0)
    us_latency = (std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()))
                     .count();
  return 1;
}

int TrxLog::wr_trx_commiting(TrxID id) {
  trx_no = id;
  trx_is_commited = true;
  if (DEBUG_TRX_TIME != 0)
    us_latency = (std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()))
                     .count() -
                 us_latency;
  return 1;
}

int TrxLog::add_redolog_record(plugin_mlog_id_t type,
                               plugin_space_id_t space_id,
                               plugin_page_no_t page_no,
                               plugin_page_offset_t offset) {
  // RedoLogRec * rec = allocate_redolog_record();
  if (now_rec == NULL) {
    return -1;
  }
  now_rec->type = type;
  now_rec->space_id = space_id;
  now_rec->page_no = page_no;
  now_rec->offset = offset;
  // now_rec->no_header = false;
  // now_rec->no_rec = true;
  // rec->rec_size = size;
  completed_rec_list.push_back(now_rec);
  now_rec = NULL;

  return 1;
}

int TrxLog::new_redolog_record(size_t size) {
  if (now_rec != NULL) {
    destory_redolog_record(now_rec);
    now_rec = NULL;
    return -1;
  }
  now_rec = allocate_redolog_record(size);

  return 1;
}

/***
 * TrxInfo
 */
int TrxInfo::init() {
  global_reply_status = 0;

  id_factory.init((ID_FACTORY_TYPE)SELECT_TRX_ID_ALLOCATE_TYPE);

  log_transfer.init();
  // m_paxos.init(local_str,peers_str);
  // m_paxos.RunPaxos();
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
      send_ret = log_transfer.sync_send_log(global_id, true , msg, &latency);
    } else if (SELECT_LOG_ASYNC_TYPE == 1) {
      send_ret = log_transfer.async_send_log(global_id, true, msg, &latency);
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
    // TODO：多线程添加保护 global_trx_redo_map[id] = trx_redo;
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
