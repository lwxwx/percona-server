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


#include "mmlp_type.h"
#include "trx_log.h"
#include "id_allocate.h"
#include "log_transfer.h"
#include "trx_log.pb.h"
#include "trx_log_hole.h"

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
		TrxLogHoleSet trx_hole_set;

		MessageHandle * send_service_handle_ptr;
		MessageHandle * require_request_handle_ptr;
		MessageHandle * require_response_handle_ptr;
		
		//check trx_id in global_trx_log_map
		int check_global_trx_id(TrxID id);
		//insert trx log into global_trx_log_map
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

		//check trx hole and wait for completion
		int global_trx_hole_check(TrxID start_id,TrxID commit_id); // start_id = -1 : start from head
		
		// Message Handle functions
		int handle_log_send_request(MMLP_BRPC::LogSendRequest & request);
		int handle_log_require_request(MMLP_BRPC::LogRequireRequest & request,MMLP_BRPC::LogRequireResponse & response);
		int handle_log_require_response(MMLP_BRPC::LogRequireResponse & response);
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

	virtual int handle(void * request,void * response)
	{
		return trx_info_ptr->handle_log_send_request(*((MMLP_BRPC::LogSendRequest*)request));
	}
};

class TrxLogRequireRequestHandle:public MessageHandle
{
	private:
		TrxInfo * trx_info_ptr;
	public:
		TrxLogRequireRequestHandle(TrxInfo * trx_info)
		{
			trx_info_ptr = trx_info;
		}

		virtual int handle(void * request,void * response)
		{
			return trx_info_ptr->handle_log_require_request(*(MMLP_BRPC::LogRequireRequest*)request,*(MMLP_BRPC::LogRequireResponse*)response);
		}

};

class TrxLogRequireResponseHandle: public MessageHandle
{
	private:
		TrxInfo * trx_info_ptr;
	public:
		TrxLogRequireResponseHandle(TrxInfo * trx_info)
		{
			trx_info_ptr = trx_info;
		}

		virtual int handle(void * request,void * response)
		{
			return trx_info_ptr->handle_log_require_response(*(MMLP_BRPC::LogRequireResponse*)response);
		}
};


// Redo Log replay

#endif
