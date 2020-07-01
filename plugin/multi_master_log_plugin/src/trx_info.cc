/*
 * @Author: wei
 * @Date: 2020-06-15 10:41:24
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-01 16:25:17
 * @Description: trx_info and redo log functions
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/trx_info.cc
 */
#include "trx_info.h"
#include "debug.h"
#include "easylogger.h"
#if DEBUG_REDO_LOG_COLLECT
#include <iostream>
#endif
TrxInfo * plugin_trx_info_ptr = NULL;

#include<chrono>

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

RedoLogRec *  allocate_redolog_record()
{
    RedoLogRec * result = new RedoLogRec;
    result->no_header = true;
    result->no_rec = true;
    result->rec = NULL;
    result->rec_size = 0;
    return result;
}


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


    for(auto it = uncompleted_rec_list.begin(); it != uncompleted_rec_list.end(); it++)
    {
        destory_redolog_record(*it);
    }

    for(auto it = completed_rec_list.begin();it != completed_rec_list.end();it++)
    {
        destory_redolog_record(*it);
    }

    // for(auto i = page_record_map.begin();i!=page_record_map.end();i++ )
    // {
    //     for(auto j = (i->second).begin();j!= (i->second).end();j++)
    //     {
    //         for(auto k = (j->second).begin();k!= (j->second).end(); k++)
    //             destory_redolog_record(*k);
    //     }
    // }
}


int TrxLog::log_clear()
{
    for(auto it = uncompleted_rec_list.begin(); it != uncompleted_rec_list.end(); it++)
    {
        destory_redolog_record(*it);
    }
    uncompleted_rec_list.clear();
    for(auto it = completed_rec_list.begin();it != completed_rec_list.end();it++)
    {
        destory_redolog_record(*it);
    }
    completed_rec_list.clear();
    return 1;
}

int TrxLog::trx_started()
{
    trx_is_started = true;
    log_clear();
    return 1;
}

int TrxLog::wr_trx_commiting(TrxID id)
{
    trx_no = id;
    trx_is_commited = true;
    return 1;
}

int TrxLog::add_redolog_record(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset)
{
   // RedoLogRec * rec = allocate_redolog_record();
    if(now_rec == NULL)
    {
        return -1;
    }
    now_rec->type = type;
    now_rec->space_id = space_id;
    now_rec->page_no = page_no;
    now_rec->offset = offset;
    now_rec->no_header = false;
    now_rec->no_rec = true;
    //rec->rec_size = size;
    uncompleted_rec_list.push_back(now_rec);
    now_rec = NULL;

    return 1;
}

int TrxLog::new_redolog_record(size_t size)
{
    if(now_rec != NULL)
    {
        now_rec = NULL;
        return -1;
    }
    now_rec = allocate_redolog_record(size);

    return 1;
}

/***
 * TrxInfo
 */
int TrxInfo::init(const char * g_name,const char * local,const char * peers)
{
    //xcom_gcs.init(g_name,local,peers);
    std::string local_str = std::string(local);
    std::string peers_str = std::string(peers);
    m_paxos.init(local_str,peers_str);
    m_paxos.RunPaxos();
    return 1;
}


int TrxInfo::trx_started()
{
    ThreadID tid = std::this_thread::get_id();
    TrxLog * trx_redo = NULL;

    if(working_thread_map.find(tid) == working_thread_map.end())
    {
        working_thread_map[tid] = NULL;
    }

    if(working_thread_map[tid] != NULL)
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
        "【trx clear】",EasyLogger::LOG_LEVEL::debug);
#endif

        trx_redo = working_thread_map[tid];
    }
    else
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
        "【trx started】",EasyLogger::LOG_LEVEL::debug);
#endif
        //TODO mutex保护，避免多线程添加
        trx_redo = new TrxLog();
        working_thread_map[tid] = trx_redo;
    }

    trx_redo->trx_started();

    return 1;
}

int TrxInfo::wr_trx_commiting(TrxID id)
{
    ThreadID tid = std::this_thread::get_id();
    TrxLog * trx_redo = NULL;

    auto tid_it = working_thread_map.find(tid);

    if ( tid_it == working_thread_map.end() || tid_it->second == NULL )
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
        "【No trx start in this thread】 : " + std::to_string(id),EasyLogger::LOG_LEVEL::debug);
