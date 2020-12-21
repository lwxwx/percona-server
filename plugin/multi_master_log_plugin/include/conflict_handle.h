/*
 * @Author: wei
 * @Date: 2020-06-28 17:00:34
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-07 15:09:33
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/conflict_handle.h
 */

#ifndef MMLP_CONFLICT_HANDLE_HEADER
#define MMLP_CONFLICT_HANDLE_HEADER

#include <list>
#include <map>
#include <vector>
#include "mmlp_type.h"
#include "trx_log.h"
#include "conflict_thread_pool.h"
#include "benchlog.h"


enum ConflictType{

	YCSB_UPDATE_CONFLICT = 1,

	// PERCENTAGE_CONFLICT_HANDLE,
    // LOG_ROW_CONFLICT_HANDLE,
    // LOG_PAGE_CONFLICT_HANDLE,
    // LOCK_PAGE_CONFLICT_HANDLE
};

enum ConflictHandleType
{
	SerialScan = 1,
	ArgMode = 2,
	PipelineWithoutHole = 3,
	Pipeline = 4,
};

enum ConflictHandleLevel
{
	PAGE = 1,
	ROW = 2
};

class ConflictHandle
{
	public:
		void init(ConflictHandleLevel c_level, ConflictHandleType h_type);	
		bool conflict_detect(std::vector<const TrxLog*> & ref_list , const TrxLog * cur_log);
		static int arg_detect(TrxID id);
	
	private:
		ConflictHandleLevel handle_level;
		ConflictHandleType handle_type;

		/* log scan */
		bool pipeline_without_hole(std::vector<const TrxLog *> & ref_list , const TrxLog & cur_log);
		/* verify */
};

#endif
