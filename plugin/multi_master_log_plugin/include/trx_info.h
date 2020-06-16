/*
 * @Author: wei
 * @Date: 2020-06-15 09:59:12
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-16 10:01:06
 * @Description: trx_info、trx_redolog
 * @FilePath: /multi_master_log_plugin/include/trx_info.h
 */

#ifndef TRX_INFO_HEADER
#define TRX_INFO_HEADER

storage/innobase/include/log0types.h
#include "../../../storage/innobase/include/log0types.h"
#include "../../../storage/innobase/include/trx0types.h"

#include<map>
#include<vector>
#include<set>
#include<list>
#include<string.h>
#include<thread>

//#define GET_INFO_PTR(x) (int*)(x)

using ThreadID =  std::thread::id;
using redolog_lsn_t =  lsn_t;
using TrxID = trx_id_t;

/* Return Value : Int
	1 : success
	0 : none
	-1 : error
*/

struct RedoLogSection
{
	redolog_lsn_t start_lsn;
	redolog_lsn_t end_lsn;
	void * buf;
};

#define RedoLogBufferSize(s,e) e-s+1

RedoLogSection * allocate_redolog_section(redolog_lsn_t start_lsn,redolog_lsn_t end_lsn);
int copy_log_buffer(RedoLogSection * redo,void * log_buffer);  //copy (end_lsn-start_lsn) bytes
int destory_redolog_section(RedoLogSection * redo);

class TrxLog
{
	private:
		TrxID trx_no;
		bool trx_is_started;
		bool trx_is_commited;
		std::list<RedoLogSection * > redolog_list;

	public:
		TrxLog()
		{
			trx_no = 0;
			trx_is_started = false;
			trx_is_commited = false;
		}
		~TrxLog();
		int trx_started();
		int trx_commiting(TrxID id);
		int add_redolog_section(redolog_lsn_t start_lsn,redolog_lsn_t end_lsn,void * log_buf);
		//RedoLogSection * get_redolog_section_from_no(int i);
};


class TrxInfo
{
	private:
		// Redo Log
		std::map<ThreadID,TrxLog * > working_thread_map;
		std::map<TrxID,TrxLog *> complete_trx_redo_map;

	public:
		// Redo Log
		// call in one thread
		int trx_started();
		int trx_commiting(TrxID id);
		int mtr_redolog_collect(redolog_lsn_t start_lsn, redolog_lsn_t end_lsn,void * log_buf);
};

extern TrxInfo * plugin_trx_info_ptr;

// Redo Log replay

#endif