#endif

        return -1;
    }
    else
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(), std::to_string(std::hash<std::thread::id>{}(tid)),
        "【trx commit】 : " + std::to_string(id),EasyLogger::LOG_LEVEL::debug);
#endif

        trx_redo = working_thread_map[tid];
        trx_redo->wr_trx_commiting(id);
        working_thread_map[tid] = NULL;

        std::string id_str = std::to_string(id);
       // xcom_gcs.send_test_message(id_str.c_str(), id_str.length() + 1);
#if DEBUG_PHXPAXOS_CONFLICT
        auto before_propose = std::chrono::steady_clock::now();
#endif
       int ret = m_paxos.propose(id_str);
#if DEBUG_PHXPAXOS_CONFLICT
       if(ret == phxpaxos::PaxosTryCommitRet_Conflict)
       {
            phxpaxos_conflict_time += (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - before_propose)).count();
            phxpaxos_conflict_count++;

       }
       else if(ret == phxpaxos::PaxosTryCommitRet_OK)
       {
            phxpaxos_propose_count++;
            phxpaxos_propose_time += (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - before_propose)).count();
       }
       else
       {
            phxpaxos_other_count++;
       }
#endif
        // rollback : delete trx_redo
       // TODO：多线程添加保护 global_trx_redo_map[id] = trx_redo;
        return 1;
    }
}

int TrxInfo::mtr_redolog_record_add(plugin_mlog_id_t type,plugin_space_id_t space_id,plugin_page_no_t page_no,plugin_page_offset_t offset)
{
    ThreadID tid = std::this_thread::get_id();

    if (working_thread_map.find(tid) == working_thread_map.end())
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR add:No this thread】 " ,EasyLogger::LOG_LEVEL::debug);
#endif

        return -1;
    }
    if(working_thread_map[tid] == NULL)
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR add:No this trx】 " ,EasyLogger::LOG_LEVEL::debug);
#endif

        return -1;
    }

    TrxLog * trx_redo = working_thread_map[tid];

    if(trx_redo->add_redolog_record(type,space_id,page_no,offset) <= 0)
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR LOG ADD ERROR】: " + std::to_string(type) + "; 【Space_id】 " + std::to_string(space_id) + "; 【Page_no】 " + std::to_string(page_no) + "; 【Offset】"+ std::to_string(offset),EasyLogger::LOG_LEVEL::debug);
#endif
        return -1;
    }
    else
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR LOG TYPE】: " + std::to_string(type) + "; 【Space_id】 " + std::to_string(space_id) + "; 【Page_no】 " + std::to_string(page_no) + "; 【Offset】"+ std::to_string(offset),EasyLogger::LOG_LEVEL::debug);
#endif
        return 1;
    }
//    return 1;
}

int TrxInfo::mtr_redolog_record_new(size_t size)
{
    ThreadID tid = std::this_thread::get_id();

    if (working_thread_map.find(tid) == working_thread_map.end())
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR new:No this thread】 " ,EasyLogger::LOG_LEVEL::debug);
#endif

        return -1;
    }
    if(working_thread_map[tid] == NULL)
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR new:No this trx】 " ,EasyLogger::LOG_LEVEL::debug);
#endif

        return -1;
    }

    TrxLog * trx_redo = working_thread_map[tid];
    if( trx_redo->new_redolog_record(size) <= 0 )
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR LOG NEW ERROR】: " + std::to_string(size) ,EasyLogger::LOG_LEVEL::debug);
#endif
        return -1;
    }
    else
    {
#if DEBUG_REDO_LOG_COLLECT
    std::stringstream ss;
    ss << LOG_DIR << REDO_LOG_FILE_NAME << "_" << tid << LOG_FILE_SUFFIX;
    EasyStringLog(ss.str(),std::to_string(std::hash<std::thread::id>{}(tid)),
        "【MTR LOG SIZE】: " + std::to_string(size) ,EasyLogger::LOG_LEVEL::debug);
#endif
        return 1;
    }
}




