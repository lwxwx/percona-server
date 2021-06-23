/*
 * @Author: wei
 * @Date: 2020-06-15 10:41:24
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 16:04:10
 * @Description: trx_info and redo log functions
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/trx_info.cc
 */
#include "trx_info.h"
// #include <bits/stdint-uintn.h>
#include <cstdint>
#include "conflict_handle.h"
#include "debug.h"
#include "easylogger.h"
#include "mmlp_type.h"
#include "trx_log.h"
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
  //@liu-erase global_reply_status是指全局日志回放到哪里了
  // global_reply_status = -1;
  //@liu-try test
  global_reply_status = (TrxID){-1,-1,-1};
  //liu-try-end

  id_factory.init((ID_FACTORY_TYPE)SELECT_TRX_ID_ALLOCATE_TYPE);

  send_service_handle_ptr = new TrxLogServiceMessageHandle(this);
  require_request_handle_ptr =  new TrxLogRequireRequestHandle(this);
  require_response_handle_ptr = new TrxLogRequireResponseHandle(this);
  log_transfer.init(send_service_handle_ptr,require_request_handle_ptr);

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

  trx_redo->set_snapshot_state(global_reply_status);
  trx_redo->trx_started();

  //id_factory.requestGlobalTrxID();

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
      //@liu-erase
      // EasyStringLog(
      //     ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
      //     "【No trx start in this thread】 : " + std::to_string(local_id),
      //     EasyLogger::LOG_LEVEL::debug);
      //@liu-try test
      EasyStringLog(
          ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
          "【No trx start in this thread】 : " + std::to_string(local_id.p_id)+std::to_string(local_id.s_id)+std::to_string(local_id.m_id),
          EasyLogger::LOG_LEVEL::debug);
      //liu-try-end
    }

    return -1;
  } else {
    if (DEBUG_REDO_LOG_COLLECT != 0) {
      std::stringstream ss;
      ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
      //@liu-erase
      // EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
      //               "【trx commit】 : " + std::to_string(local_id),
      //               EasyLogger::LOG_LEVEL::debug);
      //@liu-try test
      EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
                    "【trx commit】 : " + std::to_string(local_id.p_id)+std::to_string(local_id.s_id)+std::to_string(local_id.m_id),
                    EasyLogger::LOG_LEVEL::debug);
      //liu-try-end
    }

    trx_redo = working_thread_map[tid];
    // 检查事务是否为读写事务（是否包含redo log）

    //#### Assign Global Transaction ID
    TrxID global_id = id_factory.getGlobalTrxID();
	  trx_hole_set.add_local_hole((UnifID){global_id.p_id, global_id.p_id == 0 ? global_id.m_id : global_id.s_id});
    // std::cout << global_id << std::endl;

    //#### Trx Log Hole Check
    //@liu-erase
	// if(trx_redo->get_snapshot_state() > 0)
  //@liu-try test
  if(trx_redo->get_snapshot_state() > (TrxID){-1,-1,-1})
  //liu-try-end
		global_trx_hole_check(trx_redo->get_snapshot_state(), global_id);
    if(DEBUG_CODE != 0)
    {
      std::cout << "trxid " << global_id << "final , come to comflict test\n";
    }
    
	// 冲突检测
  uint64_t conflict_latency = (std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()))
                     .count();
	int conflict_res = ConflictHandle::arg_detect(global_id);
	conflict_latency = (std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()))
                     .count() - conflict_latency;
	if(conflict_res > 1)
	{
		if(conflict_res == 2)
		{
			conflict_row_failed = conflict_row_failed + 1;
		}
		conflict_failed_count = conflict_failed_count + 1;
		conflict_failed_time = conflict_failed_time + conflict_latency;
	}
	else
	{
		if(conflict_res == 1)
		{
			conflict_page_failed = conflict_page_failed + 1;
		}
		conflict_succeed_count = conflict_succeed_count + 1; 
		conflict_succeed_time = conflict_succeed_time + conflict_latency;
	}

    //#### Commit or Rollback Trx
	if(conflict_res)
	{
		trx_redo->rollback_trx_log(global_id);
	}
	else
	{
		trx_redo->wr_trx_commiting(global_id);
		if(global_id > global_reply_status)
			global_reply_status  = global_id;
	}
	std::cout <<"complement_hole" << global_id << " " << trx_hole_set.complement_hole((UnifID){global_id.p_id, global_id.p_id == 0 ? global_id.m_id : global_id.s_id});
	insert_global_trx_log(trx_redo);
    
	//#### Log Broadcast
    uint64_t latency = 0;
    int send_ret = 1;
    if (SELECT_LOG_ASYNC_TYPE == 0) {
      send_ret = log_transfer.sync_send_log(*trx_redo, &latency);
    } else if (SELECT_LOG_ASYNC_TYPE == 1) {
      send_ret = log_transfer.async_send_log(*trx_redo, &latency);
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

  //first redo log need a global id
  if(trx_redo->is_empty())
  {
	  id_factory.requestGlobalTrxID();
  }

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

  if(global_trx_log_map.find((UnifID){id.p_id,id.p_id==0?id.m_id:id.s_id}) != global_trx_log_map.end())
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

  UnifID tmp_id = (UnifID){id.p_id, id.p_id == 0 ? id.m_id : id.s_id};
  if(DEBUG_CODE != 0)
  {
    std::cout << "start insert uid " << id << std::endl;
  }
  if(global_trx_log_map.find(tmp_id) == global_trx_log_map.end())
  {
    if(DEBUG_CODE != 0)
    {
      std::cout << "success insert uid " << id << std::endl;
    }
    global_trx_log_map[tmp_id] = log;
    trx_hole_set.erase_hole(tmp_id);
    if(id > global_reply_status)
    {
      global_reply_status = id;
    }
      result = 1;
  }
  else
  {
    result = -1;
  }

  if(DEBUG_CODE != 0)
  {
    std::cout << "global trx map :"  ;
    for(auto &it : global_trx_log_map)
      std::cout << it.first << " ";
    // std::cout << std::endl << "hole set part" << tmp_id.p_id << " :";
    // for(auto &it : trx_hole_set)
  }
  global_trx_log_map_mutex.unlock();
  return result;
}

