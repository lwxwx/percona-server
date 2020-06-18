/*
 * @Author: wei
 * @Date: 2020-06-15 10:41:24
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-06-17 19:03:01
 * @Description: trx_info and redo log functions
 * @FilePath: /multi_master_log_plugin/src/trx_info.cc
 */
#include "trx_info.h"
#include "debug.h"
#include "easylogger.h"

TrxInfo * plugin_trx_info_ptr = NULL;

/***
 * Redo Log Section
 */

RedoLogRec *  allocate_redolog_record(uint32_t size)
{
    RedoLogRec * result = new RedoLogRec;
    result->no_header = true;
    result->no_rec = true;
    result->rec = new char[size];
    result->rec_size = size;
    return result;
}

// int copy_log_buffer(RedoLogRec * redo,void * log_buffer)
// {
//    //  (log_buffer,RedoLogBufferSize(redo->start_lsn,redo->end_lsn),redo->buf);
//     if(redo->buf == NULL || log_buffer == NULL) return -1;
//     memcpy(redo->buf,log_buffer,RedoLogBufferSize(redo->start_lsn,redo->end_lsn));
//     return 1;
// }

int destory_redolog_record(RedoLogRec * redo)
{
    if(redo == NULL) return -2;

    if(redo->rec != NULL)
    {
        delete [] redo->rec;
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
    // for(auto it = redolog_list.begin(); it != redolog_list.end(); it++)
    // {
    //     destory_redolog_record(*it);
    // }

    for(auto it = uncompleted_rec_list.begin(); it != uncompleted_rec_list.end(); it++)
    {
        destory_redolog_record(*it);
    }

    for(auto i = page_record_map.begin();i!=page_record_map.end();i++ )
    {
        for(auto j = (i->second).begin();j!= (i->second).end();j++)
        {
            for(auto k = (j->second).begin();k!= (j->second).end(); k++)
                destory_redolog_record(*k);
        }
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

// int TrxLog::add_redolog_section(redolog_lsn_t start_lsn,redolog_lsn_t end_lsn,void * log_buf)
// {
//     RedoLogRec * section = allocate_redolog_record(start_lsn,end_lsn);
//     copy_log_buffer(section,log_buf);
// }

int TrxLog::add_redolog_record(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,uint32_t size)
{
    // if(page_record_map.find(space_id) == page_record_map.end())
    // {
    //     page_record_map[space_id].clear(); //new space_id list
    // }

    // if(page_record_map[space_id].find(page_no) == page_record_map[space_id].end())
    // {
    //     page_record_map[space_id][page_no].clear();
    // }

    RedoLogRec * rec = allocate_redolog_record(size);
    rec->type = type;
    rec->space_id = space_id;
    rec->page_no = page_no;
    rec->rec_size = size;

    uncompleted_rec_list.push_back(rec);

    return 1;
}

/***
 * TrxInfo
 */
int TrxInfo::trx_started()
{
    ThreadID tid = std::this_thread::get_id();

#if DEBUG_REDO_LOG_COLLECT
    std::string  log_file_name = LOG_DIR ;
    log_file_name +=  REDO_LOG_FILE_NAME;
    log_file_name += "_" + std::to_string(std::hash<std::thread::id>{}(tid)) + LOG_FILE_SUFFIX;
    EasyStringLog(log_file_name, std::to_string(std::hash<std::thread::id>{}(tid)),
        "【trx started】",EasyLogger::LOG_LEVEL::debug);
#endif

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

#if DEBUG_REDO_LOG_COLLECT
    std::string  log_file_name = LOG_DIR ;
    log_file_name +=  REDO_LOG_FILE_NAME;
    log_file_name += "_" + std::to_string(std::hash<std::thread::id>{}(tid)) + LOG_FILE_SUFFIX;
    EasyStringLog(log_file_name, std::to_string(std::hash<std::thread::id>{}(tid)),
        "【trx commit】 : " + std::to_string(id),EasyLogger::LOG_LEVEL::debug);
#endif

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

int TrxInfo::mtr_redolog_record_add(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,uint32_t size)
{
    ThreadID tid = std::this_thread::get_id();

#if DEBUG_REDO_LOG_COLLECT
    std::string  log_file_name = LOG_DIR ;
    log_file_name +=  REDO_LOG_FILE_NAME;
    log_file_name += "_" + std::to_string(std::hash<std::thread::id>{}(tid)) + LOG_FILE_SUFFIX;
    EasyStringLog(log_file_name,std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR LOG TYPE】: " + std::to_string(type) + "; 【Space_id】 " + std::to_string(space_id) + "; 【Page_no】 " + std::to_string(page_no),EasyLogger::LOG_LEVEL::debug);
#endif

    if (working_thread_map.find(tid) == working_thread_map.end())
    {
        return -1;
    }

    TrxLog * trx_redo = working_thread_map[tid];

    trx_redo->add_redolog_record(type,space_id,page_no,size);

    return 1;
}




