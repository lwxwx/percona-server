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

#include "trx_log.h"
enum ConflictHandleType{
	PERCENTAGE_CONFLICT_HANDLE,
    LOG_ROW_CONFLICT_HANDLE,
    LOG_PAGE_CONFLICT_HANDLE,
    LOCK_PAGE_CONFLICT_HANDLE
};

enum ConflictArgType
{
	// PERCENTAGE_CONFLICT_HANDLE
	MAX_PERCENT,

	//
};

/**
 * Static ConflictHandle Class
 * **/
class ConflictHandle
{
	public:
	// set conflict handle type
	static void setHandleType(ConflictHandleType type)
	{
		handle_type = type;
	}
	static ConflictHandleType getHandleType()
	{
		return handle_type;
	}

	// set args for conflict_check
	static void setArg(ConflictArgType key,void * val)
	{
		arg_map[key] = val;
	}
	static void * getArg(ConflictArgType key)
	{
		return arg_map[key];
	}

	// static interface -  conflict check function for any handle type
	// TODO:仅支持了百分比冲突判断
	static int conflict_check(TrxLog * trx);

	private:
	static int percent_conflict_check(TrxLog * trx);

	static ConflictHandleType handle_type;	
	static std::map<ConflictArgType,void *> arg_map;
};

#endif