//@liu-try test
// bool TrxInfo::findfrom_global_trx_log_map(int32_t pid, int64_t sid)
// {
//   for(std::map<TrxID,TrxLog *>::iterator it = global_trx_log_map.begin(); it != global_trx_log_map.end(); it++)
//   {
//     if(it->first.p_id == pid && it->first.s_id == sid) return true;
//   }
//   return false;
// }

void TrxInfo::find_in_global_trx_log_map(UnifID id)
{
	int tmp_result = 0;
	uint64_t latency = 0;
  if(DEBUG_CODE != 0)
  {
    std::cout << "checking unif_id " << id << ", global_trx_log_map_size = " << global_trx_log_map.size() << std::endl;
    for(auto &uid : global_trx_log_map)
      std::cout << uid.first << ",";
    std::cout << std::endl;
  }
  if(global_trx_log_map.find(id) == global_trx_log_map.end())
  {
    if(DEBUG_CODE != 0)
    {
      std::cout << "have hole [" << id << "]\n";
    }
    tmp_result = trx_hole_set.find_hole(id);
    if(tmp_result == 1)
    {
      int require_ret = log_transfer.async_require_log(id, &latency, require_response_handle_ptr);
      if(require_ret == 1)
      {
        log_require_succeed_count++;
        log_require_succeed_sum_time += latency; 
      }
      else
      {
        log_require_failed_count++;
        log_require_failed_sum_time += latency;
      }
    }
  }
}
//检查这个区间里面有没有空洞
int TrxInfo::global_trx_hole_check(TrxID start_id, TrxID commit_id)
{
	int tmp_result = 0;
	uint64_t latency = 0;
	global_trx_log_map_mutex.lock_shared();
  //@liu-erase
	// for(TrxID id = start_id; id < commit_id ; id++)
  //@liu-try test
  if(DEBUG_CODE != 0)
  {
    std::cout << "part" << commit_id.p_id << ", start id = " <<start_id << ", end id = " <<commit_id << std::endl;
  }
  if(commit_id.p_id == 0)
  {
    //P0
    for(int i = start_id.m_id; i < commit_id.m_id; i++)
    {
      UnifID id = (UnifID){0, i};      
      find_in_global_trx_log_map(id);
    }
    //pn
    for(auto either_part_lastest_txn : each_part_laster_tsn[commit_id])
    {
      if(DEBUG_CODE != 0)
      {
        std::cout << "wait for part" << either_part_lastest_txn.p_id << ", lastest id = " << either_part_lastest_txn <<std::endl;
      }
      for(int i = 0; i < either_part_lastest_txn.s_id; i++)
      {
        UnifID id = (UnifID){either_part_lastest_txn.p_id, i};
        find_in_global_trx_log_map(id);
      }
    }
  }
  else
  {
    //pn hole
    for(int i = start_id.s_id; i < commit_id.s_id; i++)
    {
      UnifID id = (UnifID){commit_id.p_id, i};
      if(DEBUG_CODE != 0)
      {
        std::cout << "checking unif_id " << id << ", global_trx_log_map_size = " << global_trx_log_map.size() << std::endl;
        for(auto &uid : global_trx_log_map)
          std::cout << uid.first << ",";
        std::cout << std::endl;
      }
      if(global_trx_log_map.find(id) == global_trx_log_map.end())
      {
        if(DEBUG_CODE != 0)
        {
          std::cout << "have hole [" << id << "]\n";
        }
        tmp_result = trx_hole_set.find_hole(id);
        if(tmp_result == 1)
        {
          int require_ret = log_transfer.async_require_log(id, &latency, require_response_handle_ptr);
          if(require_ret == 1)
          {
            log_require_succeed_count++;
            log_require_succeed_sum_time += latency; 
          }
          else
          {
            log_require_failed_count++;
            log_require_failed_sum_time += latency;
          }
        }
      }
    }
    //p0 hole
    for(int i = start_id.m_id; i < commit_id.m_id; i++)
    {
      UnifID id = (UnifID){0, i};
      // find_in_global_trx_log_map(id);
      if(DEBUG_CODE != 0)
      {
        std::cout << "checking unif_id " << id << ", global_trx_log_map_size = " << global_trx_log_map.size() << std::endl;
        for(auto &uid : global_trx_log_map)
          std::cout << uid.first << ",";
        std::cout << std::endl;
      }
      if(global_trx_log_map.find(id) == global_trx_log_map.end())
      {
        if(DEBUG_CODE != 0)
        {
          std::cout << "have hole [" << id << "]\n";
        }
        tmp_result = trx_hole_set.find_hole(id);
        if(tmp_result == 1)
        {
          int require_ret = log_transfer.async_require_log(id, &latency, require_response_handle_ptr);
          if(require_ret == 1)
          {
            log_require_succeed_count++;
            log_require_succeed_sum_time += latency; 
          }
          else
          {
            log_require_failed_count++;
            log_require_failed_sum_time += latency;
          }
        }
      }
    }
  }
  //liu-try-end

	global_trx_log_map_mutex.unlock_shared();

	// trx_hole_set.wait_hole(commit_id);
  if(DEBUG_CODE != 0)
  {
    std::cout << "wait phase start" << std::endl;
  }
  if(commit_id.p_id == 0)
  {
    trx_hole_set.wait_hole((UnifID){0,commit_id.m_id});
    for(auto part_it :  each_part_laster_tsn[commit_id])
      trx_hole_set.wait_hole((UnifID){part_it.p_id,part_it.s_id});
  }
  else
  {
    trx_hole_set.wait_hole((UnifID){0,commit_id.m_id});
    if(DEBUG_CODE != 0)
    {
      std::cout << "wait until [0-" << commit_id.m_id << "]" << std::endl;
    }
    trx_hole_set.wait_hole((UnifID){commit_id.p_id,commit_id.s_id-1});
    if(DEBUG_CODE != 0)
    {
      std::cout << "wait until [" << commit_id.p_id << "-" << commit_id.s_id<< "]" << std::endl;
    }
  }

	return 1;
}

