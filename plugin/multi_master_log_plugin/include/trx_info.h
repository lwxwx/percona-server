/*
 * @Author: wei
 * @Date: 2020-06-15 09:59:12
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-16 09:40:46
 * @Description: trx_info、trx_redolog
 * @FilePath: /multi_master_log_plugin/include/trx_info.h
 */

#ifndef TRX_INFO_HEADER
#define TRX_INFO_HEADER

#include<map>
#include<vector>
#include<set>
#include<list>
#include<mutex>

#include "mmlp_type.h"
#include "id_allocate.h"
#include "log_transfer.h"

//#define GET_INFO_PTR(x) (int*)(x)

/* Return Value : Int
	1 : success
	0 : none
	-1 : error
*/

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
		//std::list<RedoLogRec *> uncompleted_rec_list;
		std::list<RedoLogRec *> completed_rec_list;
		RedoLogRec * now_rec;

	public:
		uint64_t us_latency;

		TrxLog()
		{
			trx_no = 0;
			trx_is_started = false;
			trx_is_commited = false;
			now_rec = NULL;
		}
		~TrxLog();
		int trx_started(); // clear and start
		int wr_trx_commiting(TrxID id);
		int add_redolog_record(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset);
		int new_redolog_record(size_t size);

		int log_clear();
		int trx_log_encode(std::string & msg,bool wr_commit);
		int trx_log_decode(std::string & msg);
};

class TrxInfo
{
	private:
		// Redo Log
		std::mutex  working_thread_map_mutex;
		std::mutex  global_trx_redo_map_mutex;
		std::map<ThreadID,TrxLog * > working_thread_map;
		std::map<TrxID,TrxLog *> global_trx_redo_map;

		TrxID global_reply_status;
		GlobalTrxIDFactory id_factory;
		LogTransfer log_transfer;
	public:
		int init();
		// Redo Log
		// call in one thread
		int trx_started();
		int wr_trx_commiting(TrxID id);
		int mtr_redolog_record_add(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset);
		int mtr_redolog_record_new(size_t size);

		int global_trx_hole_check();
};

extern TrxInfo * plugin_trx_info_ptr;

// Redo Log replay

#endif
