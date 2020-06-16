/*
 * @Author: wei
 * @Date: 2020-06-15 10:41:24
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-16 10:02:03
 * @Description: trx_info and redo log functions
 * @FilePath: /multi_master_log_plugin/src/trx_info.cc
 */
#include "trx_info.h"

TrxInfo * plugin_trx_info_ptr = NULL;

/***
 * Redo Log Section
 */

RedoLogSection *  allocate_redolog_section(redolog_lsn_t start_lsn,redolog_lsn_t end_lsn)
{
    RedoLogSection * result = new RedoLogSection;
    result->start_lsn = start_lsn;
    result->end_lsn = end_lsn;
    result->buf = new char[RedoLogBufferSize(start_lsn,end_lsn)];
    return result;
}

int copy_log_buffer(RedoLogSection * redo,void * log_buffer)
{
   //  (log_buffer,RedoLogBufferSize(redo->start_lsn,redo->end_lsn),redo->buf);
    if(redo->buf == NULL || log_buffer == NULL) return -1;
    memcpy(redo->buf,log_buffer,RedoLogBufferSize(redo->start_lsn,redo->end_lsn));
    return 1;
}

int destory_redolog_section(RedoLogSection * redo)
{
    if(redo->buf != NULL)
    {
        delete [] redo->buf;
        return 1;
    }
    else
    {
        return -1;
    }
}

/***
 * TrxLog
 */
TrxLog::~TrxLog()
{
    for(auto it = redolog_list.begin(); it != redolog_list.end(); it++)
    {
        destory_redolog_section(*it);
    }
}

int TrxLog::trx_started()
{
    trx_is_started = true;
    return 1;
}

int TrxLog::trx_commiting(TrxID id)
{
    trx_no = id;
    trx_is_commited = true;
    return 1;
}

int TrxLog::add_redolog_section(redolog_lsn_t start_lsn,redolog_lsn_t end_lsn,void * log_buf)
{
    RedoLogSection * section = allocate_redolog_section(start_lsn,end_lsn);
    copy_log_buffer(section,log_buf);
}


/***
 * TrxInfo
 */
int TrxInfo::trx_started()
{
    ThreadID tid = std::this_thread::get_id();

    if(working_thread_map.find(tid) != working_thread_map.end())
    {
        /*need to record error*/
        return 0; // last trx not commit;
    }

    TrxLog * trx_redo = new TrxLog();
    working_thread_map[tid] = trx_redo;

    trx_redo->trx_started();

    return 1;
}

int TrxInfo::trx_commiting(TrxID id)
{
    ThreadID tid = std::this_thread::get_id();

    if (working_thread_map.find(tid) == working_thread_map.end())
    {
        return -1;
    }

    TrxLog * trx_redo = working_thread_map[tid];

    int commit_reply = trx_redo->trx_commiting(id);

    working_thread_map.erase(tid);

    if(commit_reply > 0)
    {
        complete_trx_redo_map[id] = trx_redo;
        return 1;
    }
    else
    {
        delete trx_redo;
        return -1;
    }
}

int TrxInfo::mtr_redolog_collect(redolog_lsn_t start_lsn,redolog_lsn_t end_lsn,void * log_buf)
{
    ThreadID tid = std::this_thread::get_id();

    if (working_thread_map.find(tid) == working_thread_map.end())
    {
        return -1;
    }

    TrxLog * trx_redo = working_thread_map[tid];

    trx_redo->add_redolog_section(start_lsn,end_lsn,log_buf);

    return 1;
}




