/*
 * @Author: wei
 * @Date: 2020-07-24 17:01:02
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 14:39:40
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/trx_log.h
 */

#ifndef MMLP_TRX_LOG_HEADER
#define MMLP_TRX_LOG_HEADER

#include "mmlp_type.h"
#include "trx_log.pb.h"
#include<map>
#include<vector>
#include<set>
#include<list>
#include<mutex>

struct RedoLogRec
{
	// bool no_header; // no type
	// bool no_rec; // no rec
	plugin_mlog_id_t type;
	plugin_space_id_t space_id;
	plugin_page_no_t page_no;
	plugin_page_offset_t offset;

	char * rec;
	uint32_t rec_size;
};

#define RedoLogBufferSize(s,e) e-s+1

RedoLogRec * allocate_redolog_record(uint32_t size);
RedoLogRec * allocate_redolog_record();
int destory_redolog_record(RedoLogRec * redo);

using PageRedoMap = std::map<plugin_space_id_t, std::map<plugin_page_no_t, std::list<RedoLogRec*> > >;

class TrxLog
{
	private:
		TrxID trx_no;
		bool trx_is_started;
		bool trx_is_commited;
		bool trx_is_rollback;
		//std::list<RedoLogRec *> uncompleted_rec_list;
		std::list<RedoLogRec *> completed_rec_list;
		RedoLogRec * now_rec;

		TrxID snapshot_state;

	public:
		uint64_t us_latency;

		TrxLog()
		{
			trx_no = 0;
			snapshot_state = 0;
			trx_is_started = false;
			trx_is_commited = false;
			trx_is_rollback = false;
			us_latency = 0;
			now_rec = NULL;
		}
		~TrxLog();
		int trx_started(); // clear and start
		int wr_trx_commiting(TrxID id);
		int add_redolog_record(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset);
		int new_redolog_record(size_t size);

		int rollback_trx_log(TrxID id);

		//Tool functions
		int log_clear();
		TrxID get_trxID()
		{
			return trx_no;
		}
		bool get_rollback_status()
		{
			return trx_is_rollback;
		}
		TrxID get_snapshot_state()
		{
			return snapshot_state;
		}
		void  set_snapshot_state(TrxID id)
		{
			snapshot_state = id;
		}
		bool is_empty()
		{
			return completed_rec_list.empty();
		}

		//message encode
		int trx_log_encode_into_msg(MMLP_BRPC::LogSendRequest & res);
		int trx_log_decode_from_msg(MMLP_BRPC::LogSendRequest & res);
		int trx_log_encode_into_msg(MMLP_BRPC::LogRequireResponse & res);
		int trx_log_decode_from_msg(MMLP_BRPC::LogRequireResponse & res);
};

#endif
