/*
 * @Author: wei
 * @Date: 2020-07-14 07:24:06
 * @LastEditors: Do not edit
 * @LastEditTime: 2020-07-23 09:38:58
 * @Description: file content
 * @FilePath: /percona-server/plugin/multi_master_log_plugin/src/id_allocate.cc
 */

#include "id_allocate.h"
#include <chrono>
#include <iostream>

/**
 * class GlobalTrxIDFactory
 * **/
int GlobalTrxIDFactory::setFactoryType(ID_FACTORY_TYPE type)
{
    factory_type = type;
    return 1;
}

ID_FACTORY_TYPE GlobalTrxIDFactory::getFactoryType()
{
    return factory_type;
}

int GlobalTrxIDFactory::init(ID_FACTORY_TYPE type)
{
    setFactoryType(type);

    //request_count = 0;
    global_id_gen = NULL;
    if(factory_type == ID_FACTORY_TYPE::SLICE_ID_GEN_SYNC)
    {
        global_id_gen = new SliceTrxIdGen_SYNC;
    }
#if PHXPAXOS_ID_COMPLIE
    if(factory_type == ID_FACTORY_TYPE::PHXPAXOS_PROPOSE_SYNC)
    {
        global_id_gen = new PhxPaxosTrxIdGen_SYNC;
    }
#endif

    if(global_id_gen != NULL)
        global_id_gen->init();
    else
        return -1;

    return 1;
}

int GlobalTrxIDFactory::requestGlobalTrxID()
{
    global_id_gen->handle_request();
    return 1;
}

TrxID GlobalTrxIDFactory::getGlobalTrxID()
{
    return global_id_gen->get_id();
}

/**
 * class SliceTrxIdGen_SYNC
 * **/
int SliceTrxIdGen_SYNC::init()
{
    slice_len = 0;
    std::string peers_str = std::string(brpc_peer_nodes_ptr);
    std::stringstream peers_ss(peers_str);
    std::string tmp;
    //std::cout <<"Peer Nodes : " << peers_str << std::endl;
    while(!peers_ss.eof())
    {
        getline(peers_ss,tmp,',');
     //   std::cout << "Line Node : " << tmp << std::endl;
        slice_len++;
    }

    node_no = slice_node_no;
    slice_count = 0;

    return 1;
}

TrxID SliceTrxIdGen_SYNC::get_id()
{
    uint64_t before_slice_cal = 0;
    if(DEBUG_SLICE_TIME != 0)
    {
        before_slice_cal = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count();
    }

    slice_count++;
    TrxID id = node_no + slice_count * slice_len;

    if(DEBUG_SLICE_TIME != 0)
    {
        slice_gen_count++;
        slice_gen_time += (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count()- before_slice_cal;
    }

    return id;
}

#if PHXPAXOS_ID_COMPLIE
/**
 * class PhxPaxosTrxIdGen_SYNC
 * **/
int PhxPaxosTrxIdGen_SYNC::init()
{
    std::string local_str = std::string(phxpaxos_local_node_ptr);
    std::string peer_str = std::string(phxpaxos_peer_nodes_ptr);
    //std::cout << "phxpaxos init 1" << std::endl;
    m_paxos.init(local_str,peer_str);
    //std::cout << "phxpaxos init 2" << std::endl;
    m_paxos.RunPaxos();
    //std::cout << "phxpaxos init 3" << std::endl;

    return 1;
}

TrxID PhxPaxosTrxIdGen_SYNC::get_id()
{
    TrxID id = 0;
    bool has_conflict = false;
    std::string id_str = "";

    uint64_t before_propose = 0;
    if(DEBUG_PHXPAXOS_TIME != 0)
    {
        before_propose = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count();
    }

    int ret = m_paxos.propose(id_str,id);

    // propose conflict
    while (ret == phxpaxos::PaxosTryCommitRet_Conflict)
    {
        has_conflict = true;
        ret = m_paxos.propose(id_str,id);
        //std::cout << "PhxPaxos Conflict" << std::endl;
    }

    if(has_conflict)
    {
        if(DEBUG_PHXPAXOS_TIME != 0)
        {
            phxpaxos_conflict_count++;
            phxpaxos_conflict_time +=  (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count() - before_propose;
        }
    }

    if(ret == phxpaxos::PaxosTryCommitRet_OK)
    {
        if(DEBUG_PHXPAXOS_TIME != 0)
        {
            phxpaxos_propose_count++;
            phxpaxos_propose_time +=  (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())).count() - before_propose;
        }
    }
    else
    {
        if(DEBUG_PHXPAXOS_TIME != 0)
            phxpaxos_other_count++;
    }

    return id;
}
#endif