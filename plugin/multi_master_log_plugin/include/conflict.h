/*
 * @Author: wei
 * @Date: 2020-06-28 17:00:34
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-28 17:24:50
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/include/conflict.h
 */

#ifndef MMLP_CONFLICT_HEADER
#define MMLP_CONFLICT_HEADER

enum ConflictHandleType{
    LOG_ROW_CONFLICT_HANDLE,
    LOG_PAGE_CONFLICT_HANDLE,
    LOCK_PAGE_CONFLICT_HANDLE
};

class LogConflictHandle
{
    public:
    private:
};


#endif