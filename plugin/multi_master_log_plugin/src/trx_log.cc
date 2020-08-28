/*
 * @Author: wei
 * @Date: 2020-07-28 10:36:04
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 16:10:14
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/trx_log.cc
 */

#include "trx_log.h"
#include <cstring>
#include "debug.h"
#include "easylogger.h"
#include "mmlp_type.h"
#include "trx_log.pb.h"
#if DEBUG_REDO_LOG_COLLECT
#include <iostream>
#endif

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
  trx_is_rollback = false;
  trx_no = 0;
  if (now_rec != NULL) {
    destory_redolog_record(now_rec);
    now_rec = NULL;
  }
  return 1;
}

int TrxLog::rollback_trx_log(TrxID id)
{
    log_clear();
    trx_is_rollback = true;
    trx_no = id;
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

int TrxLog::trx_log_encode_into_msg(MMLP_BRPC::LogSendRequest & res)
{
  if(!trx_is_commited || res.log_msg_size() > 0)
  {
    return -1;
  }

  res.set_trxid(trx_no);

  if(trx_is_rollback)
  {
    res.set_is_valid(false);
    return 0;
  }

  for(auto it = completed_rec_list.begin(); it!= completed_rec_list.end(); it++)
  {
    MMLP_BRPC::TrxLogMsg * msg_ptr = res.add_log_msg();
    msg_ptr->set_type((*it)->type);
    msg_ptr->set_space_id((*it)->space_id);
    msg_ptr->set_page_no((*it)->page_no);
    msg_ptr->set_offset((*it)->offset);
    msg_ptr->set_rec((*it)->rec,(*it)->rec_size);
  }
  return 1;
}

int TrxLog::trx_log_encode_into_msg(MMLP_BRPC::LogRequireResponse & res)
{
	if(!trx_is_commited || res.log_msg_size() > 0)
	{
		return -1;
	}

	res.set_trxid(trx_no);

	if(trx_is_rollback)
	{
		res.set_is_valid(false);
		return 0;
	}
	
	res.set_is_valid(true);
	MMLP_BRPC::TrxLogMsg * msg_ptr = NULL;
	for(auto it = completed_rec_list.begin(); it != completed_rec_list.end() ; it++)
	{
		msg_ptr = res.add_log_msg();
		msg_ptr->set_type((*it)->type);
		msg_ptr->set_space_id((*it)->space_id);
		msg_ptr->set_page_no((*it)->page_no);
		msg_ptr->set_offset((*it)->offset);
		msg_ptr->set_rec((*it)->rec,(*it)->rec_size);
	}
	return 1;
}

int TrxLog::trx_log_decode_from_msg(MMLP_BRPC::LogSendRequest & res)
{
  if(trx_is_commited || trx_is_started || !completed_rec_list.empty())
  {
    return -1;
  }

  //trx_no = res.trxid();

  if(!res.is_valid())
  {
    rollback_trx_log(res.trxid());
    return 0;
  }

  wr_trx_commiting(res.trxid());
  us_latency = 0;
  RedoLogRec * tmp_rec;
  for(int i = 0; i < res.log_msg_size();i++)
  {
    tmp_rec = allocate_redolog_record(res.log_msg(i).rec().size());
    memcpy(tmp_rec->rec,res.log_msg(i).rec().c_str(),tmp_rec->rec_size);
    tmp_rec->type = res.log_msg(i).type();
    tmp_rec->space_id = res.log_msg(i).space_id();
    tmp_rec->page_no = res.log_msg(i).page_no();
    tmp_rec->offset = res.log_msg(i).offset();

    completed_rec_list.push_back(tmp_rec);
  }

  return 1;
}

int TrxLog::trx_log_decode_from_msg(MMLP_BRPC::LogRequireResponse & res)
{
	if(trx_is_commited || trx_is_started || !completed_rec_list.empty())
	{
		return -1;
	}

//	trx_no = res.trxid();

	if(!res.is_valid())
	{
		rollback_trx_log(res.trxid());
		return 0;
	}

	wr_trx_commiting(res.trxid());
	RedoLogRec * tmp_rec;
	us_latency = 0;
	for(int i = 0; i < res.log_msg_size(); i++)
	{
		tmp_rec =  allocate_redolog_record(res.log_msg(i).rec().size());
		memcpy(tmp_rec->rec,res.log_msg(i).rec().c_str(), tmp_rec->rec_size);
		tmp_rec->type = res.log_msg(i).type();
		tmp_rec->space_id = res.log_msg(i).space_id();
		tmp_rec->page_no = res.log_msg(i).page_no();
		tmp_rec->offset = res.log_msg(i).offset();

		completed_rec_list.push_back(tmp_rec);
	}

	return 1;
}
