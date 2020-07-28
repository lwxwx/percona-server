/*
 * @Author: wei
 * @Date: 2020-06-15 09:59:12
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-28 16:07:16
 * @Description: trx_info、trx_redolog
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/trx_info.h
 */

#ifndef TRX_INFO_HEADER
#define TRX_INFO_HEADER


#include "trx_log.h"
#include "id_allocate.h"
#include "log_transfer.h"

#include <shared_mutex>

//#define GET_INFO_PTR(x) (int*)(x)

/* Return Value : Int
	1 : success
	0 : none
	-1 : error
*/


class TrxInfo
{
	private:
		// Redo Log
		std::mutex  working_thread_map_mutex;
		std::shared_mutex  global_trx_log_map_mutex;
		std::map<ThreadID,TrxLog * > working_thread_map;
		std::map<TrxID,TrxLog *> global_trx_log_map;

		TrxID global_reply_status;
		GlobalTrxIDFactory id_factory;
		LogTransfer log_transfer;

		MessageHandle * send_service_handle_ptr;

		int check_global_trx_id(TrxID id);
		int insert_global_trx_log(TrxLog * log);

	public:
		~TrxInfo();
		int init();
		// Redo Log
		// call in one thread
		int trx_started();
		int wr_trx_commiting(TrxID id);
		int mtr_redolog_record_add(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset);
		int mtr_redolog_record_new(size_t size);

		int handle_trxlog_message(MMLP_BRPC::LogSendRequest & request);
		int global_trx_hole_check();
};

extern TrxInfo * plugin_trx_info_ptr;

class TrxLogServiceMessageHandle: public MessageHandle
{
	private:
	TrxInfo * trx_info_ptr;

	public:

	TrxLogServiceMessageHandle(TrxInfo * trx_info)
	{
		trx_info_ptr = trx_info;
	}

	virtual int handle(void * msg)
	{
		return trx_info_ptr->handle_trxlog_message(*((MMLP_BRPC::LogSendRequest*)msg));
	}
};

// Redo Log replay

#endif