int TrxInfo::handle_log_send_request(MMLP_BRPC::LogSendRequest & request)
{
  if(DEBUG_CODE != 0)
  {
    std::cout << "receive log ?";
  }
  if(check_global_trx_id((TrxID){request.trxid_part_id(), request.trxid_s_id(), request.trxid_m_id()}) > 0)
  {
    return 0;
  }
  TrxLog * log = new TrxLog();
  log->trx_log_decode_from_msg(request);

  if(DEBUG_CODE != 0)
  {
    std::cout << "yes!" << log->get_trxID() << std::endl;
  }

  if(insert_global_trx_log(log) <= 0)
  {
    return -1;
  }

  return 1;
}

int TrxInfo::handle_log_require_request(MMLP_BRPC::LogRequireRequest &request, MMLP_BRPC::LogRequireResponse &response)
{
	TrxLog * trx_log = NULL;
	global_trx_log_map_mutex.lock_shared();
	// if(global_trx_log_map.find(request.trxid()) != global_trx_log_map.end() )
  // TrxID id = (TrxID){request.trxid_part_id(), request.trxid_s_id(), request.trxid_m_id()};
  UnifID id = (UnifID){request.trxid_part_id(), request.trxid_own_id()};
  if(global_trx_log_map.find(id) != global_trx_log_map.end() )
	{
    if(DEBUG_CODE != 0)
    {
      std::cout << "can send unif_id " << id << std::endl;
    }
		trx_log = global_trx_log_map[id];
	}
	global_trx_log_map_mutex.unlock_shared();

	if(trx_log == NULL)
	{
    if(DEBUG_CODE != 0)
    {
      std::cout << "don't have unif_id " << id << std::endl;
    }
		// response.set_trxid(request.trxid());
    response.set_trxid_part_id(request.trxid_part_id());
    response.set_trxid_s_id(request.trxid_own_id());
    response.set_trxid_m_id(request.trxid_own_id());
		response.set_is_valid(false);
		response.set_require_reply(-1);
		return -1;
	}
	else
	{
		response.set_require_reply(1);
    response.set_trxid_part_id(trx_log->get_trxID().p_id);
    response.set_trxid_s_id(trx_log->get_trxID().s_id);
    response.set_trxid_m_id(trx_log->get_trxID().m_id);
		trx_log->trx_log_encode_into_msg(response);
		return 1;
	}
}

int TrxInfo::handle_log_require_response(MMLP_BRPC::LogRequireResponse &response)
{
	if(check_global_trx_id((TrxID){response.trxid_part_id(), response.trxid_s_id(), response.trxid_m_id()}) == 1 )
	{
		return 0;
	}

	TrxLog * log = new TrxLog();
	log->trx_log_decode_from_msg(response);

	if(insert_global_trx_log(log)<=0)
	{
		return -1;
	}

  if(DEBUG_CODE != 0)
  {
    std::cout << "insert " << log->get_trxID() << "to global " <<std::endl;
  }
	return 1;
}